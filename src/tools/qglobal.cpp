/****************************************************************************
** $Id: //depot/qt/main/src/tools/qglobal.cpp#16 $
**
** Global functions
**
** Author  : Haavard Nord
** Created : 920604
**
** Copyright (C) 1992-1995 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#include "qglobal.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

RCSTAG("$Id: //depot/qt/main/src/tools/qglobal.cpp#16 $")


/*----------------------------------------------------------------------------
  Returns the Qt version number for the library, typically "1.1".
 ----------------------------------------------------------------------------*/

const char *qVersion()
{
    return "0.93";
}


/*****************************************************************************
  System detection routines
 *****************************************************************************/

static bool si_alreadyDone = FALSE;
static int  si_wordSize;
static bool si_bigEndian;

/*----------------------------------------------------------------------------
  Obtains information about the system.

  The system's word size in bits (typically 32) is returned in \e *wordSize.
  The \e *bigEndian is set to TRUE if this is a big-endian machine,
  or to FALSE if this is a little-endian machine.

  This function calls fatal() with a message if the computer is truely weird
  (i.e. different endianness for 16 bit and 32 bit integers).
 ----------------------------------------------------------------------------*/

bool qSysInfo( int *wordSize, bool *bigEndian )
{
    if ( si_alreadyDone ) {			// run it only once
	*wordSize  = si_wordSize;
	*bigEndian = si_bigEndian;
	return TRUE;
    }
    si_alreadyDone = TRUE;

    si_wordSize = 1;
    uint n = (uint)(~0);
    while ( n >>= 1 )				// detect word size
	si_wordSize++;
    *wordSize = si_wordSize;

    if ( *wordSize != 32 && *wordSize != 16 ) { // word size should be 16 or 32
#if defined(CHECK_RANGE)
	fatal( "qSysInfo: Unsupported system word size %d", *wordSize );
#endif
	return FALSE;
    }
    if ( sizeof(INT8) != 1 || sizeof(INT16) != 2 || sizeof(INT32) != 4 ||
	 sizeof(float) != 4 || sizeof(double) != 8 ) {
#if defined(CHECK_RANGE)
	fatal( "qSysInfo: Unsupported system data type size" );
#endif
	return FALSE;
    }

    bool be16, be32;				// determine byte ordering
    short ns = 0x1234;
    long nl = 0x12345678;

    unsigned char *p = (unsigned char *)(&ns);	// 16-bit integer
    be16 = *p == 0x12;

    p = (unsigned char *)(&nl);			// 32-bit integer
    if ( p[0] == 0x12 && p[1] == 0x34 && p[2] == 0x56 && p[3] == 0x78 )
	be32 = TRUE;
    else
    if ( p[0] == 0x78 && p[1] == 0x56 && p[2] == 0x34 && p[3] == 0x12 )
	be32 = FALSE;
    else
	be32 = !be16;

    if ( be16 != be32 ) {			// strange machine!
#if defined(CHECK_RANGE)
	fatal( "qSysInfo: Inconsistent system byte order" );
#endif
	return FALSE;
    }

    *bigEndian = si_bigEndian = be32;
    return TRUE;
}


/*****************************************************************************
  Debug output routines
 *****************************************************************************/

static msg_handler handler = 0;			// pointer to debug handler

/*----------------------------------------------------------------------------
  Prints a debug message, or calls the message handler (if it has been
  installed).

  This function takes a format string and a stack arguments, similar to
  the C printf() function.

  Example:
  \code
    debug( "my window handle = %x", myWidget->id() );
  \endcode

  Under X-Windows, the text is printed to stderr.  Under Windows,
  the text is sent to the debugger.

  \warning The internal buffer is limited to 512 bytes (including the
  0-terminator.

  \sa warning(), fatal(), qInstallMsgHandler()
 ----------------------------------------------------------------------------*/

void debug( const char *msg, ... )
{
    char buf[512];
    va_list ap;
    va_start( ap, msg );			// use variable arg list
    if ( handler ) {
	vsprintf( buf, msg, ap );
	va_end( ap );
	(*handler)( QtDebugMsg, buf );
    }
    else {
	vfprintf( stderr, msg, ap );
	va_end( ap );
	fprintf( stderr, "\n" );		// add newline
    }
}

/*----------------------------------------------------------------------------
  Prints a warning message, or calls the message handler (if it has been
  installed).

  This function takes a format string and a stack arguments, similar to
  the C printf() function.

  Example:
  \code
    void f( int c )
    {
        if ( c > 200 )
            warning( "f: bad argument, c == %d", c );
    }
  \endcode

  Under X-Windows, the text is printed to stderr.  Under Windows,
  the text is sent to the debugger.

  \warning The internal buffer is limited to 512 bytes (including the
  0-terminator.

  \sa debug(), fatal(), qInstallMsgHandler()
 ----------------------------------------------------------------------------*/

void warning( const char *msg, ... )
{
    char buf[512];
    va_list ap;
    va_start( ap, msg );			// use variable arg list
    if ( handler ) {
	vsprintf( buf, msg, ap );
	va_end( ap );
	(*handler)( QtWarningMsg, buf );
    }
    else {
	vfprintf( stderr, msg, ap );
	va_end( ap );
	fprintf( stderr, "\n" );		// add newline
    }
}

/*----------------------------------------------------------------------------
  Prints a fatal error message and exits, or calls the message handler (if it
  has been installed).

  This function takes a format string and a stack arguments, similar to
  the C printf() function.

  Example:
  \code
    int divide( int a, int b )
    {
        if ( b == 0 )				// program error
	    fatal( "divide: cannot divide by zero" );
	return a/b;
    }
  \endcode

  Under X-Windows, the text is printed to stderr.  Under Windows,
  the text is sent to the debugger.

  \warning The internal buffer is limited to 512 bytes (including the
  0-terminator.

  \sa debug(), warning(), qInstallMsgHandler()
 ----------------------------------------------------------------------------*/

void fatal( const char *msg, ... )
{
    char buf[512];
    va_list ap;
    va_start( ap, msg );			// use variable arg list
    if ( handler ) {
	vsprintf( buf, msg, ap );
	va_end( ap );
	(*handler)( QtFatalMsg, buf );
    }
    else {
	vfprintf( stderr, msg, ap );
	va_end( ap );
	fprintf( stderr, "\n" );		// add newline
#if defined(UNIX) && defined(DEBUG)
	abort();				// trap; generates core dump
#else
	exit( 1 );				// goodbye cruel world
#endif
    }
}



/*----------------------------------------------------------------------------
  \fn void ASSERT( bool test )
  Prints a warning message containing the source code file name and line number
  if \e test is FALSE.

  This is really a macro defined in qglobal.h.

  ASSERT is useful for testing required conditions in your program.

  \sa warning()
 ----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  \fn void CHECK_PTR( void *p )
  If \e p is null, a fatal messages says that the program ran out of memory
  and exits.  If \e p is not null, nothing happens.

  This is really a macro defined in qglobal.h.

  \warning CHECK_PTR only works for the development release of the Qt
  library.  In the release library, CHECK_PTR will be substituted with
  nothing.

  Example:
  \code
    int *a;
    CHECK_PTR( a = new int[80] );	// DO NOT DO THIS
      // do this instead
    a = new int[80];
    CHECK_PTR( a );			// this is fine
  \endcode

  \sa fatal()
 ----------------------------------------------------------------------------*/


//
// The CHECK_PTR macro calls this function to check if an allocation went ok.
//

bool chk_pointer( bool c, const char *n, int l )
{
    if ( c )
	fatal( "In file %s, line %d: Out of memory", n, l );
    return TRUE;
}


/*----------------------------------------------------------------------------
  Installs a Qt message handler.  Returns a pointer to the message handler
  previously defined.

  The message handler is a function that prints out debug messages,
  warnings and fatal error messages.  The Qt library (debug version)
  contains hundreds of warning messages that are printed when internal
  errors (usually invalid function arguments) occur.  If you implement
  your own message handler, you get total control of these messages.

  The default message handler prints the message to the standard output
  under X-Windows or to the debugger under Windows.  If it is a fatal
  message, the application aborts immediately.

  Only one message handler can be defined, since this is usually done on
  an application-wide basis to control debug output.

  To restore the message handler, call \c qInstallMsgHandler(0).

  Example:
  \code
    #include <qapp.h>
    #include <stdio.h>
    #include <stdlib.h>

    void myMessageOutput( QtMsgType type, const char *msg )
    {
        switch ( type ) {
	    case QtDebugMsg:
	        fprintf( stderr, "Debug: %s\n", msg );
		break;
	    case QtWarningMsg:
	        fprintf( stderr, "Warning: %s\n", msg );
		break;
	    case QtFatalMsg:
	        fprintf( stderr, "Fatal: %s\n", msg );
		abort();			// dump core on purpose
	}
    }

    int main( int argc, char **argv )
    {
        qInstallMsgHandler( myMessageOutput );
	QApplication a( argc, argv );
	...
	return a.exec();
    }
  \endcode

  \sa debug(), warning(), fatal()
 ----------------------------------------------------------------------------*/

msg_handler qInstallMsgHandler( msg_handler h )
{
    msg_handler old = handler;
    handler = h;
    return old;
}
