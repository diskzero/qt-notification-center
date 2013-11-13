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

#ifndef TESTNOTIFICATIONCENTER_H_HAS_BEEN_INCLUDED
#define TESTNOTIFICATIONCENTER_H_HAS_BEEN_INCLUDED

// Local
#include "../NotificationCenter.h"

// Qt
#include <QCoreApplication>
#include <QObject>
#include <QVariant>

// Studio
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestCase.h>
#include <pdevunit/pdevunit.h>

// Constants
static const framework::EventId BoostId("com.mightytoad.ApplicationFramework.TestNotificationCenter.Boost");
static const framework::EventId QtId("com.mightytoad.ApplicationFramework.TestNotificationCenter.Qt");
static const framework::EventId DeferredId("com.mightytoad.ApplicationFramework.TestNotificationCenter.Deferred");

// Local prototypes
static void boostCallback(const framework::Event& inEvent);

// Globals
static framework::ConnectionId gBoostId;
static framework::ConnectionId gBoostDeferredId;
static framework::ConnectionId gQtId;
static framework::NotificationCenter* sNotificationCenter = NULL;


//=============================================================================
// class TestNotificationApp
//=============================================================================
class TestNotificationApp : public QCoreApplication
{
    Q_OBJECT

public:
    TestNotificationApp(int& argc, char** argv)
        :   QCoreApplication(argc, argv)
        ,   mSlotCalled(false)
    {
    }

    virtual ~TestNotificationApp() {}
    
    bool getMessageReceived() const;

    
public Q_SLOTS:

    void testSlot(const framework::Event& inEvent)
    {
        Q_UNUSED(inEvent);
        mSlotCalled = true;
    }

public:
    bool mSlotCalled;    
};



//=============================================================================
// class TestNotificationCenter
//=============================================================================
class TestNotificationCenter : public CppUnit::TestFixture
{
public:
    TestNotificationCenter() 
        :   mTestValue(false)
    {
        if (sNotificationCenter == NULL)
            sNotificationCenter = new framework::NotificationCenter();
    }
    
    void 
    setUp() 
    {
    }
    
    void 
    tearDown() 
    {
    }

    void 
    testEventRegistration() 
    {        
        static bool registerOnce = true;

        if (registerOnce) {
            registerOnce = false;
            
            const int beforeCount = sNotificationCenter->registeredEventCount();

            sNotificationCenter->registerEvent(BoostId);
            sNotificationCenter->registerEvent(QtId);
            QCoreApplication::processEvents();

            const int afterCount = sNotificationCenter->registeredEventCount();

            // The after count should be the same as the before count + the new event.
            CPPUNIT_ASSERT_EQUAL_MESSAGE("test event registration", 
                                         beforeCount + 2,
                                         afterCount);
        }
    }

    void 
    testDuplicateEventRegistration() 
    {
        static bool duplicatedOnce = true;

        if (duplicatedOnce) {
            duplicatedOnce = false;

            const int curCount = sNotificationCenter->registeredEventCount();

            sNotificationCenter->registerEvent(BoostId);
            QCoreApplication::processEvents();

            // Make sure the registered count is the same.
            CPPUNIT_ASSERT_EQUAL_MESSAGE("test duplicate event registration", 
                                         curCount,
                                         sNotificationCenter->registeredEventCount());
        }
    }

    void 
    testDeferredEventRegistration() 
    {
        static bool deferredOnce = true;

        if (deferredOnce) {
            deferredOnce = false;

            const int deferredCount = sNotificationCenter->deferredEventCount();

            gBoostDeferredId = sNotificationCenter->connect(DeferredId, boostCallback);
            QCoreApplication::processEvents();

            CPPUNIT_ASSERT_EQUAL_MESSAGE("test deferred event registration", 
                                         deferredCount + 1,
                                         sNotificationCenter->deferredEventCount());
        }
    }

    void 
    testQtEventPosting() 
    {
        TestNotificationApp* testApp = qobject_cast<TestNotificationApp*>(qApp);
        CPPUNIT_ASSERT(testApp != NULL);

        testApp->mSlotCalled = false;

        gQtId = sNotificationCenter->connect(QtId, 
                                             testApp, 
                                             "testSlot(framework::Event)");

        sNotificationCenter->postEvent(new framework::Event(QtId));
        testApp->processEvents();

        CPPUNIT_ASSERT_EQUAL_MESSAGE("test qt event posting", 
                                     true, 
                                     testApp->mSlotCalled);    
    }


    void 
    testQtEventSending() 
    {
        TestNotificationApp* testApp = qobject_cast<TestNotificationApp*>(qApp);
        CPPUNIT_ASSERT(testApp != NULL);

        testApp->mSlotCalled = false;

        sNotificationCenter->postEvent(QtId, framework::NotificationCenter::POST_NOW);
        QCoreApplication::processEvents();

        CPPUNIT_ASSERT_EQUAL_MESSAGE("test qt event sending", 
                                     true, 
                                     testApp->mSlotCalled);    
    }


    void 
    testQtEventDisconnect() 
    {
        TestNotificationApp* testApp = qobject_cast<TestNotificationApp*>(qApp);
        CPPUNIT_ASSERT(testApp != NULL);

        testApp->mSlotCalled = false;

        sNotificationCenter->disconnect(gQtId);


        framework::Event* event = new framework::Event(QtId);
        event->dictionary["test"] = qVariantFromValue((void *) this);

        sNotificationCenter->postEvent(event, framework::NotificationCenter::POST_NOW);
        QCoreApplication::processEvents();

        CPPUNIT_ASSERT_EQUAL_MESSAGE("test qt event disconnect", 
                                     false, 
                                     testApp->mSlotCalled);    
    }
 	
    void 
    testBoostEventPosting() 
    {
        mTestValue = false;
        
        gBoostId = sNotificationCenter->connect(BoostId, boostCallback);

        framework::Event* event = new framework::Event(BoostId);
        event->dictionary["test"] = qVariantFromValue((void *) this);

        sNotificationCenter->postEvent(event);
        QCoreApplication::processEvents();

        CPPUNIT_ASSERT_EQUAL_MESSAGE("test boost event posting", 
                                     true, 
                                     mTestValue);
    }

    void 
    testBoostEventSending() 
    {
        mTestValue = false;
        
        framework::Event* event = new framework::Event(BoostId);
        event->dictionary["test"] = qVariantFromValue((void *) this);

        sNotificationCenter->postEvent(event, framework::NotificationCenter::POST_NOW);
        QCoreApplication::processEvents();

        CPPUNIT_ASSERT_EQUAL_MESSAGE("test boost event sending", 
                                     true, 
                                     mTestValue);    
    }

    void 
    testBoostEventDisconnect() 
    {
        mTestValue = false;

        sNotificationCenter->disconnect(gBoostId);

        framework::Event* event = new framework::Event(BoostId);
        event->dictionary["test"] = qVariantFromValue((void *) this);

        sNotificationCenter->postEvent(event, framework::NotificationCenter::POST_NOW);
        QCoreApplication::processEvents();

        CPPUNIT_ASSERT_EQUAL_MESSAGE("test boost event disconnect", 
                                     false, 
                                     mTestValue);    
    }

    void 
    testEventIsDeferred() 
    {
        const bool isDeferred = sNotificationCenter->isDeferred(gBoostDeferredId);
        QCoreApplication::processEvents();

        CPPUNIT_ASSERT_EQUAL_MESSAGE("test event is deferred", 
                                     true, 
                                     isDeferred);    
    }

    void 
    testEventIsNotDeferred() 
    {
        const bool isDeferred = sNotificationCenter->isDeferred(gBoostId);
        QCoreApplication::processEvents();

        CPPUNIT_ASSERT_EQUAL_MESSAGE("test event is not deferred", 
                                     false, 
                                     isDeferred);    
    }

    void 
    testEventIsActive() 
    {
        const bool isActive = sNotificationCenter->isActive(gBoostDeferredId);
        QCoreApplication::processEvents();

        CPPUNIT_ASSERT_EQUAL_MESSAGE("test event is active", 
                                     false, 
                                     isActive);    
    }

    void 
    testEventIsNotActive() 
    {
        const bool isActive = sNotificationCenter->isActive(gBoostId);
        QCoreApplication::processEvents();

        CPPUNIT_ASSERT_EQUAL_MESSAGE("test event is not active", 
                                     false, 
                                     isActive);    
    }


    
    void toggleTestValue();

    // Set up the unit tests
    CPPUNIT_TEST_SUITE(TestNotificationCenter);

    CPPUNIT_TEST(testEventRegistration);
   	CPPUNIT_TEST(testDuplicateEventRegistration);
    CPPUNIT_TEST(testDeferredEventRegistration);

    CPPUNIT_TEST(testQtEventPosting);
    CPPUNIT_TEST(testQtEventSending);
    CPPUNIT_TEST(testQtEventDisconnect);
	
    CPPUNIT_TEST(testBoostEventPosting);
    CPPUNIT_TEST(testBoostEventSending);
    CPPUNIT_TEST(testBoostEventDisconnect);

	CPPUNIT_TEST(testEventIsDeferred);
	CPPUNIT_TEST(testEventIsNotDeferred);
	CPPUNIT_TEST(testEventIsActive);
	CPPUNIT_TEST(testEventIsNotActive);

    
    CPPUNIT_TEST_SUITE_END();

private:
    bool mTestValue;
};

inline void TestNotificationCenter::toggleTestValue() { mTestValue = !mTestValue; }


//=============================================================================
// boostCallback
//=============================================================================
void
boostCallback(const framework::Event& inEvent)
{
    if (inEvent.dictionary.contains("test")) {
        TestNotificationCenter* testCenter = (TestNotificationCenter *)inEvent.dictionary.value("test").value<void *>();
        assert(testCenter != NULL);
        testCenter->toggleTestValue();
    }    
}

// Register this test for execution
CPPUNIT_TEST_SUITE_REGISTRATION(TestNotificationCenter);


#endif // TESTNOTIFICATIONCENTER_H_HAS_BEEN_INCLUDED

