/****************************************************************************
** $Id: //depot/qt/main/src/tools/qtextstream.cpp#61 $
**
** Implementation of QTextStream class
**
** Created : 940922
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

#include "qtextstream.h"
#include "qbuffer.h"
#include "qfile.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

/*!
  \class QTextStream qtextstream.h

  \brief The QTextStream class provides basic functions for reading and
  writing text using a QIODevice.

  \ingroup io

  \define endl
  \define bin
  \define oct
  \define dec
  \define hex
  \define flush
  \define ws

  The text stream class has a functional interface that is very
  similar to that of the standard C++ iostream class.  The difference
  between iostream and QTextStream is that our stream operates on a
  QIODevice, which is easily subclassed, while iostream operates on
  FILE * pointers, which can not be subclassed.

  Qt provides several global functions similar to the ones in iostream:
  <ul>
  <li> \c bin sets the QTextStream to output binary numbers
  <li> \c oct sets the QTextStream to output octal numbers
  <li> \c dec sets the QTextStream to output decimal numbers
  <li> \c hex sets the QTextStream to output hexadecimal numbers
  <li> \c endl forces a line break
  <li> \c flush forces the QIODevice to flush any buffered data
  <li> \c ws eats any available white space (on input)
  <li> \c reset resets the QTextStream to its default mode (see reset()).
  </ul>

  The QTextStream class reads and writes ASCII text and it is not
  appropriate for dealing with binary data (but QDataStream is).

  \sa QDataStream
*/

/*
  \class QTSManip qtextstream.h

  \brief The QTSManip class is an internal helper class for the
  QTextStream.

  It is generally a very bad idea to use this class directly in
  application programs.

  \internal

  This class makes it possible to give the QTextStream function objects
  with arguments, like this:
  \code
    QTextStream cout( stdout, IO_WriteOnly );
    cout << setprecision( 8 );		// QTSManip used here!
    cout << 3.14159265358979323846;
  \endcode

  The setprecision() function returns a QTSManip object.
  The QTSManip object contains a pointer to a member function in
  QTextStream and an integer argument.
  When serializing a QTSManip into a QTextStream, the function
  is executed with the argument.
*/

/* \fn QTSManip::QTSManip (QTSMFI m, int a)

  Constructs a QTSManip object which will call \m (a member function
  in QTextStream which accepts a single int) with argument \a a when
  QTSManip::exec() is called.  Used internally in e.g. endl:

  \code
    s << "some text" << endl << "more text";
  \endcode
*/

/* \fn void QTSManip::exec (QTextStream& s)

  Calls the member function specified in the constructor, for object
  \a s.  Used internally in e.g. endl:

  \code
    s << "some text" << endl << "more text";
  \endcode
*/


/*****************************************************************************
  QTextStream member functions
 *****************************************************************************/

#if defined(CHECK_STATE)
#undef  CHECK_STREAM_PRECOND
#define CHECK_STREAM_PRECOND  if ( !dev ) {				\
				warning( "QTextStream: No device" );	\
				return *this; }
#else
#define CHECK_STREAM_PRECOND
#endif


#define I_SHORT		0x0010
#define I_INT		0x0020
#define I_LONG		0x0030
#define I_TYPE_MASK	0x00f0

#define I_BASE_2	QTS::bin
#define I_BASE_8	QTS::oct
#define I_BASE_10	QTS::dec
#define I_BASE_16	QTS::hex
#define I_BASE_MASK	(QTS::bin | QTS::oct | QTS::dec | QTS::hex)

#define I_SIGNED	0x0100
#define I_UNSIGNED	0x0200
#define I_SIGN_MASK	0x0f00


const int QTextStream::basefield   = I_BASE_MASK;
const int QTextStream::adjustfield = ( QTextStream::left |
				       QTextStream::right |
				       QTextStream::internal );
const int QTextStream::floatfield  = ( QTextStream::scientific |
				       QTextStream::fixed );

int QTextStream::eat_ws()
{
    int c;
    do { c = ts_getc(); } while ( c != EOF && ts_isspace(c) );
    return c;
}


/*!
  Constructs a data stream that has no IO device.
*/

QTextStream::QTextStream()
{
    dev = 0;					// no device set
    fstrm = owndev = FALSE;
    cmode = Ascii;
    reset();
}

/*!
  Constructs a text stream that uses the IO device \e d.
*/

QTextStream::QTextStream( QIODevice *d )
{
    dev = d;					// set device
    fstrm = owndev = FALSE;
    cmode = Ascii;
    reset();
}

class QStringBuffer : public QIODevice {
public:
    QStringBuffer( QString& str ) :
	s(str)
    {
    }

   ~QStringBuffer()
    {
    }


    bool  open( int m )
    {
	if ( isOpen() ) {                           // buffer already open
#if defined(CHECK_STATE)
	    warning( "QStringBuffer::open: Buffer already open" );
#endif
	    return FALSE;
	}
	setMode( m );
	if ( m & IO_Truncate ) {                    // truncate buffer
	    s.truncate( 0 );
	}
	if ( m & IO_Append ) {                      // append to end of buffer
	    index = s.length()*sizeof(QChar);
	} else {
	    index = 0;
	}
	setState( IO_Open );
	setStatus( 0 );
	return TRUE;
    }

    void  close()
    {
	if ( isOpen() ) {
	    setFlags( IO_Direct );
	    index = 0;
	}
    }

    void  flush()
    {
    }

    uint  size() const
    {
	return s.length()*sizeof(QChar);
    }

    int   at()   const
    {
	return index;
    }

    bool  at( int pos )
    {
#if defined(CHECK_STATE)
	if ( !isOpen() ) {
	    warning( "QStringBuffer::at: Buffer is not open" );
	    return FALSE;
	}
#endif
	if ( (uint)pos >= s.length()*2 ) {
#if defined(CHECK_RANGE)
	    warning( "QStringBuffer::at: Index %d out of range", pos );
#endif
	    return FALSE;
	}
	index = pos;
	return TRUE;
    }


    int   readBlock( char *p, uint len )
    {
#if defined(CHECK_STATE)
	CHECK_PTR( p );
	if ( !isOpen() ) {                          // buffer not open
	    warning( "QBuffer::readBlock: Buffer not open" );
	    return -1;
	}
	if ( !isReadable() ) {                      // reading not permitted
	    warning( "QBuffer::readBlock: Read operation not permitted" );
	    return -1;
	}
#endif
	if ( (uint)index + len > s.length()*sizeof(QChar) ) {
					     	    // overflow
	    if ( (uint)index >= s.length()*sizeof(QChar) ) {
		setStatus( IO_ReadError );
		return -1;
	    } else {
		len = s.length()*2 - (uint)index;
	    }
	}
	memcpy( p, ((char*)(s.unicode()))+index, len );
	index += len;
	return len;
    }

    int writeBlock( const char *p, uint len )
    {
#if defined(CHECK_NULL)
	if ( p == 0 && len != 0 )
	    warning( "QBuffer::writeBlock: Null pointer error" );
#endif
#if defined(CHECK_STATE)
	if ( !isOpen() ) {                          // buffer not open
	    warning( "QBuffer::writeBlock: Buffer not open" );
	    return -1;
	}
	if ( !isWritable() ) {                      // writing not permitted
	    warning( "QBuffer::writeBlock: Write operation not permitted" );
	    return -1;
	}
#endif
	if ( (uint)index + len >= s.length()*2 ) {  // overflow
	    s.setLength((index+len+1)/2);
	}
	memcpy( ((char*)s.unicode())+index, p, len );
	index += len;
	return len;
    }

    int   getch()
    {
#if defined(CHECK_STATE)
	if ( !isOpen() ) {                          // buffer not open
	    warning( "QBuffer::getch: Buffer not open" );
	    return -1;
	}
	if ( !isReadable() ) {                      // reading not permitted
	    warning( "QBuffer::getch: Read operation not permitted" );
	    return -1;
	}
#endif
	if ( (uint)index >= s.length()*2 ) {           // overflow
	    setStatus( IO_ReadError );
	    return -1;
	}
	return *((char*)s.unicode() + index++);
    }

    int   putch( int ch )
    {
	char c = ch;
	if ( writeBlock(&c,1) < 0 )
	    return -1;
	else
	    return ch;
    }

    int   ungetch( int ch )
    {
#if defined(CHECK_STATE)
	if ( !isOpen() ) {                          // buffer not open
	    warning( "QBuffer::ungetch: Buffer not open" );
	    return -1;
	}
	if ( !isReadable() ) {                      // reading not permitted
	    warning( "QBuffer::ungetch: Read operation not permitted" );
	    return -1;
	}
#endif
	if ( ch != -1 ) {
	    if ( index )
		index--;
	    else
		ch = -1;
	}
	return ch;
    }

protected:
    QString& s;

private:        // Disabled copy constructor and operator=
    QStringBuffer( const QStringBuffer & );
    QStringBuffer &operator=( const QStringBuffer & );
};

/*!
  Constructs a text stream that operates on a Unicode QString through an
  internal device.

  Example:
  \code
    QString str;
    QTextStream ts( str, IO_WriteOnly );
    ts << "pi = " << 3.14;			// str == "pi = 3.14..."
  \endcode

  The \a mode argument cannot be Ascii.

  Writing data to the text stream will modify the contents of the string.
  The string will be expanded when data is written beyond the end of the
  string.
*/

QTextStream::QTextStream( QString& str, int filemode, Encoding mode )
{
    dev = new QStringBuffer( str );
    ((QStringBuffer *)dev)->open( filemode );
    fstrm = FALSE;
    owndev = TRUE;
    ASSERT(mode != Ascii);
    cmode = mode;
    reset();
}

/*!
  Constructs a text stream that operates on a byte array through an
  internal QBuffer device.

  Example:
  \code
    QByteArray array;
    QTextStream ts( array, IO_WriteOnly );
    ts << "pi = " << 3.14 << '\0';		// array == "pi = 3.14"
  \endcode

  Writing data to the text stream will modify the contents of the array.
  The array will be expanded when data is written beyond the end of the
  string.

  Same example, using a QBuffer:
  \code
    QByteArray array;
    QBuffer buf( array );
    buf.open( IO_WriteOnly );
    QTextStream ts( &buf );
    ts << "pi = " << 3.14 << '\0';		// array == "pi = 3.14"
    buf.close();
  \endcode
*/

QTextStream::QTextStream( QByteArray a, int mode )
{
    dev = new QBuffer( a );
    ((QBuffer *)dev)->open( mode );
    fstrm = FALSE;
    owndev = TRUE;
    cmode = Ascii;
    reset();
}

/*!
  Constructs a text stream that operates on an existing file handle \e fh
  throught an internal QFile device.

  Example:
  \code
    QTextStream cout( stdout, IO_WriteOnly );
    QTextStream cin ( stdin,  IO_ReadOnly );
    QTextStream cerr( stderr, IO_WriteOnly );
 \endcode
*/

QTextStream::QTextStream( FILE *fh, int mode )
{
    dev = new QFile;
    ((QFile *)dev)->open( mode, fh );
    fstrm = owndev = TRUE;
    cmode = Ascii;
    reset();
}

/*!
  Destroys the text stream.

  The destructor will not affect the current IO device.
*/

QTextStream::~QTextStream()
{
    if ( owndev )
	delete dev;
}

/*!
  Sets the encoding mode for the stream to \a mode.

  \define Encoding

  The availables modes are:
  <ol>
    <li>Ascii - 8-bit text
    <li>Unicode - Unicode text
    <li>UnicodeBigEndian - big endian Unicode text
    <li>UnicodeLittleEndian - little endian Unicode text
  </ol>

  Normally you choose one of Ascii or Unicode.  Using Unicode causes
  Qt to attempt to auto-detect the endianness of the file using the
  Unicode standard technique for this.

  This should only be done, at most once, before any characters are written
  to or read from the stream, and the stream must have a device set.
*/
void QTextStream::setEncoding(Encoding mode)
{
    if ( cmode == mode )
	return;

    cmode = mode;

    if ( mode != Ascii ) {
	const int bom = 0xfeff;
	const int mob = 0xfffe;
	if ( dev->mode() & IO_WriteOnly ) {
	    // Write the byte order marker
	    ts_putc(bom);
	} else {
	    // Try to read the byte order marker
	    int ch = ts_getc();
	    if ( ch == bom ) {
		// Good guess.
	    } else if ( ch == mob ) {
		// Bad guess.
		mode = mode == UnicodeLittleEndian
		    ? UnicodeBigEndian
		    : UnicodeLittleEndian;
	    } else {
		// No clue.
		ts_ungetc(ch);
	    }
	}
    }
}

/*!
  \fn void QTextStream::eatWhiteSpace()

  Equivalent to *this << ws.
*/

/*!
  \fn Encoding QTextStream::encoding() const

  Returns the encoding mode of the stream.

  \sa setEncoding()
*/

/*!
  Returns one character from the stream, or EOF.
*/
int QTextStream::ts_getc()
{
    // WARNING: some QTextStream functions call getch directly.

    switch (cmode) {
     case Ascii:
	return dev->getch();
     case UnicodeBigEndian: {
	    int c = dev->getch();
	    if ( c == EOF )
		return c;
	    return (c << 8) | dev->getch();
	}
     case UnicodeLittleEndian: {
	    int c = dev->getch();
	    if ( c == EOF )
		return c;
	    return c | (dev->getch() << 8);
	}
    }
    return EOF;
}

/*!
  Puts one character to the stream.
*/
void QTextStream::ts_putc(int ch)
{
    switch (cmode) {
     case Ascii:
	dev->putch(ch);
	break;
     case UnicodeBigEndian:
	dev->putch(ch>>8);
	dev->putch(ch&0xff);
	break;
     case UnicodeLittleEndian:
	dev->putch(ch&0xff);
	dev->putch(ch>>8);
    }
}

bool QTextStream::ts_isdigit(int ch)
{
    if ( cmode == Ascii )
	return isdigit(ch);
    else // ######## see QString ucdigit()
	return isdigit(ch&0xff);
}

bool QTextStream::ts_isspace(int ch)
{
    if ( cmode == Ascii )
	return isspace(ch);
    else // ######## see QString ucspace()
	return isspace(ch&0xff);
}

void QTextStream::ts_ungetc(int ch)
{
    switch (cmode) {
     case Ascii:
	dev->ungetch(ch);
	break;
     case UnicodeBigEndian:
	// Reverse of put
	dev->ungetch(ch&0xff);
	dev->ungetch(ch>>8);
	break;
     case UnicodeLittleEndian:
	// Reverse of put
	dev->ungetch(ch>>8);
	dev->ungetch(ch&0xff);
    }
}



/*!
  Resets the text stream.

  <ul>
  <li> All flags are set to 0.
  <li> The field width is set to 0.
  <li> The fill character is set to ' ' (space).
  <li> The precision is set to 6.
  </ul>

  \sa setf(), width(), fill(), precision()
*/

void QTextStream::reset()
{
    fflags = 0;
    fwidth = 0;
    fillchar = ' ';
    fprec = 6;
}


/*!
  \fn QIODevice *QTextStream::device() const
  Returns the IO device currently set.
  \sa setDevice(), unsetDevice()
*/

/*!
  Sets the IO device to \e d.
  \sa device(), unsetDevice()
*/

void QTextStream::setDevice( QIODevice *d )
{
    if ( owndev ) {
	delete dev;
	owndev = FALSE;
    }
    dev = d;
}

/*!
  Unsets the IO device.	 Equivalent to setDevice( 0 ).
  \sa device(), setDevice()
*/

void QTextStream::unsetDevice()
{
    setDevice( 0 );
}

/*!
  \fn bool QTextStream::eof() const
  Returns TRUE if the IO device has reached the end position (end of
  stream or file) or if there is no IO device set.

  Returns FALSE if the current position of the read/write head of the IO
  device is somewhere before the end position.

  \sa QIODevice::atEnd()
*/


/*****************************************************************************
  QTextStream read functions
 *****************************************************************************/


/*!
  Reads a \c char from the stream and returns a reference to the stream.
*/

QTextStream &QTextStream::operator>>( char &c )
{
    CHECK_STREAM_PRECOND
    c = eat_ws();
    return *this;
}


ulong QTextStream::input_bin()
{
    ulong val = 0;
    register int c = eat_ws();
    while ( c == '0' || c == '1' ) {
	val <<= 1;
	val += c - '0';
	c = ts_getc();
    }
    if ( c != EOF )
	ts_ungetc( c );
    return val;
}

ulong QTextStream::input_oct()
{
    ulong val = 0;
    register int c = eat_ws();
    while ( c >= '0' && c <= '7' ) {
	val <<= 3;
	val += c - '0';
	c = ts_getc();
    }
    if ( c == '8' || c == '9' ) {
	while ( ts_isdigit(c) )
	    c = ts_getc();
    }
    if ( c != EOF )
	ts_ungetc( c );
    return val;
}

ulong QTextStream::input_dec()
{
    ulong val = 0;
    register int c = eat_ws();
    while ( ts_isdigit(c) ) {
	val *= 10;
	val += c - '0';
	c = ts_getc();
    }
    if ( c != EOF )
	ts_ungetc( c );
    return val;
}

ulong QTextStream::input_hex()
{
    ulong val = 0;
    register int c = eat_ws();
    while ( isxdigit(c) ) {
	val <<= 4;
	if ( ts_isdigit(c) )
	    val += c - '0';
	else
	    val += 10 + tolower(c) - 'a';
	c = ts_getc();
    }
    if ( c != EOF )
	ts_ungetc( c );
    return val;
}

long QTextStream::input_int()
{
    long val;
    int c;
    switch ( flags() & basefield ) {
	case bin:
	    val = (long)input_bin();
	    break;
	case oct:
	    val = (long)input_oct();
	    break;
	case dec:
	    c = eat_ws();
	    if ( c == EOF ) {
		val = 0;
	    } else {
		if ( !(c == '-' || c == '+') )
		    ts_ungetc( c );
		if ( c == '-' ) {
		    ulong v = input_dec();
		    if ( v ) {		// ensure that LONG_MIN can be read
			v--;
			val = -((long)v) - 1;
		    } else {
			val = 0;
		    }
		} else {
		    val = (long)input_dec();
		}
	    }
	    break;
	case hex:
	    val = (long)input_hex();
	    break;
	default:
	    val = 0;
	    c = eat_ws();
	    if ( c == '0' ) {		// bin, oct or hex
		c = ts_getc();
		if ( tolower(c) == 'x' )
		    val = (long)input_hex();
		else if ( tolower(c) == 'b' )
		    val = (long)input_bin();
		else {			// octal
		    if ( c >= '0' && c <= '7' ) {
			ts_ungetc( c );
			val = (long)input_oct();
		    } else {
			val = 0;
		    }
		}
	    }
	    else if ( c >= '1' && c <= '9' ) {
		ts_ungetc( c );
		val = (long)input_dec();
	    }
	    else if ( c == '-' || c == '+' ) {
		val = (long)input_dec();
		if ( c == '-' )
		    val = -val;
	    }
    }
    return val;
}


//
// We use a table-driven FSM to parse floating point numbers
// strtod() cannot be used directly since we're reading from a QIODevice
//

double QTextStream::input_double()
{
    const int Init	 = 0;			// states
    const int Sign	 = 1;
    const int Mantissa	 = 2;
    const int Dot	 = 3;
    const int Abscissa	 = 4;
    const int ExpMark	 = 5;
    const int ExpSign	 = 6;
    const int Exponent	 = 7;
    const int Done	 = 8;

    const int InputSign	 = 1;			// input tokens
    const int InputDigit = 2;
    const int InputDot	 = 3;
    const int InputExp	 = 4;

    static uchar table[8][5] = {
     /* None	 InputSign   InputDigit InputDot InputExp */
	{ 0,	    Sign,     Mantissa,	 Dot,	   0,	   }, // Init
	{ 0,	    0,	      Mantissa,	 Dot,	   0,	   }, // Sign
	{ Done,	    Done,     Mantissa,	 Dot,	   ExpMark,}, // Mantissa
	{ 0,	    0,	      Abscissa,	 0,	   0,	   }, // Dot
	{ Done,	    Done,     Abscissa,	 Done,	   ExpMark,}, // Abscissa
	{ 0,	    ExpSign,  Exponent,	 0,	   0,	   }, // ExpMark
	{ 0,	    0,	      Exponent,	 0,	   0,	   }, // ExpSign
	{ Done,	    Done,     Exponent,	 Done,	   Done	   }  // Exponent
    };

    int state = Init;				// parse state
    int input;					// input token

    char buf[256];
    int i = 0;
    int c = eat_ws();

    while ( TRUE ) {

	switch ( c ) {
	    case '+':
	    case '-':
		input = InputSign;
		break;
	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
		input = InputDigit;
		break;
	    case '.':
		input = InputDot;
		break;
	    case 'e':
	    case 'E':
		input = InputExp;
		break;
	    default:
		input = 0;
		break;
	}

	state = table[state][input];

	if  ( state == 0 || state == Done || i > 250 ) {
	    if ( i > 250 ) {			// ignore rest of digits
		do { c = ts_getc(); } while ( c != EOF && ts_isdigit(c) );
	    }
	    if ( c != EOF )
		ts_ungetc( c );
	    buf[i] = '\0';
	    char *end;
	    return strtod( buf, &end );
	}

	buf[i++] = c;
	c = ts_getc();
    }

#if !defined(NO_DEADCODE)
    return 0.0;
#endif
}


/*!
  Reads a signed \c short integer from the stream and returns a reference to
  the stream. See flags() for an explanation of expected input format.
*/

QTextStream &QTextStream::operator>>( signed short &i )
{
    CHECK_STREAM_PRECOND
    i = (signed short)input_int();
    return *this;
}


/*!
  Reads an unsigned \c short integer from the stream and returns a reference to
  the stream. See flags() for an explanation of expected input format.
*/

QTextStream &QTextStream::operator>>( unsigned short &i )
{
    CHECK_STREAM_PRECOND
    i = (unsigned short)input_int();
    return *this;
}


/*!
  Reads a signed \c int from the stream and returns a reference to the
  stream. See flags() for an explanation of expected input format.
*/

QTextStream &QTextStream::operator>>( signed int &i )
{
    CHECK_STREAM_PRECOND
    i = (signed int)input_int();
    return *this;
}


/*!
  Reads an unsigned \c int from the stream and returns a reference to the
  stream. See flags() for an explanation of expected input format.
*/

QTextStream &QTextStream::operator>>( unsigned int &i )
{
    CHECK_STREAM_PRECOND
    i = (unsigned int)input_int();
    return *this;
}


/*!
  Reads a signed \c long int from the stream and returns a reference to the
  stream. See flags() for an explanation of expected input format.
*/

QTextStream &QTextStream::operator>>( signed long &i )
{
    CHECK_STREAM_PRECOND
    i = (signed long)input_int();
    return *this;
}


/*!
  Reads an unsigned \c long int from the stream and returns a reference to the
  stream. See flags() for an explanation of expected input format.
*/

QTextStream &QTextStream::operator>>( unsigned long &i )
{
    CHECK_STREAM_PRECOND
    i = (unsigned long)input_int();
    return *this;
}


/*!
  Reads a \c float from the stream and returns a reference to the stream.
  See flags() for an explanation of expected input format.
*/

QTextStream &QTextStream::operator>>( float &f )
{
    CHECK_STREAM_PRECOND
    f = (float)input_double();
    return *this;
}


/*!
  Reads a \c double from the stream and returns a reference to the stream.
  See flags() for an explanation of expected input format.
*/

QTextStream &QTextStream::operator>>( double &f )
{
    CHECK_STREAM_PRECOND
    f = input_double();
    return *this;
}


/*!
  Reads a word from the stream and returns a reference to the stream.
*/

QTextStream &QTextStream::operator>>( char *s )
{
    CHECK_STREAM_PRECOND
    int maxlen = width( 0 );
    int c = eat_ws();
    if ( !maxlen )
	maxlen = -1;
    while ( c != EOF ) {
	if ( ts_isspace(c) || maxlen-- == 0 ) {
	    ts_ungetc( c );
	    break;
	}
	*s++ = c;
	c = ts_getc();
    }

    *s = '\0';
    return *this;
}

/*!
  Reads a word from the stream and returns a reference to the stream.
*/

QTextStream &QTextStream::operator>>( QString &str )
{
    CHECK_STREAM_PRECOND
    str="";
    int	i = 0;
    int	c = eat_ws();

    while ( c != EOF ) {
	if ( ts_isspace(c) ) {
	    ts_ungetc( c );
	    break;
	}
	str[i++] = c;
	c = ts_getc();
    }
    return *this;
}

/*!
  Reads a word from the stream and returns a reference to the stream.
*/

QTextStream &QTextStream::operator>>( Q1String &str )
{
    CHECK_STREAM_PRECOND
    Q1String  *dynbuf = 0;
    const int buflen = 256;
    char      buffer[buflen];
    char     *s = buffer;
    int	      i = 0;
    int	      c = eat_ws();

    while ( c != EOF ) {
	if ( ts_isspace(c) ) {
	    ts_ungetc( c );
	    break;
	}
	if ( i >= buflen-1 ) {
	    if ( !dynbuf )  {			// create dynamic buffer
		dynbuf = new Q1String(buflen*2);
		memcpy( dynbuf->data(), s, i );	// copy old data
	    } else if ( i >= (int)dynbuf->size()-1 ) {
		dynbuf->resize( dynbuf->size()*2 );
	    }
	    s = dynbuf->data();
	}
	s[i++] = c;
	c = ts_getc();
    }
    str.resize( i+1 );
    memcpy( str.data(), s, i );
    delete dynbuf;
    return *this;
}


/*!
  Reads a line from the stream and returns a string containing the text.

  The returned string does not contain any trailing newline or carriage
  return. Note that this is different from QIODevice::readLine(), which
  does not strip the newline at the end of the line.

  \sa QIODevice::readLine()
*/

QString QTextStream::readLine()
{
#if defined(CHECK_STATE)
    if ( !dev ) {
	warning( "QTextStream::readLine: No device" );
	QString nullString;
	return nullString;
    }
#endif
    QString   result;
    int	      i = 0;
    int	      c = ts_getc();

    while ( c != EOF ) {
	if ( c == '\n' ) {
	    break;
	}
	result[i++] = c;
	c = ts_getc();
    }
    if ( i > 0 && result[i-1] == '\r' )
	result.setLength(i-1);			// if there are two \r, let one stay
    return result;
}



/*****************************************************************************
  QTextStream write functions
 *****************************************************************************/

/*!
  Writes a \c char to the stream and returns a reference to the stream.
*/

QTextStream &QTextStream::operator<<( char c )
{
    CHECK_STREAM_PRECOND
    ts_putc( c );
    return *this;
}


QTextStream &QTextStream::output_int( int format, ulong n, bool neg )
{
    static char hexdigits_lower[] = "0123456789abcdef";
    static char hexdigits_upper[] = "0123456789ABCDEF";
    CHECK_STREAM_PRECOND
    char buf[76];
    register char *p;
    int	  len;
    char *hexdigits;

    switch ( flags() & I_BASE_MASK ) {

	case I_BASE_2:				// output binary number
	    switch ( format & I_TYPE_MASK ) {
		case I_SHORT: len=16; break;
		case I_INT:   len=sizeof(int)*8; break;
		case I_LONG:  len=32; break;
		default:      len = 0;
	    }
	    p = &buf[74];			// go reverse order
	    *p = '\0';
	    while ( len-- ) {
		*--p = (char)(n&1) + '0';
		n >>= 1;
		if ( !n )
		    break;
	    }
	    if ( flags() & showbase ) {		// show base
		*--p = (flags() & uppercase) ? 'B' : 'b';
		*--p = '0';
	    }
	    break;

	case I_BASE_8:				// output octal number
	    p = &buf[74];
	    *p = '\0';
	    do {
		*--p = (char)(n&7) + '0';
		n >>= 3;
	    } while ( n );
	    if ( flags() & showbase )
		*--p = '0';
	    break;

	case I_BASE_16:				// output hexadecimal number
	    p = &buf[74];
	    *p = '\0';
	    hexdigits = (flags() & uppercase) ?
		hexdigits_upper : hexdigits_lower;
	    do {
		*--p = hexdigits[(int)n&0xf];
		n >>= 4;
	    } while ( n );
	    if ( flags() & showbase ) {
		*--p = (flags() & uppercase) ? 'X' : 'x';
		*--p = '0';
	    }
	    break;

	default:				// decimal base is default
	    p = &buf[74];
	    *p = '\0';
	    if ( neg )
		n = (ulong)(-(long)n);
	    do {
		*--p = ((int)(n%10)) + '0';
		n /= 10;
	    } while ( n );
	    if ( neg )
		*--p = '-';
	    else if ( flags() & showpos )
		*--p = '+';
	    if ( (flags() & internal) && fwidth && !ts_isdigit(*p) ) {
		ts_putc( *p );			// special case for internal
		++p;				//   padding
		fwidth--;
		return *this << (const char*)p;
	    }
    }
    if ( fwidth ) {				// adjustment required
	if ( !(flags() & left) ) {		// but NOT left adjustment
	    len = strlen(p);
	    int padlen = fwidth - len;
	    if ( padlen <= 0 ) {		// no padding required
		writeBlock( p, len );
	    } else if ( padlen < (int)(p-buf) ) { // speeds up padding
		memset( p-padlen, fillchar, padlen );
		writeBlock( p-padlen, padlen+len );
	    }
	    else				// standard padding
		*this << (const char*)p;
	}
	else
	    *this << (const char*)p;
	fwidth = 0;				// reset field width
    }
    else
	writeBlock( p, strlen(p) );
    return *this;
}


/*!
  Writes a \c short integer to the stream and returns a reference to
  the stream.
*/

QTextStream &QTextStream::operator<<( signed short i )
{
    return output_int( I_SHORT | I_SIGNED, i, i < 0 );
}


/*!
  Writes an \c unsigned \c short integer to the stream and returns a reference
  to the stream.
*/

QTextStream &QTextStream::operator<<( unsigned short i )
{
    return output_int( I_SHORT | I_UNSIGNED, i, FALSE );
}


/*!
  Writes an \c int to the stream and returns a reference to
  the stream.
*/

QTextStream &QTextStream::operator<<( signed int i )
{
    return output_int( I_INT | I_SIGNED, i, i < 0 );
}


/*!
  Writes an \c unsigned \c int to the stream and returns a reference to
  the stream.
*/

QTextStream &QTextStream::operator<<( unsigned int i )
{
    return output_int( I_INT | I_UNSIGNED, i, FALSE );
}


/*!
  Writes a \c long \c int to the stream and returns a reference to
  the stream.
*/

QTextStream &QTextStream::operator<<( signed long i )
{
    return output_int( I_LONG | I_SIGNED, i, i < 0 );
}


/*!
  Writes an \c unsigned \c long \c int to the stream and returns a reference to
  the stream.
*/

QTextStream &QTextStream::operator<<( unsigned long i )
{
    return output_int( I_LONG | I_UNSIGNED, i, FALSE );
}


/*!
  Writes a \c float to the stream and returns a reference to the stream.
*/

QTextStream &QTextStream::operator<<( float f )
{
    return *this << (double)f;
}


/*!
  Writes a \c double to the stream and returns a reference to the stream.
*/

QTextStream &QTextStream::operator<<( double f )
{
    CHECK_STREAM_PRECOND
    char buf[64];
    char f_char;
    char format[16];
    if ( (flags()&floatfield) == fixed )
	f_char = 'f';
    else if ( (flags()&floatfield) == scientific )
	f_char = (flags() & uppercase) ? 'E' : 'e';
    else
	f_char = (flags() & uppercase) ? 'G' : 'g';
    register char *fs = format;			// generate format string
    *fs++ = '%';				//   "%.<prec>l<f_char>"
    *fs++ = '.';
    int prec = precision();
    if ( prec > 99 )
	prec = 99;
    if ( prec >= 10 ) {
	*fs++ = prec / 10 + '0';
	*fs++ = prec % 10 + '0';
    } else {
	*fs++ = prec + '0';
    }
    *fs++ = 'l';
    *fs++ = f_char;
    *fs = '\0';
    sprintf( buf, format, f );			// convert to text
    if ( fwidth )				// padding
	*this << (const char*)buf;
    else					// just write it
	writeBlock( buf, strlen(buf) );
    return *this;
}


/*!
  Writes a string to the stream and returns a reference to the stream.
*/

QTextStream &QTextStream::operator<<( const char* s )
{
    CHECK_STREAM_PRECOND
    char padbuf[48];
    uint len = strlen( s );			// don't write null terminator
    if ( fwidth ) {				// field width set
	int padlen = fwidth - len;
	fwidth = 0;				// reset width
	if ( padlen > 0 ) {
	    char *ppad;
	    if ( padlen > 46 ) {		// create extra big fill buffer
		ppad = new char[padlen];
		CHECK_PTR( ppad );
	    } else {
		ppad = padbuf;
	    }
	    memset( ppad, fillchar, padlen );	// fill with fillchar
	    if ( !(flags() & left) ) {
		writeBlock( ppad, padlen );
		padlen = 0;
	    }
	    writeBlock( s, len );
	    if ( padlen )
		writeBlock( ppad, padlen );
	    if ( ppad != padbuf )		// delete extra big fill buf
		delete[] ppad;
	    return *this;
	}
    }
    writeBlock( s, len );
    return *this;
}

/*!
  Writes a string to the stream and returns a reference to the stream.
*/

QTextStream &QTextStream::operator<<( const QString& s )
{
    CHECK_STREAM_PRECOND
    uint len = s.length();
    QString s1 = s;
    if ( fwidth ) {				// field width set
	if ( !(flags() & left) ) {
	    s1 = s.rightJustify(fwidth, fillchar);
	} else {
	    s1 = s.leftJustify(fwidth, fillchar);
	}
	fwidth = 0;				// reset width
    }
    writeBlock( s1.unicode(), len );
    return *this;
}


/*!
  Writes a pointer to the stream and returns a reference to the stream.

  The \e ptr is output as an unsigned long hexadecimal integer.
*/

QTextStream &QTextStream::operator<<( void *ptr )
{
    int f = flags();
    setf( hex, basefield );
    setf( showbase );
    unsetf( uppercase );
    output_int( I_LONG | I_UNSIGNED, (ulong)ptr, FALSE );
    flags( f );
    return *this;
}


/*!
  Reads \e len bytes from the stream into \e e s and returns a reference to
  the stream.

  The buffer \e s must be preallocated.

  \note No Encoding is done by this function.

  \sa QIODevice::readBlock()
*/

QTextStream &QTextStream::readRawBytes( char *s, uint len )
{
    dev->readBlock( s, len );
    return *this;
}

/*!
  Writes the \e len bytes from \e s to the stream and returns a reference to
  the stream.

  \note No Encoding is done by this function.

  \sa QIODevice::writeBlock()
*/

QTextStream &QTextStream::writeRawBytes( const char* s, uint len )
{
    dev->writeBlock( s, len );
    return *this;
}

QTextStream &QTextStream::writeBlock( const char* p, uint len )
{
    switch ( cmode ) {
      case Ascii:
	dev->writeBlock( p, len );
	break;
      case UnicodeBigEndian: {
	    QChar *u = new QChar[len];
	    for (uint i=0; i<len; i++)
		u[i] = p[i];
	    dev->writeBlock( (char*)u, len*sizeof(QChar) );
	    delete [] u;
	}
	break;
      case UnicodeLittleEndian: {
	    QChar *u = new QChar[len];
	    for (uint i=0; i<len; i++)
		u[i] = QChar(0,p[i]);
	    dev->writeBlock( (char*)u, len*sizeof(QChar) );
	    delete [] u;
	}
	break;
    }
    return *this;
}

QTextStream &QTextStream::writeBlock( const QChar* p, uint len )
{
    switch ( cmode ) {
      case Ascii: {
	    char *u = new char[len];
	    for (uint i=0; i<len; i++)
		u[i] = p[i];
	    dev->writeBlock( (char*)u, len );
	    delete [] u;
	}
	break;
      case UnicodeBigEndian: {
	    QChar *u = new QChar[len];
	    for (uint i=0; i<len; i++)
		u[i] = p[i].reversedBytes();
	    dev->writeBlock( (char*)u, len*sizeof(QChar) );
	    delete [] u;
	}
	break;
      case UnicodeLittleEndian: {
	    dev->writeBlock( (char*)p, len*sizeof(QChar) );
	}
	break;
    }
    return *this;
}

/*!
  \fn int QTextStream::flags() const
  Returns the current stream flags. The default value is 0.

  The meaning of the flags are:
  <ul>
    <li> \e skipws - Not currently used - whitespace always skipped
    <li> \e left - Numeric fields are left-aligned
    <li> \e right - Not currently used (by default numerics are right aligned)
    <li> \e internal - Put any padding spaces between +/- and value
    <li> \e bin - Output \e and input only in binary
    <li> \e oct - Output \e and input only in octal
    <li> \e dec - Output \e and input only in decimal
    <li> \e hex - Output \e and input only in hexadecimal
    <li> \e showbase - Annotate numeric outputs with 0b, 0, or 0x if in
		\e bin, \e oct, or \e hex format
    <li> \e showpoint - Not currently used
    <li> \e uppercase - Use 0B and 0X rather than 0b and 0x
    <li> \e showpos - Show + for positive numeric values
    <li> \e scientific - Use scientific notation for floating point values
    <li> \e fixed - Use fixed-point notation for floating point values
  </ul>

  Note that unless \e bin, \e oct, \e dec, or \e hex is set, the input base is
    octal if the value starts with 0, hexadecimal if it starts with 0x, binary
    if the value starts with 0b, and decimal otherwise.

  \sa setf(), unsetf()
*/

/*!
  \fn int QTextStream::flags( int f )
  Sets the stream flags to \e f.
  Returns the previous stream flags.

  \sa setf(), unsetf(), flags()
*/

/*!
  \fn int QTextStream::setf( int bits )
  Sets the stream flag bits \e bits.
  Returns the previous stream flags.

  Equivalent to <code>flags( flags() | bits )</code>.

  \sa setf(), unsetf()
*/

/*!
  \fn int QTextStream::setf( int bits, int mask )
  Sets the stream flag bits \e bits with a bit mask \e mask.
  Returns the previous stream flags.

  Equivalent to <code>flags( (flags() & ~mask) | (bits & mask) )</code>.

  \sa setf(), unsetf()
*/

/*!
  \fn int QTextStream::unsetf( int bits )
  Clears the stream flag bits \e bits.
  Returns the previous stream flags.

  Equivalent to <code>flags( flags() & ~mask )</code>.

  \sa setf()
*/

/*!
  \fn int QTextStream::width() const
  Returns the field width. The default value is 0.
*/

/*!
  \fn int QTextStream::width( int w )
  Sets the field width to \e w. Returns the previous field width.
*/

/*!
  \fn int QTextStream::fill() const
  Returns the fill character. The default value is ' ' (space).
*/

/*!
  \fn int QTextStream::fill( int f )
  Sets the fill character to \e f. Returns the previous fill character.
*/

/*!
  \fn int QTextStream::precision() const
  Returns the precision. The default value is 6.
*/

/*!
  \fn int QTextStream::precision( int p )
  Sets the precision to \e p. Returns the previous precision setting.
*/


 /*****************************************************************************
  QTextStream manipulators
 *****************************************************************************/

QTextStream &bin( QTextStream &s )
{
    s.setf(QTS::bin,QTS::basefield);
    return s;
}

QTextStream &oct( QTextStream &s )
{
    s.setf(QTS::oct,QTS::basefield);
    return s;
}

QTextStream &dec( QTextStream &s )
{
    s.setf(QTS::dec,QTS::basefield);
    return s;
}

QTextStream &hex( QTextStream &s )
{
    s.setf(QTS::hex,QTS::basefield);
    return s;
}

QTextStream &endl( QTextStream &s )
{
    return s << '\n';
}

QTextStream &flush( QTextStream &s )
{
    if ( s.device() )
	s.device()->flush();
    return s;
}

QTextStream &ws( QTextStream &s )
{
    s.eatWhiteSpace();
    return s;
}

QTextStream &reset( QTextStream &s )
{
    s.reset();
    return s;
}
