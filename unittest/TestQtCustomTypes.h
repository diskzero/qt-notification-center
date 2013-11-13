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

#ifndef TESTQTCUSTOMTYPES_H_HAS_BEEN_INCLUDED
#define TESTQTCUSTOMTYPES_H_HAS_BEEN_INCLUDED

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>
#include <notification_center/QtCustomTypes.h>
#include <QDataStream>
#include <vector>
#include <QVariant>


/**
   This templatized test function attempts to write a type out
   to a qt datastream, then read it back.

   It also attempts to wrap the variable with a variant then
   write/read that variant.

   It's only intended for custom types as native qt types
   should be readable/writable.
 */
template<class _T>
void
verifyCustomCollection(const _T& T_src)
{
    _T T_dest;
    QByteArray buffer;
    {
        QDataStream writer( &buffer, QIODevice::WriteOnly);
        writer << T_src;
    }
    {
        QDataStream reader( &buffer, QIODevice::ReadOnly);
        reader >> T_dest;
    }

    CPPUNIT_ASSERT_EQUAL(T_src.size(), T_dest.size());
    CPPUNIT_ASSERT(T_src == T_dest);
    QVariant T_variant( QVariant::fromValue(T_src));
    QVariant T_variant_dest;
    buffer.clear();
    {
        QDataStream writer( &buffer, QIODevice::WriteOnly);
        writer << T_variant;
    }
    {
        QDataStream reader( &buffer, QIODevice::ReadOnly);
        reader >> T_variant_dest;
        T_dest = T_variant_dest.value< _T >();
    }
    CPPUNIT_ASSERT_EQUAL(T_src.size(), T_dest.size());
    CPPUNIT_ASSERT(T_src == T_dest);

}

/**
   This templatized test function attempts to write a type out
   to a qt datastream, then read it back.

   It also attempts to wrap the variable with a variant then
   write/read that variant.

   It's only intended for custom types as native qt types
   should be readable/writable.
 */
template<class _T>
void
verifyCustomType(const _T& T_src)
{
    _T T_dest;
    QByteArray buffer;
    {
        QDataStream writer( &buffer, QIODevice::WriteOnly);
        writer << T_src;
    }
    {
        QDataStream reader( &buffer, QIODevice::ReadOnly);
        reader >> T_dest;
    }

    CPPUNIT_ASSERT(T_src == T_dest);
    QVariant T_variant( QVariant::fromValue(T_src));
    QVariant T_variant_dest;
    buffer.clear();
    {
        QDataStream writer( &buffer, QIODevice::WriteOnly);
        writer << T_variant;
    }
    {
        QDataStream reader( &buffer, QIODevice::ReadOnly);
        reader >> T_variant_dest;
        T_dest = T_variant_dest.value< _T >();
    }

    CPPUNIT_ASSERT(T_src == T_dest);

}

class TestQtCustomTypes : public CppUnit::TestFixture
{
public:

    void runTest() {
        //Run tests for vector of uint
        std::vector<uint> uint_src(5, 5);
        verifyCustomCollection<std::vector<uint> >( uint_src );

        //Run tests for vector of double
        std::vector<double> f64_src(30, 3.0/4.0);
        verifyCustomCollection<std::vector<double> >( f64_src );

        std::vector<std::string> string_src(1);
        string_src.push_back("Test this string");
        verifyCustomCollection<std::vector<std::string> >( string_src );

        QVariant variant_src(1);
        verifyCustomType<QVariant>( variant_src );

        gmath::Vec2d v2_src(3.1,4.2);
        verifyCustomType<gmath::Vec2d >( v2_src );
        gmath::Vec3d v3_src(3.1,4.2,5.4);
        verifyCustomType<gmath::Vec3d >( v3_src );
        color::Rgb rgb_src(0.1,0.2,0.4);
        verifyCustomType<color::Rgb >( rgb_src );
        color::Rgba rgba_src(0.5,0.6,0.7,0.8);
        verifyCustomType<color::Rgba >( rgba_src );
    }

    // Set up the unit tests
    CPPUNIT_TEST_SUITE(TestQtCustomTypes);
    CPPUNIT_TEST(runTest);
    CPPUNIT_TEST_SUITE_END();

};

// Register this test for execution
CPPUNIT_TEST_SUITE_REGISTRATION(TestQtCustomTypes);

#endif // TESTQTCUSTOMTYPES_H_HAS_BEEN_INCLUDED
