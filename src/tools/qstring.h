/****************************************************************************
** $Id: //depot/qt/main/src/tools/qstring.h#64 $
**
** Definition of the QString class, extended char array operations,
** and QByteArray and Q1String classes
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

void *qmemmove( void *dst, const void *src, uint len );

#if defined(_OS_SUN_) || defined(_CC_OC_)
#define memmove qmemmove
#endif

char *qstrdup( const char * );

inline uint cstrlen( const char *str )
{ return strlen(str); }

inline uint qstrlen( const char *str )
{ return str ? strlen(str) : 0; }

#undef	strlen
#define strlen qstrlen

inline char *cstrcpy( char *dst, const char *src )
{ return strcpy(dst,src); }

inline char *qstrcpy( char *dst, const char *src )
{ return src ? strcpy(dst, src) : 0; }

#undef	strcpy
#define strcpy qstrcpy

char *qstrncpy( char *dst, const char *src, uint len );

inline int cstrcmp( const char *str1, const char *str2 )
{ return strcmp(str1,str2); }

inline int qstrcmp( const char *str1, const char *str2 )
{ return (str1 && str2) ? strcmp(str1,str2) : (int)((long)str2 - (long)str1); }

#undef	strcmp
#define strcmp qstrcmp

inline int cstrncmp( const char *str1, const char *str2, uint len )
{ return strncmp(str1,str2,len); }

inline int qstrncmp( const char *str1, const char *str2, uint len )
{ return (str1 && str2) ? strncmp(str1,str2,len) :
			  (int)((long)str2 - (long)str1); }

#undef	strncmp
#define strncmp qstrncmp

int qstricmp( const char *, const char * );
int qstrnicmp( const char *, const char *, uint len );

#undef	stricmp
#define stricmp	 qstricmp
#undef	strnicmp
#define strnicmp qstrnicmp


// qchecksum: Internet checksum

#if 1	/* OBSOLETE */
#if !defined(QT_CLEAN_NAMESPACE)
UINT16 qchecksum( const char *s, uint len );
#endif
#endif
Q_UINT16 qChecksum( const char *s, uint len );

/*****************************************************************************
  QByteArray class
 *****************************************************************************/

#if defined(USE_TEMPLATECLASS)
#define QByteArray QArrayT<char>
#else
Q_DECLARE(QArrayM,char);
#define QByteArray QArrayM(char)
#endif


/*****************************************************************************
  QByteArray stream functions
 *****************************************************************************/

QDataStream &operator<<( QDataStream &, const QByteArray & );
QDataStream &operator>>( QDataStream &, QByteArray & );


/*****************************************************************************
  QString class
 *****************************************************************************/

class QRegExp;

#define UNICODE // ###### We need to decide - libqtu AND libqt? ???
#ifdef UNICODE

class QChar {
public:
    QChar() : row(0), cell(0) { }
    QChar( uchar c, uchar r=0 ) : row(r), cell(c) { }
    QChar( const QChar& c ) : row(c.row), cell(c.cell) { }

    static const QChar null;

    operator char() const { return row?0:cell; }

    bool operator==( const QChar& c )
    {
	return c.cell == cell
	    && c.row == row;
    }
    bool operator==( char c )
    {
	return c == cell && !row;
    }

    bool operator!=( const QChar& c )
    {
	return c.cell != cell
	    || c.row != row;
    }
    bool operator!=( char c )
    {
	return c != cell || row;
    }

    int bytes()
    {
	return row << 8 || cell;
    }

    int reversedBytes()
    {
	return row || cell << 8;
    }

private:
    uchar row;
    uchar cell;
};

#else

class QChar {
public:
    QChar() cell(0) { }
    QChar( uchar l ) : cell(l) { }
    QChar( const QChar& c ) : cell(c.cell) { }

    static const QChar null;

    operator char() const { return cell; }

    bool operator==( const QChar& c )
    {
	return c.cell == cell;
    }
    bool operator==( char c )
    {
	return c == cell;
    }

    bool operator!=( const QChar& c )
    {
	return c.cell != cell;
    }
    bool operator!=( char c )
    {
	return c != cell;
    }

private:
    uchar cell;
};

#endif


class QString
{
public:
    QString();					// make null string
    QString( int size );			// allocate size incl. \0
    QString( const QString & );			// impl-shared copy
    QString( const QByteArray& );			// deep copy
    QString( const char *str );			// deep copy
    QString( const char *str, uint maxlen );	// deep copy, max length
    ~QString();

    QString    &operator=( const QString & );	// impl-shared copy
    QString    &operator=( const char * );	// deep copy
    QString    &operator=( const QByteArray& );	// deep copy

    static const QString null;

    bool	isNull()	const;
    bool	isEmpty()	const;
    uint	length()	const;
    void	truncate( uint pos );
    void	setLength( uint pos );
    void	resize( uint pos ); // OBS
    void	fill( QChar c, int len = -1 );

    QString	copy()	const;

    QString    &sprintf( const char* format, ... );

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
    const QChar& at( uint i ) const { return i<d->len ? unicode()[i] : QChar::null; }
    QChar& at( uint i ); // detaches, enlarges
    const QChar& operator[]( int i ) const { return at(i); }
    QChar& operator[]( int i ) { return at(i); }

    const QChar* unicode() const { return d->unicode; }
    const char* ascii() const;
    operator const char *() const { return ascii(); }

    static QChar* asciiToUnicode( const char*, uint& len );
    static QChar* asciiToUnicode( const QByteArray&, uint& len );
    static char* unicodeToAscii( const QChar*, uint len );

    friend QDataStream &operator>>( QDataStream &, QString & );

#ifndef QT_NO_COMPAT
    const char* data() const { return ascii(); }
    void detach() { }
    uint size() const;
#endif

private:
    void deref();
    void real_detach();

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
};


/*****************************************************************************
  QString stream functions
 *****************************************************************************/

QDataStream &operator<<( QDataStream &, const QString & );
QDataStream &operator>>( QDataStream &, const QString & );


/*****************************************************************************
  QString inline functions
 *****************************************************************************/

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

bool operator==( const QString &s1, const QString &s2 );
bool operator==( const QString &s1, const char *s2 );
bool operator==( const char *s1, const QString &s2 );
bool operator!=( const QString &s1, const QString &s2 );
bool operator!=( const QString &s1, const char *s2 );
bool operator!=( const char *s1, const QString &s2 );
bool operator<( const QString &s1, const QString &s2 );
bool operator<( const QString &s1, const char *s2 );
bool operator<( const char *s1, const QString &s2 );
bool operator<=( const QString &s1, const QString &s2 );
bool operator<=( const QString &s1, const char *s2 );
bool operator<=( const char *s1, const QString &s2 );
bool operator>( const QString &s1, const QString &s2 );
bool operator>( const QString &s1, const char *s2 );
bool operator>( const char *s1, const QString &s2 );
bool operator>=( const QString &s1, const QString &s2 );
bool operator>=( const QString &s1, const char *s2 );
bool operator>=( const char *s1, const QString &s2 );

inline QString operator+( const QString &s1, const QString &s2 )
{
    QString tmp( s1 );
    tmp += s2;
    return tmp;
}

inline QString operator+( const QString &s1, const char *s2 )
{
    QString tmp( s1 );
    tmp += s2;
    return tmp;
}

inline QString operator+( const char *s1, const QString &s2 )
{
    QString tmp( s1 );
    tmp += s2;
    return tmp;
}

inline QString operator+( const QString &s1, QChar c2 )
{
    QString tmp( s1 );
    tmp += c2;
    return tmp;
}

inline QString operator+( const QString &s1, char c2 )
{
    QString tmp( s1 );
    tmp += c2;
    return tmp;
}

inline QString operator+( QChar c1, const QString &s2 )
{
    QString tmp;
    tmp += c1;
    tmp += s2;
    return tmp;
}

inline QString operator+( char c1, const QString &s2 )
{
    QString tmp;
    tmp += c1;
    tmp += s2;
    return tmp;
}


/*****************************************************************************
  Q1String class
 *****************************************************************************/

class QRegExp;

class Q1String : public QByteArray		// string class
{
public:
    Q1String() {}				// make null string
    Q1String( int size );			// allocate size incl. \0
    Q1String( const Q1String &s ) : QByteArray( s ) {}
    Q1String( const char *str );			// deep copy
    Q1String( const char *str, uint maxlen );	// deep copy, max length

    Q1String    &operator=( const Q1String &s );	// shallow copy
    Q1String    &operator=( const char *str );	// deep copy

    bool	isNull()	const;
    bool	isEmpty()	const;
    uint	length()	const;
    bool	resize( uint newlen );
    bool	truncate( uint pos );
    bool	fill( char c, int len = -1 );

    Q1String	copy()	const;

    Q1String    &sprintf( const char *format, ... );

    int		find( char c, int index=0, bool cs=TRUE ) const;
    int		find( const char *str, int index=0, bool cs=TRUE ) const;
    int		find( const QRegExp &, int index=0 ) const;
    int		findRev( char c, int index=-1, bool cs=TRUE) const;
    int		findRev( const char *str, int index=-1, bool cs=TRUE) const;
    int		findRev( const QRegExp &, int index=-1 ) const;
    int		contains( char c, bool cs=TRUE ) const;
    int		contains( const char *str, bool cs=TRUE ) const;
    int		contains( const QRegExp & ) const;

    Q1String	left( uint len )  const;
    Q1String	right( uint len ) const;
    Q1String	mid( uint index, uint len) const;

    Q1String	leftJustify( uint width, char fill=' ', bool trunc=FALSE)const;
    Q1String	rightJustify( uint width, char fill=' ',bool trunc=FALSE)const;

    Q1String	lower() const;
    Q1String	upper() const;

    Q1String	stripWhiteSpace()	const;
    Q1String	simplifyWhiteSpace()	const;

    Q1String    &insert( uint index, const char * );
    Q1String    &insert( uint index, char );
    Q1String    &append( const char * );
    Q1String    &prepend( const char * );
    Q1String    &remove( uint index, uint len );
    Q1String    &replace( uint index, uint len, const char * );
    Q1String    &replace( const QRegExp &, const char * );

    short	toShort( bool *ok=0 )	const;
    ushort	toUShort( bool *ok=0 )	const;
    int		toInt( bool *ok=0 )	const;
    uint	toUInt( bool *ok=0 )	const;
    long	toLong( bool *ok=0 )	const;
    ulong	toULong( bool *ok=0 )	const;
    float	toFloat( bool *ok=0 )	const;
    double	toDouble( bool *ok=0 )	const;

    Q1String    &setStr( const char *s );
    Q1String    &setNum( short );
    Q1String    &setNum( ushort );
    Q1String    &setNum( int );
    Q1String    &setNum( uint );
    Q1String    &setNum( long );
    Q1String    &setNum( ulong );
    Q1String    &setNum( float, char f='g', int prec=6 );
    Q1String    &setNum( double, char f='g', int prec=6 );

    bool	setExpand( uint index, char c );

		operator const char *() const;
    Q1String    &operator+=( const char *str );
    Q1String    &operator+=( char c );
};


/*****************************************************************************
  Q1String stream functions
 *****************************************************************************/

QDataStream &operator<<( QDataStream &, const Q1String & );
QDataStream &operator>>( QDataStream &, Q1String & );


/*****************************************************************************
  Q1String inline functions
 *****************************************************************************/

inline Q1String &Q1String::operator=( const Q1String &s )
{ return (Q1String&)assign( s ); }

inline Q1String &Q1String::operator=( const char *str )
{ return (Q1String&)duplicate( str, strlen(str)+1 ); }

inline bool Q1String::isNull() const
{ return data() == 0; }

inline bool Q1String::isEmpty() const
{ return data() == 0 || *data() == '\0'; }

inline uint Q1String::length() const
{ return strlen( data() ); }

inline bool Q1String::truncate( uint pos )
{ return resize(pos+1); }

inline Q1String Q1String::copy() const
{ return Q1String( data() ); }

inline Q1String &Q1String::prepend( const char *s )
{ return insert(0,s); }

inline Q1String &Q1String::append( const char *s )
{ return operator+=(s); }

inline Q1String &Q1String::setNum( short n )
{ return setNum((long)n); }

inline Q1String &Q1String::setNum( ushort n )
{ return setNum((ulong)n); }

inline Q1String &Q1String::setNum( int n )
{ return setNum((long)n); }

inline Q1String &Q1String::setNum( uint n )
{ return setNum((ulong)n); }

inline Q1String &Q1String::setNum( float n, char f, int prec )
{ return setNum((double)n,f,prec); }

inline Q1String::operator const char *() const
{ return (const char *)data(); }


/*****************************************************************************
  Q1String non-member operators
 *****************************************************************************/

inline bool operator==( const Q1String &s1, const Q1String &s2 )
{ return strcmp(s1.data(),s2.data()) == 0; }

inline bool operator==( const Q1String &s1, const char *s2 )
{ return strcmp(s1.data(),s2) == 0; }

inline bool operator==( const char *s1, const Q1String &s2 )
{ return strcmp(s1,s2.data()) == 0; }

inline bool operator!=( const Q1String &s1, const Q1String &s2 )
{ return strcmp(s1.data(),s2.data()) != 0; }

inline bool operator!=( const Q1String &s1, const char *s2 )
{ return strcmp(s1.data(),s2) != 0; }

inline bool operator!=( const char *s1, const Q1String &s2 )
{ return strcmp(s1,s2.data()) != 0; }

inline bool operator<( const Q1String &s1, const char *s2 )
{ return strcmp(s1.data(),s2) < 0; }

inline bool operator<( const char *s1, const Q1String &s2 )
{ return strcmp(s1,s2.data()) < 0; }

inline bool operator<=( const Q1String &s1, const char *s2 )
{ return strcmp(s1.data(),s2) <= 0; }

inline bool operator<=( const char *s1, const Q1String &s2 )
{ return strcmp(s1,s2.data()) <= 0; }

inline bool operator>( const Q1String &s1, const char *s2 )
{ return strcmp(s1.data(),s2) > 0; }

inline bool operator>( const char *s1, const Q1String &s2 )
{ return strcmp(s1,s2.data()) > 0; }

inline bool operator>=( const Q1String &s1, const char *s2 )
{ return strcmp(s1.data(),s2) >= 0; }

inline bool operator>=( const char *s1, const Q1String &s2 )
{ return strcmp(s1,s2.data()) >= 0; }

inline Q1String operator+( const Q1String &s1, const Q1String &s2 )
{
    Q1String tmp( s1.data() );
    tmp += s2;
    return tmp;
}

inline Q1String operator+( const Q1String &s1, const char *s2 )
{
    Q1String tmp( s1.data() );
    tmp += s2;
    return tmp;
}

inline Q1String operator+( const char *s1, const Q1String &s2 )
{
    Q1String tmp( s1 );
    tmp += s2;
    return tmp;
}

inline Q1String operator+( const Q1String &s1, char c2 )
{
    Q1String tmp( s1.data() );
    tmp += c2;
    return tmp;
}

inline Q1String operator+( char c1, const Q1String &s2 )
{
    Q1String tmp;
    tmp += c1;
    tmp += s2;
    return tmp;
}

#endif // QSTRING_H
