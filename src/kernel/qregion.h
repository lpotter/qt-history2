/****************************************************************************
**
** Definition of QRegion class.
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QREGION_H
#define QREGION_H

#ifndef QT_H
#include "qshared.h"
#include "qrect.h"
#include "qmemarray.h"
#endif // QT_H

#ifdef Q_WS_X11
struct QRegionPrivate;
#endif

class Q_EXPORT QRegion
{
public:
    enum RegionType { Rectangle, Ellipse };

    QRegion();
    QRegion( int x, int y, int w, int h, RegionType = Rectangle );
    QRegion( const QRect &, RegionType = Rectangle );
    QRegion( const QPointArray &, bool winding=FALSE );
    QRegion( const QRegion & );
    QRegion( const QBitmap & );
   ~QRegion();
    QRegion &operator=( const QRegion & );

    bool    isNull()   const;
    bool    isEmpty()  const;

    bool    contains( const QPoint &p ) const;
    bool    contains( const QRect &r )	const;

    void    translate( int dx, int dy );

    QRegion unite( const QRegion & )	const;
    QRegion intersect( const QRegion &) const;
    QRegion subtract( const QRegion & ) const;
    QRegion eor( const QRegion & )	const;

    QRect   boundingRect() const;
    QMemArray<QRect> rects() const;
    void setRects( const QRect *, int );

    const QRegion operator|( const QRegion & ) const;
    const QRegion operator+( const QRegion & ) const;
    const QRegion operator&( const QRegion & ) const;
    const QRegion operator-( const QRegion & ) const;
    const QRegion operator^( const QRegion & ) const;
    QRegion& operator|=( const QRegion & );
    QRegion& operator+=( const QRegion & );
    QRegion& operator&=( const QRegion & );
    QRegion& operator-=( const QRegion & );
    QRegion& operator^=( const QRegion & );

    bool    operator==( const QRegion & )  const;
    bool    operator!=( const QRegion &r ) const
			{ return !(operator==(r)); }

#if defined(Q_WS_WIN)
    HRGN    handle() const { return data->rgn; }
#elif defined(Q_WS_X11)
    Region handle() const { if(!data->rgn) updateX11Region(); return data->rgn; }
#elif defined(Q_WS_MAC)
    RgnHandle handle(bool require_rgn=FALSE) const;
#elif defined(Q_WS_QWS)
    // QGfx_QWS needs this for region drawing
    void * handle() const { return data->rgn; }
#endif

#ifndef QT_NO_DATASTREAM
    friend Q_EXPORT QDataStream &operator<<( QDataStream &, const QRegion & );
    friend Q_EXPORT QDataStream &operator>>( QDataStream &, QRegion & );
#endif
private:
    QRegion( bool );
    QRegion copy() const;
    void    detach();
#if defined(Q_WS_WIN)
    QRegion winCombine( const QRegion &, int ) const;
#endif
#if defined(Q_WS_X11)
    void updateX11Region() const;
    void *clipRectangles( int &num ) const;
    friend void *qt_getClipRects( const QRegion &, int & );
#endif
    void    exec( const QByteArray &, int ver = 0 );
    struct QRegionData : public QShared {
#if defined(Q_WS_WIN)
	HRGN   rgn;
#elif defined(Q_WS_X11)
	Region rgn;
	void *xrectangles;
	QRegionPrivate *region;
#elif defined(Q_WS_MAC)
	uint is_rect:1;
	QRect rect;
	RgnHandle rgn;
#elif defined(Q_WS_QWS)
	void * rgn;
#endif
	bool   is_null;
    } *data;
#if defined(Q_WS_MAC)
    friend struct qt_mac_rgn_data_cache;
    friend QRegionData *qt_mac_get_rgn_data();
    friend void qt_mac_free_rgn_data(QRegionData *);
    void rectifyRegion();
#endif

};


#define QRGN_SETRECT		1		// region stream commands
#define QRGN_SETELLIPSE		2		//  (these are internal)
#define QRGN_SETPTARRAY_ALT	3
#define QRGN_SETPTARRAY_WIND	4
#define QRGN_TRANSLATE		5
#define QRGN_OR			6
#define QRGN_AND		7
#define QRGN_SUB		8
#define QRGN_XOR		9
#define QRGN_RECTS	       10


/*****************************************************************************
  QRegion stream functions
 *****************************************************************************/

#ifndef QT_NO_DATASTREAM
Q_EXPORT QDataStream &operator<<( QDataStream &, const QRegion & );
Q_EXPORT QDataStream &operator>>( QDataStream &, QRegion & );
#endif


#endif // QREGION_H
