#include <math.h>               // isnan

Q_DECLARE_METATYPE(QVariant)
Q_DECLARE_METATYPE(QList<bool>)
Q_DECLARE_METATYPE(QList<short>)
Q_DECLARE_METATYPE(QList<ushort>)
Q_DECLARE_METATYPE(QList<int>)
Q_DECLARE_METATYPE(QList<uint>)
Q_DECLARE_METATYPE(QList<qlonglong>)
Q_DECLARE_METATYPE(QList<qulonglong>)
Q_DECLARE_METATYPE(QList<double>)
Q_DECLARE_METATYPE(QList<QDBusObjectPath>)
Q_DECLARE_METATYPE(QList<QDBusSignature>)
Q_DECLARE_METATYPE(QList<QDBusVariant>)
Q_DECLARE_METATYPE(QList<QDateTime>)

Q_DECLARE_METATYPE(QList<QByteArray>)
Q_DECLARE_METATYPE(QList<QVariantList>)
Q_DECLARE_METATYPE(QList<QStringList>)
Q_DECLARE_METATYPE(QList<QList<bool> >)
Q_DECLARE_METATYPE(QList<QList<short> >)
Q_DECLARE_METATYPE(QList<QList<ushort> >)
Q_DECLARE_METATYPE(QList<QList<int> >)
Q_DECLARE_METATYPE(QList<QList<uint> >)
Q_DECLARE_METATYPE(QList<QList<qlonglong> >)
Q_DECLARE_METATYPE(QList<QList<qulonglong> >)
Q_DECLARE_METATYPE(QList<QList<double> >)
Q_DECLARE_METATYPE(QList<QList<QDBusObjectPath> >)
Q_DECLARE_METATYPE(QList<QList<QDBusSignature> >)

typedef QMap<int, QString> IntStringMap;
typedef QMap<QString, QString> StringStringMap;
typedef QMap<qlonglong, QDateTime> LLDateTimeMap;
Q_DECLARE_METATYPE(IntStringMap)
Q_DECLARE_METATYPE(StringStringMap)
Q_DECLARE_METATYPE(LLDateTimeMap)

struct MyStruct
{
    int i;
    QString s;

    inline bool operator==(const MyStruct &other) const
    { return i == other.i && s == other.s; }
};
Q_DECLARE_METATYPE(MyStruct)

QDBusArgument &operator<<(QDBusArgument &arg, const MyStruct &ms)
{
    arg.beginStructure();
    arg << ms.i << ms.s;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, MyStruct &ms)
{
    arg.beginStructure();
    arg >> ms.i >> ms.s;
    arg.endStructure();
    return arg;
}

void commonInit()
{
    qDBusRegisterMetaType<QList<QDateTime> >();
    qDBusRegisterMetaType<QList<QStringList> >();
    qDBusRegisterMetaType<QList<QByteArray> >();
    qDBusRegisterMetaType<QList<QList<bool> > >();
    qDBusRegisterMetaType<QList<QList<short> > >();
    qDBusRegisterMetaType<QList<QList<ushort> > >();
    qDBusRegisterMetaType<QList<QList<int> > >();
    qDBusRegisterMetaType<QList<QList<uint> > >();
    qDBusRegisterMetaType<QList<QList<qlonglong> > >();
    qDBusRegisterMetaType<QList<QList<qulonglong> > >();
    qDBusRegisterMetaType<QList<QList<double> > >();
    qDBusRegisterMetaType<QList<QList<QDBusObjectPath> > >();
    qDBusRegisterMetaType<QList<QList<QDBusSignature> > >();
    qDBusRegisterMetaType<QList<QVariantList> >();

    qDBusRegisterMetaType<QMap<int, QString> >();
    qDBusRegisterMetaType<QMap<QString, QString> >();
    qDBusRegisterMetaType<QMap<qlonglong, QDateTime> >();
}
#ifdef USE_PRIVATE_CODE
#include "private/qdbusintrospection_p.h"

// just to make it easier:
typedef QDBusIntrospection::Interfaces InterfaceMap;
typedef QDBusIntrospection::Objects ObjectMap;
typedef QDBusIntrospection::Arguments ArgumentList;
typedef QDBusIntrospection::Annotations AnnotationsMap;
typedef QDBusIntrospection::Methods MethodMap;
typedef QDBusIntrospection::Signals SignalMap;
typedef QDBusIntrospection::Properties PropertyMap;

Q_DECLARE_METATYPE(QDBusIntrospection::Method)
Q_DECLARE_METATYPE(QDBusIntrospection::Signal)
Q_DECLARE_METATYPE(QDBusIntrospection::Property)
Q_DECLARE_METATYPE(MethodMap)
Q_DECLARE_METATYPE(SignalMap)
Q_DECLARE_METATYPE(PropertyMap)

inline QDBusIntrospection::Argument arg(const char* type, const char *name = 0)
{
    QDBusIntrospection::Argument retval;
    retval.type = QLatin1String(type);
    retval.name = QLatin1String(name);
    return retval;
}

template<typename T>
inline QMap<QString, T>& operator<<(QMap<QString, T>& map, const T& m)
{ map.insertMulti(m.name, m); return map; }

inline const char* mapName(const MethodMap&)
{ return "MethodMap"; }

inline const char* mapName(const SignalMap&)
{ return "SignalMap"; }

inline const char* mapName(const PropertyMap&)
{ return "PropertyMap"; }

QString printable(const QDBusIntrospection::Method& m)
{
    QString result = "method " + m.name + "(";
    foreach (QDBusIntrospection::Argument arg, m.inputArgs)
        result += QString("in %1 %2, ")
        .arg(arg.type, arg.name);
    foreach (QDBusIntrospection::Argument arg, m.outputArgs)
        result += QString("out %1 %2, ")
        .arg(arg.type, arg.name);
    AnnotationsMap::const_iterator it = m.annotations.begin();
    for ( ; it != m.annotations.end(); ++it)
        result += QString("%1 \"%2\", ").arg(it.key()).arg(it.value());

    result += ")";
    return result;
}    

QString printable(const QDBusIntrospection::Signal& s)
{
    QString result = "signal " + s.name + "(";
    foreach (QDBusIntrospection::Argument arg, s.outputArgs)
        result += QString("out %1 %2, ")
        .arg(arg.type, arg.name);
    AnnotationsMap::const_iterator it = s.annotations.begin();
    for ( ; it != s.annotations.end(); ++it)
        result += QString("%1 \"%2\", ").arg(it.key()).arg(it.value());

    result += ")";
    return result;
}    

QString printable(const QDBusIntrospection::Property& p)
{
    QString result;
    if (p.access == QDBusIntrospection::Property::Read)
        result = "property read %1 %2, ";
    else if (p.access == QDBusIntrospection::Property::Write)
        result = "property write %1 %2, ";
    else
        result = "property readwrite %1 %2, ";
    result = result.arg(p.type, p.name);
    
    AnnotationsMap::const_iterator it = p.annotations.begin();
    for ( ; it != p.annotations.end(); ++it)
        result += QString("%1 \"%2\", ").arg(it.key()).arg(it.value());

    return result;
}    

template<typename T>
char* printableMap(const QMap<QString, T>& map)
{
    QString contents = "\n";
    typename QMap<QString, T>::const_iterator it = map.begin();
    for ( ; it != map.end(); ++it) {
        if (it.key() != it.value().name)
            contents += it.value().name + ":";
        contents += printable(it.value());
        contents += ";\n";
    }

    QString result("%1(size = %2): {%3}");
    return qstrdup(qPrintable(result
                              .arg(mapName(map))
                              .arg(map.size())
                              .arg(contents)));
}

namespace QTest {
    template<>
    inline char* toString(const MethodMap& map)
    {
        return printableMap(map);
    }

    template<>
    inline char* toString(const SignalMap& map)
    {
        return printableMap(map);
    }

    template<>
    inline char* toString(const PropertyMap& map)
    {
        return printableMap(map);
    }
}
#endif
//bool compare(const QVariantList &l1, const QVariantList &l2);
//bool compare(const QVariantMap &m1, const QVariantMap &m2);
template<typename T>
bool compare(const T &t1, const T &t2)
{ return t1 == t2; }

template<>
bool compare(const QVariant &v1, const QVariant &v2);

bool compare(double d1, double d2)
{
    if (isnan(d1) && isnan(d2))
        return true;
    return d1 == d2;
}

template<>
bool compare(const QString &s1, const QString &s2)
{
    if (s1.isEmpty() && s2.isEmpty())
        return true;            // regardless of whether one of them is null
    return s1 == s2;
}

template<>
bool compare(const QByteArray &ba1, const QByteArray &ba2)
{
    if (ba1.isEmpty() && ba2.isEmpty())
        return true;            // regardless of whether one of them is null
    return ba1 == ba2;
}

template<>
bool compare(const QDBusObjectPath &op1, const QDBusObjectPath &op2)
{
    return compare(op1.value, op2.value);
}

template<>
bool compare(const QDBusSignature &s1, const QDBusSignature &s2)
{
    return compare(s1.value, s2.value);
}

template<>
bool compare(const QDBusVariant &s1, const QDBusVariant &s2)
{
    return compare(s1.value, s2.value);
}

template<typename T>
bool compare(const QList<T> &l1, const QList<T> &l2)
{
    if (l1.count() != l2.count())
        return false;

    typename QList<T>::ConstIterator it1 = l1.constBegin();
    typename QList<T>::ConstIterator it2 = l2.constBegin();
    typename QList<T>::ConstIterator end = l1.constEnd();
    for ( ; it1 != end; ++it1, ++it2)
        if (!compare(*it1, *it2))
            return false;
    return true;
}

template<typename Key, typename T>
bool compare(const QMap<Key, T> &m1, const QMap<Key, T> &m2)
{
    if (m1.count() != m2.size())
        return false;
    typename QMap<Key, T>::ConstIterator i1 = m1.constBegin();
    typename QMap<Key, T>::ConstIterator end = m1.constEnd();
    for ( ; i1 != end; ++i1) {
        typename QMap<Key, T>::ConstIterator i2 = m2.find(i1.key());
        if (i2 == m2.constEnd())
            return false;
        if (!compare(*i1, *i2))
            return false;
    }
    return true;
}

template<typename T>
inline bool compare(const QDBusArgument &arg, const QVariant &v2, T * = 0)
{
    return compare(qdbus_cast<T>(arg), qvariant_cast<T>(v2));
}

bool compareToArgument(const QDBusArgument &arg, const QVariant &v2)
{
    if (arg.currentSignature() != QDBusMetaType::typeToSignature(v2.userType()))
        return false;

    // try to demarshall the arg according to v2
    switch (v2.userType())
    {
    case QVariant::Bool:
        return compare<bool>(arg, v2);
    case QMetaType::UChar:
        return compare<uchar>(arg, v2);
    case QMetaType::Short:
        return compare<short>(arg, v2);
    case QMetaType::UShort:
        return compare<ushort>(arg, v2);
    case QVariant::Int:
        return compare<int>(arg, v2);
    case QVariant::UInt:
        return compare<uint>(arg, v2);
    case QVariant::LongLong:
        return compare<qlonglong>(arg, v2);
    case QVariant::ULongLong:
        return compare<qulonglong>(arg, v2);
    case QVariant::Double:
        return compare<double>(arg, v2);
    case QVariant::String:
        return compare<QString>(arg, v2);
    case QVariant::ByteArray:
        return compare<QByteArray>(arg, v2);
    case QVariant::List:
        return compare<QVariantList>(arg, v2);
    case QVariant::Map:
        return compare<QVariantMap>(arg, v2);
    case QVariant::Point:
        return compare<QPoint>(arg, v2);
    case QVariant::PointF:
        return compare<QPointF>(arg, v2);
    case QVariant::Size:
        return compare<QSize>(arg, v2);
    case QVariant::SizeF:
        return compare<QSizeF>(arg, v2);
    case QVariant::Line:
        return compare<QLine>(arg, v2);
    case QVariant::LineF:
        return compare<QLineF>(arg, v2);
    case QVariant::Rect:
        return compare<QRect>(arg, v2);
    case QVariant::RectF:
        return compare<QRectF>(arg, v2);
    case QVariant::Date:
        return compare<QDate>(arg, v2);
    case QVariant::Time:
        return compare<QTime>(arg, v2);
    case QVariant::DateTime:
        return compare<QDateTime>(arg, v2);
    default:
        register int id = v2.userType();
        if (id == qMetaTypeId<QDBusObjectPath>())
            return compare<QDBusObjectPath>(arg, v2);
        else if (id == qMetaTypeId<QDBusSignature>())
            return compare<QDBusSignature>(arg, v2);
        else if (id == qMetaTypeId<QDBusVariant>())
            return compare<QDBusVariant>(arg, v2);
        else if (id == qMetaTypeId<QList<bool> >())
            return compare<QList<bool> >(arg, v2);
        else if (id == qMetaTypeId<QList<short> >())
            return compare<QList<short> >(arg, v2);
        else if (id == qMetaTypeId<QList<ushort> >())
            return compare<QList<ushort> >(arg, v2);
        else if (id == qMetaTypeId<QList<int> >())
            return compare<QList<int> >(arg, v2);
        else if (id == qMetaTypeId<QList<uint> >())
            return compare<QList<uint> >(arg, v2);
        else if (id == qMetaTypeId<QList<qlonglong> >())
            return compare<QList<qlonglong> >(arg, v2);
        else if (id == qMetaTypeId<QList<qulonglong> >())
            return compare<QList<qulonglong> >(arg, v2);
        else if (id == qMetaTypeId<QList<double> >())
            return compare<QList<double> >(arg, v2);
        else if (id == qMetaTypeId<QList<QDBusObjectPath> >())
            return compare<QList<QDBusObjectPath> >(arg, v2);
        else if (id == qMetaTypeId<QList<QDBusSignature> >())
            return compare<QList<QDBusSignature> >(arg, v2);
        else if (id == qMetaTypeId<QList<QDateTime> >())
            return compare<QList<QDateTime> >(arg, v2);

        else if (id == qMetaTypeId<QMap<int, QString> >())
            return compare<QMap<int, QString> >(arg, v2);
        else if (id == qMetaTypeId<QMap<QString, QString> >())
            return compare<QMap<QString, QString> >(arg, v2);
        else if (id == qMetaTypeId<QMap<qlonglong, QDateTime> >())
            return compare<QMap<qlonglong, QDateTime> >(arg, v2);

        else if (id == qMetaTypeId<QList<QByteArray> >())
            return compare<QList<QByteArray> >(arg, v2);
        else if (id == qMetaTypeId<QList<QList<bool> > >())
            return compare<QList<QList<bool> > >(arg, v2);
        else if (id == qMetaTypeId<QList<QList<short> > >())
            return compare<QList<QList<short> > >(arg, v2);
        else if (id == qMetaTypeId<QList<QList<ushort> > >())
            return compare<QList<QList<ushort> > >(arg, v2);
        else if (id == qMetaTypeId<QList<QList<int> > >())
            return compare<QList<QList<int> > >(arg, v2);
        else if (id == qMetaTypeId<QList<QList<uint> > >())
            return compare<QList<QList<uint> > >(arg, v2);
        else if (id == qMetaTypeId<QList<QList<qlonglong> > >())
            return compare<QList<QList<qlonglong> > >(arg, v2);
        else if (id == qMetaTypeId<QList<QList<qulonglong> > >())
            return compare<QList<QList<qulonglong> > >(arg, v2);
        else if (id == qMetaTypeId<QList<QList<double> > >())
            return compare<QList<QList<double> > >(arg, v2);
        else if (id == qMetaTypeId<QList<QStringList> >())
            return compare<QList<QStringList> >(arg, v2);
        else if (id == qMetaTypeId<QList<QVariantList> >())
            return compare<QList<QVariantList> >(arg, v2);

        else if (id == qMetaTypeId<MyStruct>())
            return compare<MyStruct>(arg, v2);
    }

    qWarning() << "Unexpected QVariant type" << v2.userType()
               << QByteArray(QDBusMetaType::typeToSignature(v2.userType()))
               << QVariant::typeToName(QVariant::Type(v2.userType()));
    return false;
}

template<> bool compare(const QVariant &v1, const QVariant &v2)
{
    // v1 is the one that came from the network
    // v2 is the one that we sent

    if (v1.userType() == qMetaTypeId<QDBusArgument>())
        // this argument has been left un-demarshalled
        return compareToArgument(qvariant_cast<QDBusArgument>(v1), v2);

    if (v1.userType() != v2.userType())
        return false;

    int id = v1.userType();
    if (id == QVariant::List)
        return compare(v1.toList(), v2.toList());

    else if (id == QVariant::Map)
        return compare(v1.toMap(), v2.toMap());

    else if (id == QVariant::String)
        return compare(v1.toString(), v2.toString());

    else if (id == QVariant::ByteArray)
        return compare(v1.toByteArray(), v2.toByteArray());

    else if (id < int(QVariant::UserType)) // yes, v1.type()
        // QVariant can compare
        return v1 == v2;

    else if (id == QMetaType::UChar)
        return qvariant_cast<uchar>(v1) == qvariant_cast<uchar>(v2);

    else if (id == QMetaType::Short)
        return qvariant_cast<short>(v1) == qvariant_cast<short>(v2);

    else if (id == QMetaType::UShort)
        return qvariant_cast<ushort>(v1) == qvariant_cast<ushort>(v2);

    else if (id == qMetaTypeId<QDBusObjectPath>())
        return qvariant_cast<QDBusObjectPath>(v1).value == qvariant_cast<QDBusObjectPath>(v2).value;

    else if (id == qMetaTypeId<QDBusSignature>())
        return qvariant_cast<QDBusSignature>(v1).value == qvariant_cast<QDBusSignature>(v2).value;

    else if (id == qMetaTypeId<QDBusVariant>())
        return compare(qvariant_cast<QDBusVariant>(v1).value, qvariant_cast<QDBusVariant>(v2).value);

    else if (id == qMetaTypeId<QVariant>())
        return compare(qvariant_cast<QVariant>(v1), qvariant_cast<QVariant>(v2));

    else if (id == qMetaTypeId<QList<bool> >()) 
        return qvariant_cast<QList<bool> >(v1) == qvariant_cast<QList<bool> >(v2);

    else if (id == qMetaTypeId<QList<short> >())
        return qvariant_cast<QList<short> >(v1) == qvariant_cast<QList<short> >(v2);

    else if (id == qMetaTypeId<QList<ushort> >())
        return qvariant_cast<QList<ushort> >(v1) == qvariant_cast<QList<ushort> >(v2);

    else if (id == qMetaTypeId<QList<int> >())
        return qvariant_cast<QList<int> >(v1) == qvariant_cast<QList<int> >(v2);

    else if (id == qMetaTypeId<QList<uint> >())
        return qvariant_cast<QList<uint> >(v1) == qvariant_cast<QList<uint> >(v2);

    else if (id == qMetaTypeId<QList<qlonglong> >())
        return qvariant_cast<QList<qlonglong> >(v1) == qvariant_cast<QList<qlonglong> >(v2);

    else if (id == qMetaTypeId<QList<qulonglong> >())
        return qvariant_cast<QList<qulonglong> >(v2) == qvariant_cast<QList<qulonglong> >(v2);

    else if (id == qMetaTypeId<QList<double> >())
        return compare(qvariant_cast<QList<double> >(v1), qvariant_cast<QList<double> >(v2));

    else if (id == qMetaTypeId<QVariant>())
        return compare(qvariant_cast<QVariant>(v1), qvariant_cast<QVariant>(v2));

    else if (id == qMetaTypeId<QList<QList<bool> > >()) 
        return qvariant_cast<QList<QList<bool> > >(v1) == qvariant_cast<QList<QList<bool> > >(v2);

    else if (id == qMetaTypeId<QList<QList<short> > >())
        return qvariant_cast<QList<QList<short> > >(v1) == qvariant_cast<QList<QList<short> > >(v2);

    else if (id == qMetaTypeId<QList<QList<ushort> > >())
        return qvariant_cast<QList<QList<ushort> > >(v1) == qvariant_cast<QList<QList<ushort> > >(v2);

    else if (id == qMetaTypeId<QList<QList<int> > >())
        return qvariant_cast<QList<QList<int> > >(v1) == qvariant_cast<QList<QList<int> > >(v2);

    else if (id == qMetaTypeId<QList<QList<uint> > >())
        return qvariant_cast<QList<QList<uint> > >(v1) == qvariant_cast<QList<QList<uint> > >(v2);

    else if (id == qMetaTypeId<QList<QList<qlonglong> > >())
        return qvariant_cast<QList<QList<qlonglong> > >(v1) == qvariant_cast<QList<QList<qlonglong> > >(v2);

    else if (id == qMetaTypeId<QList<QList<qulonglong> > >())
        return qvariant_cast<QList<QList<qulonglong> > >(v1) == qvariant_cast<QList<QList<qulonglong> > >(v2);

    else if (id == qMetaTypeId<QList<QList<double> > >())
        return compare(qvariant_cast<QList<QList<double> > >(v1), qvariant_cast<QList<QList<double> > >(v2));

    else if (id == qMetaTypeId<QList<QStringList> >())
        return qvariant_cast<QList<QStringList> >(v1) == qvariant_cast<QList<QStringList> >(v2);

    else if (id == qMetaTypeId<QList<QByteArray> >())
        return qvariant_cast<QList<QByteArray> >(v1) == qvariant_cast<QList<QByteArray> >(v2);

    else if (id == qMetaTypeId<QList<QVariantList> >())
        return compare(qvariant_cast<QList<QVariantList> >(v1), qvariant_cast<QList<QVariantList> >(v2));

    else if (id == qMetaTypeId<QMap<int, QString> >())
        return compare(qvariant_cast<QMap<int, QString> >(v1), qvariant_cast<QMap<int, QString> >(v2));

    else if (id == qMetaTypeId<QMap<QString, QString> >())
        return compare(qvariant_cast<QMap<QString, QString> >(v1), qvariant_cast<QMap<QString, QString> >(v2));

    else {
        qWarning() << "Please write a comparison case for type" << v1.typeName();
        return false;           // unknown type
    }
}

#if 0
bool compare(const QVariantList &l1, const QVariantList &l2)
{
    if (l1.count() != l2.size())
        return false;
    QVariantList::ConstIterator i1 = l1.constBegin();
    QVariantList::ConstIterator i2 = l2.constBegin();
    QVariantList::ConstIterator end = l1.constEnd();
    for ( ; i1 != end; ++i1, ++i2) {
        if (!compare(*i1, *i2))
            return false;
    }
    return true;
}

bool compare(const QVariantMap &m1, const QVariantMap &m2)
{
    if (m1.count() != m2.size())
        return false;
    QVariantMap::ConstIterator i1 = m1.constBegin();
    QVariantMap::ConstIterator end = m1.constEnd();
    for ( ; i1 != end; ++i1) {
        QVariantMap::ConstIterator i2 = m2.find(i1.key());
        if (i2 == m2.constEnd())
            return false;
        if (!compare(*i1, *i2))
            return false;
    }
    return true;
}
#endif
