/****************************************************************************
**
** Definition of QWMatrix class.
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

#ifndef QWMATRIX_H
#define QWMATRIX_H

#ifndef QT_H
#include "qwindowdefs.h"
#include "qpointarray.h"
#include "qrect.h"
#ifdef QT_GUI_LIB
# include "qregion.h"
#endif
#endif // QT_H

#ifndef QT_NO_WMATRIX


class Q_GUI_EXPORT QWMatrix					// 2D transform matrix
{
public:
    QWMatrix();
    QWMatrix( double m11, double m12, double m21, double m22,
	      double dx, double dy );
    QWMatrix(const QWMatrix &matrix);

    void	setMatrix( double m11, double m12, double m21, double m22,
			   double dx,  double dy );

    double	m11() const { return _m11; }
    double	m12() const { return _m12; }
    double	m21() const { return _m21; }
    double	m22() const { return _m22; }
    double	dx()  const { return _dx; }
    double	dy()  const { return _dy; }

    void	map( int x, int y, int *tx, int *ty )	      const;
    void	map( double x, double y, double *tx, double *ty ) const;
    QRect	mapRect( const QRect & )	const;

    QPoint	map( const QPoint &p )	const { return operator *( p ); }
    QRect	map( const QRect &r )	const { return mapRect ( r ); }
    QPointArray map( const QPointArray &a ) const { return operator * ( a ); }
#ifdef QT_GUI_LIB
    QRegion     map( const QRegion &r ) const { return *this * r; }
    QRegion     mapToRegion( const QRect &r ) const { return *this * r; }
#endif
    QPointArray	mapToPolygon( const QRect &r )	const;

    void	reset();
    bool	isIdentity() const;

    QWMatrix   &translate( double dx, double dy );
    QWMatrix   &scale( double sx, double sy );
    QWMatrix   &shear( double sh, double sv );
    QWMatrix   &rotate( double a );

    bool isInvertible() const { return (_m11*_m22 - _m12*_m21) != 0; }
    double det() const { return _m11*_m22 - _m12*_m21; }

    QWMatrix	invert( bool * = 0 ) const;

    bool	operator==( const QWMatrix & ) const;
    bool	operator!=( const QWMatrix & ) const;
    QWMatrix   &operator*=( const QWMatrix & );

    /* we use matrix multiplication semantics here */
    QPoint operator * (const QPoint & ) const;
    QPointArray operator *  ( const QPointArray &a ) const;

    QWMatrix &operator=(const QWMatrix &);

    enum TransformationMode {
	Points, Areas
    };
    static void setTransformationMode( QWMatrix::TransformationMode m );
    static TransformationMode transformationMode();
private:
    double	_m11, _m12;
    double	_m21, _m22;
    double	_dx,  _dy;
};

Q_GUI_EXPORT QWMatrix operator*( const QWMatrix &, const QWMatrix & );


/*****************************************************************************
  QWMatrix stream functions
 *****************************************************************************/

Q_GUI_EXPORT QDataStream &operator<<( QDataStream &, const QWMatrix & );
Q_GUI_EXPORT QDataStream &operator>>( QDataStream &, QWMatrix & );


#endif // QT_NO_WMATRIX

#endif // QWMATRIX_H
