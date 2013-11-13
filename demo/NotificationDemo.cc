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
#include "NotificationDemo.h"

// Qt
#include <QHBoxLayout>
#include <QPushButton>
#include <QtDebug>
#include <QVBoxLayout>

// Local
#include "../BindToEvent.h"
#include "NotificationLogging.h"


// Namespaces
using namespace framework;


//=============================================================================
// class NotificationDemo
//=============================================================================

// Constants
static const EventId sBoostId("com.mightytoad.NotificationDemo.boost");
static const EventId sQtId("com.mightytoad.NotificationDemo.qt");
static const EventId sPythonId("com.mightytoad.NotificationDemo.python");

static const int kTestCount = 1;

//-----------------------------------------------------------------------------
// NotificationDemo::NotificationDemo()
//-----------------------------------------------------------------------------
NotificationDemo::NotificationDemo()
	:	QDialog()
{
	// Register the events
	mNotificationCenter.registerEvent(sBoostId);
	mNotificationCenter.registerEvent(sQtId);
	mNotificationCenter.registerEvent(sPythonId);
	
	// Make the connections
    mBoostId = mNotificationCenter.connect(
    		sBoostId,
			BindToEvent(NotificationDemo::boostCallback, this));

	mQtId = mNotificationCenter.connect(sQtId, 
                              			this, 
                              			"qtCallback(const framework::Event&)"); 
		
	// Create the layouts
	QVBoxLayout* vBoxLayout = new QVBoxLayout(this);
	QHBoxLayout* hBoxLayout = new QHBoxLayout();
	vBoxLayout->addLayout(hBoxLayout);
	
	// Create the buttons
	QPushButton* boostButton = new QPushButton("boost");
	boostButton->setAutoDefault(false);
	boostButton->setDefault(false);
	connect(boostButton,
			SIGNAL(clicked()),
			this,
			SLOT(boostClicked()));
	hBoxLayout->addWidget(boostButton);

	QPushButton* qtButton = new QPushButton("Qt");
	qtButton->setAutoDefault(false);
	qtButton->setDefault(false);
	connect(qtButton,
			SIGNAL(clicked()),
			this,
			SLOT(qtClicked()));
	hBoxLayout->addWidget(qtButton);

	QPushButton* pythonButton = new QPushButton("python");
	pythonButton->setAutoDefault(false);
	pythonButton->setDefault(false);
	connect(pythonButton,
			SIGNAL(clicked()),
			this,
			SLOT(pythonClicked()));
	hBoxLayout->addWidget(pythonButton);
	
	
	resize(500, 200);
}


//-----------------------------------------------------------------------------
// NotificationDemo::~NotificationDemo()
//-----------------------------------------------------------------------------
NotificationDemo::~NotificationDemo()
{
	mNotificationCenter.disconnect(mBoostId);
	mNotificationCenter.disconnect(mQtId);
	mNotificationCenter.disconnect(mPythonId);
}


//-----------------------------------------------------------------------------
// NotificationDemo::boostCallback()
//-----------------------------------------------------------------------------
void
NotificationDemo::boostCallback(const Event& inEvent)
{
	Q_UNUSED(inEvent);

	if (inEvent.dictionary.contains("order"))
		qDebug() << "boostCallback: " << inEvent.dictionary["order"].toInt();	
}


//-----------------------------------------------------------------------------
// NotificationDemo::qtCallback()
//-----------------------------------------------------------------------------
void
NotificationDemo::qtCallback(const Event& inEvent)
{
	if (inEvent.dictionary.contains("order"))
		qDebug() << "qtCallback: " << inEvent.dictionary["order"].toInt();	
}


//-----------------------------------------------------------------------------
// NotificationDemo::pythonCallback()
//-----------------------------------------------------------------------------
void
NotificationDemo::pythonCallback(const Event& inEvent)
{
	Q_UNUSED(inEvent);

	qDebug() << "pythonCallback";
}


//-----------------------------------------------------------------------------
// NotificationDemo::boostClicked()
//-----------------------------------------------------------------------------
void
NotificationDemo::boostClicked()
{
    for (int index = 0; index < kTestCount; ++index) {
    	Event* event = new Event(sBoostId);
    	event->dictionary["order"] = 1;
    	mNotificationCenter.postEvent(event, NotificationCenter::PRIORITY_LOW);
    
    	event = new Event(sBoostId);
    	event->dictionary["order"] = 2;	
    	mNotificationCenter.postEvent(event, NotificationCenter::PRIORITY_HIGH);
    
    	event = new Event(sBoostId);
    	event->dictionary["order"] = 3;	
    	mNotificationCenter.postEvent(event, NotificationCenter::PRIORITY_NORMAL);
    
    	event = new Event(sBoostId);
    	event->dictionary["order"] = 4;	
    	mNotificationCenter.postEvent(event);
    
    	event = new Event(sBoostId);
    	event->dictionary["order"] = 5;	
    	mNotificationCenter.postEvent(event);
    }
}


//-----------------------------------------------------------------------------
// NotificationDemo::qtClicked()
//-----------------------------------------------------------------------------
void
NotificationDemo::qtClicked()
{
    for (int index = 0; index < kTestCount; ++index) {
    	Event* event = new Event(sQtId);
    	event->dictionary["order"] = 1;
    	mNotificationCenter.postEvent(event, NotificationCenter::PRIORITY_LOW);
    
    	event = new Event(sQtId);
    	event->dictionary["order"] = 2;	
    	mNotificationCenter.postEvent(event, NotificationCenter::PRIORITY_HIGH);
    
    	event = new Event(sQtId);
    	event->dictionary["order"] = 3;	
    	mNotificationCenter.postEvent(event, NotificationCenter::PRIORITY_NORMAL);
    
    	event = new Event(sQtId);
    	event->dictionary["order"] = 4;	
    	mNotificationCenter.postEvent(event);
    
    	event = new Event(sQtId);
    	event->dictionary["order"] = 5;	
    	mNotificationCenter.postEvent(event);
    
    	event = new Event(sQtId);
    	event->dictionary["order"] = 6;	
    	mNotificationCenter.postEvent(event, NotificationCenter::POST_NOW);
    }
}


//-----------------------------------------------------------------------------
// NotificationDemo::pythonClicked()
//-----------------------------------------------------------------------------
void
NotificationDemo::pythonClicked()
{
	mNotificationCenter.postEvent(sPythonId);
}
