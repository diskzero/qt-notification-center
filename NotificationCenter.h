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

#ifndef AF_NC_HAS_BEEN_INCLUDED
#define AF_NC_HAS_BEEN_INCLUDED

// Boost
#include <boost/signals.hpp>

// Qt
#include <QEvent>
#include <QHash>
#include <QList>
#include <QMap>
#include <QObject>
#include <QTime>
#include <QVariant>


// Python
struct _object;
typedef struct _object PyObject;

namespace framework {

// Forward declarations
class NotificationCenter;

//=============================================================================
// class EventId
//
// This contains the identifiers needed to construct an Event.
// Usually, these will be constructed statically and then registered
// with the Notification Center.
//=============================================================================
class EventId
{
public:
    EventId();
    EventId(const QString& inId);
    EventId(const EventId& inFrom);

    bool operator ==(const EventId& other) const;
    bool operator !=(const EventId& other) const;
    bool operator < (const EventId& other) const;
    unsigned int getHash() const;
    int getEventType() const;
    const QString& getStringId() const;

private:
    friend class NotificationCenter;
    void registerSelf();

    QString mStringId;
    unsigned int mCrc32;
    int mEventType;
};

//=============================================================================
// class inlines
//=============================================================================
inline bool EventId::operator ==(const EventId& other) const { return mCrc32 == other.mCrc32; }
inline bool EventId::operator !=(const EventId& other) const { return mCrc32 != other.mCrc32; }
inline bool EventId::operator < (const EventId& other) const { return mCrc32 < other.mCrc32; }
inline unsigned int EventId::getHash() const { return mCrc32; }
inline int EventId::getEventType() const { return mEventType; }
inline const QString& EventId::getStringId() const { return mStringId; }


//=============================================================================
// class Event
//
// This represents the event that is generated passed to all interested
// listeners.  It contains a basic data dictionary payload.  It can be
// subclassed to provide additional data and functionality.
//=============================================================================
typedef QHash<QString, QVariant> EventDictionary;

class Event
{
public:
    Event(const EventId& inId);
    virtual ~Event() {}

    EventId id;
    EventDictionary dictionary;
};

//=============================================================================
// Event Notification
//=============================================================================
typedef boost::signal<void (const Event&)> EventCallbackSignal;
typedef EventCallbackSignal::slot_function_type EventCallbackType;
typedef boost::shared_ptr<EventCallbackSignal> EventCallbackRefType;
typedef unsigned int ConnectionId;
typedef boost::signals::connection Connection;
typedef QList<ConnectionId> ConnectionList;

//=============================================================================
// struct EventCallback
//=============================================================================
struct EventCallback
{
    EventCallback(EventCallbackType inCallback);
    EventCallback(const EventCallback& inCallback);

    void operator()(const Event& inEvent);

    EventCallbackType mCallback;
};


//=============================================================================
// struct PythonFunctionInfo
//=============================================================================
/** Stores information needed to recreate a python function.    
    Used internally by NotificationCenter.
 */
struct PythonFunctionInfo
{
    PythonFunctionInfo();

    PythonFunctionInfo(PyObject* inCallable);

    PythonFunctionInfo(const PythonFunctionInfo& info);
    ~PythonFunctionInfo();

    inline bool isValid() const { return (functionMethod != NULL && functionSelf != NULL && functionClass != NULL);  }

    PyObject* functionMethod;
    PyObject* functionSelf;
    PyObject* functionClass;
};

typedef boost::shared_ptr<PythonFunctionInfo> PythonFunctionInfoRef;
typedef QList<PythonFunctionInfoRef> PythonFunctionList;


//=============================================================================
// enum ConnectionType
//
// The types of connections that the Notification Center supports.
//=============================================================================
enum ConnectionType
{
    CONNECTION_TYPE_NONE        = 0,
    CONNECTION_TYPE_BOOST       = 1,
    CONNECTION_TYPE_QT          = 2,
    CONNECTION_TYPE_PYTHON      = 4
};


//=============================================================================
// struct ConnectionInfo
//=============================================================================
/** Internal data structure used by the Notification Center.
    Handles the housekeeping of maintaining various signalling architectures.
 */
struct ConnectionInfo
{
    ConnectionInfo()
        :   type(CONNECTION_TYPE_NONE),
            connectionId(static_cast<ConnectionId>(-1)),
            qtObject(NULL)
    {
    }

    // Connection state
    EventId eventId;
    ConnectionType type;
    ConnectionId connectionId;

    // boost::signal info
    Connection boostId;
    EventCallbackType boostCallbackType;

    // Qt info
    QString qtSignal;
    QString qtMethod;
    QObject* qtObject;

    // Python info
    PythonFunctionInfoRef pythonFunctionInfo;
};

/**<
 * @class EventRegistry
 * @brief Map of all registered events.
 */
typedef QMap<unsigned int, EventId> EventRegistry;



/**<
 * @class DeferredCallbackList
 * @brief List of deferred EventCallbackTypes to register.
 */
typedef QList<ConnectionInfo*> DeferredCallbackList;


/**<
 * @class DeferredEventMap
 * @brief Map of events waiting to be connected.
 */
typedef QMap<unsigned int, DeferredCallbackList> DeferredEventMap;


/**<
 * @class SignalTable
 * @brief Map of signals.
 */
typedef QHash<QByteArray, int> SignalTable;

/**<
 * @class EventCallbackInfo
 * @brief boost::signal and qt::signal information.
 */
struct EventCallbackInfo
{
    EventCallbackInfo() {}

    EventCallbackInfo(EventCallbackRefType inBoostSignal,
                      const QString& inSlotSignature,
                      const PythonFunctionList& inPythonFunctionList)
        :   boostSignal(inBoostSignal),
            qtSlotSignature(inSlotSignature),
            pythonFunctionList(inPythonFunctionList)
    {
    }

    EventCallbackRefType boostSignal;
    QString qtSlotSignature;
    PythonFunctionList pythonFunctionList;
};


/**<
 * @class EventMap
 * @brief Map of all connected callabacks based on the EventId.
 */
typedef QMap<EventId, EventCallbackInfo> EventMap;


/**<
 * @class ConnectionMap
 * @brief Map of connection information based on connection id's.
 */
typedef QMap<ConnectionId, ConnectionInfo> ConnectionMap;


// Default name given to unanmed connections
static const std::string DEFAULT_CALLBACK_NAME("unknown");


//=============================================================================
// class NotificationCenter
//=============================================================================
/*!
    An event registration and notification library.
    This class allows the registration events and event suites and the
    connection of interested listeners to those events.  Events and
    listeners may be registered
*/
class NotificationCenter : public QObject
{
public:
    static framework::EventId EventRegistered;
    static framework::EventId EventConnected;
    static framework::EventId EventDisconnected;

    static ConnectionId INVALID_CONNECTION_ID;

    typedef QSet<QString> EventIdSet;

    enum PostType {
        POST_SOON,
        POST_NOW
    };

    enum PostPriority {
        PRIORITY_LOW = -1,
        PRIORITY_NORMAL = 0,
        PRIORITY_HIGH = 1
    };
    
    NotificationCenter();
    virtual ~NotificationCenter();

    // QObject
    virtual bool event(QEvent* inEvent);

    // Event registration
    bool registerEvent(const EventId& inEventId);
    EventIdSet registeredEvents() const;

    // Event dispatching
    void postEvent(const EventId& inId, PostType inPostType = POST_SOON);
    void postEvent(const EventId& inId, PostPriority inPriority, PostType inPostType = POST_SOON);
    void postEvent(Event* inEvent, PostType inPostType = POST_SOON);
    void postEvent(Event* inEvent, PostPriority inPriority, PostType inPostType = POST_SOON);

    // Connection management
	ConnectionId connect(const EventId& inId, QObject* inReceiver, const char* inSlot, const std::string& inName = DEFAULT_CALLBACK_NAME);
    ConnectionId connect(const EventId& inId, EventCallbackType inCallback, const std::string& inName = DEFAULT_CALLBACK_NAME);
    ConnectionId connect(const EventId& inId, PyObject* inObject, const std::string& inName = DEFAULT_CALLBACK_NAME);
    ConnectionId connect(const QString& inId, PyObject* inObject, const std::string& inName = DEFAULT_CALLBACK_NAME);

    void disconnect(const ConnectionId& inId);
    void disconnect(ConnectionList& inList);

    int getCoalesceInterval() const;
    void setCoalesceInterval(int inAmount);

    // Diagnostics
    int registeredEventCount() const;
    int deferredEventCount() const;

    bool isValid(ConnectionId inId) const;
    bool isDeferred(ConnectionId inId) const;
    bool isActive(ConnectionId inId) const;

    // Introspection
    const EventRegistry& getEventRegistry() const;
    void dumpRegisteredEvents() const;

    static QString connectionTypeToString(ConnectionType inType);

protected:
    virtual void timerEvent(QTimerEvent* inEvent);
        
private:
    // No copying allowed
    NotificationCenter(const NotificationCenter& theValue);
    NotificationCenter& operator=(const NotificationCenter& theValue);

    // Dynamic signal and slot handling
    bool connectDynamicSignal(ConnectionInfo& ioInfo);
    bool emitDynamicSignal(const QString& inSignal, void** inArgs);

    bool handleCustomEvent(QEvent* inEvent);

    ConnectionInfo& addConnectionInfo(ConnectionType inType, const EventId& inId);

    void addDeferredEvent(const EventId& inId, ConnectionInfo& outInfoRef);
    void checkForAndConnectDeferredEvents(const EventId& inId);
    bool connectQtEvent(const EventId& inId, ConnectionInfo& ioInfoRef);
	
	void dumpMethods() const;
	void dumpSignals() const;
	void dumpConnectionMethods(const ConnectionInfo& inInfo) const;

    typedef QPair<Event*, PostPriority> EventPriorityPair;
	typedef QList<EventPriorityPair> EventList;

    // [TODO] We want to protect all of these with a mutex
    EventRegistry mEventRegistry;               // Contains all registered EventIds the Notification Center is aware of
    EventMap mEvents;
    DeferredEventMap mDeferredEvents;
    SignalTable mQtSignalIndices;
    ConnectionId mConnectionIdCount;            // Not an index, but a running count.
    ConnectionMap mConnectionMap;
    EventList mCoalesceList;
    int mCoalesceInterval;
    int mTimerId;
    
    bool mDebugOutput;

};

// Inlines
inline int NotificationCenter::registeredEventCount() const { return mEventRegistry.size(); }
inline int NotificationCenter::deferredEventCount() const { return mDeferredEvents.size(); }
inline const EventRegistry& NotificationCenter::getEventRegistry() const { return mEventRegistry; }
inline int NotificationCenter::getCoalesceInterval() const { return mCoalesceInterval; }
    
} // namespace framework

#endif // AF_NC_HAS_BEEN_INCLUDED

