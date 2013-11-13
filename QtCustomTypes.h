
#ifndef AF_QtCustomTypes_h_
#define AF_QtCustomTypes_h_

#include <QVector>
#include <string>
#include <QMetaType>
#include <QVariant>

#include <gmath/Vec2.h>
#include <gmath/Vec3.h>
#include <color/Rgb.h>
#include <color/Rgba.h>

/**
   @file 

   This file declares types which we have extended qt to read/write 
   from datastream.  We also make it possible to add the type
   to a QVariant.


   see also ArgReference.h, which can use template specialization to convert from a common 
     type to a Qt type so that extension here is not needed.

   to register your type with Qt you must do the following:

    *  Your type must have a default constructor, a copy constructor, and a public destructor.
    * Q_DECLARE_METATYPE(AnArbitraryType); probably this belongs in the header with the declaration of your new type, outside of any function.
    * Add operators for serialization.
          * QDataStream & operator<< (QDataStream& stream, const AnArbitraryType& src)
          * QDataStream & operator>> (QDataStream& stream, AnArbitraryType& dest)
          * Note that there are already existing operators for most qt types.  It's probably a good idea for us to write our own operators for common types so that programmers have to do less translation.
    * in a function make it so your type can be constructed dynamically
          * qRegisterMetaType<AnArbitraryType>("AnArbitraryType");
    * Tell qt that your type can be read/written to a data stream
          * qRegisterMetaTypeStreamOperators<AnArbitraryType>("AnArbitraryType");
    * Write methods to translate your type from/to python.
          * We have not standardized a way to do this yet. 
          * This is a major stumbling block to this work actually being useful, but this is also a very solvable problem via sip, boost::python, or rolling your own conversion functions.  We just need to decide how these functions get registered and called.
    * Note this is not possible for all types. std::string for instance can not be used here.
 */
Q_DECLARE_METATYPE(std::vector<uint>);
QDataStream & operator<< (QDataStream& stream, const std::vector<uint>& src);
QDataStream & operator>> (QDataStream& stream, std::vector<uint>& dest);

Q_DECLARE_METATYPE(std::vector<double>);
QDataStream & operator<< (QDataStream& stream, const std::vector<double>& src);
QDataStream & operator>> (QDataStream& stream, std::vector<double>& dest);

Q_DECLARE_METATYPE(std::vector<std::string>);
QDataStream & operator<< (QDataStream& stream, const std::vector<std::string>& src);
QDataStream & operator>> (QDataStream& stream, std::vector<std::string>& dest);

Q_DECLARE_METATYPE(gmath::Vec2d);
QDataStream & operator<< (QDataStream& stream, const gmath::Vec2d& src);
QDataStream & operator>> (QDataStream& stream, gmath::Vec2d& dest);

Q_DECLARE_METATYPE(gmath::Vec3d);
QDataStream & operator<< (QDataStream& stream, const gmath::Vec3d& src);
QDataStream & operator>> (QDataStream& stream, gmath::Vec3d& dest);

Q_DECLARE_METATYPE(color::Rgb);
QDataStream & operator<< (QDataStream& stream, const color::Rgb& src);
QDataStream & operator>> (QDataStream& stream, color::Rgb& dest);

Q_DECLARE_METATYPE(color::Rgba);
QDataStream & operator<< (QDataStream& stream, const color::Rgba& src);
QDataStream & operator>> (QDataStream& stream, color::Rgba& dest);

Q_DECLARE_METATYPE(QVariant);

// Some additional serializers that don't (yet) need to be registered
QDataStream & operator<< (QDataStream& stream, const std::vector<int>& src);
QDataStream & operator>> (QDataStream& stream, std::vector<int>& dest);

QDataStream & operator<< (QDataStream& stream, const std::vector<float>& src);
QDataStream & operator>> (QDataStream& stream, std::vector<float>& dest);

QDataStream & operator<< (QDataStream& stream, const std::string& src);
QDataStream & operator>> (QDataStream& stream, std::string& dest);

namespace framework
{

    /** 
       @brief registers our custom types

       Create an instance of this class to
       register any of the custom types listed in
       this file. 
     */
    struct QtCustomTypes{
        QtCustomTypes();

        /**
           registers custom types when created
           safe for multiple calls.
        */
        static void registerCustomTypes();


        /**
           utility function to convert string vecs to vectors of byte array
        */
        static void VecStringToVecBytes(const std::vector<std::string>& src,
                                         QVector<QByteArray>& dest);

        /**
           utility function to convert vectors of byte array to string vectors
        */
        static void VecBytesToVecString(const QVector<QByteArray>& src,
                                        std::vector<std::string>& dest);
        static const char UINT_VECTOR_TYPESTRING[];
        static const char DOUBLE_VECTOR_TYPESTRING[];
        static const char STRING_VECTOR_TYPESTRING[];
        static const char QVARIANT_TYPESTRING[];
        static const char GMATH_VEC2_TYPESTRING[];
        static const char GMATH_VEC3_TYPESTRING[];
        static const char COLOR_RGB_TYPESTRING[];
        static const char COLOR_RGBA_TYPESTRING[];

    };
}

#endif

