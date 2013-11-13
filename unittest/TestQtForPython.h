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

#ifndef TESTQTFORPYTHON_HAS_BEEN_INCLUDED
#define TESTQTFORPYTHON_HAS_BEEN_INCLUDED

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>
#include <notification_center/QtForPython.h>

class TestQtForPython : public CppUnit::TestFixture
{

public:
    void setUp();

    void PyObjectToString();
    void PyObjectToVariant();
    void PyObjectToAny();
    void StringVecToPyList();
    void QStringSetToPyList();
    void VariantToString();
    void VariantToPython();
    void VariantToPythonWithLists();
    void VariantToPythonWithDicts();    
    void VariantToPythonWithTuples();
   // Set up the unit tests
    CPPUNIT_TEST_SUITE(TestQtForPython);
    CPPUNIT_TEST(PyObjectToString);
    CPPUNIT_TEST(PyObjectToVariant);
    CPPUNIT_TEST(PyObjectToAny);
    CPPUNIT_TEST(StringVecToPyList);
    CPPUNIT_TEST(QStringSetToPyList);
    CPPUNIT_TEST(VariantToString);
    CPPUNIT_TEST(VariantToPython);
    CPPUNIT_TEST(VariantToPythonWithLists);
    CPPUNIT_TEST(VariantToPythonWithDicts); 
    CPPUNIT_TEST(VariantToPythonWithTuples);
    CPPUNIT_TEST_SUITE_END();
    
};

// Register this test for execution
CPPUNIT_TEST_SUITE_REGISTRATION(TestQtForPython);

#endif
