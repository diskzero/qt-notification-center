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

#ifndef AF_NC_LOGGING_BEEN_INCLUDED
#define AF_NC_LOGGING_BEEN_INCLUDED

#ifdef USE_LOCAL_LOGGING

#include <iostream>
#include <sstream>

#define LOG_DEBUG(logEvent) \
    do { \
		std::ostringstream _buf; \
		_buf << logEvent; \
		std::cout << "DEBUG: " << logEvent << std::endl; \
    } while (0); 

#define LOG_INFO(logEvent) \
    do { \
		std::ostringstream _buf; \
		_buf << logEvent; \
		std::cout << "INFO: " << logEvent << std::endl; \
    } while (0);
    
#define LOG_WARN(logEvent) \
    do { \
		std::ostringstream _buf; \
		_buf << logEvent; \
		std::cout << "WARNING: " << logEvent << std::endl; \
    } while (0);   

#define LOG_ERROR(logEvent) \
    do { \
		std::ostringstream _buf; \
		_buf << logEvent; \
		std::cout << "ERROR: " << logEvent << std::endl; \
    } while (0);   
#else
#include <logging_base/logging.h>
#endif // USE_LOCAL_LOGGING

#endif // AF_NC_LOGGING_BEEN_INCLUDED

