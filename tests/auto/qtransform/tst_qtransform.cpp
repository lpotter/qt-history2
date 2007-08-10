/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QtTest/QtTest>
#include "qtransform.h"
#include <math.h>
#include <qpolygon.h>
#include <qdebug.h>

Q_DECLARE_METATYPE(QRect)

//TESTED_CLASS=QTransform
//TESTED_FILES=gui/painting/qtransform.h gui/painting/qtransform.cpp

class tst_QTransform : public QObject
{
    Q_OBJECT

public:
    tst_QTransform();
    virtual ~tst_QTransform();


public slots:
    void init();
    void cleanup();
private slots:
    void mapRect_data();
    void operator_star_qrect_data();
    void mapToPolygon_data();
    void mapRect();
    void operator_star_qrect();
    void assignments();
    void mapToPolygon();
    void translate();
    void scale();
    void matrix();
    void testOffset();
    void types();
    void scalarOps();
    void transform();

private:
    void mapping_data();
};

Q_DECLARE_METATYPE(QTransform)
Q_DECLARE_METATYPE(QPolygon)

tst_QTransform::tst_QTransform()
{
}

tst_QTransform::~tst_QTransform()
{
}

void tst_QTransform::init()
{
    // No initialisation is required
}

void tst_QTransform::cleanup()
{
    // No cleanup is required.
}

void tst_QTransform::mapRect_data()
{
    mapping_data();
}

void tst_QTransform::operator_star_qrect_data()
{
    mapping_data();
}

void tst_QTransform::mapToPolygon_data()
{
    mapping_data();
}

void tst_QTransform::mapping_data()
{
    //create the testtable instance and define the elements
    QTest::addColumn<QTransform>("matrix");
    QTest::addColumn<QRect>("src");
    QTest::addColumn<QPolygon>("res");

    //next we fill it with data

    // identity
    QTest::newRow( "identity" )  << QTransform( 1, 0, 0, 1, 0, 0 )
			       << QRect( 10, 20, 30, 40 )
			       << QPolygon( QRect( 10, 20, 30, 40 ) );
    // scaling
    QTest::newRow( "scale 0" )  << QTransform( 2, 0, 0, 2, 0, 0 )
			      << QRect( 10, 20, 30, 40 )
			      << QPolygon( QRect( 20, 40, 60, 80 ) );
    QTest::newRow( "scale 1" )  << QTransform( 10, 0, 0, 10, 0, 0 )
			      << QRect( 10, 20, 30, 40 )
			      << QPolygon( QRect( 100, 200, 300, 400 ) );
    // mirroring
    QTest::newRow( "mirror 0" )  << QTransform( -1, 0, 0, 1, 0, 0 )
			       << QRect( 10, 20, 30, 40 )
			       << QPolygon( QRect( -40, 20, 30, 40 ) );
    QTest::newRow( "mirror 1" )  << QTransform( 1, 0, 0, -1, 0, 0 )
			       << QRect( 10, 20, 30, 40 )
			       << QPolygon( QRect( 10, -60, 30, 40 ) );
    QTest::newRow( "mirror 2" )  << QTransform( -1, 0, 0, -1, 0, 0 )
			       << QRect( 10, 20, 30, 40 )
			       << QPolygon( QRect( -40, -60, 30, 40 ) );
    QTest::newRow( "mirror 3" )  << QTransform( -2, 0, 0, -2, 0, 0 )
			       << QRect( 10, 20, 30, 40 )
			       << QPolygon( QRect( -80, -120, 60, 80 ) );
    QTest::newRow( "mirror 4" )  << QTransform( -10, 0, 0, -10, 0, 0 )
			       << QRect( 10, 20, 30, 40 )
			       << QPolygon( QRect( -400, -600, 300, 400 ) );
    QTest::newRow( "mirror 5" )  << QTransform( -1, 0, 0, 1, 0, 0 )
			       << QRect( 0, 0, 30, 40 )
			       << QPolygon( QRect( -30, 0, 30, 40 ) );
    QTest::newRow( "mirror 6" )  << QTransform( 1, 0, 0, -1, 0, 0 )
			       << QRect( 0, 0, 30, 40 )
			       << QPolygon( QRect( 0, -40, 30, 40 ) );
    QTest::newRow( "mirror 7" )  << QTransform( -1, 0, 0, -1, 0, 0 )
			       << QRect( 0, 0, 30, 40 )
			       << QPolygon( QRect( -30, -40, 30, 40 ) );
    QTest::newRow( "mirror 8" )  << QTransform( -2, 0, 0, -2, 0, 0 )
			       << QRect( 0, 0, 30, 40 )
			       << QPolygon( QRect( -60, -80, 60, 80 ) );
    QTest::newRow( "mirror 9" )  << QTransform( -10, 0, 0, -10, 0, 0 )
			       << QRect( 0, 0, 30, 40 )
			       << QPolygon( QRect( -300, -400, 300, 400 ) );

#ifdef Q_OS_WIN32
#define M_PI 3.14159265897932384626433832795f
#endif

    // rotations
    float deg = 0.;
    QTest::newRow( "rot 0 a" )  << QTransform( cos( M_PI*deg/180. ), -sin( M_PI*deg/180. ),
						sin( M_PI*deg/180. ),  cos( M_PI*deg/180. ), 0, 0 )
			      << QRect( 0, 0, 30, 40 )
			      << QPolygon ( QRect( 0, 0, 30, 40 ) );
    deg = 0.00001f;
    QTest::newRow( "rot 0 b" )  << QTransform( cos( M_PI*deg/180. ), -sin( M_PI*deg/180. ),
						sin( M_PI*deg/180. ),  cos( M_PI*deg/180. ), 0, 0 )
			      << QRect( 0, 0, 30, 40 )
			      << QPolygon ( QRect( 0, 0, 30, 40 ) );
    deg = 0.;
    QTest::newRow( "rot 0 c" )  << QTransform( cos( M_PI*deg/180. ), -sin( M_PI*deg/180. ),
						sin( M_PI*deg/180. ),  cos( M_PI*deg/180. ), 0, 0 )
			       << QRect( 10, 20, 30, 40 )
			      << QPolygon ( QRect( 10, 20, 30, 40 ) );
    deg = 0.00001f;
    QTest::newRow( "rot 0 d" )  << QTransform( cos( M_PI*deg/180. ), -sin( M_PI*deg/180. ),
						sin( M_PI*deg/180. ),  cos( M_PI*deg/180. ), 0, 0 )
			       << QRect( 10, 20, 30, 40 )
			      << QPolygon ( QRect( 10, 20, 30, 40 ) );

#if 0
    // rotations
    deg = 90.;
    QTest::newRow( "rotscale 90 a" )  << QTransform( 10*cos( M_PI*deg/180. ), -10*sin( M_PI*deg/180. ),
						10*sin( M_PI*deg/180. ),  10*cos( M_PI*deg/180. ), 0, 0 )
			       << QRect( 0, 0, 30, 40 )
			       << QPolygon( QRect( 0, -299, 400, 300 ) );
    deg = 90.00001;
    QTest::newRow( "rotscale 90 b" )  << QTransform( 10*cos( M_PI*deg/180. ), -10*sin( M_PI*deg/180. ),
						10*sin( M_PI*deg/180. ),  10*cos( M_PI*deg/180. ), 0, 0 )
			       << QRect( 0, 0, 30, 40 )
			       << QPolygon( QRect( 0, -299, 400, 300 ) );
    deg = 90.;
    QTest::newRow( "rotscale 90 c" )  << QTransform( 10*cos( M_PI*deg/180. ), -10*sin( M_PI*deg/180. ),
						10*sin( M_PI*deg/180. ),  10*cos( M_PI*deg/180. ), 0, 0 )
			       << QRect( 10, 20, 30, 40 )
				     << QPolygon( QRect( 200, -399, 400, 300 ) );
    deg = 90.00001;
    QTest::newRow( "rotscale 90 d" )  << QTransform( 10*cos( M_PI*deg/180. ), -10*sin( M_PI*deg/180. ),
						10*sin( M_PI*deg/180. ),  10*cos( M_PI*deg/180. ), 0, 0 )
			       << QRect( 10, 20, 30, 40 )
				     << QPolygon( QRect( 200, -399, 400, 300 ) );

    deg = 180.;
    QTest::newRow( "rotscale 180 a" )  << QTransform( 10*cos( M_PI*deg/180. ), -10*sin( M_PI*deg/180. ),
						10*sin( M_PI*deg/180. ),  10*cos( M_PI*deg/180. ), 0, 0 )
			       << QRect( 0, 0, 30, 40 )
				      << QPolygon( QRect( -299, -399, 300, 400 ) );
    deg = 180.000001;
    QTest::newRow( "rotscale 180 b" )  << QTransform( 10*cos( M_PI*deg/180. ), -10*sin( M_PI*deg/180. ),
						       10*sin( M_PI*deg/180. ),  10*cos( M_PI*deg/180. ), 0, 0 )
				      << QRect( 0, 0, 30, 40 )
				      << QPolygon( QRect( -299, -399, 300, 400 ) );
    deg = 180.;
    QTest::newRow( "rotscale 180 c" )  << QTransform( 10*cos( M_PI*deg/180. ), -10*sin( M_PI*deg/180. ),
						10*sin( M_PI*deg/180. ),  10*cos( M_PI*deg/180. ), 0, 0 )
				      << QRect( 10, 20, 30, 40 )
				      << QPolygon( QRect( -399, -599, 300, 400 ) );
    deg = 180.000001;
    QTest::newRow( "rotscale 180 d" )  << QTransform( 10*cos( M_PI*deg/180. ), -10*sin( M_PI*deg/180. ),
						10*sin( M_PI*deg/180. ),  10*cos( M_PI*deg/180. ), 0, 0 )
				      << QRect( 10, 20, 30, 40 )
				      << QPolygon( QRect( -399, -599, 300, 400 ) );

    deg = 270.;
    QTest::newRow( "rotscale 270 a" )  << QTransform( 10*cos( M_PI*deg/180. ), -10*sin( M_PI*deg/180. ),
						10*sin( M_PI*deg/180. ),  10*cos( M_PI*deg/180. ), 0, 0 )
				      << QRect( 0, 0, 30, 40 )
				      << QPolygon( QRect( -399, 00, 400, 300 ) );
    deg = 270.0000001;
    QTest::newRow( "rotscale 270 b" )  << QTransform( 10*cos( M_PI*deg/180. ), -10*sin( M_PI*deg/180. ),
						10*sin( M_PI*deg/180. ),  10*cos( M_PI*deg/180. ), 0, 0 )
				      << QRect( 0, 0, 30, 40 )
				      << QPolygon( QRect( -399, 00, 400, 300 ) );
    deg = 270.;
    QTest::newRow( "rotscale 270 c" )  << QTransform( 10*cos( M_PI*deg/180. ), -10*sin( M_PI*deg/180. ),
						10*sin( M_PI*deg/180. ),  10*cos( M_PI*deg/180. ), 0, 0 )
				      << QRect( 10, 20, 30, 40 )
				      << QPolygon( QRect( -599, 100, 400, 300 ) );
    deg = 270.000001;
    QTest::newRow( "rotscale 270 d" )  << QTransform( 10*cos( M_PI*deg/180. ), -10*sin( M_PI*deg/180. ),
						10*sin( M_PI*deg/180. ),  10*cos( M_PI*deg/180. ), 0, 	0 )
				      << QRect( 10, 20, 30, 40 )
				      << QPolygon( QRect( -599, 100, 400, 300 ) );

    // rotations that are not multiples of 90 degrees. mapRect returns the bounding rect here.
    deg = 45;
    QTest::newRow( "rot 45 a" )  << QTransform( cos( M_PI*deg/180. ), -sin( M_PI*deg/180. ),
						sin( M_PI*deg/180. ),  cos( M_PI*deg/180. ), 0, 0 )
				<< QRect( 0, 0, 10, 10 )
				<< QPolygon( QRect( 0, -7, 14, 14 ) );
    QTest::newRow( "rot 45 b" )  << QTransform( cos( M_PI*deg/180. ), -sin( M_PI*deg/180. ),
						sin( M_PI*deg/180. ),  cos( M_PI*deg/180. ), 0, 0 )
			       << QRect( 10, 20, 30, 40 )
				<< QPolygon( QRect( 21, -14, 49, 49 ) );
    QTest::newRow( "rot 45 c" )  << QTransform( 10*cos( M_PI*deg/180. ), -10*sin( M_PI*deg/180. ),
						10*sin( M_PI*deg/180. ),  10*cos( M_PI*deg/180. ), 0, 0 )
			       << QRect( 0, 0, 10, 10 )
				<< QPolygon( QRect( 0, -70, 141, 141 ) );
    QTest::newRow( "rot 45 d" )  << QTransform( 10*cos( M_PI*deg/180. ), -10*sin( M_PI*deg/180. ),
						10*sin( M_PI*deg/180. ),  10*cos( M_PI*deg/180. ), 0, 0 )
			       << QRect( 10, 20, 30, 40 )
				<< QPolygon( QRect( 212, -141, 495, 495 ) );

    deg = -45;
    QTest::newRow( "rot -45 a" )  << QTransform( cos( M_PI*deg/180. ), -sin( M_PI*deg/180. ),
						sin( M_PI*deg/180. ),  cos( M_PI*deg/180. ), 0, 0 )
			       << QRect( 0, 0, 10, 10 )
				 << QPolygon( QRect( -7, 0, 14, 14 ) );
    QTest::newRow( "rot -45 b" )  << QTransform( cos( M_PI*deg/180. ), -sin( M_PI*deg/180. ),
						sin( M_PI*deg/180. ),  cos( M_PI*deg/180. ), 0, 0 )
			       << QRect( 10, 20, 30, 40 )
				 << QPolygon( QRect( -35, 21, 49, 49 ) );
    QTest::newRow( "rot -45 c" )  << QTransform( 10*cos( M_PI*deg/180. ), -10*sin( M_PI*deg/180. ),
						10*sin( M_PI*deg/180. ),  10*cos( M_PI*deg/180. ), 0, 0 )
			       << QRect( 0, 0, 10, 10 )
				 << QPolygon( QRect( -70, 0, 141, 141 ) );
    QTest::newRow( "rot -45 d" )  << QTransform( 10*cos( M_PI*deg/180. ), -10*sin( M_PI*deg/180. ),
						10*sin( M_PI*deg/180. ),  10*cos( M_PI*deg/180. ), 0, 0 )
			       << QRect( 10, 20, 30, 40 )
				 << QPolygon( QRect( -353, 212, 495, 495 ) );
#endif
}

void tst_QTransform::mapRect()
{
    QFETCH( QTransform, matrix );
    QFETCH( QRect, src );
    QTEST( QPolygon( matrix.mapRect(src) ), "res" );
}

void tst_QTransform::operator_star_qrect()
{
#if 0
    QFETCH( QTransform, matrix );
    QFETCH( QRect, src );
    QFETCH( QPolygon, res );

    QCOMPARE( (matrix * src), QRegion(res) );
#endif
}

void tst_QTransform::assignments()
{
    QTransform m;
    m.scale(2, 3);
    m.rotate(45);
    m.shear(4, 5);

    QTransform c1(m);

    QCOMPARE(m.m11(), c1.m11());
    QCOMPARE(m.m12(), c1.m12());
    QCOMPARE(m.m21(), c1.m21());
    QCOMPARE(m.m22(), c1.m22());
    QCOMPARE(m.dx(), c1.dx());
    QCOMPARE(m.dy(), c1.dy());

    QTransform c2 = m;
    QCOMPARE(m.m11(), c2.m11());
    QCOMPARE(m.m12(), c2.m12());
    QCOMPARE(m.m21(), c2.m21());
    QCOMPARE(m.m22(), c2.m22());
    QCOMPARE(m.dx(),  c2.dx());
    QCOMPARE(m.dy(),  c2.dy());
}


void tst_QTransform::mapToPolygon()
{
    QFETCH( QTransform, matrix );
    QFETCH( QRect, src );
    QFETCH( QPolygon, res );

    QCOMPARE( matrix.mapToPolygon( src ), res );
}


void tst_QTransform::translate()
{
    QTransform m( 1, 2, 3, 4, 5, 6 );
    QTransform res2( m );
    QTransform res( 1, 2, 3, 4, 75, 106 );
    m.translate( 10,  20 );
    QVERIFY( m == res );
    m.translate( -10,  -20 );
    QVERIFY( m == res2 );
}

void tst_QTransform::scale()
{
    QTransform m( 1, 2, 3, 4, 5, 6 );
    QTransform res2( m );
    QTransform res( 10, 20, 60, 80, 5, 6 );
    m.scale( 10,  20 );
    QVERIFY( m == res );
    m.scale( 1./10.,  1./20. );
    QVERIFY( m == res2 );
}

void tst_QTransform::matrix()
{
    QMatrix mat1;
    mat1.scale(0.3, 0.7);
    mat1.translate(53.3, 94.4);
    mat1.rotate(45);

    QMatrix mat2;
    mat2.rotate(33);
    mat2.scale(0.6, 0.6);
    mat2.translate(13.333, 7.777);

    QTransform tran1(mat1);
    QTransform tran2(mat2);
    QTransform dummy;
    dummy.setMatrix(mat1.m11(), mat1.m12(), 0,
                    mat1.m21(), mat1.m22(), 0,
                    mat1.dx(), mat1.dy(), 1);

    QVERIFY(tran1 == dummy);
    QVERIFY(tran1.inverted() == dummy.inverted());
    QVERIFY(tran1.inverted() == QTransform(mat1.inverted()));
    QVERIFY(tran2.inverted() == QTransform(mat2.inverted()));

    QMatrix mat3 = mat1 * mat2;
    QTransform tran3 = tran1 * tran2;
    QVERIFY(QTransform(mat3) == tran3);
    QVERIFY(mat3 == tran3.toAffine());

    QTransform tranInv = tran1.inverted();
    QMatrix   matInv = mat1.inverted();

    QRect rect(43, 70, 200, 200);
    QPoint pt(43, 66);
    QVERIFY(tranInv.map(pt) == matInv.map(pt));
    QVERIFY(tranInv.map(pt) == matInv.map(pt));

    QPainterPath path;
    path.moveTo(55, 60);
    path.lineTo(110, 110);
    path.quadTo(220, 50, 10, 20);
    path.closeSubpath();
    QVERIFY(tranInv.map(path) == matInv.map(path));
}

void tst_QTransform::testOffset()
{
    QTransform trans;
    const QMatrix &aff = trans.toAffine();
    QCOMPARE((void*)(&aff), (void*)(&trans));
}

void tst_QTransform::types()
{
    QTransform m1;
    QCOMPARE(m1.type(), QTransform::TxNone);

    m1.translate(1.0f, 0.0f);
    QCOMPARE(m1.type(), QTransform::TxTranslate);
    QCOMPARE(m1.inverted().type(), QTransform::TxTranslate);

    m1.scale(1.0f, 2.0f);
    QCOMPARE(m1.type(), QTransform::TxScale);
    QCOMPARE(m1.inverted().type(), QTransform::TxScale);

    m1.rotate(45.0f);
    QCOMPARE(m1.type(), QTransform::TxRotate);
    QCOMPARE(m1.inverted().type(), QTransform::TxRotate);

    m1.shear(0.5f, 0.25f);
    QCOMPARE(m1.type(), QTransform::TxShear);
    QCOMPARE(m1.inverted().type(), QTransform::TxShear);

    m1.rotate(45.0f, Qt::XAxis);
    QCOMPARE(m1.type(), QTransform::TxProject);
    m1.shear(0.5f, 0.25f);
    QCOMPARE(m1.type(), QTransform::TxProject);
    m1.rotate(45.0f);
    QCOMPARE(m1.type(), QTransform::TxProject);
    m1.scale(1.0f, 2.0f);
    QCOMPARE(m1.type(), QTransform::TxProject);
    m1.translate(1.0f, 0.0f);
    QCOMPARE(m1.type(), QTransform::TxProject);

    QTransform m2(1.0f, 0.0f, 0.0f,
                  0.0f, 1.0f, 0.0f,
                  -1.0f, -1.0f, 1.0f);

    QCOMPARE(m2.type(), QTransform::TxTranslate);
    QCOMPARE((m1 * m2).type(), QTransform::TxProject);

    m1 *= QTransform();
    QCOMPARE(m1.type(), QTransform::TxProject);

    m1 *= QTransform(1.0f, 0.0f, 0.0f,
                     0.0f, 1.0f, 0.0f,
                     1.0f, 0.0f, 1.0f);
    QCOMPARE(m1.type(), QTransform::TxProject);

    m2.reset();
    QCOMPARE(m2.type(), QTransform::TxNone);

    m2.setMatrix(1.0f, 0.0f, 0.0f,
                 0.0f, 1.0f, 0.0f,
                 0.0f, 0.0f, 1.0f);
    QCOMPARE(m2.type(), QTransform::TxNone);

    m2 *= QTransform();
    QCOMPARE(m2.type(), QTransform::TxNone);

    m2.setMatrix(2.0f, 0.0f, 0.0f,
                 0.0f, 1.0f, 0.0f,
                 0.0f, 0.0f, 1.0f);
    QCOMPARE(m2.type(), QTransform::TxScale);
    m2 *= QTransform();
    QCOMPARE(m2.type(), QTransform::TxScale);

    m2.setMatrix(0.0f, 1.0f, 0.0f,
                 1.0f, 0.0f, 0.0f,
                 0.0f, 1.0f, 1.0f);
    QCOMPARE(m2.type(), QTransform::TxRotate);
    m2 *= QTransform();
    QCOMPARE(m2.type(), QTransform::TxRotate);

    m2.setMatrix(1.0f, 0.0f, 0.5f,
                 0.0f, 1.0f, 0.0f,
                 0.0f, 0.0f, 1.0f);
    QCOMPARE(m2.type(), QTransform::TxProject);
    m2 *= QTransform();
    QCOMPARE(m2.type(), QTransform::TxProject);

    m2.setMatrix(1.0f, 1.0f, 0.0f,
                 1.0f, 0.0f, 0.0f,
                 0.0f, 1.0f, 1.0f);
    QCOMPARE(m2.type(), QTransform::TxShear);

    m2 *= m2.inverted();
    QCOMPARE(m2.type(), QTransform::TxNone);

    m2.translate(5.0f, 5.0f);
    m2.rotate(45.0f);
    m2.rotate(-45.0f);
    QCOMPARE(m2.type(), QTransform::TxTranslate);

    m2.scale(2.0f, 3.0f);
    m2.shear(1.0f, 0.0f);
    m2.shear(-1.0f, 0.0f);
    QCOMPARE(m2.type(), QTransform::TxScale);

    m2 *= QTransform(1.0f, 1.0f, 0.0f,
                     0.0f, 1.0f, 0.0f,
                     0.0f, 0.0f, 1.0f);
    QCOMPARE(m2.type(), QTransform::TxShear);

    m2 *= QTransform(1.0f, 0.0f, 0.0f,
                     0.0f, 1.0f, 0.0f,
                     1.0f, 0.0f, 1.0f);
    QCOMPARE(m2.type(), QTransform::TxShear);

    QTransform m3(1.8f, 0.0f, 0.0f,
                  0.0f, 1.8f, 0.0f,
                  0.0f, 0.0f, 1.0f);

    QCOMPARE(m3.type(), QTransform::TxScale);
    m3.translate(5.0f, 5.0f);
    QCOMPARE(m3.type(), QTransform::TxScale);
    QCOMPARE(m3.inverted().type(), QTransform::TxScale);
}


void tst_QTransform::scalarOps()
{
    QTransform t;
    QCOMPARE(t.m11(), 1.);
    QCOMPARE(t.m33(), 1.);
    QCOMPARE(t.m21(), 0.);

    t = QTransform() + 3;
    QCOMPARE(t.m11(), 4.);
    QCOMPARE(t.m33(), 4.);
    QCOMPARE(t.m21(), 3.);

    t = t - 3;
    QCOMPARE(t.m11(), 1.);
    QCOMPARE(t.m33(), 1.);
    QCOMPARE(t.m21(), 0.);
    QCOMPARE(t.isIdentity(), true);

    t += 3;
    t = t * 2;
    QCOMPARE(t.m11(), 8.);
    QCOMPARE(t.m33(), 8.);
    QCOMPARE(t.m21(), 6.);
}

void tst_QTransform::transform()
{
    QTransform t;
    t.rotate(30, Qt::YAxis);
    t.translate(15, 10);
    t.scale(2, 2);
    t.rotate(30);
    t.shear(0.5, 0.5);

    QTransform a, b, c, d, e;
    a.rotate(30, Qt::YAxis);
    b.translate(15, 10);
    c.scale(2, 2);
    d.rotate(30);
    e.shear(0.5, 0.5);

    QCOMPARE(t, e * d * c * b * a);
}

QTEST_APPLESS_MAIN(tst_QTransform)


#include "tst_qtransform.moc"
