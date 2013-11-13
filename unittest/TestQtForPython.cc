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

#include <Python.h>
#include "TestQtForPython.h"
#include "../QtForPython.h"
#include "../QtCustomTypes.h"
#include <QSet>
#include <iostream>
#include <QRect>
#include <except/except.h>

const char* BLAH = "'blah'";
const char* EMPTY_QUOTES = "''";
const char* LIST_ONE_TWO = "[1, '2']";
const char* NUMBER_ONE = "1";
const char* NUMBER_ONE_HUNDRED = "100";
const char* NUMBER_ONE_HUNDRED_ONE = "100.1";
const char* STRING_ONE = "one";
const char* STRING_THREE = "three";
const char* STRING_TWO = "two";
const char* STRING_ONE_TWO_THREE = "['one', 'two', 'three']";
QString QSTRING_ONE(STRING_ONE);
QString QSTRING_TWO(STRING_TWO);
QString QSTRING_THREE(STRING_THREE);


void TestQtForPython::setUp()
{
    if (!Py_IsInitialized()) {
        Py_Initialize();
    }
}

void
TestQtForPython::PyObjectToString()
{
    PyObject* pyInt = PyInt_FromLong(100);
    std::string tmp;
    QtForPython_PyObjectToString( pyInt, tmp);
    CPPUNIT_ASSERT(tmp == NUMBER_ONE_HUNDRED);

    PyObject* pyStr = PyString_FromString(NUMBER_ONE_HUNDRED);
    QtForPython_PyObjectToString( pyStr, tmp);
    CPPUNIT_ASSERT(tmp == NUMBER_ONE_HUNDRED);

    PyObject* pyFloat = PyFloat_FromDouble(100.1);
    QtForPython_PyObjectToString( pyFloat, tmp);
    CPPUNIT_ASSERT(tmp == NUMBER_ONE_HUNDRED_ONE);

    PyObject* pyTrue = PyBool_FromLong(1);
    QtForPython_PyObjectToString( pyTrue, tmp);
    CPPUNIT_ASSERT(tmp == NUMBER_ONE);
}

void
TestQtForPython::PyObjectToVariant()
{
    PyObject* pyInt = PyInt_FromLong(100);
    QVariant tmp;
    QtForPython_PyObjectToVariant( pyInt, tmp);
    CPPUNIT_ASSERT(tmp== QVariant(100));

    PyObject* pyStr = PyString_FromString(NUMBER_ONE_HUNDRED);
    QtForPython_PyObjectToVariant( pyStr, tmp);
    CPPUNIT_ASSERT(tmp == QVariant(NUMBER_ONE_HUNDRED));

    PyObject* pyFloat = PyFloat_FromDouble(100.1);
    QtForPython_PyObjectToVariant( pyFloat, tmp);
    CPPUNIT_ASSERT_EQUAL(tmp.toDouble(),100.1);

    PyObject* pyTrue = PyBool_FromLong(1);
    QtForPython_PyObjectToVariant( pyTrue, tmp);
    CPPUNIT_ASSERT(tmp == QVariant(true));

    PyObject *pyList = PyList_New(0);
    PyList_Append(pyList, pyInt);
    PyList_Append(pyList, pyStr);
    PyList_Append(pyList, pyFloat);

    QList<QVariant> qList;
    qList.push_back( QVariant(100) );
    qList.push_back( QVariant(NUMBER_ONE_HUNDRED) );
    qList.push_back( QVariant(100.1) );
    QtForPython_PyObjectToVariant( pyList, tmp);

    PyObject *pyTuple = PyTuple_New(3);
    PyTuple_SetItem(pyTuple, 0, pyInt);
    PyTuple_SetItem(pyTuple, 1, pyStr);
    PyTuple_SetItem(pyTuple, 2, pyFloat);
    QtForPython_PyObjectToVariant( pyTuple, tmp);

    CPPUNIT_ASSERT(tmp.toList() == qList);

    PyObject *pyDict = PyDict_New();
    PyDict_SetItemString(pyDict, "int", pyInt);
    PyDict_SetItemString(pyDict, "string", pyStr);
    PyDict_SetItemString(pyDict, "float", pyFloat);
    PyDict_SetItemString(pyDict, "list", pyList);
    QMap<QString, QVariant> qMap;
    qMap["int"] = QVariant(100);
    qMap["string"] = QVariant(NUMBER_ONE_HUNDRED);
    qMap["float"] = QVariant(100.1);
    qMap["list"] = QVariant(qList);

    QtForPython_PyObjectToVariant( pyDict, tmp);
    CPPUNIT_ASSERT(tmp.toMap() == qMap);
}

void
TestQtForPython::PyObjectToAny()
{
    boost::any tmp;
    PyObject* pyInt = PyInt_FromLong(100);
    QtForPython_PyObjectToAny( pyInt, tmp);
    CPPUNIT_ASSERT_EQUAL(  boost::any_cast<int>(tmp),100);

    PyObject* pyStr = PyString_FromString(NUMBER_ONE_HUNDRED);
    QtForPython_PyObjectToAny( pyStr, tmp);
    CPPUNIT_ASSERT(boost::any_cast<std::string>(tmp) == NUMBER_ONE_HUNDRED);

    PyObject* pyFloat = PyFloat_FromDouble(100.1);
    QtForPython_PyObjectToAny( pyFloat, tmp);
    CPPUNIT_ASSERT_EQUAL(boost::any_cast<double>(tmp),100.1);

    //    PyObject* pyTrue = PyBool_FromLong(1);
    //    QtForPython_PyObjectToAny(pyTrue, tmp);
    //    CPPUNIT_ASSERT_EQUAL( boost::any_cast<bool>(tmp),true);
}

void
TestQtForPython::StringVecToPyList()
{
    std::vector<std::string> stringList;
    stringList.push_back( std::string(STRING_ONE) );
    stringList.push_back( std::string(STRING_TWO) );
    stringList.push_back( std::string(STRING_THREE) );

    PyObject* pyList = QtForPython_StringVecToPyList( stringList );
    CPPUNIT_ASSERT_EQUAL(PyList_Check(pyList), true);
    CPPUNIT_ASSERT_EQUAL(PyList_Size(pyList), 3l);

    PyObject* firstItem;
    PyObject* secondItem;
    PyObject* thirdItem;
    std::string firstString;
    std::string secondString;
    std::string thirdString;
    firstItem = PyList_GetItem(pyList, 0);
    QtForPython_PyObjectToString(firstItem, firstString);

    CPPUNIT_ASSERT(STRING_ONE == firstString);

    secondItem = PyList_GetItem(pyList, 1);
    QtForPython_PyObjectToString(secondItem, secondString);
    CPPUNIT_ASSERT(STRING_TWO == secondString);

    thirdItem = PyList_GetItem(pyList, 2);
    QtForPython_PyObjectToString(thirdItem, thirdString);
    CPPUNIT_ASSERT(STRING_THREE == thirdString);
}

void
TestQtForPython::QStringSetToPyList()
{
    QSet<QString> stringSet;
    stringSet.insert( QSTRING_ONE   );
    stringSet.insert( QSTRING_TWO   );
    stringSet.insert( QSTRING_THREE );

    PyObject* pyList = QtForPython_QStringSetToPyList( stringSet );
    CPPUNIT_ASSERT_EQUAL(PyList_Check(pyList), true);
    CPPUNIT_ASSERT_EQUAL(PyList_Size(pyList), 3l);

    PyObject* firstItem;
    PyObject* secondItem;
    PyObject* thirdItem;
    QVariant tmp;
    QString firstString;
    QString secondString;
    QString thirdString;
    firstItem = PyList_GetItem(pyList, 0);
    QtForPython_PyObjectToVariant(firstItem, tmp);
    firstString = tmp.toString();
    CPPUNIT_ASSERT(QSTRING_ONE == firstString);

    secondItem = PyList_GetItem(pyList, 1);
    QtForPython_PyObjectToVariant(secondItem, tmp);
    secondString = tmp.toString();
    CPPUNIT_ASSERT(QSTRING_TWO == secondString);

    thirdItem = PyList_GetItem(pyList, 2);
    QtForPython_PyObjectToVariant(thirdItem, tmp);
    thirdString = tmp.toString();
    CPPUNIT_ASSERT(QSTRING_THREE == thirdString);
}

void
TestQtForPython::VariantToString()
{
    std::string result;
    QVariant empty;

    QtForPython_VariantToString(empty, result);
    CPPUNIT_ASSERT( result == EMPTY_QUOTES);

    QVariant one(1);
    QVariant two("2");

    QList<QVariant> list;
    list << one << two;
    result.clear();
    QtForPython_VariantToString(list, result);
    CPPUNIT_ASSERT(result == LIST_ONE_TWO);

    QVariant str("blah");
    result.clear();
    QtForPython_VariantToString(str, result);
    CPPUNIT_ASSERT(result == BLAH);
    
    result.clear();

    typedef std::string mystring;
    std::vector< mystring > stringVec;
    stringVec.push_back(STRING_ONE);
    stringVec.push_back(STRING_TWO);
    stringVec.push_back(STRING_THREE);
    QVariant strVec( QVariant::fromValue(stringVec) );
    QtForPython_VariantToString(strVec, result);
    CPPUNIT_ASSERT(result == STRING_ONE_TWO_THREE);


    
}



void
TestQtForPython::VariantToPython()
{
    PyObject*result;
    QVariant empty;

    result = QtForPython_VariantToPython(empty);
    CPPUNIT_ASSERT( result == Py_None);
    Py_DECREF(result);

    QVariant boolFalse( (bool)false );
    result = QtForPython_VariantToPython(boolFalse);
    CPPUNIT_ASSERT( result == Py_False);
    Py_DECREF(result);

    QVariant boolTrue( (bool)true );
    result = QtForPython_VariantToPython(boolTrue);
    CPPUNIT_ASSERT( result == Py_True);
    Py_DECREF(result);

    result = QtForPython_VariantToPython( QVariant(100.1) );
    CPPUNIT_ASSERT( PyFloat_Check(result) );
    CPPUNIT_ASSERT_EQUAL( PyFloat_AsDouble(result), 100.1);
    Py_DECREF(result);

    result = QtForPython_VariantToPython( QVariant(100) );
    CPPUNIT_ASSERT( PyInt_Check(result) );
    CPPUNIT_ASSERT_EQUAL( PyInt_AsLong(result), long(100));
    Py_DECREF(result);

    result = QtForPython_VariantToPython( QVariant(unsigned(100)) );
    CPPUNIT_ASSERT( PyInt_Check(result) );
    CPPUNIT_ASSERT_EQUAL( PyInt_AsLong(result), long(100));
    Py_DECREF(result);

    result = QtForPython_VariantToPython( QVariant( qlonglong(100)) );
    CPPUNIT_ASSERT( PyLong_Check(result) );
    CPPUNIT_ASSERT_EQUAL( PyLong_AsLongLong(result), qlonglong(100));
    Py_DECREF(result);

    result = QtForPython_VariantToPython( QVariant( qulonglong(100)) );
    CPPUNIT_ASSERT( PyLong_Check(result) );
    CPPUNIT_ASSERT_EQUAL( PyLong_AsLongLong(result), qlonglong(100));
    Py_DECREF(result);

    result = QtForPython_VariantToPython( QVariant(NUMBER_ONE_HUNDRED) );
    CPPUNIT_ASSERT( PyString_Check(result) );
    CPPUNIT_ASSERT( !strcmp(PyString_AsString(result), NUMBER_ONE_HUNDRED));
    Py_DECREF(result);
    
    CPPUNIT_ASSERT(PyErr_Occurred() == NULL);
    QVariant unimplmented( QRect(0, 0, 1, 1) );
    CPPUNIT_ASSERT(QtForPython_VariantToPython(unimplmented) == NULL);
    CPPUNIT_ASSERT(PyErr_Occurred() != NULL);
    PyErr_Clear();
}

void
TestQtForPython::VariantToPythonWithDicts()
{
    PyObject*result;
    QMap<QString, QVariant> qMap;
    QVariant unimplemented( QRect(0, 0, 1, 1) );
    
    qMap.insert( STRING_ONE, QVariant(100) );
    qMap.insert( STRING_TWO, QVariant(NUMBER_ONE_HUNDRED) );
    qMap.insert( STRING_THREE, QVariant(100.1) );

    result = QtForPython_VariantToPython(qMap);
    CPPUNIT_ASSERT(PyDict_Check(result));
    CPPUNIT_ASSERT_EQUAL(PyDict_Size(result), Py_ssize_t(3));
    PyObject* iter = PyDict_GetItemString(result, STRING_ONE);
    CPPUNIT_ASSERT(PyInt_Check(iter));
    CPPUNIT_ASSERT_EQUAL( PyInt_AsLong(iter), long(100));

    iter = PyDict_GetItemString(result, STRING_TWO);
    CPPUNIT_ASSERT( PyString_Check(iter) );
    CPPUNIT_ASSERT( !strcmp(PyString_AsString(iter),NUMBER_ONE_HUNDRED) );

    iter = PyDict_GetItemString(result, STRING_THREE);
    CPPUNIT_ASSERT( PyFloat_Check(iter) );
    CPPUNIT_ASSERT_EQUAL( PyFloat_AsDouble(iter), 100.1);
    Py_DECREF(result);
        
    CPPUNIT_ASSERT(PyErr_Occurred() == NULL);
    qMap.insert( "unimplemented", unimplemented );
    result = QtForPython_VariantToPython(qMap);
    CPPUNIT_ASSERT(result == NULL);
    CPPUNIT_ASSERT(PyErr_Occurred() != NULL);
    PyErr_Clear();

    QHash<QString, QVariant> qHash;

    qHash.insert( STRING_ONE, QVariant(100) );
    qHash.insert( STRING_TWO, QVariant(NUMBER_ONE_HUNDRED) );
    qHash.insert( STRING_THREE, QVariant(100.1) );

    result = QtForPython_VariantToPython(qHash);
    CPPUNIT_ASSERT(PyDict_Check(result));
    CPPUNIT_ASSERT_EQUAL(PyDict_Size(result), Py_ssize_t(3));
    iter = PyDict_GetItemString(result, STRING_ONE);
    CPPUNIT_ASSERT(PyInt_Check(iter));
    CPPUNIT_ASSERT_EQUAL( PyInt_AsLong(iter), long(100));

    iter = PyDict_GetItemString(result, STRING_TWO);
    CPPUNIT_ASSERT( PyString_Check(iter) );
    CPPUNIT_ASSERT( !strcmp(PyString_AsString(iter),NUMBER_ONE_HUNDRED) );

    iter = PyDict_GetItemString(result, STRING_THREE);
    CPPUNIT_ASSERT( PyFloat_Check(iter) );
    CPPUNIT_ASSERT_EQUAL( PyFloat_AsDouble(iter), 100.1);
    Py_DECREF(result);
    
    CPPUNIT_ASSERT(PyErr_Occurred() == NULL);
    qHash.insert( "unimplemented", unimplemented );
    result = QtForPython_VariantToPython(qHash);
    CPPUNIT_ASSERT(result == NULL);
    CPPUNIT_ASSERT(PyErr_Occurred() != NULL);
    PyErr_Clear();
}

void
TestQtForPython::VariantToPythonWithLists()
{
    PyObject*result;
    QVariant unimplemented( QRect(0, 0, 1, 1) );
    QList<QVariant> qList;
    qList.push_back( QVariant(100) );
    qList.push_back( QVariant(NUMBER_ONE_HUNDRED) );
    qList.push_back( QVariant(100.1) );

    result = QtForPython_VariantToPython(qList);
    CPPUNIT_ASSERT(PyList_Check(result));
    CPPUNIT_ASSERT_EQUAL(PyList_Size(result), Py_ssize_t(3));
    PyObject* iter = PyList_GetItem(result, 0);
    CPPUNIT_ASSERT( PyInt_Check(iter) );
    CPPUNIT_ASSERT_EQUAL( PyInt_AsLong(iter), long(100));

    iter = PyList_GetItem(result, 1);
    CPPUNIT_ASSERT( PyString_Check(iter) );
    CPPUNIT_ASSERT( !strcmp(PyString_AsString(iter),NUMBER_ONE_HUNDRED) );

    iter = PyList_GetItem(result, 2);
    CPPUNIT_ASSERT( PyFloat_Check(iter) );
    CPPUNIT_ASSERT_EQUAL( PyFloat_AsDouble(iter), 100.1);
    Py_DECREF(result);

    CPPUNIT_ASSERT(PyErr_Occurred() == NULL);
    qList.push_back(unimplemented );
    result = QtForPython_VariantToPython(qList);
    CPPUNIT_ASSERT(result == NULL);
    CPPUNIT_ASSERT(PyErr_Occurred() != NULL);
    PyErr_Clear();

    std::vector< std::string >  stringVec;
    stringVec.push_back( NUMBER_ONE_HUNDRED  );
    stringVec.push_back( NUMBER_ONE );
    stringVec.push_back( NUMBER_ONE_HUNDRED_ONE );

    QVariant stringVecVariant;
    stringVecVariant.setValue(stringVec);
    result = QtForPython_VariantToPython(stringVecVariant);
    CPPUNIT_ASSERT(PyList_Check(result));
    CPPUNIT_ASSERT_EQUAL(PyList_Size(result), Py_ssize_t(3));
    iter = PyList_GetItem(result, 0);
    CPPUNIT_ASSERT( PyString_Check(iter) );
    CPPUNIT_ASSERT( !strcmp(PyString_AsString(iter),NUMBER_ONE_HUNDRED) );

    iter = PyList_GetItem(result, 1);
    CPPUNIT_ASSERT( PyString_Check(iter) );
    CPPUNIT_ASSERT( !strcmp(PyString_AsString(iter),NUMBER_ONE) );

    iter = PyList_GetItem(result, 2);
    CPPUNIT_ASSERT( PyString_Check(iter) );
    CPPUNIT_ASSERT( !strcmp(PyString_AsString(iter),NUMBER_ONE_HUNDRED_ONE) );
    Py_DECREF(result);

    std::vector< double >  doubleVec;
    doubleVec.push_back( 100.0  );
    doubleVec.push_back( 1.0 );
    doubleVec.push_back( 100.1 );

    QVariant doubleVecVariant;
    doubleVecVariant.setValue(doubleVec);
    result = QtForPython_VariantToPython(doubleVecVariant);
    CPPUNIT_ASSERT(PyList_Check(result));
    CPPUNIT_ASSERT_EQUAL(PyList_Size(result), Py_ssize_t(3));
    iter = PyList_GetItem(result, 0);
    CPPUNIT_ASSERT( PyFloat_Check(iter) );
    CPPUNIT_ASSERT_EQUAL( PyFloat_AsDouble(iter),100.0);

    iter = PyList_GetItem(result, 1);
    CPPUNIT_ASSERT( PyFloat_Check(iter) );
    CPPUNIT_ASSERT_EQUAL( PyFloat_AsDouble(iter), 1.0);

    iter = PyList_GetItem(result, 2);
    CPPUNIT_ASSERT( PyFloat_Check(iter) );
    CPPUNIT_ASSERT_EQUAL( PyFloat_AsDouble(iter),100.1);
    Py_DECREF(result);

    std::vector< uint >  uintVec;
    uintVec.push_back( 100  );
    uintVec.push_back( 1 );

    QVariant uintVecVariant;
    uintVecVariant.setValue(uintVec);
    result = QtForPython_VariantToPython(uintVecVariant);
    CPPUNIT_ASSERT(PyList_Check(result));
    CPPUNIT_ASSERT_EQUAL(PyList_Size(result), Py_ssize_t(2));
    iter = PyList_GetItem(result, 0);
    CPPUNIT_ASSERT( PyInt_Check(iter) );
    CPPUNIT_ASSERT_EQUAL( PyInt_AsLong(iter), long(100));

    iter = PyList_GetItem(result, 1);
    CPPUNIT_ASSERT( PyInt_Check(iter) );
    CPPUNIT_ASSERT_EQUAL( PyInt_AsLong(iter), long(1));
    Py_DECREF(result);

}

template<typename T>
void
testTupleToPython(const T& t)
{
    QVariant var;
    var.setValue(t);
    PyObject *result = QtForPython_VariantToPython(var);
    CPPUNIT_ASSERT(PyList_Check(result));
    CPPUNIT_ASSERT_EQUAL((unsigned)PyList_Size(result), (unsigned)t.size);
    for (unsigned i = 0; i < t.size; ++i) {
        PyObject* item = PyList_GetItem(result, i);
        CPPUNIT_ASSERT( PyFloat_Check(item) );
        CPPUNIT_ASSERT_EQUAL( PyFloat_AsDouble(item), (double)t[i]);
    }
    Py_DECREF(result);
}
    
void
TestQtForPython::VariantToPythonWithTuples()
{
    gmath::Vec2d v2(3.1,4.2);
    testTupleToPython( v2 );
    gmath::Vec3d v3(3.1,4.2,5.4);
    testTupleToPython( v3 );
    color::Rgb rgb(0.1,0.2,0.4);
    testTupleToPython( rgb );
    color::Rgba rgba(0.5,0.6,0.7,0.8);
    testTupleToPython( rgba );
}
