/****************************************************************************
** $Id: //depot/qt/main/src/tools/qbuffer.cpp#2 $
**
** Implementation of QBuffer class
**
** Author  : Haavard Nord
** Created : 930812
**
** Copyright (C) 1993,1994 by Troll Tech as.  All rights reserved.
**
*****************************************************************************/

#include "qbuffer.h"
#include <stdlib.h>

#if defined(DEBUG)
static char ident[] = "$Id: //depot/qt/main/src/tools/qbuffer.cpp#2 $";
#endif


QBuffer::QBuffer()
{
    setFlags( IO_Direct );
    a_inc = 512;				// initial increment
    a_len = 0;
}

QBuffer::QBuffer( QByteArray ba )
{
    setFlags( IO_Direct );
    a_inc = 512;				// initial increment
    setBuffer( ba );				// set buffer
}

QBuffer::~QBuffer()
{
}


bool QBuffer::setBuffer( QByteArray ba )	// set buffer
{
    if ( isOpen() ) {
#if defined(CHECK_STATE)
	warning( "QBuffer::setBuffer: Buffer is open");
#endif
	return FALSE;
    }
    a = ba;
    a_len = a.size();
    return TRUE;
}


bool QBuffer::open( int m  )			// open buffer
{
    if ( isOpen() ) {				// buffer already open
#if defined(CHECK_STATE)
	warning( "QBuffer::open: Buffer already open" );
#endif
	return FALSE;
    }
    setMode( m );
    setState( IO_Open );
    setStatus( 0 );
    index = 0;
    return TRUE;
}

void QBuffer::close()				// close buffer
{
    if ( isOpen() ) {
	setFlags( IO_Direct );
	index = 0;
    }
}

void QBuffer::flush()				// flush buffer
{
    return;					// nothing to do
}


bool QBuffer::at( long n )			// set buffer index
{
#if defined(CHECK_STATE)
    if ( !isOpen() ) {
	warning( "QBuffer::at: Buffer is not open" );
	return FALSE;
    }
#endif
    if ( (uint)n > a_len ) {
#if defined(CHECK_RANGE)
	warning( "QBuffer::at: Index %lu out of range", n );
#endif
	return FALSE;
    }
    index = n;
    return TRUE;
}


int QBuffer::readBlock( char *p, uint len )	// read data from buffer
{
#if defined(CHECK_STATE)
    CHECK_PTR( p );
    if ( !isOpen() ) {				// buffer not open
	warning( "QBuffer::readBlock: Buffer not open" );
	return -1;
    }
    if ( !isReadable() ) {			// reading not permitted
	warning( "QBuffer::readBlock: Read operation not permitted" );
	return -1;
    }
#endif
    if ( (uint)index + len > a.size() ) {	// overflow
#if defined(CHECK_RANGE)
	warning( "QBuffer::readBlock: Buffer read error" );
#endif
	setStatus( IO_ReadError );
	return -1;
    }
    memcpy( p, a.data()+index, len );
    index += len;
    return len;
}

int QBuffer::writeBlock( const char *p, uint len )// write data info buffer
{
#if defined(CHECK_STATE)
    CHECK_PTR( p );
    if ( !isOpen() ) {				// buffer not open
	warning( "QBuffer::writeBlock: Buffer not open" );
	return -1;
    }
    if ( !isWritable() ) {			// writing not permitted
	warning( "QBuffer::writeBlock: Write operation not permitted" );
	return -1;
    }
#endif
    if ( (uint)index + len >= a_len ) {		// overflow
	uint new_len = a_len + a_inc*(((uint)index+len-a_len)/a_inc+1);
	if ( !a.resize( new_len ) ) {		// could not resize
#if defined(CHECK_NULL)
	    warning( "QBuffer::writeBlock: Memory allocation error" );
#endif
	    setStatus( IO_ResourceError );
	    return -1;
	}
	a_inc *= 2;				// double increment
	a_len = new_len;
	a.p->len = (uint)index + len;
    }
    memcpy( a.data()+index, p, len );
    index += len;
    if ( a.p->len < (uint)index )
	a.p->len = (uint)index;			// fake (not alloc'd) length
    return len;
}


int QBuffer::getch()				// get next char
{
#if defined(CHECK_STATE)
    if ( !isOpen() ) {				// buffer not open
	warning( "QBuffer::getch: Buffer not open" );
	return -1;
    }
    if ( !isReadable() ) {			// reading not permitted
	warning( "QBuffer::getch: Read operation not permitted" );
	return -1;
    }
#endif
    if ( (uint)index+1 > a.size() ) {		// overflow
#if defined(CHECK_RANGE)
	warning( "QBuffer::getch: Buffer read error" );
#endif
	setStatus( IO_ReadError );
	return -1;
    }
    return *(a.data()+index++);
}

int QBuffer::putch( int ch )			// put char
{
#if defined(CHECK_STATE)
    if ( !isOpen() ) {				// buffer not open
	warning( "QBuffer::putch: Buffer not open" );
	return -1;
    }
    if ( !isWritable() ) {			// writing not permitted
	warning( "QBuffer::putch: Write operation not permitted" );
	return -1;
    }
#endif
    if ( (uint)index + 1 >= a_len ) {		// overflow
	char buf[1];
	buf[0] = (char)ch;
	writeBlock( buf, 1 );
    }
    else {
	*(a.data() + index++) = (char)ch;
	if ( a.p->len < (uint)index )
	    a.p->len = (uint)index;
    }
    return ch;
}

int QBuffer::ungetch( int ch )			// put back char
{
#if defined(CHECK_STATE)
    if ( !isOpen() ) {				// buffer not open
	warning( "QBuffer::ungetch: Buffer not open" );
	return -1;
    }
    if ( !isReadable() ) {			// reading not permitted
	warning( "QBuffer::ungetch: Read operation not permitted" );
	return -1;
    }
#endif
     if ( index )
	index--;
    else
	ch = -1;
    return ch;
}
