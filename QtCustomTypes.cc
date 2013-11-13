// System
#include <iostream>

// Studio
#include <notification_center/QtCustomTypes.h>
#include <toolkit/Utils.h>

static framework::QtCustomTypes static_initializer;
const char framework::QtCustomTypes::UINT_VECTOR_TYPESTRING[] = "std::vector<uint>";
const char framework::QtCustomTypes::DOUBLE_VECTOR_TYPESTRING[] = "std::vector<double>";
const char framework::QtCustomTypes::STRING_VECTOR_TYPESTRING[] = "std::vector<std::string>";
const char framework::QtCustomTypes::GMATH_VEC2_TYPESTRING[] = "gmath::Vec2d";
const char framework::QtCustomTypes::GMATH_VEC3_TYPESTRING[] = "gmath::Vec3d";
const char framework::QtCustomTypes::COLOR_RGB_TYPESTRING[] = "color::Rgb";
const char framework::QtCustomTypes::COLOR_RGBA_TYPESTRING[] = "color::Rgba";
const char framework::QtCustomTypes::QVARIANT_TYPESTRING[] = "QVariant";

framework::QtCustomTypes::QtCustomTypes()
{
    registerCustomTypes();
}


void
framework::QtCustomTypes::registerCustomTypes()
{
    static bool been_here = 0;
    if( been_here)
        return;
    {
        toolkit::StThreadSafeStaticWrite locker;
        been_here = 1;
    }        
    
    qRegisterMetaType<QVariant>(QVARIANT_TYPESTRING);
    
    qRegisterMetaType<std::vector<uint> >(UINT_VECTOR_TYPESTRING);
    qRegisterMetaTypeStreamOperators<std::vector<uint> >(UINT_VECTOR_TYPESTRING);
        
    qRegisterMetaType<std::vector<double> >(DOUBLE_VECTOR_TYPESTRING);
    qRegisterMetaTypeStreamOperators<std::vector<double> >(DOUBLE_VECTOR_TYPESTRING);

    qRegisterMetaType<std::vector<std::string> >(STRING_VECTOR_TYPESTRING);
    qRegisterMetaTypeStreamOperators<std::vector<std::string> >(STRING_VECTOR_TYPESTRING); 

    qRegisterMetaType<gmath::Vec2d>(GMATH_VEC2_TYPESTRING);
    qRegisterMetaTypeStreamOperators<gmath::Vec2d>(GMATH_VEC2_TYPESTRING);

    qRegisterMetaType<gmath::Vec3d>(GMATH_VEC3_TYPESTRING);
    qRegisterMetaTypeStreamOperators<gmath::Vec3d>(GMATH_VEC3_TYPESTRING);

    qRegisterMetaType<color::Rgb>(COLOR_RGB_TYPESTRING);
    qRegisterMetaTypeStreamOperators<color::Rgb>(COLOR_RGB_TYPESTRING);
 
    qRegisterMetaType<color::Rgba>(COLOR_RGBA_TYPESTRING);
    qRegisterMetaTypeStreamOperators<color::Rgba>(COLOR_RGBA_TYPESTRING);
}   

void
framework::QtCustomTypes::VecStringToVecBytes(const std::vector<std::string>& src,
                                                     QVector< QByteArray >& dest)
{
    dest.clear();
    std::vector<std::string>::const_iterator start,stop;
    start = src.begin();
    stop = src.end();
    for(;start!=stop;++start) {
        dest.push_back(QByteArray( (*start).data(), (*start).size() ));
    }
}
void
framework::QtCustomTypes::VecBytesToVecString(const QVector< QByteArray >& src,
                                                     std::vector<std::string>& dest)
{
    dest.clear();
    dest.reserve( src.size() );
    QVector< QByteArray >::const_iterator start,stop;
    start = src.begin();
    stop = src.end();
    for(;start!=stop;++start) {
        dest.push_back( std::string( (*start).data(), (*start).size()));
    }
}


QDataStream & operator<< (QDataStream& stream, const std::vector<uint>& src)
{
    QVector<uint> tmp( QVector<uint>::fromStdVector(src) );
    stream << tmp;
    return stream;
}

QDataStream & operator>> (QDataStream& stream, std::vector<uint>& dest)
{
    QVector<uint> tmp;
    stream >> tmp;
    dest = tmp.toStdVector();
    return stream;
}

QDataStream & operator<< (QDataStream& stream, const std::vector<double>& src)
{
    QVector<double> tmp( QVector<double>::fromStdVector(src) );
    stream << tmp;
    return stream;
}

QDataStream & operator>> (QDataStream& stream, std::vector<double>& dest)
{
    QVector<double> tmp;
    stream >> tmp;
    dest = tmp.toStdVector();
    return stream;
}

QDataStream & operator<< (QDataStream& stream, const std::vector<float>& src)
{
    QVector<float> tmp( QVector<float>::fromStdVector(src) );
    stream << tmp;
    return stream;
}

QDataStream & operator>> (QDataStream& stream, std::vector<float>& dest)
{
    QVector<float> tmp;
    stream >> tmp;
    dest = tmp.toStdVector();
    return stream;
}

QDataStream & operator<< (QDataStream& stream, const std::vector<int>& src)
{
    QVector<int> tmp( QVector<int>::fromStdVector(src) );
    stream << tmp;
    return stream;
}

QDataStream & operator>> (QDataStream& stream, std::vector<int>& dest)
{
    QVector<int> tmp;
    stream >> tmp;
    dest = tmp.toStdVector();
    return stream;
}

QDataStream & operator<< (QDataStream& stream, const std::vector<std::string>& src)
{
    QVector<QByteArray> tmp;
    framework::QtCustomTypes::VecStringToVecBytes(src,tmp);
    stream << tmp;
    return stream;
}

QDataStream & operator>> (QDataStream& stream, std::vector<std::string>& dest)
{
    QVector<QByteArray> tmp;
    stream >> tmp;
    framework::QtCustomTypes::VecBytesToVecString(tmp,dest);
    return stream;
}

QDataStream & operator<< (QDataStream& stream, const std::string& src)
{
    QByteArray tmp = QByteArray::fromRawData(src.data(), src.size());
    stream << tmp;
    return stream;
}

QDataStream & operator>> (QDataStream& stream, std::string& dest)
{
    QByteArray tmp;
    stream >> tmp;
    std::string str(tmp.data(), tmp.size());
    dest.swap(str);
    return stream;
}

namespace {

    // templates for streaming gmath types
    
    template <typename T>
    QDataStream& writeTuple(QDataStream& stream, const T& t) 
    {
        for (unsigned i = 0; i < t.size; ++i) stream << t[i];
        return stream;
    }
    template <typename T>
    QDataStream& readTuple(QDataStream& stream, T& t) 
    {
        for (unsigned i = 0; i < t.size; ++i) stream >> t[i];
        return stream;
    }
}

QDataStream & operator<< (QDataStream& stream, 
                          const gmath::Vec2d& src) { return writeTuple(stream,src); }
QDataStream & operator<< (QDataStream& stream, 
                          const gmath::Vec3d& src) { return writeTuple(stream,src); }
QDataStream & operator<< (QDataStream& stream, 
                          const color::Rgb& src)   { return writeTuple(stream,src); }
QDataStream & operator<< (QDataStream& stream, 
                          const color::Rgba& src)  { return writeTuple(stream,src); }
QDataStream & operator>> (QDataStream& stream, 
                          gmath::Vec2d& dest)      { return readTuple(stream,dest); }
QDataStream & operator>> (QDataStream& stream, 
                          gmath::Vec3d& dest)      { return readTuple(stream,dest); }
QDataStream & operator>> (QDataStream& stream, 
                          color::Rgb& dest)        { return readTuple(stream,dest); }
QDataStream & operator>> (QDataStream& stream, 
                          color::Rgba& dest)       { return readTuple(stream,dest); }

