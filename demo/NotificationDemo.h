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

#ifndef NC_DEMO_HAS_BEEN_INCLUDED
#define NC_DEMO_HAS_BEEN_INCLUDED

// Qt
#include <QDialog>

// Local
#include "../NotificationCenter.h"


//=============================================================================
// class NotificationDemo
//=============================================================================
class NotificationDemo : public QDialog
{
	Q_OBJECT
	
public:
    NotificationDemo();
    virtual ~NotificationDemo();

	void boostCallback(const framework::Event& inEvent);
	void pythonCallback(const framework::Event& inEvent);

public Q_SLOTS:
	void qtCallback(const framework::Event& inEvent);

	void boostClicked();
	void qtClicked();
	void pythonClicked();
	
private:
	framework::NotificationCenter mNotificationCenter;
	framework::ConnectionId mBoostId;
	framework::ConnectionId mQtId;
	framework::ConnectionId mPythonId;

};


#endif // NC_DEMO_HAS_BEEN_INCLUDED
