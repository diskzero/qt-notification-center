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

#ifndef _PYTHON_GIL_GIL_STATE_H
#define _PYTHON_GIL_GIL_STATE_H

// Python
#include <Python.h>

namespace python_gil {

/*!
 * @brief Wrap Python GIL State acquisition/release to ensure RAII
 *
 * This class provides a wrapper around the PyGILState_Ensure/PyGILState_Release
 * calls, to allow the Release to happen automatically when instances
 * of this object go out of scope.
 *
 * Calls to the Python API can be prefaced by creating an instance of
 * this class:
 *
 * @code
 * python_gil::GilState gilstate;
 * // Python Calls
 * @endcode
 *
 * While the GIL State is released when the object is destroyed, you
 * can explicitly release the state by calling GilState::release(). There
 * is no way to reacquire the GIL State after releasing in this way.
 *
 */
class GilState
{
public:
    //! Constructor automatically gets the GIL State
    GilState();
    //! Destructor automatically releases the GIL State
    ~GilState();

    /**
       Release the GIL State. There is no way to reacquire
       the GIL state on an instance of this class after releasing, 
    */
    void release();
    
private:
    PyGILState_STATE mState;
    bool             mIsLocked;

};

} // namespace python_gil

#endif // _PYTHON_GIL_GIL_STATE_H

