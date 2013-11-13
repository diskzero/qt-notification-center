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

#ifndef AF_QTFORPYTHON_HAS_BEEN_INCLUDED
#define AF_QTFORPYTHON_HAS_BEEN_INCLUDED

#include <vector>

#include <QVariant>
#include <QHash>
#include <QMap>
#include <QList>
#include <QString>

#include <boost/any.hpp>


// Python
struct _object;
typedef struct _object PyObject;

///Used by the python interface to convert python objects into command args.
void QtForPython_PyObjectToVariant( PyObject* source, QVariant& dest);

/**
   convert to a python object

    This works recursively to process QVariants, maps, and lists of QVariants
    which can be converted 
*/
PyObject*
QtForPython_VariantToPython(const QVariant& source);

/**
   convert a qt map to a py dict
*/
PyObject*
QtForPython_MapToPython(const QMap<QString, QVariant>& source);

/**
   convert a qt hash to a py dict
*/
PyObject*
QtForPython_HashToPython(const QHash<QString, QVariant>& source);

//This may throw if python returns an error
PyObject * QtForPython_StringVecToPyList(const std::vector<std::string>& source);

//This may throw if python returns an error
PyObject * QtForPython_QStringSetToPyList(const QSet<QString>& source);

/**
   Convert a python dictionary to a qhash
 */
void QtForPython_PyObjectToHash(PyObject* src, QHash<QString, QVariant>& dest);

/**
   Checks type and uses boost python extract to convert
   ints,bools,floats and strings.

   This will throw an exception if the type is unknown.
 */
void QtForPython_PyObjectToString( PyObject* source, std::string& dest);

/**
   Attempts to print a QVariant in format usable by python
 */
void QtForPython_VariantToString(const QVariant& source, std::string& dest);


/**
   Checks type and uses boost python extract to convert
   ints,bools,floats and strings.

   This will throw an exception if the type is unknown.
 */
void QtForPython_PyObjectToAny( PyObject* source, boost::any& dest);

#endif
