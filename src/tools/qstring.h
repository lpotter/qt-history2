/****************************************************************************
** $Id: //depot/qt/main/src/tools/qstring.h#89 $
**
** Definition of the QString class, extended char array operations,
** and QByteArray and QCString classes
**
** Created : 920609
**
** Copyright (C) 1992-1998 Troll Tech AS.  All rights reserved.
**
** This file is part of Troll Tech's internal development tree for Qt.
**
** This header text will be replaced by an appropriate text by the
** mkdist script which generates external distributions.
**
** If you are using the Qt Professional Edition or the Qt Free Edition,
** please notify Troll Tech at <info@troll.no> if you see this text.
**
** To Troll Tech developers: This header was generated by the script
** fixcopyright-int. It has the same number of text lines as the free
** and professional editions to avoid line number inconsistency.
**
*****************************************************************************/

#ifndef QSTRING_H
#define QSTRING_H

#ifndef QT_H
#include "qarray.h"
#endif // QT_H

#include <string.h>

#if defined(_OS_SUN_) && defined(_CC_GNU_)
#include <strings.h>
#endif


/*****************************************************************************
  Fixes and workarounds for some platforms
 *****************************************************************************/

#if defined(_OS_HPUX_)
// HP-UX has badly defined strstr() etc.
inline char *hack_strstr( const char *s1, const char *s2 )
{ return (char *)strstr(s1, s2); }
inline char *hack_strchr( const char *s, int c )
{ return (char *)strchr(s, c); }
inline char *hack_strrchr( const char *s, int c )
{ return (char *)strrchr(s, c); }
#define strstr	hack_strstr
#define strchr	hack_strchr
#define strrchr hack_strrchr
#endif


/*****************************************************************************
  Safe and portable C string functions; extensions to standard string.h
 *****************************************************************************/

Q_EXPORT void *qmemmove( void *dst, const void *src, uint len );

#if defined(_OS_SUN_) || defined(_CC_OC_)
#define memmove qmemmove
#endif

Q_EXPORT char *qstrdup( const char * );

Q_EXPORT inline uint cstrlen( const char *str )
{ return strlen(str); }

Q_EXPORT inline uint qstrlen( const char *str )
{ return str ? strlen(str) : 0; }

#undef	strlen
#define strlen qstrlen

Q_EXPORT inline char *cstrcpy( char *dst, const char *src )
{ return strcpy(dst,src); }

Q_EXPORT inline char *qstrcpy( char *dst, const char *src )
{ return src ? strcpy(dst, src) : 0; }

#undef	strcpy
#define strcpy qstrcpy

Q_EXPORT char *qstrncpy( char *dst, const char *src, uint len );

Q_EXPORT inline int cstrcmp( const char *str1, const char *str2 )
{ return strcmp(str1,str2); }

Q_EXPORT inline int qstrcmp( const char *str1, const char *str2 )
{ return (str1 && str2) ? strcmp(str1,str2) : (int)((long)str2 - (long)str1); }

#undef	strcmp
#define strcmp qstrcmp

Q_EXPORT inline int cstrncmp( const char *str1, const char *str2, uint len )
{ return strncmp(str1,str2,len); }

Q_EXPORT inline int qstrncmp( const char *str1, const char *str2, uint len )
{ return (str1 && str2) ? strncmp(str1,str2,len) :
			  (int)((long)str2 - (long)str1); }

#undef	strncmp
#define strncmp qstrncmp

Q_EXPORT int qstricmp( const char *, const char * );
Q_EXPORT int qstrnicmp( const char *, const char *, uint len );

#undef	stricmp
#define stricmp	 qstricmp
#undef	strnicmp
#define strnicmp qstrnicmp


// qchecksum: Internet checksum

#if 1	/* OBSOLETE */
#if !defined(QT_CLEAN_NAMESPACE)
Q_EXPORT UINT16 qchecksum( const char *s, uint len );
#endif
#endif
Q_EXPORT Q_UINT16 qChecksum( const char *s, uint len );

/*****************************************************************************
  QByteArray class
 *****************************************************************************/

#if defined(Q_TEMPLATEDLL)
template class Q_EXPORT QArray<char>;
#endif
typedef QArray<char> QByteArray;


/*****************************************************************************
  QByteArray stream functions
 *****************************************************************************/

Q_EXPORT QDataStream &operator<<( QDataStream &, const QByteArray & );
Q_EXPORT QDataStream &operator>>( QDataStream &, QByteArray & );


/*****************************************************************************
  QString class
 *****************************************************************************/

class QRegExp;


class Q_EXPORT QChar {
public:
    // The alternatives just avoid order-of-construction warnings.
#if defined(_WS_X11_) || defined(_OS_WIN32_BYTESWAP_)
    QChar() : row(0), cell(0) { }
    QChar( char c ) : row(0), cell(c) { }
    QChar( uchar c ) : row(0), cell(c) { }
    QChar( uchar c, uchar r ) : row(r), cell(c) { }
    QChar( const QChar& c ) : row(c.row), cell(c.cell) { }
    QChar( ushort rc ) : row((rc>>8)&0xff), cell(rc&0xff) { }
    QChar( short rc ) : row((rc>>8)&0xff), cell(rc&0xff) { }
#else
    QChar() : cell(0), row(0) { }
    QChar( char c ) : cell(c), row(0) { }
    QChar( uchar c ) : cell(c), row(0) { }
    QChar( uchar c, uchar r ) : cell(c), row(r) { }
    QChar( const QChar& c ) : cell(c.cell), row(c.row) { }
    QChar( ushort rc ) : cell(rc&0xff), row((rc>>8)&0xff) { }
    QChar( short rc ) : cell(rc&0xff), row((rc>>8)&0xff) { }
#endif

    QT_STATIC_CONST QChar null;            // 0000
    QT_STATIC_CONST QChar replacement;     // FFFD
    QT_STATIC_CONST QChar byteOrderMark;     // FEFF
    QT_STATIC_CONST QChar byteOrderSwapped;     // FFFE

    bool isSpace() const;

    operator char() const { return row?0:cell; }

    friend int operator==( const QChar& c1, const QChar& c2 );
    friend int operator==( const QChar& c1, char c );
    friend int operator==( char ch, const QChar& c );
    friend int operator!=( const QChar& c1, const QChar& c2 );
    friend int operator!=( const QChar& c, char ch );
    friend int operator!=( char ch, const QChar& c );
    friend int operator<=( const QChar& c1, const QChar& c2 );
    friend int operator<=( const QChar& c1, char c );
    friend int operator<=( char ch, const QChar& c );
    friend int operator>=( const QChar& c1, const QChar& c2 );
    friend int operator>=( const QChar& c, char ch );
    friend int operator>=( char ch, const QChar& c );
    friend int operator<( const QChar& c1, const QChar& c2 );
    friend int operator<( const QChar& c1, char c );
    friend int operator<( char ch, const QChar& c );
    friend int operator>( const QChar& c1, const QChar& c2 );
    friend int operator>( const QChar& c, char ch );
    friend int operator>( char ch, const QChar& c );

#if defined(_WS_X11_) || defined(_OS_WIN32_BYTESWAP_)
    // XChar2b on X11, ushort on(_OS_WIN32_BYTESWAP_
    uchar row;
    uchar cell;
    enum { networkOrdered = 1 };
#else
    // ushort on _OS_WIN32_
    uchar cell;
    uchar row;
    enum { networkOrdered = 0 };
#endif
};

inline int operator==( char ch, const QChar& c )
{
    return ch == c.cell && !c.row;
}

inline int operator==( const QChar& c, char ch )
{
    return ch == c.cell && !c.row;
}

inline int operator==( const QChar& c1, const QChar& c2 )
{
    return c1.cell == c2.cell
	&& c1.row == c2.row;
}

inline int operator!=( const QChar& c1, const QChar& c2 )
{
    return c1.cell != c2.cell
	|| c1.row != c2.row;
}

inline int operator!=( char ch, const QChar& c )
{
    return ch != c.cell || c.row;
}

inline int operator!=( const QChar& c, char ch )
{
    return ch != c.cell || c.row;
}

inline int operator<=( const QChar& c, char ch )
{
    return !(ch < c.cell || c.row);
}

inline int operator<=( char ch, const QChar& c )
{
    return ch <= c.cell || c.row;
}

inline int operator<=( const QChar& c1, const QChar& c2 )
{
    return c1.row > c2.row
	? FALSE
	: c1.row < c2.row
	    ? TRUE
	    : c1.row <= c2.row;
}

inline int operator>=( const QChar& c, char ch ) { return ch <= c; }
inline int operator>=( char ch, const QChar& c ) { return c <= ch; }
inline int operator>=( const QChar& c1, const QChar& c2 ) { return c2 <= c1; }
inline int operator<( const QChar& c, char ch ) { return !(ch<=c); }
inline int operator<( char ch, const QChar& c ) { return !(c<=ch); }
inline int operator<( const QChar& c1, const QChar& c2 ) { return !(c2<=c1); }
inline int operator>( const QChar& c, char ch ) { return !(ch>=c); }
inline int operator>( char ch, const QChar& c ) { return !(c>=ch); }
inline int operator>( const QChar& c1, const QChar& c2 ) { return !(c2>=c1); }


class Q_EXPORT QString
{
public:
    QString();					// make null string
    QString( const QChar& );			// one-char string
    QString( int size );			// allocate size incl. \0
    QString( const QString & );			// impl-shared copy
    QString( const QByteArray& );		// deep copy
    QString( QChar* unicode, uint length );	// deep copy
    QString( const char *str );			// deep copy
    QString( const char *str, uint maxlen );	// deep copy, max length
    ~QString();

    QString    &operator=( const QString & );	// impl-shared copy
    QString    &operator=( const char * );	// deep copy
    QString    &operator=( const QByteArray& );	// deep copy

    QT_STATIC_CONST QString null;

    bool	isNull()	const;
    bool	isEmpty()	const;
    uint	length()	const;
    void	truncate( uint pos );
    void	setLength( uint pos );
    void	resize( uint pos ); // OBS
    void	fill( QChar c, int len = -1 );

    QString	copy()	const;

    QString arg(int a, int fieldwidth=0) const;
    QString arg(uint a, int fieldwidth=0) const;
    QString arg(char a, int fieldwidth=0) const;
    QString arg(QChar a, int fieldwidth=0) const;
    QString arg(const QString& a, int fieldwidth=0) const;
    QString arg(double a, int fieldwidth=0, char fmt='g', int prec=-1);

    QString    &sprintf( const char* format, ... )
#if defined(_CC_GNU_)
	__attribute__ ((format (printf, 2, 3)))
#endif
	;

    int		find( QChar c, int index=0, bool cs=TRUE ) const;
    int		find( char c, int index=0, bool cs=TRUE ) const
		    { return find(QChar(c), index, cs); }
    int		find( const QString &str, int index=0, bool cs=TRUE ) const;
    int		find( const QRegExp &, int index=0 ) const;
    int		find( const char* str, int index=0 ) const
		    { return find(QString(str), index); }
    int		findRev( QChar c, int index=-1, bool cs=TRUE) const;
    int		findRev( char c, int index=-1, bool cs=TRUE) const
		    { return findRev( QChar(c), index, cs ); }
    int		findRev( const QString &str, int index=-1, bool cs=TRUE) const;
    int		findRev( const QRegExp &, int index=-1 ) const;
    int		findRev( const char* str, int index=-1 ) const
		    { return findRev(QString(str), index); }
    int		contains( QChar c, bool cs=TRUE ) const;
    int		contains( char c, bool cs=TRUE ) const
		    { return contains(QChar(c), cs); }
    int		contains( const char* str, bool cs=TRUE ) const;
    int		contains( const QString &str, bool cs=TRUE ) const;
    int		contains( const QRegExp & ) const;

    QString	left( uint len )  const;
    QString	right( uint len ) const;
    QString	mid( uint index, uint len=0xffffffff) const;

    QString	leftJustify( uint width, QChar fill=' ', bool trunc=FALSE)const;
    QString	rightJustify( uint width, QChar fill=' ',bool trunc=FALSE)const;

    QString	lower() const;
    QString	upper() const;

    QString	stripWhiteSpace()	const;
    QString	simplifyWhiteSpace()	const;

    QString    &insert( uint index, const QString & );
    QString    &insert( uint index, QChar );
    QString    &insert( uint index, char c ) { return insert(index,QChar(c)); }
    QString    &append( const QString & );
    QString    &prepend( const QString & );
    QString    &remove( uint index, uint len );
    QString    &replace( uint index, uint len, const QString & );
    QString    &replace( const QRegExp &, const QString & );

    short	toShort( bool *ok=0 )	const;
    ushort	toUShort( bool *ok=0 )	const;
    int		toInt( bool *ok=0 )	const;
    uint	toUInt( bool *ok=0 )	const;
    long	toLong( bool *ok=0 )	const;
    ulong	toULong( bool *ok=0 )	const;
    float	toFloat( bool *ok=0 )	const;
    double	toDouble( bool *ok=0 )	const;

    QString    &setStr( const char* );
    QString    &setNum( short );
    QString    &setNum( ushort );
    QString    &setNum( int );
    QString    &setNum( uint );
    QString    &setNum( long );
    QString    &setNum( ulong );
    QString    &setNum( float, char f='g', int prec=6 );
    QString    &setNum( double, char f='g', int prec=6 );

    void	setExpand( uint index, QChar c );

    QString    &operator+=( const QString &str );
    QString    &operator+=( QChar c );
    QString    &operator+=( char c );

    // Your compiler is smart enough to use the const one if it can.
    const QChar& at( uint i ) const
	{ return i<d->len ? unicode()[i] : QChar::null; }
    QChar& at( uint i )
	{ // Optimized for easy-inlining by simple compilers.
	    if (d->count!=1 || i>=d->len)
		subat(i);
	    d->dirtyascii=1;
	    return d->unicode[i];
	}
    const QChar& operator[]( int i ) const { return at((uint)i); }
    QChar& operator[]( int i ) { return at((uint)i); }

    const QChar* unicode() const { return d->unicode; }
    const char* ascii() const;
    operator const char *() const { return ascii(); }

    static QChar* asciiToUnicode( const char*, uint& len, uint maxlen=(uint)-1 );
    static QChar* asciiToUnicode( const QByteArray&, uint& len );
    static char* unicodeToAscii( const QChar*, uint len );
    int compare( const QString& s ) const;
    static int compare( const QString& s1, const QString& s2 )
	{ return s1.compare(s2); }

    friend Q_EXPORT QDataStream &operator>>( QDataStream &, QString & );

#ifndef QT_NO_COMPAT
    const char* data() const { return ascii(); }
    void detach() { }
    uint size() const;
#endif

private:
    void deref();
    void real_detach();
    void subat( uint );
    bool findArg(int& pos, int& len) const;

    struct Data : public QShared {
	Data() :
	    unicode(0), ascii(0), len(0), maxl(0), dirtyascii(0) { ref(); }
	Data(QChar *u, uint l, uint m) :
	    unicode(u), ascii(0), len(l), maxl(m), dirtyascii(0) { }
	~Data() { if ( unicode ) delete [] unicode;
		  if ( ascii ) delete [] ascii; }
	QChar *unicode;
	char *ascii;
	uint len;
	uint maxl:30;
	uint dirtyascii:1;
    };
    Data *d;
    static Data* shared_null;
    friend int ucstrcmp( const QString &a, const QString &b );

    friend class QConstString;
    QString(Data* dd) : d(dd) { }
};

class QConstString : private QString {
public:
    QConstString( QChar* unicode, uint length );
    ~QConstString();
    const QString& string() const { return *this; }
};


/*****************************************************************************
  QString stream functions
 *****************************************************************************/

Q_EXPORT QDataStream &operator<<( QDataStream &, const QString & );
Q_EXPORT QDataStream &operator>>( QDataStream &, QString & );


/*****************************************************************************
  QString inline functions
 *****************************************************************************/

// No safe way to pre-init shared_null on ALL compilers/linkers.
inline QString::QString() :
    d(shared_null ? shared_null : shared_null=new Data)
{
    d->ref();
}

//inline QString &QString::operator=( const QString &s )
//{ return (const QString &)assign( s ); }

//inline QString &QString::operator=( const char *str )
//{ return (const QString &)duplicate( str, strlen(str)+1 ); }

inline bool QString::isNull() const
{ return unicode() == 0; }

inline uint QString::length() const
{ return d->len; }

#ifndef QT_NO_COMPAT
inline uint QString::size() const
{ return length()+1; }
#endif

inline bool QString::isEmpty() const
{ return length() == 0; }

inline QString QString::copy() const
{ return QString( *this ); }

inline QString &QString::prepend( const QString & s )
{ return insert(0,s); }

inline QString &QString::append( const QString & s )
{ return operator+=(s); }

inline QString &QString::setNum( short n )
{ return setNum((long)n); }

inline QString &QString::setNum( ushort n )
{ return setNum((ulong)n); }

inline QString &QString::setNum( int n )
{ return setNum((long)n); }

inline QString &QString::setNum( uint n )
{ return setNum((ulong)n); }

inline QString &QString::setNum( float n, char f, int prec )
{ return setNum((double)n,f,prec); }



/*****************************************************************************
  QString non-member operators
 *****************************************************************************/

Q_EXPORT bool operator==( const QString &s1, const QString &s2 );
Q_EXPORT bool operator==( const QString &s1, const char *s2 );
Q_EXPORT bool operator==( const char *s1, const QString &s2 );
Q_EXPORT bool operator!=( const QString &s1, const QString &s2 );
Q_EXPORT bool operator!=( const QString &s1, const char *s2 );
Q_EXPORT bool operator!=( const char *s1, const QString &s2 );
Q_EXPORT bool operator<( const QString &s1, const QString &s2 );
Q_EXPORT bool operator<( const QString &s1, const char *s2 );
Q_EXPORT bool operator<( const char *s1, const QString &s2 );
Q_EXPORT bool operator<=( const QString &s1, const QString &s2 );
Q_EXPORT bool operator<=( const QString &s1, const char *s2 );
Q_EXPORT bool operator<=( const char *s1, const QString &s2 );
Q_EXPORT bool operator>( const QString &s1, const QString &s2 );
Q_EXPORT bool operator>( const QString &s1, const char *s2 );
Q_EXPORT bool operator>( const char *s1, const QString &s2 );
Q_EXPORT bool operator>=( const QString &s1, const QString &s2 );
Q_EXPORT bool operator>=( const QString &s1, const char *s2 );
Q_EXPORT bool operator>=( const char *s1, const QString &s2 );

Q_EXPORT inline QString operator+( const QString &s1, const QString &s2 )
{
    QString tmp( s1 );
    tmp += s2;
    return tmp;
}

Q_EXPORT inline QString operator+( const QString &s1, const char *s2 )
{
    QString tmp( s1 );
    tmp += s2;
    return tmp;
}

Q_EXPORT inline QString operator+( const char *s1, const QString &s2 )
{
    QString tmp( s1 );
    tmp += s2;
    return tmp;
}

Q_EXPORT inline QString operator+( const QString &s1, QChar c2 )
{
    QString tmp( s1 );
    tmp += c2;
    return tmp;
}

Q_EXPORT inline QString operator+( const QString &s1, char c2 )
{
    QString tmp( s1 );
    tmp += c2;
    return tmp;
}

Q_EXPORT inline QString operator+( QChar c1, const QString &s2 )
{
    QString tmp;
    tmp += c1;
    tmp += s2;
    return tmp;
}

Q_EXPORT inline QString operator+( char c1, const QString &s2 )
{
    QString tmp;
    tmp += c1;
    tmp += s2;
    return tmp;
}


/*****************************************************************************
  QCString class
 *****************************************************************************/

class QRegExp;

class Q_EXPORT QCString : public QByteArray	// string class
{
public:
    QCString() {}				// make null string
    QCString( int size );			// allocate size incl. \0
    QCString( const QCString &s ) : QByteArray( s ) {}
    QCString( const char *str );		// deep copy
    QCString( const char *str, uint maxlen );	// deep copy, max length

    QCString    &operator=( const QCString &s );// shallow copy
    QCString    &operator=( const char *str );	// deep copy

    bool	isNull()	const;
    bool	isEmpty()	const;
    uint	length()	const;
    bool	resize( uint newlen );
    bool	truncate( uint pos );
    bool	fill( char c, int len = -1 );

    QCString	copy()	const;

    QCString    &sprintf( const char *format, ... );

    int		find( char c, int index=0, bool cs=TRUE ) const;
    int		find( const char *str, int index=0, bool cs=TRUE ) const;
    int		find( const QRegExp &, int index=0 ) const;
    int		findRev( char c, int index=-1, bool cs=TRUE) const;
    int		findRev( const char *str, int index=-1, bool cs=TRUE) const;
    int		findRev( const QRegExp &, int index=-1 ) const;
    int		contains( char c, bool cs=TRUE ) const;
    int		contains( const char *str, bool cs=TRUE ) const;
    int		contains( const QRegExp & ) const;

    QCString	left( uint len )  const;
    QCString	right( uint len ) const;
    QCString	mid( uint index, uint len) const;

    QCString	leftJustify( uint width, char fill=' ', bool trunc=FALSE)const;
    QCString	rightJustify( uint width, char fill=' ',bool trunc=FALSE)const;

    QCString	lower() const;
    QCString	upper() const;

    QCString	stripWhiteSpace()	const;
    QCString	simplifyWhiteSpace()	const;

    QCString    &insert( uint index, const char * );
    QCString    &insert( uint index, char );
    QCString    &append( const char * );
    QCString    &prepend( const char * );
    QCString    &remove( uint index, uint len );
    QCString    &replace( uint index, uint len, const char * );
    QCString    &replace( const QRegExp &, const char * );

    short	toShort( bool *ok=0 )	const;
    ushort	toUShort( bool *ok=0 )	const;
    int		toInt( bool *ok=0 )	const;
    uint	toUInt( bool *ok=0 )	const;
    long	toLong( bool *ok=0 )	const;
    ulong	toULong( bool *ok=0 )	const;
    float	toFloat( bool *ok=0 )	const;
    double	toDouble( bool *ok=0 )	const;

    QCString    &setStr( const char *s );
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
    QCString    &operator+=( const char *str );
    QCString    &operator+=( char c );
};


/*****************************************************************************
  QCString stream functions
 *****************************************************************************/

QDataStream &operator<<( QDataStream &, const QCString & );
QDataStream &operator>>( QDataStream &, QCString & );


/*****************************************************************************
  QCString inline functions
 *****************************************************************************/

inline QCString &QCString::operator=( const QCString &s )
{ return (QCString&)assign( s ); }

inline QCString &QCString::operator=( const char *str )
{ return (QCString&)duplicate( str, strlen(str)+1 ); }

inline bool QCString::isNull() const
{ return data() == 0; }

inline bool QCString::isEmpty() const
{ return data() == 0 || *data() == '\0'; }

inline uint QCString::length() const
{ return strlen( data() ); }

inline bool QCString::truncate( uint pos )
{ return resize(pos+1); }

inline QCString QCString::copy() const
{ return QCString( data() ); }

inline QCString &QCString::prepend( const char *s )
{ return insert(0,s); }

inline QCString &QCString::append( const char *s )
{ return operator+=(s); }

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
{ return strcmp(s1.data(),s2.data()) == 0; }

Q_EXPORT inline bool operator==( const QCString &s1, const char *s2 )
{ return strcmp(s1.data(),s2) == 0; }

Q_EXPORT inline bool operator==( const char *s1, const QCString &s2 )
{ return strcmp(s1,s2.data()) == 0; }

Q_EXPORT inline bool operator!=( const QCString &s1, const QCString &s2 )
{ return strcmp(s1.data(),s2.data()) != 0; }

Q_EXPORT inline bool operator!=( const QCString &s1, const char *s2 )
{ return strcmp(s1.data(),s2) != 0; }

Q_EXPORT inline bool operator!=( const char *s1, const QCString &s2 )
{ return strcmp(s1,s2.data()) != 0; }

Q_EXPORT inline bool operator<( const QCString &s1, const char *s2 )
{ return strcmp(s1.data(),s2) < 0; }

Q_EXPORT inline bool operator<( const char *s1, const QCString &s2 )
{ return strcmp(s1,s2.data()) < 0; }

Q_EXPORT inline bool operator<=( const QCString &s1, const char *s2 )
{ return strcmp(s1.data(),s2) <= 0; }

Q_EXPORT inline bool operator<=( const char *s1, const QCString &s2 )
{ return strcmp(s1,s2.data()) <= 0; }

Q_EXPORT inline bool operator>( const QCString &s1, const char *s2 )
{ return strcmp(s1.data(),s2) > 0; }

Q_EXPORT inline bool operator>( const char *s1, const QCString &s2 )
{ return strcmp(s1,s2.data()) > 0; }

Q_EXPORT inline bool operator>=( const QCString &s1, const char *s2 )
{ return strcmp(s1.data(),s2) >= 0; }

Q_EXPORT inline bool operator>=( const char *s1, const QCString &s2 )
{ return strcmp(s1,s2.data()) >= 0; }

Q_EXPORT inline QCString operator+( const QCString &s1, const QCString &s2 )
{
    QCString tmp( s1.data() );
    tmp += s2;
    return tmp;
}

Q_EXPORT inline QCString operator+( const QCString &s1, const char *s2 )
{
    QCString tmp( s1.data() );
    tmp += s2;
    return tmp;
}

Q_EXPORT inline QCString operator+( const char *s1, const QCString &s2 )
{
    QCString tmp( s1 );
    tmp += s2;
    return tmp;
}

Q_EXPORT inline QCString operator+( const QCString &s1, char c2 )
{
    QCString tmp( s1.data() );
    tmp += c2;
    return tmp;
}

Q_EXPORT inline QCString operator+( char c1, const QCString &s2 )
{
    QCString tmp;
    tmp += c1;
    tmp += s2;
    return tmp;
}

#if defined(_OS_WIN32_)
extern Q_EXPORT QString qt_winQString(void*);
extern Q_EXPORT const void* qt_winTchar(const QString& str, bool addnul);
extern Q_EXPORT void* qt_winTchar_new(const QString& str);
#endif

#endif // QSTRING_H
