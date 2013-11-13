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

CONFIG	+=	qt ordered no_keywords

SOURCES += 	main.cc \
		    ../NotificationCenter.cc \
            NotificationDemo.cc \
		    		    
HEADERS +=	../BindToEvent.h \
			../NotificationCenter.h \
			../NotificationLogging.h \
		    NotificationDemo.h \
		    
OBJECTS_DIR = ./obj

MOC_DIR = ./moc

macx {
CONFIG 		-= 	app_bundle

DEFINES 	+= 	USE_LOCAL_LOGGING \
                DISABLE_PYTHON

INCLUDEPATH	+=	/System/Library/Frameworks/Carbon.framework/Headers \
				/usr/local/include \
				/usr/local/include/boost \
			   	/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7 \
			   	../ \
			   	../../ \
				../../../../../rnd/lib \
				../../../../lib/ \

LIBS		=	-framework Carbon \
				-L/usr/local/lib \
   				-lboost_filesystem \
   				-lboost_python \
   				-lboost_regex \
   				-lboost_serialization \
   				-lboost_signals \
   				-lboost_system \
   				-lboost_thread \
				-lpython \
}
