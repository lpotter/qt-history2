/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qregion.cpp#24 $
**
** Implementation of QRegion class
**
** Created : 950726
**
** Copyright (C) 1995-1997 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#include "qregion.h"
#include "qpntarry.h"
#include "qbuffer.h"
#include "qdstream.h"

/*!
  \class QRegion qregion.h
  \brief The QRegion class specifies a clip region for a painter.

  \ingroup drawing

  A region can be a rectangle, an ellipse, a polygon or a combination
  of these.

  Regions are combined by creating a new region which is a
  union, intersection or difference between any two regions.

  The region XOR operation is defined as:
  \code
    a XOR b = (a UNION b) - (a INTERSECTION b)
  \endcode

  Example of using complex regions:
  \code
    void MyWidget::paintEvent( QPaintEvent * )
    {
	QPainter p;				// our painter
	QRegion r1( QRect(100,100,200,80),	// r1 = elliptic region
		    QRegion::Ellipse );
	QRegion r2( QRect(100,120,90,30) );	// r2 = rectangular region
	QRegion r3 = r1.intersect( r2 );	// r3 = intersection
	p.begin( this );			// start painting widget
	p.setClipRegion( r3 );			// set clip region
	...					// paint clipped graphics
	p.end();				// painting done
    }
  \endcode

  \sa QPainter::setClipRegion(), QPainter::setClipRect()
*/


#if QT_VERSION == 200
#error "Remove QRegionData::unused* entirely"
#endif

QByteArray QRegion::QRegionData::unused_shared;	// ### remove in 2.0


/*****************************************************************************
  QRegion member functions
 *****************************************************************************/

/*!
  Constructs a rectangular or elliptic region.

  \a x, \a y, \a w, and \a h specify the region rectangle.
  \a t is the region type: QRegion::Rectangle (default) or
  QRegion::Ellipse.
*/
QRegion::QRegion( int x, int y, int w, int h, RegionType t )
{
    QRegion tmp(QRect(x,y,w,h),t);
    tmp.data->ref();
    data = tmp.data;
}

/*!
  Detaches from shared region data to makes sure that this region is the
  only one referring the data.

  If multiple regions share common data, this region dereferences the data
  and gets a copy of the data. Nothing is done if there is just a single
  reference.
*/

void QRegion::detach()
{
    if ( data->count != 1 )
	*this = copy();
}


/*!
  Obsolete function.
*/

void QRegion::cmd( int, void *, const QRegion *, const QRegion * )
{
#if defined(DEBUG)
    debug( "QRegion::cmd: This is an obsolete function and should not be called" );
#endif
}


/*!
  Executes region commands in the internal buffer and rebuild the original
  region.

  We do this when we read a region from the data stream.
*/

void QRegion::exec( const QByteArray &buffer )
{
    QBuffer buf( buffer );
    QDataStream s( &buf );
    buf.open( IO_ReadOnly );
    QRegion rgn;
#if defined(DEBUG)
    int test_cnt = 0;
#endif
    while ( !s.eof() ) {
	int id;
	s >> id;
#if defined(DEBUG)
	if ( test_cnt > 0 && id != QRGN_TRANSLATE )
	    warning( "QRegion::exec: Internal error" );
	test_cnt++;
#endif
	if ( id == QRGN_SETRECT || id == QRGN_SETELLIPSE ) {
	    QRect r;
	    s >> r;
	    rgn = QRegion( r, id == QRGN_SETRECT ? Rectangle : Ellipse );
	} else if ( id == QRGN_SETPTARRAY_ALT || id == QRGN_SETPTARRAY_WIND ) {
	    QPointArray a;
	    s >> a;
	    rgn = QRegion( a, id == QRGN_SETPTARRAY_WIND );
	} else if ( id == QRGN_TRANSLATE ) {
	    QPoint p;
	    s >> p;
	    rgn.translate( p.x(), p.y() );
	} else if ( id >= QRGN_OR && id <= QRGN_XOR ) {
	    QByteArray bop1, bop2;
	    QRegion r1, r2;
	    s >> bop1;	r1.exec( bop1 );
	    s >> bop2;	r2.exec( bop2 );
	    switch ( id ) {
		case QRGN_OR:
		    rgn = r1.unite( r2 );
		    break;
		case QRGN_AND:
		    rgn = r1.intersect( r2 );
		    break;
		case QRGN_SUB:
		    rgn = r1.subtract( r2 );
		    break;
		case QRGN_XOR:
		    rgn = r1.eor( r2 );
		    break;
	    }
	} else if ( id == QRGN_RECTS ) {	// ### will appear in Qt 2.0
	    Q_UINT32 n;
	    s >> n;
	    QRect r;
	    for ( int i=0; i<(int)n; i++ ) {
		s >> r;
		rgn = rgn.unite( QRegion(r) );
	    }
	}
    }
    buf.close();
    *this = rgn;
}


/*****************************************************************************
  QRegion stream functions
 *****************************************************************************/

/*!
  \relates QRegion
  Writes a region to the stream and returns a reference to the stream.
*/

QDataStream &operator<<( QDataStream &s, const QRegion &r )
{
    QRect  rect = r.data->rect;
    QPoint offs = r.data->offs;
    rect.moveBy( offs.x(), offs.y() );
    QPointArray poly;
    if ( r.data->poly ) {
	poly = *(r.data->poly);
	if ( !offs.isNull() )
	    poly.translate( offs.x(), offs.y() );
    }
    // First comes the size of the total data, then the
    // actual data.
    switch ( r.data->type ) {
	case QRegion::QRegionData::NullRgn:
	    s << (Q_UINT32)0;
	    break;
	case QRegion::QRegionData::RectangleRgn:
	    s << (Q_UINT32)(4+8) << (int)QRGN_SETRECT << rect;
	    break;
	case QRegion::QRegionData::EllipseRgn:
	    s << (Q_UINT32)(4+8) << (int)QRGN_SETELLIPSE << rect;
	    break;
	case QRegion::QRegionData::PolygonAltRgn:
	    s << (Q_UINT32)(4+4*poly.size()) << (int)QRGN_SETPTARRAY_ALT
	      << poly;
	    break;
	case QRegion::QRegionData::PolygonWindRgn:
	    s << (Q_UINT32)(4+4*poly.size()) << (int)QRGN_SETPTARRAY_WIND
	      << poly;
	    break;
#if QT_VERSION >= 200
	case QRegion::QRegionData::ComplexRgn:
	    {
		QArray<QRect> a = r.rects();
		s << (Q_UINT32)(4+4+8*a.size());
		s << (int)QRGN_RECTS;
		s << (Q_UINT32)a.size();
		for ( int i=0; i<(int)a.size(); i++ )
		    s << a[i];
	    } break;
#else
	case QRegion::QRegionData::ComplexRgn:
	    {
		QArray<QRect> a = r.rects();
		if ( a.isEmpty() ) {
		    s << (Q_UINT32)0;
		} else {
		    int i;
		    for ( i=(int)a.size()-1; i>0; i-- ) {
			s << (Q_UINT32)(12+i*24);
			s << (int)QRGN_OR;
		    }
		    for ( i=0; i<(int)a.size(); i++ ) {
			s << (Q_UINT32)(4+8) << (int)QRGN_SETRECT << a[i];
		    }
		}
	    } break;
#endif
    }
    //    return s << r.data->bop;
    return s;
}

/*!
  \relates QRegion
  Reads a region from the stream and returns a reference to the stream.
*/

QDataStream &operator>>( QDataStream &s, QRegion &r )
{
    QByteArray b;
    s >> b;
    r.exec( b );
    return s;
}


// OBSOLETE
#if !(defined(__STRICT_ANSI__) && defined(_CC_GNU_)) && !defined(_CC_EDG_) && !defined(_CC_HP_) && !defined(_CC_HP_ACC_) && !defined(_CC_USLC_) && !defined(xor)
/*!
  OBSOLETE - Use eor() instead.
*/
QRegion QRegion::xor( const QRegion &r ) const
{
    return eor(r);
}
#endif
