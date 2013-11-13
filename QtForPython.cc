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
#include "QtForPython.h"

// Qt
#include <QDebug>
#include <QStringList>

// System
#include <boost/python/list.hpp>
#include <boost/python/extract.hpp>
#include <boost/shared_ptr.hpp>

// Studio
#include <command_manager/CommandUtils.h>
#include <except/except.h>

// Local
#include "NotificationLogging.h"


//local class for conversion to a python dictionary
namespace
{
//Local functor to assist in crating a python list
//from string in another type of collection.
//user is responsible for the python ref count
struct AccumToPyList {
    AccumToPyList() :
        mPyList(new PyObject*)
    {
        *mPyList = PyList_New( 0 );
        if ( *mPyList == NULL) {
            LOG_ERROR("Failed create python object.");
            PyErr_SetString(PyExc_RuntimeError, "Failed to create python list");
        }
    }
    
    ~AccumToPyList() {
    }

    //insert a given pyobject into a list
    inline void
    appendPyObject(PyObject* pyObj) {
        if( *mPyList == NULL ) {
            Py_XDECREF(pyObj);
            //An error occured earlier. We can not add to list.
            //release this object.
            return;
        }
        
        if ( pyObj == NULL ) {
            Py_DECREF(*mPyList);
            *mPyList = NULL;
            if( PyErr_Occurred() == NULL) {
                LOG_ERROR("Failed convert python object.");
                PyErr_SetString(PyExc_RuntimeError, "Failed to convert python object");
            }
            return;
        }
        
        if ( PyList_Append(*mPyList, pyObj) == -1 ) {
            Py_DECREF(pyObj);
            Py_DECREF(*mPyList);
            *mPyList = NULL;
            LOG_ERROR("Failed modify python list. throwing exception");
            PyErr_SetString(PyExc_RuntimeError, "Failed to modify python list");
        }
    }

    inline void
    appendCString(const char * source) {
        PyObject* pyString = PyString_FromString( source );
        appendPyObject( pyString );
    }

    inline void
    operator()(const QString& source) {
        if ( source.isEmpty() ) {
            return;
        }
        appendCString( source.toLocal8Bit().constData() );
    }

    inline void
    operator()(const std::string& source) {
        if ( source.empty() ) {
            return;
        }
        appendCString( source.c_str() );
    }

    inline void
    operator()(const QVariant& source) {
        PyObject* pyObj = QtForPython_VariantToPython(source);
        appendPyObject( pyObj );
    }

    inline void
    operator()(const double& source) {
        PyObject* pyObj = PyFloat_FromDouble(source);
        appendPyObject( pyObj );
    }

    inline void
    operator()(const uint& source) {
        PyObject* pyObj = PyInt_FromLong(source);
        appendPyObject( pyObj );
    }
    
    //shared pointer use so that copies of this class will access same
    //memory location
    boost::shared_ptr<PyObject*> mPyList;
    bool mError;
};

struct AccumToPyDict {
    AccumToPyDict() :
        mPyDict(new PyObject*)
    {

        *mPyDict = PyDict_New();
        if ( *mPyDict == NULL) {
            LOG_ERROR("Failed create python object.");
            PyErr_SetString(PyExc_RuntimeError, "Failed to create python dict");
        }
    }

    ~AccumToPyDict() {
    }
    
    template <class T>
    inline void
    operator()(const T& source) {
        if( *mPyDict == NULL ) {
            //An error occured earlier. We can not add to list.
            //release this object.
            return;
        }

        QString key = source.key();
        QVariant value = source.value();

        PyObject* pyKey = PyString_FromString( key.toLocal8Bit().constData()  );
        if ( pyKey == NULL ) {
            Py_DECREF(*mPyDict);
            *mPyDict = NULL;
            LOG_ERROR("Failed create python object.");
            PyErr_SetString(PyExc_RuntimeError, "Failed to create python object");
            return;
        }

        PyObject* pyValue = QtForPython_VariantToPython(value);
        if ( pyValue == NULL ) {
            Py_DECREF(pyKey);
            Py_DECREF(*mPyDict);
            *mPyDict = NULL;
            if( PyErr_Occurred() == NULL) {
                LOG_ERROR("Failed convert python object.");
                PyErr_SetString(PyExc_RuntimeError, "Failed to convert python object");
            }
            return;
        }

        if ( PyDict_SetItem(*mPyDict, pyKey, pyValue) == -1 ) {
            Py_DECREF(pyKey);
            Py_DECREF(pyValue);
            Py_DECREF(*mPyDict);
            *mPyDict = NULL;
            LOG_ERROR("Failed to modify python dict.");
            PyErr_SetString(PyExc_RuntimeError, "Failed to modify python dict");
        }
    }

    //shared pointer ensures copies of this object
    //will use same memory location
    boost::shared_ptr<PyObject*> mPyDict;
};
}

//if you change this, change uCommandArg_ConvertSingleArg in
// framework_core/commadn_manager_base/UCommandARg.cc in order to ensure
// that the reverse python-to-C++ conversion can be made.
// possibly change QtForPython_VariantToPython to match
void
QtForPython_PyObjectToVariant( PyObject* source, QVariant& dest)
{
    if( source == Py_None ) {
        dest = QVariant();
    } else if ( source == Py_False ) {
        dest = (bool)false;
    } else if ( source == Py_True ) {
        dest = (bool)true;
    } else if (PyFloat_Check(source)) {
        dest = PyFloat_AsDouble(source);
    } else if (PyInt_Check(source)) {
        dest = (int)PyInt_AsLong(source);
    } else if (PyLong_Check(source)) {        
        dest = (qlonglong)PyLong_AsLongLong(source);
    } else if (PyString_Check(source)) {
        dest = QString( PyString_AsString(source) );
    } else if (PyList_Check(source)) {
        int nn =  PyList_Size(source);
        QList<QVariant> list;
        for ( int ii = 0; ii < nn; ii++ ) {
            QVariant tmp;
            PyObject* item = PyList_GetItem(source, ii);
            QtForPython_PyObjectToVariant(item, tmp);
            list.push_back(tmp);
        }
        dest =  list;
    } else if (PyTuple_Check(source)) {
        int nn =  PyTuple_Size(source);
        QList<QVariant> list;
        for ( int ii = 0; ii < nn; ii++ ) {
            QVariant tmp;
            PyObject* item = PyTuple_GetItem(source, ii);
            QtForPython_PyObjectToVariant(item, tmp);
            list.push_back(tmp);
        }
        dest =  list;
    } else if (PyDict_Check(source)) {
        PyObject *pyKey, *pyValue;
        Py_ssize_t pos = 0;
        QMap<QString, QVariant> mapResult;
        while (PyDict_Next(source, &pos, &pyKey, &pyValue)) {
            QVariant cValue;
            std::string  cKey;
            QtForPython_PyObjectToString(pyKey, cKey);
            QtForPython_PyObjectToVariant(pyValue, cValue);
            mapResult[cKey.c_str()]  = cValue;
        }
        dest = mapResult;
    } else {
        //an unsupported type.
        LOG_ERROR("Unsupported type given for source value.");
        PyErr_SetString(PyExc_RuntimeError, "Unsupported type given for source value.");
    }
}

PyObject*
QtForPython_MapToPython(const QMap<QString, QVariant>& source)
{
    AccumToPyDict convert;
    QMap<QString, QVariant>::const_iterator ii;
    for (ii = source.constBegin(); ii != source.constEnd(); ++ii)
        convert(ii);
    PyObject* returnValue = *convert.mPyDict;

    //Note: mPyDict will be NULL if an error occurred.
    //The error should have been logged at a lower level
    return returnValue;
}


//if you change this, change QtForPython_PyObjectToVariant
//to match
PyObject*
QtForPython_VariantToPython(const QVariant& source)
{
    PyObject* returnValue = NULL;
    if ( source.type() == QVariant::Invalid ||
            source.isNull() ) {
        //none/null is a special type in python
        returnValue = Py_None;
        Py_INCREF(returnValue);
    } else if ( source.type() == QVariant::Bool ) {
        //bool is a special type in python
        bool tmp = source.toBool();
        if (!tmp) {
            returnValue = Py_False;
        } else {
            returnValue = Py_True;
        }
        Py_INCREF(returnValue);
    } else if ( source.type() == QVariant::Double ) {
        double tmp = source.toDouble();
        returnValue = PyFloat_FromDouble(tmp);
    } else if ( source.type() == QVariant::Int ) {
        long tmp = source.toInt();
        returnValue = PyInt_FromLong(tmp);
    } else if ( source.type() == QVariant::UInt ) {
        long tmp = source.toUInt();
        returnValue = PyInt_FromLong(tmp);
    } else if ( source.type() == QVariant::LongLong ||
                source.type() == QVariant::ULongLong) {
        returnValue = PyLong_FromLongLong(source.toLongLong());
    } else if ( source.canConvert<QByteArray>() ) {
        //most things we didn't get already can be converted to string
        QByteArray tmp = source.toByteArray();
        returnValue = PyString_FromString( tmp.constData() );
    } else if ( source.type() == QVariant::List ) {
        //convert to variant list and recursively convert
        const QList<QVariant>& list(source.toList());
        AccumToPyList convert;
        std::for_each( list.constBegin(), list.constEnd(), convert);
        returnValue = *convert.mPyList;
    } else if ( source.type() == QVariant::StringList ) {
        //convert to variant list and recursively convert
        const QList<QString>& list(source.toStringList());
        AccumToPyList convert;
        std::for_each( list.constBegin(), list.constEnd(), convert);
        returnValue = *convert.mPyList;
    } else if ( source.QVariant::canConvert<std::vector<std::string> >() ) {
        //this type was added in QtCustomTypes.cc
        std::vector<std::string> list(source.value<std::vector<std::string> >());
        AccumToPyList convert;
        std::for_each( list.begin(), list.end(), convert);
        returnValue = *convert.mPyList;
    } else if ( source.QVariant::canConvert<std::vector<double> >() ) {
        //this type was added in QtCustomTypes.cc
        std::vector<double> list(source.value<std::vector<double> >());
        AccumToPyList convert;
        std::for_each( list.begin(), list.end(), convert);
        returnValue = *convert.mPyList;
    } else if ( source.QVariant::canConvert<std::vector<uint> >() ) {
        //this type was added in QtCustomTypes.cc
        std::vector<uint> list(source.value<std::vector<uint> >());
        AccumToPyList convert;
        std::for_each( list.begin(), list.end(), convert);
        returnValue = *convert.mPyList;
    } else if ( source.type() == QVariant::Map ) {
        QMap<QString, QVariant> map(source.toMap());
        returnValue = QtForPython_MapToPython(map);
    } else if ( source.type() == QVariant::Hash ) {
        QHash<QString, QVariant> hash(source.toHash());
        returnValue = QtForPython_HashToPython(hash);
    } else if (source.QVariant::canConvert<gmath::Vec2d >() ) {
        gmath::Vec2d v = source.QVariant::value<gmath::Vec2d>();
        AccumToPyList convert;
        convert(v[0]); convert(v[1]);
        returnValue = *convert.mPyList;
    } else if (source.QVariant::canConvert<gmath::Vec3d >() ) {
        gmath::Vec3d v = source.QVariant::value<gmath::Vec3d>();
        AccumToPyList convert;
        convert(v[0]); convert(v[1]); convert(v[2]);
        returnValue = *convert.mPyList;
    } else if (source.QVariant::canConvert<color::Rgb >() ) {
        color::Rgb v = source.QVariant::value<color::Rgb>();
        AccumToPyList convert;
        convert(v[0]); convert(v[1]); convert(v[2]);
        returnValue = *convert.mPyList;
    } else if (source.QVariant::canConvert<color::Rgba >() ) {
        color::Rgba v = source.QVariant::value<color::Rgba>();
        AccumToPyList convert;
        convert(v[0]); convert(v[1]); convert(v[2]); convert(v[3]);
        returnValue = *convert.mPyList;
    } else {
        //we don't know this type. let NULL pass through
        std::string err("Unknown type, found in conversion to python: ");
        err += source.typeName();
        LOG_ERROR(err);
        PyErr_SetString(PyExc_RuntimeError, err.c_str());
    }

    //Any of the python conversion functions may have
    //returned NULL
    //In the NULL case an error message should have already been logged.
    return returnValue;
}

PyObject*
QtForPython_HashToPython(const QHash<QString, QVariant>& source)
{
    AccumToPyDict convert;
    QHash<QString, QVariant>::const_iterator ii;
    for (ii = source.constBegin(); ii != source.constEnd(); ++ii)
        convert(ii);

    PyObject* returnValue = *convert.mPyDict;

    //Note: mPyDict will be NULL if an error occurred.
    //The error should have been logged at a lower level
    return returnValue;
}


PyObject *
QtForPython_QStringSetToPyList(const QSet<QString>& source)
{
    AccumToPyList convert;
    std::for_each( source.constBegin(), source.constEnd(), convert);
    return *convert.mPyList;
}


PyObject *
QtForPython_StringVecToPyList(const std::vector<std::string>& source)
{
    AccumToPyList convert;
    std::for_each( source.begin(), source.end(), convert);
    return *convert.mPyList;
}


void
QtForPython_PyObjectToHash(PyObject* source, QHash<QString, QVariant>& dest)
{
   if (! PyDict_Check(source)) {
       //throw exception. we expect a dictionary
       LOG_ERROR("Dictionary expected. throwing exception");
       throw except::TypeError("Dictionary expected.");
    }

    PyObject *pyKey, *pyValue;
    Py_ssize_t pos = 0;

    while (PyDict_Next(source, &pos, &pyKey, &pyValue)) {
        QVariant cValue;
        std::string cKey;
        QtForPython_PyObjectToString(pyKey, cKey);
        QtForPython_PyObjectToVariant(pyValue, cValue);
        dest.insert(QString( cKey.c_str()), cValue);
    }
}


void
QtForPython_PyObjectToString( PyObject* source, std::string& dest)
{
    if (source == NULL) {
        LOG_ERROR("null given. throwing exception");
        throw except::TypeError("NULL value given for source value.");
    }
    std::stringstream ss;
    if (PyString_Check(source)) {
        dest = boost::python::extract<std::string>(source);
    } else if (PyInt_Check(source)) {
        ss << boost::python::extract<int>(source);
        dest = ss.str();
    }  else if (PyBool_Check(source)) {
        ss << boost::python::extract<bool>(source);
        dest = ss.str();
    } else if (PyFloat_Check(source)) {
        ss << boost::python::extract<float>(source);
        dest = ss.str();
    } else {
        PyObject* pyStr = PyObject_Str(source);
        if (!pyStr) {
            //an unsupported type.
            LOG_ERROR("Unsupported type. throwing exception");
            throw except::TypeError("Unsupported type given for source value.");
        }
        dest = boost::python::extract<std::string>(pyStr);
    }
}

void
QtForPython_VariantToString(const QVariant& source, std::string& dest)
{
    PyObject* result = QtForPython_VariantToPython(source);
    if( result == NULL ) {
        dest += "''";
        if (source.type()) {
            LOG_DEBUG("QtForPython_VariantToString() Unable to convert variant with type "<< source.type());
        }
        //the error was logged at a lower level
        return;
    }

    if( result == Py_None ) {
        dest += "''";
    } else if ( PyString_Check(result) ) {
        dest += "'";
        dest += PyString_AsString(result);
        dest += "'";
    } else {
        PyObject* pyString = PyObject_Str(result);
        dest += PyString_AsString(pyString);
        Py_DECREF(pyString);
    }
    Py_DECREF(result);

    return;
}


void
QtForPython_PyObjectToAny( PyObject* source, boost::any& dest)
{
    if (PyString_Check(source)) {
        std::string tmp  = boost::python::extract<std::string>(source);
        dest = tmp;
    } else if (PyInt_Check(source)) {
        int tmp = boost::python::extract<int>(source);
        dest = tmp;
    } else if (PyBool_Check(source)) {
        bool tmp = boost::python::extract<bool>(source);
        dest = tmp;
    } else if (PyFloat_Check(source)) {
        double tmp = PyFloat_AsDouble(source);
        dest = tmp;
    } else {
        //an unsupported type.
        LOG_ERROR("Unsupported type. throwing exception");
        throw except::TypeError("Unsupported type given for source value.");
    }
}
