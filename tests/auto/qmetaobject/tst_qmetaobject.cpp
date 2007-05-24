/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QtTest/QtTest>

#include <qobject.h>
#include <qmetaobject.h>
#include <qlabel.h>

//TESTED_CLASS=
//TESTED_FILES=corelib/kernel/qmetaobject.h corelib/kernel/qmetaobject.cpp

struct MyStruct
{
    int i;
};

namespace MyNamespace {
    class MyClass : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(MyEnum myEnum READ myEnum WRITE setMyEnum)
        Q_PROPERTY(MyFlags myFlags READ myFlags WRITE setMyFlags)
        Q_ENUMS(MyEnum)
        Q_FLAGS(MyFlags)
    public:
        enum MyEnum {
            MyEnum1,
            MyEnum2,
            MyEnum3
        };

        enum MyFlag {
            MyFlag1 = 0x01,
            MyFlag2 = 0x02,
            MyFlag3 = 0x04
        };
        Q_DECLARE_FLAGS(MyFlags, MyFlag)

        MyEnum myEnum() const { return m_enum; }
        void setMyEnum(MyEnum val) { m_enum = val; }

        MyFlags myFlags() const { return m_flags; }
        void setMyFlags(MyFlags val) { m_flags = val; }

        MyClass(QObject *parent = 0)
            : QObject(parent),
              m_enum(MyEnum1),
              m_flags(MyFlag1|MyFlag2)
                { }
    private:
        MyEnum m_enum;
        MyFlags m_flags;
    };
    Q_DECLARE_OPERATORS_FOR_FLAGS(MyClass::MyFlags)
}


class tst_QMetaObject : public QObject
{
    Q_OBJECT
    Q_ENUMS(EnumType)
    Q_PROPERTY(EnumType value WRITE setValue READ getValue)
    Q_PROPERTY(EnumType value2 WRITE set_value READ get_value)
    Q_PROPERTY(MyStruct value3 WRITE setVal3 READ val3)
    Q_PROPERTY(QList<QVariant> value4 WRITE setVal4 READ val4)
    Q_PROPERTY(QVariantList value5 WRITE setVal5 READ val5)

public:
    enum EnumType { EnumType1 };

    tst_QMetaObject();
    ~tst_QMetaObject();

    void setValue(EnumType) {}
    EnumType getValue() const { return EnumType1; }

    void set_value(EnumType) {}
    EnumType get_value() const { return EnumType1; }

    void setVal3(MyStruct) {}
    MyStruct val3() const { MyStruct s = {42}; return s; }

    void setVal4(const QList<QVariant> &list) { value4 = list; }
    QList<QVariant> val4() const { return value4; }

    void setVal5(const QVariantList &list) { value5 = list; }
    QVariantList val5() const { return value5; }

    QList<QVariant> value4;
    QVariantList value5;

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
private slots:
    void connectSlotsByName();
    void invokeMetaMember();
    void invokeQueuedMetaMember();
    void invokeCustomTypes();
    void qtMetaObjectInheritance();
    void normalizedSignature_data();
    void normalizedSignature();
    void normalizedType_data();
    void normalizedType();
    void customPropertyType();
    void checkScope();

    void stdSet();
};

tst_QMetaObject::tst_QMetaObject()
{

}

tst_QMetaObject::~tst_QMetaObject()
{

}

void tst_QMetaObject::initTestCase()
{
}

void tst_QMetaObject::cleanupTestCase()
{
}

void tst_QMetaObject::init()
{
}

void tst_QMetaObject::cleanup()
{
}

void tst_QMetaObject::stdSet()
{
    const QMetaObject *mo = metaObject();

    QMetaProperty prop = mo->property(mo->indexOfProperty("value"));
    QVERIFY(prop.isValid());
    QVERIFY(prop.hasStdCppSet());

    prop = mo->property(mo->indexOfProperty("value2"));
    QVERIFY(prop.isValid());
    QVERIFY(!prop.hasStdCppSet());
}

class CTestObject: public QObject
{
    Q_OBJECT

public:
    CTestObject(): QObject(), invokeCount1(0), invokeCount2(0)
    {
    }

    void fire(const QString &name)
    {
        child = new QObject(this);
        child->setObjectName(name);
        QMetaObject::connectSlotsByName(this);
        delete child; child = 0;
    }

    int invokeCount1;
    int invokeCount2;
    QObject *child;

public slots:
    void on_child1_destroyed(QObject *obj = 0) { ++invokeCount1; Q_ASSERT(obj && obj == child); }
    void on_child2_destroyed() { ++invokeCount2; }
};

class CTestObjectOverloads: public QObject
{
    Q_OBJECT

public:
    CTestObjectOverloads(): invokeCount1(0), invokeCount2(0) {}

    int invokeCount1;
    int invokeCount2;
    QObject *child;

    void fire(const QString &name)
    {
        child = new QObject(this);
        child->setObjectName(name);
        QMetaObject::connectSlotsByName(this);
        delete child; child = 0;
    }

private slots:
    void on_child1_destroyed(QObject *obj) { ++invokeCount1; Q_ASSERT(obj && obj == child); }
    void on_child1_destroyed() { ++invokeCount2; }
};

#if QT_VERSION >= 0x040200
#define FUNCTION(x) "QMetaObject::" x ": "
#else
#define FUNCTION(x) "QMetaObject::" x "(): "
#endif

void tst_QMetaObject::connectSlotsByName()
{
    CTestObject obj;
    QCOMPARE(obj.invokeCount1, 0);
    QCOMPARE(obj.invokeCount2, 0);

    QTest::ignoreMessage(QtWarningMsg, FUNCTION("connectSlotsByName") "No matching signal for on_child1_destroyed(QObject*)");
    QTest::ignoreMessage(QtWarningMsg, FUNCTION("connectSlotsByName") "No matching signal for on_child2_destroyed()");
    obj.fire("bubu");
    QCOMPARE(obj.invokeCount1, 0);
    QCOMPARE(obj.invokeCount2, 0);

    QTest::ignoreMessage(QtWarningMsg, FUNCTION("connectSlotsByName") "No matching signal for on_child2_destroyed()");
    obj.fire("child1");
    QCOMPARE(obj.invokeCount1, 1);
    QCOMPARE(obj.invokeCount2, 0);

    QTest::ignoreMessage(QtWarningMsg, FUNCTION("connectSlotsByName") "No matching signal for on_child1_destroyed(QObject*)");
    obj.fire("child2");
    QCOMPARE(obj.invokeCount1, 1);
    QCOMPARE(obj.invokeCount2, 1);

    QTest::ignoreMessage(QtWarningMsg, FUNCTION("connectSlotsByName") "No matching signal for on_child2_destroyed()");
    obj.fire("child1");
    QCOMPARE(obj.invokeCount1, 2);
    QCOMPARE(obj.invokeCount2, 1);

    // now test with real overloads
    CTestObjectOverloads obj2;
    obj2.fire("child1");
    QCOMPARE(obj2.invokeCount1, 1);
    QCOMPARE(obj2.invokeCount2, 1);
}

class QtTestObject: public QObject
{
    Q_OBJECT

public:
    QtTestObject();

public slots:
    void sl0();
    QString sl1(QString s1);
    void sl2(QString s1, QString s2);
    void sl3(QString s1, QString s2, QString s3);
    void sl4(QString s1, QString s2, QString s3, const QString s4);
    void sl5(QString s1, QString s2, QString s3, QString s4, const QString &s5);
    void sl6(QString s1, QString s2, QString s3, QString s4, const QString s5, QString s6);
    void sl7(QString s1, QString s2, QString s3, QString s4, QString s5, QString s6, QString s7);
    void sl8(QString s1, QString s2, QString s3, QString s4, QString s5, QString s6, QString s7,
            QString s8);
    void sl9(QString s1, QString s2, QString s3, QString s4, QString s5, QString s6, QString s7,
            QString s8, QString s9);
    void sl10(QString s1, QString s2, QString s3, QString s4, QString s5, QString s6, QString s7,
            QString s8, QString s9, QString s10);
    QObject *sl11();
    const char *sl12();
    QList<QString> sl13(QList<QString> l1);
    void testSender();

    void testReference(QString &str);

    void testLongLong(long long ll1, unsigned long long ll2);

signals:
    void sig0();
    QString sig1(QString s1);

public:
    QString slotResult;
};

QtTestObject::QtTestObject()
{
    connect(this, SIGNAL(sig0()), this, SLOT(sl0()));
    connect(this, SIGNAL(sig1(QString)), this, SLOT(sl1(QString)));
}

void QtTestObject::sl0() { slotResult = "sl0"; };
QString QtTestObject::sl1(QString s1) { slotResult = "sl1:" + s1; return "yessir"; }
void QtTestObject::sl2(QString s1, QString s2) { slotResult = "sl2:" + s1 + s2; }
void QtTestObject::sl3(QString s1, QString s2, QString s3)
{ slotResult = "sl3:" + s1 + s2 + s3; }
void QtTestObject::sl4(QString s1, QString s2, QString s3, const QString s4)
{ slotResult = "sl4:" + s1 + s2 + s3 + s4; }
void QtTestObject::sl5(QString s1, QString s2, QString s3, QString s4, const QString &s5)
{ slotResult = "sl5:" + s1 + s2 + s3 + s4 + s5; }
void QtTestObject::sl6(QString s1, QString s2, QString s3, QString s4,
                      const QString s5, QString s6)
{ slotResult = "sl6:" + s1 + s2 + s3 + s4 + s5 + s6; }
void QtTestObject::sl7(QString s1, QString s2, QString s3, QString s4, QString s5,
                      QString s6, QString s7)
{ slotResult = "sl7:" + s1 + s2 + s3 + s4 + s5 + s6 + s7; }
void QtTestObject::sl8(QString s1, QString s2, QString s3, QString s4, QString s5,
                      QString s6, QString s7, QString s8)
{ slotResult = "sl8:" + s1 + s2 + s3 + s4 + s5 + s6 + s7 + s8; }
void QtTestObject::sl9(QString s1, QString s2, QString s3, QString s4, QString s5,
                      QString s6, QString s7, QString s8, QString s9)
{ slotResult = "sl9:" + s1 + s2 + s3 + s4 + s5 + s6 + s7 + s8 + s9; }
void QtTestObject::sl10(QString s1, QString s2, QString s3, QString s4, QString s5,
                      QString s6, QString s7, QString s8, QString s9, QString s10)
{ slotResult = "sl10:" + s1 + s2 + s3 + s4 + s5 + s6 + s7 + s8 + s9 + s10; }
QObject *QtTestObject::sl11()
{ slotResult = "sl11"; return this; }
const char *QtTestObject::sl12()
{ slotResult = "sl12"; return "foo"; }
QList<QString> QtTestObject::sl13(QList<QString> l1)
{ slotResult = "sl13"; return l1; }
void QtTestObject::testReference(QString &str)
{ slotResult = "testReference:" + str; str = "gotcha"; }
void QtTestObject::testLongLong(long long ll1, unsigned long long ll2)
{ slotResult = "testLongLong:" + QString::number(ll1) + "," + QString::number(ll2); }

void QtTestObject::testSender()
{
  slotResult.sprintf("%p", sender());
}


void tst_QMetaObject::invokeMetaMember()
{
    QtTestObject obj;

    QString t1("1"); QString t2("2"); QString t3("3"); QString t4("4"); QString t5("5");
    QString t6("6"); QString t7("7"); QString t8("8"); QString t9("9"); QString t10("X");

    QVERIFY(!QMetaObject::invokeMethod(0, 0));
    QVERIFY(!QMetaObject::invokeMethod(0, "sl0"));
    QVERIFY(!QMetaObject::invokeMethod(&obj, 0));

    QVERIFY(QMetaObject::invokeMethod(&obj, "sl0"));
    QCOMPARE(obj.slotResult, QString("sl0"));

    QVERIFY(QMetaObject::invokeMethod(&obj, "sl1", Q_ARG(QString, t1)));
    QCOMPARE(obj.slotResult, QString("sl1:1"));

    QVERIFY(QMetaObject::invokeMethod(&obj, "sl2", Q_ARG(const QString, t1), Q_ARG(QString, t2)));
    QCOMPARE(obj.slotResult, QString("sl2:12"));

    QVERIFY(QMetaObject::invokeMethod(&obj, "sl3", Q_ARG(QString, t1), Q_ARG(QString, t2), Q_ARG(QString, t3)));
    QCOMPARE(obj.slotResult, QString("sl3:123"));

    QVERIFY(QMetaObject::invokeMethod(&obj, "sl4", Q_ARG(QString, t1), Q_ARG(QString, t2), Q_ARG(QString, t3),
                       Q_ARG(QString, t4)));
    QCOMPARE(obj.slotResult, QString("sl4:1234"));

    QVERIFY(QMetaObject::invokeMethod(&obj, "sl5", Q_ARG(QString, t1), Q_ARG(QString, t2), Q_ARG(QString, t3),
                       Q_ARG(QString, t4), Q_ARG(QString, "5")));
    QCOMPARE(obj.slotResult, QString("sl5:12345"));

    QVERIFY(QMetaObject::invokeMethod(&obj, "sl6", Q_ARG(QString, t1), Q_ARG(QString, t2), Q_ARG(QString, t3),
                       Q_ARG(QString, t4), Q_ARG(QString, t5), Q_ARG(QString, t6)));
    QCOMPARE(obj.slotResult, QString("sl6:123456"));

    QVERIFY(QMetaObject::invokeMethod(&obj, "sl7", Q_ARG(QString, t1), Q_ARG(QString, t2), Q_ARG(QString, t3),
                       Q_ARG(QString, t4), Q_ARG(QString, t5), Q_ARG(QString, t6),
                       Q_ARG(QString, t7)));
    QCOMPARE(obj.slotResult, QString("sl7:1234567"));

    QVERIFY(QMetaObject::invokeMethod(&obj, "sl8", Q_ARG(QString, t1), Q_ARG(QString, t2), Q_ARG(QString, t3),
                       Q_ARG(QString, t4), Q_ARG(QString, t5), Q_ARG(QString, t6),
                       Q_ARG(QString, t7), Q_ARG(QString, t8)));
    QCOMPARE(obj.slotResult, QString("sl8:12345678"));

    QVERIFY(QMetaObject::invokeMethod(&obj, "sl9", Q_ARG(QString, t1), Q_ARG(QString, t2), Q_ARG(QString, t3),
                       Q_ARG(QString, t4), Q_ARG(QString, t5), Q_ARG(QString, t6),
                       Q_ARG(QString, t7), Q_ARG(QString, t8), Q_ARG(QString, t9)));
    QCOMPARE(obj.slotResult, QString("sl9:123456789"));

    QVERIFY(QMetaObject::invokeMethod(&obj, "sl11"));
    QCOMPARE(obj.slotResult, QString("sl11"));

    QVERIFY(QMetaObject::invokeMethod(&obj, "testSender"));
    QCOMPARE(obj.slotResult, QString("0"));

    QString refStr("whatever");
    QVERIFY(QMetaObject::invokeMethod(&obj, "testReference", QGenericArgument("QString&", &refStr)));
    QCOMPARE(obj.slotResult, QString("testReference:whatever"));
    QCOMPARE(refStr, QString("gotcha"));

    long long ll1 = -1ll;
    unsigned long long ll2 = 0ull;
    QVERIFY(QMetaObject::invokeMethod(&obj,
                                      "testLongLong",
                                      Q_ARG(long long, ll1),
                                      Q_ARG(unsigned long long, ll2)));
    QCOMPARE(obj.slotResult, QString("testLongLong:-1,0"));

    QString exp;
    QVERIFY(QMetaObject::invokeMethod(&obj, "sl1", Q_RETURN_ARG(QString, exp), Q_ARG(QString, "bubu")));
    QCOMPARE(exp, QString("yessir"));
    QCOMPARE(obj.slotResult, QString("sl1:bubu"));

    QObject *ptr = 0;
    QVERIFY(QMetaObject::invokeMethod(&obj, "sl11", Q_RETURN_ARG(QObject*,ptr)));
    QCOMPARE(ptr, (QObject *)&obj);
    QCOMPARE(obj.slotResult, QString("sl11"));
    // try again with a space:
    ptr = 0;
    QVERIFY(QMetaObject::invokeMethod(&obj, "sl11", Q_RETURN_ARG(QObject * , ptr)));
    QCOMPARE(ptr, (QObject *)&obj);
    QCOMPARE(obj.slotResult, QString("sl11"));

    const char *ptr2 = 0;
    QVERIFY(QMetaObject::invokeMethod(&obj, "sl12", Q_RETURN_ARG(const char*, ptr2)));
    QVERIFY(ptr2 != 0);
    QCOMPARE(obj.slotResult, QString("sl12"));
    // try again with a space:
    ptr2 = 0;
    QVERIFY(QMetaObject::invokeMethod(&obj, "sl12", Q_RETURN_ARG(char const * , ptr2)));
    QVERIFY(ptr2 != 0);
    QCOMPARE(obj.slotResult, QString("sl12"));

    // test w/ template args
    QList<QString> returnValue, argument;
    argument << QString("one") << QString("two") << QString("three");
    QVERIFY(QMetaObject::invokeMethod(&obj, "sl13",
                                      Q_RETURN_ARG(QList<QString>, returnValue),
                                      Q_ARG(QList<QString>, argument)));
    QCOMPARE(returnValue, argument);
    QCOMPARE(obj.slotResult, QString("sl13"));

    //test signals
    QVERIFY(QMetaObject::invokeMethod(&obj, "sig0"));
    QCOMPARE(obj.slotResult, QString("sl0"));

    QVERIFY(QMetaObject::invokeMethod(&obj, "sig1", Q_ARG(QString, "baba")));
    QCOMPARE(obj.slotResult, QString("sl1:baba"));

    exp.clear();
    QVERIFY(QMetaObject::invokeMethod(&obj, "sig1", Q_RETURN_ARG(QString, exp), Q_ARG(QString, "hehe")));
    QCOMPARE(exp, QString("yessir"));
    QCOMPARE(obj.slotResult, QString("sl1:hehe"));
}

void tst_QMetaObject::invokeQueuedMetaMember()
{
    QtTestObject obj;

    QVERIFY(QMetaObject::invokeMethod(&obj, "sl0", Qt::QueuedConnection));
    QVERIFY(obj.slotResult.isEmpty());
    qApp->processEvents(QEventLoop::AllEvents);
    QCOMPARE(obj.slotResult, QString("sl0"));
    obj.slotResult = QString();

    QVERIFY(QMetaObject::invokeMethod(&obj, "sl1", Qt::QueuedConnection, Q_ARG(QString, QString("hallo"))));
    QVERIFY(obj.slotResult.isEmpty());
    qApp->processEvents(QEventLoop::AllEvents);
    QCOMPARE(obj.slotResult, QString("sl1:hallo"));
    obj.slotResult = QString();

    QVERIFY(QMetaObject::invokeMethod(&obj, "sl9", Qt::QueuedConnection, Q_ARG(QString, "1"), Q_ARG(QString, "2"),
                       Q_ARG(QString, "3"), Q_ARG(QString, "4"), Q_ARG(QString, "5"),
                       Q_ARG(QString, "6"), Q_ARG(QString, "7"), Q_ARG(QString, "8"),
                       Q_ARG(QString, "9")));
    QVERIFY(obj.slotResult.isEmpty());
    qApp->processEvents(QEventLoop::AllEvents);
    QCOMPARE(obj.slotResult, QString("sl9:123456789"));

    // signals

    obj.slotResult.clear();
    QVERIFY(QMetaObject::invokeMethod(&obj, "sig0", Qt::QueuedConnection));
    QVERIFY(obj.slotResult.isEmpty());
    qApp->processEvents(QEventLoop::AllEvents);
    QCOMPARE(obj.slotResult, QString("sl0"));

    QVERIFY(QMetaObject::invokeMethod(&obj, "sig1", Qt::QueuedConnection, Q_ARG(QString, "gogo")));
    qApp->processEvents(QEventLoop::AllEvents);
    QCOMPARE(obj.slotResult, QString("sl1:gogo"));

    QString exp;
    QTest::ignoreMessage(QtWarningMsg, "QMetaObject::invokeMethod: Unable to invoke methods with return values in queued connections");
    QVERIFY(!QMetaObject::invokeMethod(&obj, "sig1", Qt::QueuedConnection, Q_RETURN_ARG(QString, exp),
                              Q_ARG(QString, "nono")));

    long long ll1 = -1ll;
    unsigned long long ll2 = 0ull;
    QVERIFY(QMetaObject::invokeMethod(&obj,
                                      "testLongLong",
                                      Qt::QueuedConnection,
                                      Q_ARG(long long, ll1),
                                      Q_ARG(unsigned long long, ll2)));
    qApp->processEvents(QEventLoop::AllEvents);
    QCOMPARE(obj.slotResult, QString("testLongLong:-1,0"));
}


void tst_QMetaObject::qtMetaObjectInheritance()
{
    QVERIFY(QObject::staticMetaObject.superClass() == 0);
    QCOMPARE(QLabel::staticMetaObject.indexOfEnumerator("Qt::Alignment"), -1);
    QCOMPARE(QLabel::staticMetaObject.indexOfEnumerator("Alignment"), -1);
    int indexOfAlignment = QLabel::staticMetaObject.indexOfProperty("alignment");
    QVERIFY(indexOfAlignment != -1);
    QMetaProperty alignment = QLabel::staticMetaObject.property(indexOfAlignment);
    QVERIFY(alignment.isValid());
    QCOMPARE(alignment.enumerator().name(), "Alignment");
}

struct MyType
{
    int i1, i2, i3;
};

class QtTestCustomObject: public QObject
{
    Q_OBJECT
public:
    QtTestCustomObject(): QObject(), sum(0) {}

public slots:
    void sl1(MyType myType);

public:
    int sum;
};

void QtTestCustomObject::sl1(MyType myType)
{
    sum = myType.i1 + myType.i2 + myType.i3;
}

void tst_QMetaObject::invokeCustomTypes()
{
    QtTestCustomObject obj;
    MyType tp = {1, 1, 1};

    QCOMPARE(obj.sum, 0);
    QVERIFY(QMetaObject::invokeMethod(&obj, "sl1", Q_ARG(MyType, tp)));
    QCOMPARE(obj.sum, 3);
}

void tst_QMetaObject::normalizedSignature_data()
{
    QTest::addColumn<QString>("signature");
    QTest::addColumn<QString>("result");

    QTest::newRow("function") << "void foo()" << "void foo()";
    QTest::newRow("spaces") << " void  foo( ) " << "void foo()";
    QTest::newRow("template args") << " void  foo( QMap<a, a>, QList<b>) "
                                   << "void foo(QMap<a,a>,QList<b>)";
    QTest::newRow("rettype") << "QList<int, int> foo()" << "QList<int,int>foo()";
    QTest::newRow("const rettype") << "const QString *foo()" << "const QString*foo()";
    QTest::newRow("const ref") << "const QString &foo()" << "const QString&foo()";
    QTest::newRow("reference") << "QString &foo()" << "QString&foo()";
    QTest::newRow("const2") << "void foo(QString const *)" << "void foo(const QString*)";
    QTest::newRow("const2") << "void foo(QString * const)" << "void foo(QString*const)";
    QTest::newRow("const3") << "void foo(QString const &)" << "void foo(QString)";
    QTest::newRow("const4") << "void foo(const int)" << "void foo(int)";
    QTest::newRow("const5") << "void foo(const int, int const, const int &, int const &)"
                            << "void foo(int,int,int,int)";
    QTest::newRow("const6") << "void foo(QList<const int>)" << "void foo(QList<const int>)";
    QTest::newRow("const7") << "void foo(QList<const int*>)" << "void foo(QList<const int*>)";
    QTest::newRow("const7") << "void foo(QList<int const*>)" << "void foo(QList<const int*>)";
}

void tst_QMetaObject::normalizedSignature()
{
    QFETCH(QString, signature);
    QFETCH(QString, result);

    QCOMPARE(QString::fromLatin1(QMetaObject::normalizedSignature(signature.toLatin1())), result);
}

void tst_QMetaObject::normalizedType_data()
{
    QTest::addColumn<QString>("type");
    QTest::addColumn<QString>("result");

    QTest::newRow("simple") << "int" << "int";
    QTest::newRow("white") << "  int  " << "int";
    QTest::newRow("const1") << "int const *" << "const int*";
    QTest::newRow("const2") << "const int *" << "const int*";
    QTest::newRow("template1") << "QList<int const *>" << "QList<const int*>";
    QTest::newRow("template2") << "QList<const int *>" << "QList<const int*>";
    QTest::newRow("template3") << "QMap<QString, int>" << "QMap<QString,int>";
    QTest::newRow("template4") << "const QMap<QString, int> &" << "QMap<QString,int>";
    QTest::newRow("value1") << "const QString &" << "QString";
    QTest::newRow("value2") << "QString const &" << "QString";
}

void tst_QMetaObject::normalizedType()
{
    QFETCH(QString, type);
    QFETCH(QString, result);

    QCOMPARE(QString::fromLatin1(QMetaObject::normalizedType(type.toLatin1())), result);
}

void tst_QMetaObject::customPropertyType()
{
    QMetaProperty prop = metaObject()->property(metaObject()->indexOfProperty("value3"));

    QCOMPARE(prop.type(), QVariant::UserType);
    QCOMPARE(prop.userType(), 0);

    qRegisterMetaType<MyStruct>("MyStruct");
    QCOMPARE(prop.userType(), QMetaType::type("MyStruct"));

    prop = metaObject()->property(metaObject()->indexOfProperty("value4"));
    QCOMPARE(prop.type(), QVariant::List);

    prop = metaObject()->property(metaObject()->indexOfProperty("value5"));
    QCOMPARE(prop.type(), QVariant::List);
}

void tst_QMetaObject::checkScope()
{
    MyNamespace::MyClass obj;

    const QMetaObject *mo = obj.metaObject();
    QMetaEnum me = mo->enumerator(mo->indexOfEnumerator("MyEnum"));
    QVERIFY(me.isValid());
    QVERIFY(!me.isFlag());
    QCOMPARE(QLatin1String(me.scope()), QLatin1String("MyNamespace::MyClass"));
    QCOMPARE(me.keyToValue("MyNamespace::MyClass::MyEnum2"), 1);
    QCOMPARE(me.keyToValue("MyClass::MyEnum2"), -1);
    QCOMPARE(me.keyToValue("MyNamespace::MyEnum2"), -1);
    QCOMPARE(me.keyToValue("MyEnum2"), 1);
    QCOMPARE(me.keyToValue("MyEnum"), -1);
    QCOMPARE(QLatin1String(me.valueToKey(1)), QLatin1String("MyEnum2"));

    QMetaEnum mf = mo->enumerator(mo->indexOfEnumerator("MyFlags"));
    QVERIFY(mf.isValid());
    QVERIFY(mf.isFlag());
    QCOMPARE(QLatin1String(mf.scope()), QLatin1String("MyNamespace::MyClass"));
    QCOMPARE(mf.keysToValue("MyNamespace::MyClass::MyFlag2"), 2);
    QCOMPARE(mf.keysToValue("MyClass::MyFlag2"), -1);
    QCOMPARE(mf.keysToValue("MyNamespace::MyFlag2"), -1);
    QCOMPARE(mf.keysToValue("MyFlag2"), 2);
    QCOMPARE(mf.keysToValue("MyFlag"), -1);
    QCOMPARE(QLatin1String(mf.valueToKey(2)), QLatin1String("MyFlag2"));
    QCOMPARE(mf.keysToValue("MyNamespace::MyClass::MyFlag1|MyNamespace::MyClass::MyFlag2"), 3);
    QCOMPARE(mf.keysToValue("MyClass::MyFlag1|MyClass::MyFlag2"), -1);
    QCOMPARE(mf.keysToValue("MyNamespace::MyFlag1|MyNamespace::MyFlag2"), -1);
    QCOMPARE(mf.keysToValue("MyFlag1|MyFlag2"), 3);
    QCOMPARE(mf.keysToValue("MyFlag2|MyFlag2"), 2);
    QCOMPARE(mf.keysToValue("MyFlag1|MyNamespace::MyClass::MyFlag2"), 3);
    QCOMPARE(mf.keysToValue("MyNamespace::MyClass::MyFlag2|MyNamespace::MyClass::MyFlag2"), 2);
    QCOMPARE(QLatin1String(mf.valueToKeys(3)), QLatin1String("MyFlag1|MyFlag2"));
}

QTEST_MAIN(tst_QMetaObject)
#include "tst_qmetaobject.moc"
