/****************************************************************************
** $Id$
**
** Definition of the extended char array operations,
** and QCString class
**
** Created : 920609
**
** Copyright (C) 1992-2002 Trolltech AS.  All rights reserved.
**
** This file is part of the tools module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef QCSTRING_H
#define QCSTRING_H

#ifndef QT_H
#include "qbytearray.h"
#endif // QT_H

/*****************************************************************************
  QCString class
 *****************************************************************************/

class QRegExp;

class Q_EXPORT QCString : public QByteArray
{
public:
    QCString() {}
    QCString(int size) : QByteArray(size, '\0') {}
    QCString(const QCString &s) : QByteArray( s ) {}
    QCString(const QByteArray &ba) : QByteArray(ba) {}
    QCString(const char *str ) : QByteArray(str) {}
    QCString(const char *str, uint maxlen) : QByteArray(str, maxlen) {}

    QCString    &operator=( const QCString &s ) {
	QByteArray::operator=(s); return *this;
    }
    QCString    &operator=( const char *str ) {
	QByteArray::operator=(str); return *this;
    }
    QCString    &operator=(const QByteArray &ba) {
	QByteArray::operator=(ba); return *this;
    }


    QCString	copy()	const { return *this; }
    QCString    &sprintf( const char *format, ... );

    QCString	left( uint len )  const { return QByteArray::left(len); }
    QCString	right( uint len ) const { return QByteArray::right(len); }
    QCString	mid( uint index, uint len=0xffffffff) const { return QByteArray::mid(index, len); }

    QCString	leftJustify( uint width, char fill=' ', bool trunc=FALSE)const;
    QCString	rightJustify( uint width, char fill=' ',bool trunc=FALSE)const;

    QCString	lower() const;
    QCString	upper() const;

    QCString	stripWhiteSpace()	const;
    QCString	simplifyWhiteSpace()	const;

    QCString    &insert( uint index, const char *c ) { QByteArray::insert(index, c); return *this; }
    QCString    &insert( uint index, char c) { QByteArray::insert(index, c); return *this; }
    QCString    &append( const char *c ) { QByteArray::append(c); return *this; }
    QCString    &prepend( const char *c ) { QByteArray::prepend(c); return *this; }
    QCString    &remove( uint index, uint len ) { QByteArray::remove(index, len); return *this; }
    QCString    &replace( uint index, uint len, const char *c ) { QByteArray::replace(index, len, c); return *this; }
    QCString    &replace( char c, const char *after ) { QByteArray::replace(c, after); return *this; }
    QCString    &replace( const char *b, const char *a ) { QByteArray::replace(b, a); return *this; }
    QCString    &replace( char b, char a ) { QByteArray::replace(b, a); return *this; }

    short	toShort( bool *ok=0 )	const;
    ushort	toUShort( bool *ok=0 )	const;
    int		toInt( bool *ok=0 )	const;
    uint	toUInt( bool *ok=0 )	const;
    long	toLong( bool *ok=0 )	const;
    ulong	toULong( bool *ok=0 )	const;
    float	toFloat( bool *ok=0 )	const;
    double	toDouble( bool *ok=0 )	const;

    QCString    &setStr( const char *s ) { *this = s; return *this; }
    QCString    &setNum( short );
    QCString    &setNum( ushort );
    QCString    &setNum( int );
    QCString    &setNum( uint );
    QCString    &setNum( long );
    QCString    &setNum( ulong );
    QCString    &setNum( float, char f='g', int prec=6 );
    QCString    &setNum( double, char f='g', int prec=6 );

    bool	setExpand( uint index, char c );

		operator const char *() const;
};


/*****************************************************************************
  QCString stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
inline Q_EXPORT QDataStream &operator<<( QDataStream &d, const QCString &s ) {
    return operator<<(d, static_cast<const QByteArray &>(s));
}
inline Q_EXPORT QDataStream &operator>>( QDataStream &d, QCString &s ) {
    return operator>>(d, static_cast<QByteArray &>(s));
}
#endif

/*****************************************************************************
  QCString inline functions
 *****************************************************************************/

inline QCString &QCString::setNum( short n )
{ return setNum((long)n); }

inline QCString &QCString::setNum( ushort n )
{ return setNum((ulong)n); }

inline QCString &QCString::setNum( int n )
{ return setNum((long)n); }

inline QCString &QCString::setNum( uint n )
{ return setNum((ulong)n); }

inline QCString &QCString::setNum( float n, char f, int prec )
{ return setNum((double)n,f,prec); }

inline QCString::operator const char *() const
{ return (const char *)data(); }


/*****************************************************************************
  QCString non-member operators
 *****************************************************************************/

Q_EXPORT inline bool operator==( const QCString &s1, const QCString &s2 )
{ return qstrcmp( s1.data(), s2.data() ) == 0; }

Q_EXPORT inline bool operator==( const QCString &s1, const char *s2 )
{ return qstrcmp( s1.data(), s2 ) == 0; }

Q_EXPORT inline bool operator==( const char *s1, const QCString &s2 )
{ return qstrcmp( s1, s2.data() ) == 0; }

Q_EXPORT inline bool operator!=( const QCString &s1, const QCString &s2 )
{ return qstrcmp( s1.data(), s2.data() ) != 0; }

Q_EXPORT inline bool operator!=( const QCString &s1, const char *s2 )
{ return qstrcmp( s1.data(), s2 ) != 0; }

Q_EXPORT inline bool operator!=( const char *s1, const QCString &s2 )
{ return qstrcmp( s1, s2.data() ) != 0; }

Q_EXPORT inline bool operator<( const QCString &s1, const QCString& s2 )
{ return qstrcmp( s1.data(), s2.data() ) < 0; }

Q_EXPORT inline bool operator<( const QCString &s1, const char *s2 )
{ return qstrcmp( s1.data(), s2 ) < 0; }

Q_EXPORT inline bool operator<( const char *s1, const QCString &s2 )
{ return qstrcmp( s1, s2.data() ) < 0; }

Q_EXPORT inline bool operator<=( const QCString &s1, const QCString &s2 )
{ return qstrcmp( s1.data(), s2.data() ) <= 0; }

Q_EXPORT inline bool operator<=( const QCString &s1, const char *s2 )
{ return qstrcmp( s1.data(), s2 ) <= 0; }

Q_EXPORT inline bool operator<=( const char *s1, const QCString &s2 )
{ return qstrcmp( s1, s2.data() ) <= 0; }

Q_EXPORT inline bool operator>( const QCString &s1, const QCString &s2 )
{ return qstrcmp( s1.data(), s2.data() ) > 0; }

Q_EXPORT inline bool operator>( const QCString &s1, const char *s2 )
{ return qstrcmp( s1.data(), s2 ) > 0; }

Q_EXPORT inline bool operator>( const char *s1, const QCString &s2 )
{ return qstrcmp( s1, s2.data() ) > 0; }

Q_EXPORT inline bool operator>=( const QCString &s1, const QCString& s2 )
{ return qstrcmp( s1.data(), s2.data() ) >= 0; }

Q_EXPORT inline bool operator>=( const QCString &s1, const char *s2 )
{ return qstrcmp( s1.data(), s2 ) >= 0; }

Q_EXPORT inline bool operator>=( const char *s1, const QCString &s2 )
{ return qstrcmp( s1, s2.data() ) >= 0; }

Q_EXPORT inline const QCString operator+( const QCString &s1,
					  const QCString &s2 )
{
    QCString tmp( s1.data() );
    tmp += s2;
    return tmp;
}

Q_EXPORT inline const QCString operator+( const QCString &s1, const char *s2 )
{
    QCString tmp( s1.data() );
    tmp += s2;
    return tmp;
}

Q_EXPORT inline const QCString operator+( const char *s1, const QCString &s2 )
{
    QCString tmp( s1 );
    tmp += s2;
    return tmp;
}

Q_EXPORT inline const QCString operator+( const QCString &s1, char c2 )
{
    QCString tmp( s1.data() );
    tmp += c2;
    return tmp;
}

Q_EXPORT inline const QCString operator+( char c1, const QCString &s2 )
{
    QCString tmp;
    tmp += c1;
    tmp += s2;
    return tmp;
}
#include "qwinexport.h"
#include "qbytearray.h"
#endif // QCSTRING_H
