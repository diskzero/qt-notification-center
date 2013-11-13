/*
The MIT License (MIT)

Copyright (c) 2011 Gene Z. Ragan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

// Self
#include "NotificationCenter.h"

// Qt
#include <QCoreApplication>
#include <QMetaObject>
#include <QMetaMethod>
#include <QObject>
#include <QSet>
#include <QtDebug>
#include <QVector>
#include <QThread>

// System
#include <boost/crc.hpp>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>

// Python
#undef _POSIX_C_SOURCE
#undef HAVE_STAT
#include <Python.h>
#include <sip.h>
#include "GilState.h"

// Local
#include "NotificationLogging.h"
#include "QtForPython.h"

// sip defines ANY to be void; this breaks mm code, which uses the identifier
// ANY as the member of an enum.  Undefine ANY here to avoid this problem.
#ifdef ANY
#undef ANY
#endif

// Define this to dump metaobject information
//#define NC_VERBOSE

// Define this to use event coalsecing
//#define NC_COALESCE_EVENTS

// Namespaces
using namespace framework;

// Static constants
ConnectionId NotificationCenter::INVALID_CONNECTION_ID = static_cast<ConnectionId>(-1);

// Constants
static const QEvent::Type kNCEventType = (QEvent::Type)(QEvent::User + 1);
static const QString kSignalSignature("(const framework::Event&)");

// Set up a logging module
#ifndef USE_LOCAL_LOGGING
LOG_THIS_FILE_TO("notification_center");
#endif

//=============================================================================
// class EventId
//
/// EventType identifier that internally uses a crc32 generated from a string.
//=============================================================================
std::ostream &
operator << (std::ostream & strm, const EventId& inId)
{
    strm << qPrintable(inId.getStringId()) << ' ' << inId.getHash() << ' ' << inId.getEventType();
    return strm;
}

//-----------------------------------------------------------------------------
// EventId::EventId()
//
/// Default constructor
/// Used internally by NotificationCenter
//-----------------------------------------------------------------------------
EventId::EventId()
    :   mCrc32(0),
        mEventType(NotificationCenter::INVALID_CONNECTION_ID)
{
}


//-----------------------------------------------------------------------------
// EventId::EventId()
//
/// Constructor from a target string id.
/// \param inStringId The event string identifier.
//-----------------------------------------------------------------------------
EventId::EventId(const QString& inId)
    :   mStringId(inId),
        mEventType(NotificationCenter::INVALID_CONNECTION_ID)
{
    Q_ASSERT(!inId.trimmed().isEmpty());
    static boost::crc_32_type computer;
    computer.reset();

    const QByteArray& bytes = mStringId.toLatin1();
    computer.process_bytes(bytes, bytes.length());
    mCrc32 = computer.checksum();
}


//-----------------------------------------------------------------------------
// EventId::EventId()
//
/// Copy constructor.
/// \param inFrom EventType to copy from.
//-----------------------------------------------------------------------------
EventId::EventId(const EventId& inFrom)
    :   mStringId(inFrom.mStringId),
        mCrc32(inFrom.mCrc32),
        mEventType(inFrom.mEventType)
{
}


//-----------------------------------------------------------------------------
// EventId::registerSelf()
//
/// Called by notification center to create a unique event ID. This call
/// should not be called by the creator of the object.
//-----------------------------------------------------------------------------
void
EventId::registerSelf()
{
    // Register for a unique Qt message id.
    mEventType = QEvent::registerEventType();
    
    //LOG_INFO("EventId::registerSelf() " << "EventType: " << mEventType);
}


//=============================================================================
// class Event
//
/// The Event class is the base class of all event notifications.
//=============================================================================

//-----------------------------------------------------------------------------
// Event::Event()
//
/// Constructor from target a target string id.
/// \param inId An event id.
//-----------------------------------------------------------------------------
Event::Event(const EventId& inId)
    :   id(inId)
{
}


//=============================================================================
// class NCEvent
//
/// Used internally by the Notification Center to wrap and dispatch
/// Notification Center events using the Qt synchronous and
/// asynchronous messaging systems. It will clean up QEvent data that
/// was passed into it.
//=============================================================================
typedef boost::shared_ptr<Event> EventRef;

class NCEvent : public QEvent
{
public:

    //-----------------------------------------------------------------------------
    // NCEvent::NCEvent()
    //
    /// Constructor using pointer to callback data.
    /// \param inEvent The event. Ownership is passed to the custom event
    /// and will be deleted.
    //-----------------------------------------------------------------------------
    explicit NCEvent(Event* inEvent)
        :   QEvent(kNCEventType),
            mEvent(inEvent)
    {
    }

    //-----------------------------------------------------------------------------
    // NCEvent::~NCEvent()
    //
    /// Delete custom data if ownership was passed to the object.
    //-----------------------------------------------------------------------------
    ~NCEvent()
    {
    }


    //-----------------------------------------------------------------------------
    // NCEvent::getEvent()
    //
    /// Return the internal event data.
    //-----------------------------------------------------------------------------
    EventRef 
    getEvent() const
    {
        return mEvent;
    }

    #ifdef DEBUG
        int mTime;        // Used to profile performance.
    #endif

private:
    NCEvent(const NCEvent& );
    NCEvent& operator=(const NCEvent& );

    boost::shared_ptr<Event> mEvent;
};


//=============================================================================
// class NotificationCenter
//
/// The Notification Center allows the free form connection of notifiers
/// and observers.
///
/// To enable runtime debug output in development builds, create a file
/// named af_notification_center_debug in tmp.
/// For example: touch /tmp/af_notification_center_debug.
/// This will cause the Notification Center to output a verbose
/// activity stream to the console.
//
//=============================================================================

// Set up our own events that we use to broadcast status.
framework::EventId NotificationCenter::EventRegistered("com.mightytoad.ApplicationFramework.NotificationCenter.EventRegistered");
framework::EventId NotificationCenter::EventConnected("com.mightytoad.ApplicationFramework.NotificationCenter.EventConnected");
framework::EventId NotificationCenter::EventDisconnected("com.mightytoad.ApplicationFramework.NotificationCenter.EventDisconnected");

// Set the default event coalescing quantuum
static const int kCoalesceInterval = 20;  // in milliseconds

//-----------------------------------------------------------------------------
// NotificationCenter::NotificationCenter()
//
/// Dynamic event dispatcher
//-----------------------------------------------------------------------------
NotificationCenter::NotificationCenter()
    :   mConnectionIdCount(0)
    ,   mCoalesceInterval(kCoalesceInterval)
    ,   mTimerId(0)
    ,   mDebugOutput(false)
{
    // Check for debug flag files
    struct stat info;
    mDebugOutput = stat("/tmp/af_notification_center_debug", &info) == 0;

    // Register our own events
    registerEvent(EventRegistered);
    registerEvent(EventConnected);
    registerEvent(EventDisconnected);

#ifdef NC_COALESCE_EVENTS
    // Begin the event coalescing timer
    mTimerId = startTimer(mCoalesceInterval);
#endif
}


//-----------------------------------------------------------------------------
// NotificationCenter::NotificationCenter()
//
/// Default destructor
//-----------------------------------------------------------------------------
NotificationCenter::~NotificationCenter()
{
#ifdef NC_COALESCE_EVENTS
    // Shut down the event coalescing timer
    killTimer(mTimerId);

    // TODO gragan: Do we care about any unsent events?
    Q_FOREACH(const EventPriorityPair& eventPair, mCoalesceList) {
        delete eventPair.first;
    }
#endif
    
    // Check for dangling connections and deal with them.
    if (!mConnectionMap.empty()) {
        LOG_WARN("Notification Center: " << mConnectionMap.size() 
            << " active connections during shutdown."); 

        ConnectionMap disconnectUs = mConnectionMap;
        
        ConnectionMap::iterator iter = disconnectUs.begin();
        for ( ; iter != disconnectUs.end(); ++iter) {

            const ConnectionInfo& connectionInfo = iter.value();
            
            LOG_WARN("active connection" 
                  << std::endl
                  << "      "
                  << "EventId: " << qPrintable(connectionInfo.eventId.mStringId) 
                  << std::endl
                  << "      "
                  << "ConnectId: " << connectionInfo.connectionId 
                  << std::endl
                  << "      "
                  << "ConnectionType: " 
                  << qPrintable(connectionTypeToString(connectionInfo.type)) << std::endl);
                  
            disconnect(connectionInfo.connectionId);
        }        
    }
}


//-----------------------------------------------------------------------------
// NotificationCenter::timerEvent()
//
/// Post queued coalesced events.
/// \result a EventIdSet (QSet<QString>) containing all event IDs
//-----------------------------------------------------------------------------
void
NotificationCenter::timerEvent(QTimerEvent* inEvent)
{
    Q_UNUSED(inEvent);

#ifdef NC_COALESCE_EVENTS
    Q_FOREACH(const EventPriorityPair& eventPair, mCoalesceList) {
    
        // Create the QEvent to send
        NCEvent* theEvent = new NCEvent(eventPair.first);

#ifdef DEBUG
        // Stamp the event with the start time
        theEvent->mTime = QTime::currentTime().elapsed();
#endif    

        QCoreApplication::postEvent(this, theEvent, eventPair.second);
    }

    // All pending events have been processed.
    mCoalesceList.clear();
#else
    Q_ASSERT(mCoalesceList.isEmpty());
#endif
}


//-----------------------------------------------------------------------------
// NotificationCenter::registeredEvents()
//
/// Print all the events we know about
/// \result a EventIdSet (QSet<QString>) containing all event IDs
//-----------------------------------------------------------------------------
QSet<QString>
NotificationCenter::registeredEvents() const
{
    EventIdSet results;
    
    EventRegistry::const_iterator start = mEventRegistry.begin();
    EventRegistry::const_iterator stop = mEventRegistry.end();

    while (start != stop) {
        results.insert(start.value().getStringId());
        ++start;
    }    
    return results;
}


//-----------------------------------------------------------------------------
// NotificationCenter::registerEvent()
//
/// Register and event ID with the event registry.
/// \param inEventId Event ID to add to the event registry
/// \result True if the event was registered, false if it has been registered.
//-----------------------------------------------------------------------------
bool
NotificationCenter::registerEvent(const EventId& inEventId)
{
    if (mDebugOutput) {
        LOG_INFO("NotificationCenter::registerEvent() ----> "
                  << "EventId: " << inEventId);
    }

    bool result = true;

    // Check and see if this event is already in the registry
    EventRegistry::iterator iter = mEventRegistry.find(inEventId.getHash());
    if (iter == mEventRegistry.end()) {
        // We did not find a registered event.
        // Register the event
        EventId& localEvent = const_cast<EventId&>(inEventId);
        localEvent.registerSelf();

        // Add the event to the list of events available.
        mEventRegistry[inEventId.getHash()] = localEvent;

        // Check the deferred event list and make the connections to anyone waiting for
        // this particular event.
        checkForAndConnectDeferredEvents(inEventId);

        // Send a notification about the event registration.
        Event* event = new Event(EventRegistered);
        event->dictionary["id"] = inEventId.mStringId;
        postEvent(event);

    } else {
        if (mDebugOutput) {
            LOG_WARN("NotificationCenter::registerEvent() event already registered ----> "
                      << "EventId: " << inEventId);
        }

        // Event already has been registered
        result = false;
    }

    return result;
}


//-----------------------------------------------------------------------------
// NotificationCenter::emitDynamicSignal()
//
/// Dynamic signal invocation mechanism.
//-----------------------------------------------------------------------------
bool
NotificationCenter::emitDynamicSignal(const QString& inSignal, void** inArgs)
{
    bool result = false;

    // Get the signal signature and id
    const QByteArray theSignal = QMetaObject::normalizedSignature(inSignal.toLatin1());
    const int signalId = mQtSignalIndices.value(theSignal, -1);

    if (signalId >= 0) {

		if (mDebugOutput) {
            LOG_INFO("NotificationCenter::emitDynamicSignal() ----> "
                      << "signal: " << theSignal.data()
                      << "     "
                      << "signalId: " << signalId);
        }

        QMetaObject::activate(this,
                              metaObject(),
                              signalId + metaObject()->methodCount(),
                              inArgs);
        result = true;
    } else {
        if (mDebugOutput) {
            LOG_ERROR("NotificationCenter::emitDynamicSignal() failed ----> "
                      << "signal: " << theSignal.data()
                      << "     "
                      << "signalId: " << signalId);
        }
    }

    return result;
}


//-----------------------------------------------------------------------------
// NotificationCenter::connectDynamicSignal()
//
/// Internal notification center event handling.
/// \param ioInfo The ConnectionInfo
/// \result True if the connection was made.
//-----------------------------------------------------------------------------
bool
NotificationCenter::connectDynamicSignal(ConnectionInfo& ioInfo)
{
    Q_ASSERT(ioInfo.qtObject != NULL);

    // Get the slot and signal names and normalize them.
    const QByteArray& convertStr = ioInfo.qtSignal.toAscii();
    QByteArray theSignal = QMetaObject::normalizedSignature(convertStr.data());
    QByteArray theSlot = QMetaObject::normalizedSignature(ioInfo.qtMethod.toAscii());
    
    if (!QMetaObject::checkConnectArgs(theSignal, theSlot)) {
        if (mDebugOutput) { 
            LOG_ERROR("NotificationCenter::connectDynamicSignal() checkConnectArgs() failed ----> "
                      << "signal: " << theSignal.data()
                      << "     "
                      << "slot: " << theSlot.data());

#ifdef NC_VERBOSE
            // Dump all the notification center object info
			dumpMethods();
            
            // Dump all the target object info
			dumpConnectionMethods(ioInfo);
#endif
        }
        return false;
    }

    const int slotId = ioInfo.qtObject->metaObject()->indexOfSlot(theSlot);
    if (slotId < 0) {
        if (mDebugOutput) {
            LOG_ERROR("NotificationCenter::connectDynamicSignal() indexOfSlot() failed ----> "
                      << "slotId: " << slotId
                      << "     "
                      << "signal: " << theSignal.data()
                      << "     "
                      << "slot: " << theSlot.data());

#ifdef NC_VERBOSE
            // Dump all the target object info
			dumpConnectionMethods(ioInfo);
#endif
        }
        return false;
    }

    int signalId = mQtSignalIndices.value(theSignal, -1);
    if (signalId < 0) {
        signalId = mQtSignalIndices.size();
        mQtSignalIndices[theSignal] = signalId;

        // Set up the conection info
        ioInfo.qtSignal = const_cast<char *>(convertStr.data());
    }
	
    return QMetaObject::connect(this,
    							signalId + metaObject()->methodCount(),
    							ioInfo.qtObject,
    							slotId);
}


//-----------------------------------------------------------------------------
// NotificationCenter::event()
//
/// Internal notification center event handling. If the event is in the
/// UserEvent space, we attempt to handle it.
//-----------------------------------------------------------------------------
bool
NotificationCenter::event(QEvent* inEvent)
{
    bool result = false;

    if (inEvent->type() >= QEvent::User) {
        result = handleCustomEvent(inEvent);
    } else {
        result = QObject::event(inEvent);
    }

    return result;
}


#ifndef DISABLE_PYTHON
//-----------------------------------------------------------------------------
// NotificationCenter::callPythonFunctor()
//-----------------------------------------------------------------------------
struct callPythonFunctor
{
    callPythonFunctor(Event* inEvent)
        :   mEvent(inEvent)
    {
    }

    void operator()(PythonFunctionInfoRef inPythonFunctionInfo)
    {
        if (inPythonFunctionInfo->isValid()) {
            // Create the puthon method from the saved properties.
            PyObject* pyMethod = PyMethod_New(inPythonFunctionInfo->functionMethod,
                                              inPythonFunctionInfo->functionSelf,
                                              inPythonFunctionInfo->functionClass);
            if (pyMethod != NULL) {
                QHash<QString, QVariant> source = mEvent->dictionary;
                PyObject* dict = QtForPython_HashToPython(source);
                PyObject* arglist = NULL;
                if (dict == NULL) {
                    LOG_ERROR(mEvent->id.getStringId().toStdString() <<
                              " : event dictionary can not be translated to python ");
                    arglist = PyTuple_New(1);
                    PyTuple_SetItem(arglist, 0, PyDict_New());
                } else {
                    // Create the argument list, place dict in tuple
                    arglist = Py_BuildValue("(O)", dict);
                }
                
                Q_ASSERT(arglist != NULL);
                    
                // Attempt to call the callback
                PyObject* pyResult = PyEval_CallObject(pyMethod, arglist);
                if (pyResult == NULL && PyErr_Occurred()) {
                    //Error is handled here because there may be no higher
                    //handler for notification callback
                    PyErr_Print();
                    PyErr_Clear();
                }
                
                Py_XDECREF(pyResult);

                // We are done with the argument and the method
                Py_DECREF(arglist);
                Py_XDECREF(dict);
                Py_DECREF(pyMethod);
            }
        }
    }

    Event* mEvent;
};
#endif

//-----------------------------------------------------------------------------
// NotificationCenter::handleCustomEvent()
//
/// Internal notification center event handling.
///
/// We take the event ID from the event passed in and check for
/// connected boost and Qt slots. If we find either, we manually
/// invoke the signal.
//-----------------------------------------------------------------------------
bool
NotificationCenter::handleCustomEvent(QEvent* inQtEvent)
{
    // We only care about a single type of custom event.
    if (inQtEvent->type() != kNCEventType)
        return false;

    if (mDebugOutput) {
        LOG_INFO("NotificationCenter Manager: handleCustomEvent() ----> "
                  << "Type:" << (int)inQtEvent->type());
    }

    // Get the NotificationCenter event out of the QEvent
#ifdef DEBUG
    NCEvent* customEvent = dynamic_cast<NCEvent *>(inQtEvent);
    Q_ASSERT(customEvent != NULL);
#else
    NCEvent* customEvent = static_cast<NCEvent *>(inQtEvent);
#endif

    EventRef event = customEvent->getEvent();
    if (event.get() == NULL) {
        // No custom event data.
        return false;
    }

    // We get the event type and attempt to retrieve the event
    // registration info.  The info will contain a list of all
    // connected boost::slots and all connected Qt::slots.
    EventMap::iterator eventIter = mEvents.find(event->id);
    if (eventIter != mEvents.end()) {

        // Get the callback info pair from the iterator
        const EventCallbackInfo& callbackInfo = eventIter.value();

        // Handle the boost signals
        if (!callbackInfo.boostSignal->empty()) {
            (*callbackInfo.boostSignal)(*event);
        }

        // Handle the Qt signals
        if (!callbackInfo.qtSlotSignature.isEmpty()) {
            QVector<void *> args(2, 0);
            args[0] = 0;
            args[1] = event.get();
            emitDynamicSignal(callbackInfo.qtSlotSignature, args.data());
        }

#ifndef DISABLE_PYTHON
        if (!callbackInfo.pythonFunctionList.empty()) {
            python_gil::GilState gilstate;
            // Handle the python callables
            std::for_each(callbackInfo.pythonFunctionList.begin(),
                          callbackInfo.pythonFunctionList.end(),
                          callPythonFunctor(event.get()));
        }
#endif
    }

#ifdef DEBUG
    if (mDebugOutput)
        LOG_INFO("Event dispatch time: " << QTime::currentTime().elapsed() - customEvent->mTime);
#endif

    return true;
}


//-----------------------------------------------------------------------------
// NotificationCenter::postEvent()
//
/// Helper method to create an event and post the event
/// using just an EventId. This is useful if you are posting
/// an event with no dictionary entries.
/// \param inId The EventId to create an event for.
/// \param inPostType The type in which to post the event.
/// \sa PostType.
//-----------------------------------------------------------------------------
void
NotificationCenter::postEvent(const EventId& inId, PostType inPostType)
{
	postEvent(new Event(inId), inPostType);
}


//-----------------------------------------------------------------------------
// NotificationCenter::postEvent()
//
/// Helper method to create an event and post the event
/// using just an EventId. This is useful if you are posting
/// an event with no dictionary entries.
/// \param inEvent A pointer to an event object.
/// \param inPriority The priority in which the event will be handled.
/// \param inPostType The type in which to post the event.
//-----------------------------------------------------------------------------
void
NotificationCenter::postEvent(const EventId& inId,
                              PostPriority inPriority,
                              PostType inPostType)
{
	postEvent(new Event(inId), inPriority, inPostType);
}



//-----------------------------------------------------------------------------
// NotificationCenter::postEvent()
//
/// Post an event to the notification center event queue. 
//  The event must be allocated on the heap as the
/// event queue will take ownership of the event. 
/// Events are processed in the order that they are posted.
/// If the event is posted with a post type of POST_NOW,
/// all events in the queue will be posted and then
/// the event will be posted synchronously.
/// \param inEvent A pointer to an event object.
/// \param inPostType The type in which to post the event.
/// \sa PostType.
//-----------------------------------------------------------------------------
void
NotificationCenter::postEvent(Event* inEvent, PostType inPostType)
{
    Q_ASSERT(inEvent != NULL);

    if (mDebugOutput) {
        LOG_INFO("NotificationCenter Manager: postEvent() ----> "
                  << "EventId: "   << inEvent->id);
    }
    
    QThread* currentThread = QThread::currentThread();
    QThread* receiverThread = this->thread();
    if (inPostType == POST_SOON || currentThread != receiverThread) {
#ifdef NC_COALESCE_EVENTS
        mCoalesceList.push_back(qMakePair(inEvent, PRIORITY_NORMAL));
#else        
        // Create the QEvent to send
        NCEvent* ncEvent = new NCEvent(inEvent);

#ifdef DEBUG
        // Stamp the event with the start time
        ncEvent->mTime = QTime::currentTime().elapsed();
#endif    
        QCoreApplication::postEvent(this, ncEvent);
#endif // NC_COALESCE_EVENTS
    } else {
    
        // Create the QEvent to send
        NCEvent* ncEvent = new NCEvent(inEvent);

    #ifdef DEBUG
        // Stamp the event with the start time
        ncEvent->mTime = QTime::currentTime().elapsed();
    #endif    
    
        // Process all events in the queue.
        QCoreApplication::sendPostedEvents();
        
        // Now post the event synchronously and wait for return.
        QCoreApplication::sendEvent(this, ncEvent);

        // Delete the event.
        delete ncEvent;
    }    
}


//-----------------------------------------------------------------------------
// NotificationCenter::postEvent()
//
/// Post an event to the notification center event queue. 
//  The event must be allocated on the heap as the
/// event queue will take ownership of the event. 
/// Events are processed in the order that they are posted.
/// The event priority can be any value between INT_MAX and INT_MIN.
/// \param inEvent A pointer to an event object.
/// \param inPriority The priority in which the event will be handled.
/// \param inPostType The type in which to post the event.
//-----------------------------------------------------------------------------
void
NotificationCenter::postEvent(Event* inEvent, 
                              PostPriority inPriority,
                              PostType inPostType)
{
    Q_ASSERT(inEvent != NULL);
    Q_ASSERT(inPostType != POST_NOW);

    if (mDebugOutput) {
        LOG_INFO("NotificationCenter Manager: postEvent() ----> "
                  << "EventId: "   << inEvent->id
                  << " Priority: " << inPriority
                  << " PostType: " << inPostType);
    }
#ifdef NC_COALESCE_EVENTS
    mCoalesceList.push_back(qMakePair(inEvent, inPriority));
#else
    // Create the QEvent to send
    NCEvent* theEvent = new NCEvent(inEvent);

#ifdef DEBUG
    // Stamp the event with the start time
    theEvent->mTime = QTime::currentTime().elapsed();
#endif    
    QCoreApplication::postEvent(this, theEvent, inPriority);
#endif // NC_COALESCE_EVENTS
}


//-----------------------------------------------------------------------------
// NotificationCenter::connectToQtSlot()
//
/// Connect the event ID to the callback.
/// \param inId The event ID used to make the connection.
/// \param inReceiver The object that owns the slot.
/// \param inSlot The callback to be signalled.
/// \param inName The name (doesn't appear to do anything).
//-----------------------------------------------------------------------------
ConnectionId
NotificationCenter::connect(const EventId& inId, 
                            QObject* inReceiver, 
                            const char* inSlot, 
                            const std::string& inName)
{
    Q_UNUSED(inName);
    
    if (mDebugOutput) {
            LOG_INFO("NotificationCenter::connect() trying to connect qt slot ----> "
                      << "EventId: " << inId
                      << "     "
                      << "StringId: " << inId.getStringId().toStdString());
    }

    // Save the id before we increment it.
    int result = mConnectionIdCount;

    ConnectionInfo& infoRef = addConnectionInfo(CONNECTION_TYPE_QT, inId);

    // Create the event string. We append the event signature to the string
    // used to create the EventId.
    infoRef.qtSignal = inId.getStringId() + kSignalSignature;

    // Store the slot info
    infoRef.qtMethod = inSlot;

    // Store the QObject
    infoRef.qtObject = inReceiver;

    // Try to locate the EventId in the registry.
    EventRegistry::iterator iter = mEventRegistry.find(inId.getHash());
    if (iter == mEventRegistry.end()) {
        addDeferredEvent(inId, infoRef);
    } else {
        if (!connectQtEvent(inId, infoRef)) {

            // Remove the info from the map.
            mConnectionMap.remove(mConnectionIdCount - 1);

            result = NotificationCenter::INVALID_CONNECTION_ID;
        }
    }

    // Send a notification about the connection
    Event* event = new Event(EventConnected);
    event->dictionary["id"] = inId.mStringId;
    event->dictionary["type"] = QString("CONNECTION_TYPE_QT");
    postEvent(event);            

    return result;
}


//-----------------------------------------------------------------------------
// NotificationCenter::connect()
//
/// Connect the event ID to the callback.
/// \param inId The event ID used to make the connection.
/// \param inCallback The callback to be signalled.
/// \param inName The name (doesn't appear to do anything).
//-----------------------------------------------------------------------------
ConnectionId
NotificationCenter::connect(const EventId& inId, 
                            EventCallbackType inCallback, 
                            const std::string& inName)
{
    Q_UNUSED(inName);
    
   if (mDebugOutput) {
            LOG_INFO("NotificationCenter::connect() trying to connect boost callback ----> "
                      << "EventId: " << inId);
    }

    // Save the id before we increment it.
    const int result = mConnectionIdCount;

    // Set up the connection info
    ConnectionInfo& infoRef = addConnectionInfo(CONNECTION_TYPE_BOOST, inId);

    // Try to locate the EventId in the registry.
    EventRegistry::iterator iter = mEventRegistry.find(inId.getHash());
    if (iter == mEventRegistry.end()) {
        infoRef.boostCallbackType = inCallback;
        addDeferredEvent(inId, infoRef);
    } else {
        // This event is located in the registry.  This means it has a good chance of
        // being invoked by someone, so we can connect the signals.

        // Check for and connect any deferred events
        checkForAndConnectDeferredEvents(inId);

        // We will either find a signal or create a new one to connect to.
        EventCallbackRefType signal;

        // Check and see if we already have callbacks attached to this event ID.
        EventMap::const_iterator eventIter = mEvents.find(inId);
        if (eventIter == mEvents.end()) {
            // There are no boost callbacks yet attached.  We need to create the
            // signal to be connected to.
            signal = EventCallbackRefType(new EventCallbackSignal());

            // Add the event to the events map.
            mEvents[inId] = EventCallbackInfo(signal, "", PythonFunctionList());
        } else {
            // We found a signal that was previously created.
            const EventCallbackInfo& callbackInfo = eventIter.value();
            signal = callbackInfo.boostSignal;
        }

        // Attach the callback to the signal
        infoRef.boostId = signal->connect(inCallback);
    }

    if (mDebugOutput) {
        LOG_INFO("NotificationCenter::connect() connecting boost callback ----> "
                  << "EventId:" << inId
                  << "     "
                  << "ConnectionId:" << result);
    }
    
    // Send a notification about the connection
    Event* event = new Event(EventConnected);
    event->dictionary["id"] = inId.mStringId;
    event->dictionary["type"] = QString("CONNECTION_TYPE_BOOST");
    postEvent(event);

    return result;
}


//-----------------------------------------------------------------------------
// NotificationCenter::connect()
//
/// Connect the event ID to the callback.
/// \param inId The string ID used to make the connection.
/// \param inObject The python object to be called.
/// \param inName The name (doesn't appear to do anything).
//-----------------------------------------------------------------------------
ConnectionId NotificationCenter::connect(const QString& inId,
                                         PyObject* inObject,
                                         const std::string& inName)
{
    EventId eId(inId);
    return connect(eId, inObject, inName);
}


//-----------------------------------------------------------------------------
// NotificationCenter::connect()
//
/// Connect the event ID to the callback.
/// \param inId The event ID used to make the connection.
/// \param inObject The python object to be called.
/// \param inName The name (doesn't appear to do anything).
//-----------------------------------------------------------------------------
ConnectionId
NotificationCenter::connect(const EventId& inId, 
                            PyObject* inObject, 
                            const std::string& /*inName*/)
{
    Q_ASSERT(inObject != NULL);

    if (mDebugOutput) {
            LOG_INFO("NotificationCenter::connect() trying to connect python callable ----> "
                      << "EventId: " << inId);
    }

    // Save the id before we increment it.
    int result = mConnectionIdCount;

    // Save the elements need to call the function at a later time
    ConnectionInfo& infoRef = addConnectionInfo(CONNECTION_TYPE_PYTHON, inId);
    infoRef.pythonFunctionInfo = PythonFunctionInfoRef(new PythonFunctionInfo(inObject));

    // Verify that this is a callable object
    if (PyCallable_Check(inObject)) {

        // Try to locate the EventId in the registry.
        EventRegistry::iterator iter = mEventRegistry.find(inId.getHash());
        if (iter == mEventRegistry.end()) {
            addDeferredEvent(inId, infoRef);
        } else {
            // Check and see if we already have objects attached to this event ID.
            EventMap::const_iterator eventIter = mEvents.find(inId);
            if (eventIter == mEvents.end()) {
                // Create an EventId for this object.
                mEvents[inId] = EventCallbackInfo(EventCallbackRefType(new EventCallbackSignal()),
                                                  "",
                                                  PythonFunctionList());
            }

            // Add the puthon object to the list.
            mEvents[inId].pythonFunctionList.push_back(PythonFunctionInfoRef(new PythonFunctionInfo(inObject)));

            // Send a notification about the connection
            Event* event = new Event(EventConnected);
            event->dictionary["id"] = inId.mStringId;
            event->dictionary["type"] = QString("CONNECTION_TYPE_PYTHON");
            postEvent(event);            

            if (mDebugOutput) {
                LOG_INFO("NotificationCenter::connect() connecting python callable ----> "
                          << "EventId:" << inId
                          << "     "
                          << "ConnectionId:" << result
                          << "     "
                          << "count:" << mEvents[inId].pythonFunctionList.size());
            }
        }
    } else {
        // Not callable
        if (mDebugOutput) {
            LOG_ERROR("NotificationCenter::connect() python object not callable ----> "
                      << "EventId:" << inId);
        }

        // Remove the info from the map.
        mConnectionMap.remove(mConnectionIdCount - 1);

        result = NotificationCenter::INVALID_CONNECTION_ID;
    }

    return result;
}


/*
[TODO] How to do a bulk disconnect of all EventId?
//-----------------------------------------------------------------------------
// NotificationCenter::disconnect()
//
/// Disconnect all callbacks attached to the event ID.
/// \param inId The event ID used to disconnect.
//-----------------------------------------------------------------------------
void
NotificationCenter::disconnect(const EventId& inId)
{
    EventCallbackRefType signal;
    EventMap::iterator iter = mEvents.find(inId);
    if (iter != mEvents.end()) {
        signal = iter.value();
        signal->disconnect_all_slots();
    }
}
*/


//-----------------------------------------------------------------------------
// NotificationCenter::disconnect()
//
/// Disconnect all callbacks attached to the event ID.
/// \param inId The connection ID that will be disconnected.
//
// NOTE: If all connections of all types are removed, the event is
// still registered.  Should there be a way to completely unregister
// an event?  I am thinking no, as right now there is no way to keep
// track of who actually registered an event in the first place.
//-----------------------------------------------------------------------------
void
NotificationCenter::disconnect(const ConnectionId& inId)
{
    // Check all the connections, both deferred and live.
    ConnectionMap::iterator iter = mConnectionMap.find(inId);
    if (iter == mConnectionMap.end()) {
        // There is no information for this ConnectionId.
        if (mDebugOutput) {
            LOG_WARN("NotificationCenter::disconnect() Connection information not found ----> "
                     << "ConnectionId:" << inId);
        }
        return;
    }

    const ConnectionInfo& connectionInfo = iter.value();
    Q_ASSERT(inId == connectionInfo.connectionId);

    if (mDebugOutput) {
        LOG_INFO("NotificationCenter::disconnect() "
                 << "EventId:" << connectionInfo.eventId);
    }

    // Check the deferred connection list first
    DeferredEventMap::iterator deferIter = mDeferredEvents.find(connectionInfo.eventId.getHash());
    if (deferIter != mDeferredEvents.end()) {

        DeferredCallbackList& deferredList = deferIter.value();
        DeferredCallbackList::iterator deferredListIter = std::find(deferredList.begin(),
                deferredList.end(),
                &connectionInfo);
        if (deferredListIter != deferredList.end()) {

            // Remove the deferred callback
            deferredList.erase(deferredListIter);

            if (mDebugOutput) {
                LOG_INFO("NotificationCenter::disconnect() removing deferred connection.");
            }
        }
    }


    // [TODO gragan] I think we should be able to check and see if any events were removed from the deferred
    // map. If any were, that means that the event in question has not been registered and we do
    // not need to proceed any further.

    // Get the EventCallbackInfo structure that contains all
    // of the connections related to it.
    EventMap::iterator eventIter = mEvents.find(connectionInfo.eventId);
    if (eventIter == mEvents.end()) {
        // There is no information for this ConnectionId.
        if (mDebugOutput) {
            LOG_WARN("NotificationCenter::disconnect() Event information not found ----> "
                     << "EventId:" << connectionInfo.eventId.mStringId
                     << " "
                     << connectionInfo.connectionId);
        }
        return;
    }

    // Get the event callback info
    EventCallbackInfo& eventCallbackInfo = eventIter.value();

    // Break the connection based on connection type.
    switch (connectionInfo.type) {
    case CONNECTION_TYPE_NONE:
        break;

    case CONNECTION_TYPE_BOOST: {
        if (mDebugOutput) {
            LOG_INFO("NotificationCenter::disconnect() disconnecting boost callback ----> "
                     << "EventId:" << connectionInfo.eventId);
        }

        connectionInfo.boostId.disconnect();
    }
    break;

    case CONNECTION_TYPE_QT: {
        if (mDebugOutput) {
            LOG_INFO("NotificationCenter::disconnect() disconnecting Qt slot ----> "
                     << "EventId:" << connectionInfo.eventId);
        }

        // Find and remove the signal info from the dynamic signal map
        const QByteArray theSignal = QMetaObject::normalizedSignature(connectionInfo.qtSignal.toLatin1());
        SignalTable::iterator iter2 = mQtSignalIndices.find(theSignal);
        if (iter2 != mQtSignalIndices.end()) {

        	// Disconnect the slot from the dynamic signal
			const int signalId = *iter2 + metaObject()->methodCount();
			const int slotId = connectionInfo.qtObject->metaObject()->indexOfMethod(connectionInfo.qtMethod.toLatin1());
			bool result = QMetaObject::disconnect(this,
					    					      signalId,
					    						  connectionInfo.qtObject,
					    						  slotId);		        	
            if (!result) {
	            LOG_WARN("NotificationCenter::disconnect() Unable to disconnect ----> "
	                     << "EventId:" << connectionInfo.eventId.mStringId
	                     << " "
	                     << connectionInfo.connectionId);
			}
			
        	// Remove the signal from the signal map.
            mQtSignalIndices.erase(iter2);
		}
    }
    break;

    case CONNECTION_TYPE_PYTHON: {
        if (mDebugOutput) {
            LOG_INFO("NotificationCenter::disconnect() disconnecting python method ----> "
                     << "EventId:" << connectionInfo.eventId);
        }

        PythonFunctionList::iterator callbackIter = eventCallbackInfo.pythonFunctionList.begin();
        for (; callbackIter != eventCallbackInfo.pythonFunctionList.end(); ++callbackIter) {
            PythonFunctionInfoRef infoRef = *callbackIter;
            if (infoRef->functionMethod == connectionInfo.pythonFunctionInfo->functionMethod) {
                eventCallbackInfo.pythonFunctionList.erase(callbackIter);
                break;
            }
        }
    }
    break;

    default:
        break;
    };

    // Send a notification about the disconnection
    Event* event = new Event(EventDisconnected);
    event->dictionary["id"] = connectionInfo.eventId.mStringId;
    event->dictionary["type"] = connectionTypeToString(connectionInfo.type);
    
    // Remove the info from the connection map
    // note connectionInfo now points to bad memory
    mConnectionMap.erase(iter);

    postEvent(event);
}


//-----------------------------------------------------------------------------
// NotificationCenter::disconnect()
//
/// Disconnect all connections in the list.
/// The list passed in will be cleared on return.
/// \param inList A ConnectionList of connections to disconnect.
//-----------------------------------------------------------------------------
void
NotificationCenter::disconnect(ConnectionList& inList)
{
    ConnectionList::iterator iter = inList.begin();
    for ( ; iter != inList.end(); ++iter) {
        disconnect(*iter); 
    }
    
    inList.clear();
}


//-----------------------------------------------------------------------------
// NotificationCenter::addDeferredEvent()
//
/// Add events for EventId's not yet registered.
/// \param inId The EventId to defer.
//-----------------------------------------------------------------------------
void
NotificationCenter::addDeferredEvent(const EventId& inId, ConnectionInfo& outInfoRef)
{
    if (mDebugOutput) {
        LOG_INFO("NotificationCenter::addDeferredEvent() adding deferred event ----> "
                  << "EventId:" << inId);
    }

    // We were unable to find the event in the active event registry.
    // Check and see if we already have the EventId in the deferred queue.
    DeferredEventMap::iterator deferIter = mDeferredEvents.find(inId.getHash());
    if (deferIter == mDeferredEvents.end()) {
        if (mDebugOutput) {
            LOG_INFO("NotificationCenter::addDeferredEvent() creating deferred event list.");
        }

        DeferredCallbackList callbackList;
        mDeferredEvents[inId.getHash()] = callbackList;
        mDeferredEvents[inId.getHash()].push_back(&outInfoRef);
    }
    else {
        if (mDebugOutput) {
            LOG_INFO("NotificationCenter::addDeferredEvent() found deferred event list.");
        }

        // There is already a deferred event to add the callback to.
        deferIter.value().push_back(&outInfoRef);
    }
}


//-----------------------------------------------------------------------------
// NotificationCenter::addConnectionInfo()
//
/// Set up some boilerplate info based on connection type.
/// \param inType The ConnectionType.
//-----------------------------------------------------------------------------
ConnectionInfo&
NotificationCenter::addConnectionInfo(ConnectionType inType, const EventId& inId)
{
    // Set up the connection info
    ConnectionInfo info;
    info.eventId = inId;
    info.type = inType;
    info.connectionId = mConnectionIdCount;

    // Save the connection info for this connection ID
    mConnectionMap[mConnectionIdCount++] = info;

    // Get a reference back out of the map so we can share the information.
    return mConnectionMap[mConnectionIdCount - 1];
}


//-----------------------------------------------------------------------------
// NotificationCenter::checkForAndConnectDeferredEvents()
//
/// Connect all deferred signals for the EventId
/// \param inId The EventId to connect deferred events to.
//-----------------------------------------------------------------------------
void
NotificationCenter::checkForAndConnectDeferredEvents(const EventId& inId)
{
    // Check and see if the event is in the deferred list. If so, we can
    // remove it and move it over to the active event list.
    DeferredEventMap::iterator deferIter = mDeferredEvents.find(inId.getHash());
    if (deferIter != mDeferredEvents.end()) {

        EventCallbackRefType signal;

        // We found a deferred event. Get the list of callbacks waiting
        // to be connected.
        DeferredCallbackList& callbackList = deferIter.value();

        // Connect the deferred callbacks
        DeferredCallbackList::iterator callbackIter = callbackList.begin();
        for (int index = 0; callbackIter != callbackList.end(); ++callbackIter, ++index) {
            ConnectionInfo* callbackConnectInfo = *callbackIter;

            if (index == 0) {
                // If this is the first instance, we need to create the signal.
                 signal = EventCallbackRefType(new EventCallbackSignal());

                // Add the event to the events map
                mEvents[inId] = EventCallbackInfo(signal, "", PythonFunctionList());

                if (mDebugOutput) {
                    LOG_INFO("NotificationCenter::checkForAndConnectDeferredEvents() found deferred event ----> "
                              << "EventId:" << inId);
                }
            }

            if (callbackConnectInfo->type == CONNECTION_TYPE_BOOST) {
                // Attach the callback to the signal
                callbackConnectInfo->boostId = signal->connect(callbackConnectInfo->boostCallbackType);

                if (mDebugOutput) {
                        LOG_INFO("NotificationCenter::checkForAndConnectDeferredEvents() connecting deferred boost event ----> "
                                  << "EventId:" << inId
                                  << " "
                                  << "ConnectionId:" << callbackConnectInfo->connectionId);
                }
            } else if (callbackConnectInfo->type == CONNECTION_TYPE_QT) {

                if (connectQtEvent(inId, *callbackConnectInfo)) {
                    // Set the slot signature so we can invoke later on.
                    mEvents[inId].qtSlotSignature = callbackConnectInfo->qtSignal;

                    if (mDebugOutput) {
                            LOG_INFO("NotificationCenter::checkForAndConnectDeferredEvents() connecting deferred Qt event ----> "
                                      << "EventId:" << inId
                                      << " "
                                      << "ConnectionId:" << callbackConnectInfo->connectionId);
                    }
                } else {
                    // We failed to make the dynamic Qt connection.
                    if (mDebugOutput) {
                        LOG_ERROR("NotificationCenter::checkForAndConnectDeferredEvents() failed to connect deferred Qt event ----> "
                                  << "EventId:" << inId
                                  << " "
                                  << "ConnectionId:" << callbackConnectInfo->connectionId);
                    }
                }
            } else if (callbackConnectInfo->type == CONNECTION_TYPE_PYTHON) {

                mEvents[inId].pythonFunctionList.push_back(callbackConnectInfo->pythonFunctionInfo);

                if (mDebugOutput) {
                        LOG_INFO("NotificationCenter::checkForAndConnectDeferredEvents() connecting deferred Python event ----> "
                                  << "EventId:" << inId
                                  << " "
                                  << "ConnectionId:" << callbackConnectInfo->connectionId);
                }
            } else {
                if (mDebugOutput) {
                    LOG_WARN("NotificationCenter::checkForAndConnectDeferredEvents() unknown deferred event ----> "
                              << "EventId:" << inId
                              << " "
                              << "ConnectionId:" << callbackConnectInfo->connectionId);
                }
            }
        }

        // Remove the deferred item. It is now in the active list.
        mDeferredEvents.erase(deferIter);
    }
}


//-----------------------------------------------------------------------------
// NotificationCenter::connectQtEvent()
//
/// Connect all deferred Qt signals.
/// \param inInfoRef The ConnectionInfo to connect deferred events to.
/// \result True if connection was made.
//-----------------------------------------------------------------------------
bool
NotificationCenter::connectQtEvent(const EventId& inId, ConnectionInfo& ioInfoRef)
{
    bool result = false;

    if (connectDynamicSignal(ioInfoRef)) {

        if (mDebugOutput) {
            LOG_INFO("NotificationCenter::connectQtEvent() connecting qt slot ----> "
                      << "EventId:" << inId
                      << " "
                      << "StringId:" << inId.getStringId().toStdString());
        }

        // Check and see if we already have slots attached to this event ID.
        EventMap::const_iterator eventIter = mEvents.find(inId);
        if (eventIter == mEvents.end()) {
            // Add the event to the events map
            mEvents[inId] = EventCallbackInfo(EventCallbackRefType(new EventCallbackSignal()),
                                              ioInfoRef.qtSignal,
                                              PythonFunctionList());
        }

        result = true;

    } else {
        if (mDebugOutput) {
            LOG_INFO("NotificationCenter::connectQtEvent() failed ----> "
                      << "EventId:" << inId
                      << " "
                      << "StringId:" << inId.getStringId().toStdString());
        }
    }

    return result;
}


//-----------------------------------------------------------------------------
// NotificationCenter::isValid()
//
/// Check the validity of a connection
/// \param inId The ConnectionInfo to connect to check.
/// \result True if the connection is valid.
//-----------------------------------------------------------------------------
bool
NotificationCenter::isValid(ConnectionId inId) const
{
    ConnectionMap::const_iterator iter = mConnectionMap.find(inId);
    return iter != mConnectionMap.end();
}


//-----------------------------------------------------------------------------
// NotificationCenter::isDeferred()
//
/// Check the deferred status of a connection
/// \param inId The ConnectionInfo to connect to check.
/// \result True if the connection is connected but deferred.
//-----------------------------------------------------------------------------
bool
NotificationCenter::isDeferred(ConnectionId inId) const
{
    ConnectionMap::const_iterator iter = mConnectionMap.find(inId);
    if (iter == mConnectionMap.end())
        return false;

    const ConnectionInfo& connectionInfo = iter.value();

    DeferredEventMap::const_iterator deferIter = mDeferredEvents.find(connectionInfo.eventId.getHash());
    return deferIter != mDeferredEvents.end();
}


//-----------------------------------------------------------------------------
// NotificationCenter::isActive()
//
/// Check the active status of a connection
/// \param inId The ConnectionInfo to connect to check.
/// \result True if the connection is connected and active.
//-----------------------------------------------------------------------------
bool
NotificationCenter::isActive(ConnectionId inId) const
{
    ConnectionMap::const_iterator iter = mConnectionMap.find(inId);
    if (iter == mConnectionMap.end())
        return false;

    const ConnectionInfo& connectionInfo = iter.value();

    EventMap::const_iterator eventIter = mEvents.find(connectionInfo.eventId);
    return eventIter != mEvents.end();
}


//-----------------------------------------------------------------------------
// NotificationCenter::dumpRegisteredEvents()
//
/// Output all registered event IDs to the console.
//-----------------------------------------------------------------------------
void
NotificationCenter::dumpRegisteredEvents() const
{
    EventRegistry::const_iterator iter = mEventRegistry.begin();
    for ( ; iter != mEventRegistry.end(); ++iter) {
        const EventId& eventId = iter.value();
        std::cout << "Registered event: " << eventId.getStringId() << std::endl;

        EventMap::const_iterator eventIter = mEvents.find(eventId);
        if (eventIter != mEvents.end()) {

            ConnectionMap::const_iterator connectIter = mConnectionMap.begin();
            for ( ; connectIter != mConnectionMap.end(); ++connectIter) {
                const ConnectionInfo& info = connectIter.value();
                if (info.eventId == eventId) {

                    // Dump the connection info
                    std::cout << "     connection: "  << std::endl
                              << "          type: " << qPrintable(connectionTypeToString(info.type)) << std::endl
                              << "          id: " << info.connectionId
                              << std::endl;

                    // Add a new line between entries.
                    std::cout << std::endl;
                }
            }
        } else {
            std::cout << "     no connections"  << std::endl;

            // Add a new line between entries.
            std::cout << std::endl;
        }
    }
}


//-----------------------------------------------------------------------------
// NotificationCenter::connectionTypeToString()
//
/// Convert a ConnectionType to a string.
/// \result A human readable string based on the ConnectionType.
//-----------------------------------------------------------------------------
QString
NotificationCenter::connectionTypeToString(ConnectionType inType)
{
    QString typeString;

    switch (inType) 
    {
        case CONNECTION_TYPE_BOOST:
            typeString = "CONNECTION_TYPE_BOOST";
            break;

        case CONNECTION_TYPE_QT:
            typeString = "CONNECTION_TYPE_QT";
            break;

        case CONNECTION_TYPE_PYTHON:
            typeString = "CONNECTION_TYPE_PYTHON";
            break;

        case CONNECTION_TYPE_NONE:
            typeString = "CONNECTION_TYPE_NONE";
            break;

        default:
            typeString = "CONNECTION_TYPE_UNKNOWN";
            break;
    }

    return typeString;
}


//-----------------------------------------------------------------------------
// NotificationCenter::setCoalesceInterval()
//
/// Set the event coalescing interval in milliseconds. Values less than
/// 10 may not be respected.
//-----------------------------------------------------------------------------
void
NotificationCenter::setCoalesceInterval(int inAmount)
{
    mCoalesceInterval = qMax(1, inAmount);
}


//-----------------------------------------------------------------------------
// NotificationCenter::dumpMethods()
//
/// Dump Notification Center metaObject methods.
//-----------------------------------------------------------------------------
void
NotificationCenter::dumpMethods() const
{
	qDebug() << "Notification Center methods: ";
	for (int index = metaObject()->methodOffset(); index < metaObject()->methodCount(); ++index)
		qDebug() << QString::fromLatin1(metaObject()->method(index).signature()); 
	qDebug() << " ";
}


//-----------------------------------------------------------------------------
// NotificationCenter::dumpConnectionMethods()
//
/// Dump ConnectionInfo metaObject methods.
/// \param inInfo The ConnectionInfo to dump methods of.
//-----------------------------------------------------------------------------
void
NotificationCenter::dumpConnectionMethods(const ConnectionInfo& inInfo) const
{
	Q_ASSERT(inInfo.qtObject != NULL);
	
	qDebug() << "Connection Methods: ";
	
	const QMetaObject* metaObject = inInfo.qtObject->metaObject();
	for(int index = metaObject->methodOffset(); index < metaObject->methodCount(); ++index) {
	    const char* methodSignature = metaObject->method(index).signature();	    
	    qDebug() << QString::fromLatin1(metaObject->method(index).signature())
	    		 << "index: " << metaObject->indexOfMethod(methodSignature);
	}		    
	qDebug() << " ";
}


//=============================================================================
// class PythonFunctionInfo
//=============================================================================

//-----------------------------------------------------------------------------
// PythonFunctionInfo::PythonFunctionInfo()
//-----------------------------------------------------------------------------
PythonFunctionInfo::PythonFunctionInfo()
    :   functionMethod(NULL),
        functionSelf(NULL),
        functionClass(NULL)
{
}


//-----------------------------------------------------------------------------
// PythonFunctionInfo::PythonFunctionInfo()
//-----------------------------------------------------------------------------
PythonFunctionInfo::PythonFunctionInfo(PyObject* inCallable)
    :   functionMethod(NULL),
        functionSelf(NULL),
        functionClass(NULL)
{
    Q_ASSERT(inCallable != NULL);

    if (PyMethod_Check(inCallable)) {
        functionMethod = PyMethod_GET_FUNCTION(inCallable);
        functionSelf = PyMethod_GET_SELF(inCallable);
        functionClass = PyMethod_GET_CLASS(inCallable);

        Py_XINCREF(functionMethod);
        Py_XINCREF(functionSelf);
        Py_XINCREF(functionClass);
    }
}


//-----------------------------------------------------------------------------
// PythonFunctionInfo::PythonFunctionInfo()
//-----------------------------------------------------------------------------
PythonFunctionInfo::PythonFunctionInfo(const PythonFunctionInfo& info)
{
    functionMethod = info.functionMethod;
    functionSelf = info.functionSelf;
    functionClass = info.functionClass;

    Py_XINCREF(functionMethod);
    Py_XINCREF(functionSelf);
    Py_XINCREF(functionClass);
}


//-----------------------------------------------------------------------------
// PythonFunctionInfo::PythonFunctionInfo()
//-----------------------------------------------------------------------------
PythonFunctionInfo::~PythonFunctionInfo()
{
    // NOTE: on exit this may be false.  Don't try to clean up
    // If python has already left the building
    if( Py_IsInitialized() ) {
        Py_XDECREF(functionMethod);
        Py_XDECREF(functionSelf);
        Py_XDECREF(functionClass);
    }
}

