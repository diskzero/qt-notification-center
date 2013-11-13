%Module studio.notification_center 0

%Import QtGui/QtGuimod.sip
%Import QtCore/QtCoremod.sip


//=============================================================================
// class EventId
//=============================================================================
class EventId
{

%TypeHeaderCode
#include <notification_center/NotificationCenter.h>
using namespace framework;
%End

public:
    EventId(const QString& stringId);     
    EventId(const EventId& inFrom);
 
    unsigned int getHash() const;
    int getEventType() const;
    
};


//=============================================================================
// class Event
//=============================================================================
class Event
{
%TypeHeaderCode
#include <notification_center/NotificationCenter.h>
using namespace framework;
%End

public:
    Event(const EventId& inId);
};


//=============================================================================
// Event Notification
//=============================================================================
typedef unsigned int ConnectionId;


//=============================================================================
// class NotificationCenter
//=============================================================================
class NotificationCenter : QObject
{

%TypeHeaderCode
#include <notification_center/NotificationCenter.h>
using namespace framework;
%End

public:
    enum PostType {
        POST_SOON,
        POST_NOW
    };

    enum PostPriority {
        PRIORITY_LOW = -1,
        PRIORITY_NORMAL = 0,
        PRIORITY_HIGH = 1
    };

    // Event registration
    void registerEvent(EventId& inEventID);

    // Event dispatching
    void postEvent(Event* inEvent, PostType inPostType = POST_SOON);
    void postEvent(Event* inEvent, PostPriority inPriority, PostType inPostType = POST_SOON);
    void postEvent(const EventId& inId /Transfer/, PostType inPostType = POST_SOON);
    void postEvent(const EventId& inId /Transfer/, PostPriority inPriority, PostType inPostType = POST_SOON);

    // Event connection
    ConnectionId connect(const EventId& inID, SIP_PYOBJECT inObject);  
%MethodCode
        // We have to manually do this step, otherwise the object will
        // not be properly callable.  It is somehow related to the
        // python threading macros.
        //Py_BEGIN_ALLOW_THREADS
        sipRes = sipCpp->connect(*a0, a1);
        //Py_END_ALLOW_THREADS
%End
    void disconnect(const ConnectionId& inID);

private:
    NotificationCenter(const NotificationCenter& command); 
};


