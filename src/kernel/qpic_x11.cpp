/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qpic_x11.cpp#3 $
**
** Implementation of QMetaFile class for X11
**
** Author  : Haavard Nord
** Created : 940729
**
** Copyright (C) 1994 by Troll Tech as.  All rights reserved.
**
*****************************************************************************/

#include "qmetafil.h"
#include "qpainter.h"
#include "qpntarry.h"
#include "qfile.h"
#include "qdstream.h"
#define	 GC GC_QQQ
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#if defined(DEBUG)
static char ident[] = "$Id: //depot/qt/main/src/kernel/qpic_x11.cpp#3 $";
#endif


static const UINT32 mfhdr_tag = 0x11140d06;	// header tag
static const int    mfhdr_maj = 1;		// major version #
static const int    mfhdr_min = 0;		// minor version #


QMetaFile::QMetaFile()
{
    setDevType( PDT_METAFILE | PDF_EXTDEV );	// set device type
}

QMetaFile::~QMetaFile()
{
}


bool QMetaFile::cmd( int c, QPDevCmdParam *p )
{
    QDataStream s;
    s.setDevice( &mfbuf );
    if ( c ==  PDC_BEGIN ) {			// begin; write header
	QByteArray empty( 0 );
	mfbuf.setBuffer( empty );		// reset byte array in buffer
	mfbuf.open( IO_WriteOnly );
	s << mfhdr_tag << (UINT16)0 << mfhdr_maj << mfhdr_min;
	return TRUE;
    }
    else if ( c == PDC_END ) {			// end; calc checksum and close
	s << c << (int)0;
	QByteArray buf = mfbuf.buffer();
	int cs_start = sizeof(UINT32);		// pos of checksum word
	int data_start = cs_start + sizeof(UINT16);
	long pos = mfbuf.at();
	mfbuf.at( cs_start );			// write checksum
	UINT16 cs = (UINT16)qchecksum( buf.data()+data_start, pos-data_start );
	s << cs;
	mfbuf.close();
	return TRUE;
    }
    s << c;					// write cmd to stream
    s << (int)0;				// write dummy length info
    int pos = (int)mfbuf.at();			// save position
    switch ( c ) {
	case PDC_DRAWPOINT:
	case PDC_MOVETO:
	case PDC_LINETO:
	    s << *p[0].p;
	    break;
	case PDC_DRAWLINE:
	    s << *p[0].p << *p[1].p;
	    break;
	case PDC_DRAWRECT:
	    s << *p[0].r;
	    break;
	case PDC_DRAWROUNDRECT:
	    s << *p[0].r << p[1].i << p[2].i;
	    break;
	case PDC_DRAWELLIPSE:
	    s << *p[0].r;
	    break;
	case PDC_DRAWARC:
	case PDC_DRAWPIE:
	case PDC_DRAWCHORD:
	    s << *p[0].r << p[1].i << p[2].i;
	    break;
	case PDC_DRAWLINESEGS:
	case PDC_DRAWPOLYLINE:
	    s << *p[0].a;
	    break;
	case PDC_DRAWPOLYGON:
	    s << *p[0].a << p[1].i;
	    break;
	case PDC_DRAWTEXT:
	    s << *p[0].p << p[1].str;
	    break;
	case PDC_SETBKCOLOR:
	    s << p[0].ul;
	    break;
	case PDC_SETBKMODE:
	    s << p[0].i;
	    break;
	case PDC_SETROP:
	    s << p[0].i;
	    break;
	case PDC_SETPEN:
	    s << p[0].i << p[1].i << p[2].ul;
	    break;
	case PDC_SETBRUSH:
	    s << p[0].i << p[1].ul;
	    break;
	case PDC_SETXFORM:
	    s << p[0].i;
	    break;
	case PDC_SETSOURCEVIEW:
	    s << *p[0].r;
	    break;
	case PDC_SETTARGETVIEW:
	    s << *p[0].r;
	    break;
#if defined(CHECK_RANGE)
	default:
	    warning( "QMetaFile::cmd: Invalid command %d", c );
#endif
    }
    int newpos = (int)mfbuf.at();		// new position
    mfbuf.at(pos - 4);				// set back and
    s << (newpos - pos);			//   write size of data
    mfbuf.at( newpos );				// set to new position
    return TRUE;
}
