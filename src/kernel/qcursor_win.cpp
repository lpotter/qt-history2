/****************************************************************************
** $Id$
**
** Implementation of QCursor class for Win32
**
** Created : 940219
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses for Windows may use this file in accordance with the Qt Commercial
** License Agreement provided with the Software.
**
** This file is not available for use under any other license without
** express written permission from the copyright holder.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "qcursor.h"
#include "qbitmap.h"
#include "qapplication.h"
#include "qapplication_p.h"
#include "qimage.h"
#include "qdatastream.h"
#include "qt_windows.h"

static QCursor cursorTable[Qt::LastCursor+1];

QT_STATIC_CONST_IMPL QCursor & Qt::arrowCursor = cursorTable[0];
QT_STATIC_CONST_IMPL QCursor & Qt::upArrowCursor = cursorTable[1];
QT_STATIC_CONST_IMPL QCursor & Qt::crossCursor = cursorTable[2];
QT_STATIC_CONST_IMPL QCursor & Qt::waitCursor = cursorTable[3];
QT_STATIC_CONST_IMPL QCursor & Qt::ibeamCursor = cursorTable[4];
QT_STATIC_CONST_IMPL QCursor & Qt::sizeVerCursor = cursorTable[5];
QT_STATIC_CONST_IMPL QCursor & Qt::sizeHorCursor = cursorTable[6];
QT_STATIC_CONST_IMPL QCursor & Qt::sizeBDiagCursor = cursorTable[7];
QT_STATIC_CONST_IMPL QCursor & Qt::sizeFDiagCursor = cursorTable[8];
QT_STATIC_CONST_IMPL QCursor & Qt::sizeAllCursor = cursorTable[9];
QT_STATIC_CONST_IMPL QCursor & Qt::blankCursor = cursorTable[10];
QT_STATIC_CONST_IMPL QCursor & Qt::splitHCursor = cursorTable[11];
QT_STATIC_CONST_IMPL QCursor & Qt::splitVCursor = cursorTable[12];
QT_STATIC_CONST_IMPL QCursor & Qt::pointingHandCursor = cursorTable[13];
QT_STATIC_CONST_IMPL QCursor & Qt::forbiddenCursor = cursorTable[14];
QT_STATIC_CONST_IMPL QCursor & Qt::whatsThisCursor = cursorTable[15];


/*****************************************************************************
  Internal QCursorData class
 *****************************************************************************/

struct QCursorData : public QShared {
    QCursorData( int s = 0 );
   ~QCursorData();
    int	      cshape;
    QBitmap  *bm, *bmm;
    short     hx, hy;
    HCURSOR   hcurs;
};

QCursorData::QCursorData( int s )
{
    cshape = s;
    hcurs = 0;
    bm = bmm = 0;
    hx = hy  = 0;
}

QCursorData::~QCursorData()
{
    if ( bm || bmm ) {
	delete bm;
	delete bmm;
#ifndef Q_OS_TEMP
	if ( hcurs )
	    DestroyCursor( hcurs );
#endif
    }
}


/*****************************************************************************
  QCursor member functions
 *****************************************************************************/

QCursor *QCursor::find_cur( int shape )	// find predefined cursor
{
    return (uint)shape <= LastCursor ? &cursorTable[shape] : 0;
}


static bool initialized = FALSE;

void QCursor::initialize()
{
    int shape;
    for( shape = 0; shape <= LastCursor; shape++ )
	cursorTable[shape].data = new QCursorData( shape );
    initialized = TRUE;
    qAddPostRoutine( cleanup );
}

void QCursor::cleanup()
{
    int shape;
    for( shape = 0; shape <= LastCursor; shape++ ) {
	delete cursorTable[shape].data;
	cursorTable[shape].data = 0;
    }
    initialized = FALSE;
}

QCursor::QCursor()
{
    if ( !initialized ) {
	if ( qApp->startingUp() ) {
	    data = 0;
	    return;
	}
	initialize();
    }
    QCursor* c = (QCursor *)&arrowCursor;
    c->data->ref();
    data = c->data;
}

QCursor::QCursor( int shape )
{
    if ( !initialized )
	initialize();
    QCursor *c = find_cur( shape );
    if ( !c )					// not found
	c = (QCursor *)&arrowCursor;	//   then use arrowCursor
    c->data->ref();
    data = c->data;
}


void QCursor::setBitmap( const QBitmap &bitmap, const QBitmap &mask,
			 int hotX, int hotY )
{
    if ( !initialized )
	initialize();
    if ( bitmap.depth() != 1 || mask.depth() != 1 ||
	 bitmap.size() != mask.size() ) {
#if defined(QT_CHECK_NULL)
	qWarning( "QCursor: Cannot create bitmap cursor; invalid bitmap(s)" );
#endif
	QCursor *c = (QCursor *)&arrowCursor;
	c->data->ref();
	data = c->data;
	return;
    }
    data = new QCursorData;
    Q_CHECK_PTR( data );
    data->bm  = new QBitmap( bitmap );
    data->bmm = new QBitmap( mask );
    data->hcurs = 0;
    data->cshape = BitmapCursor;
    data->hx = hotX >= 0 ? hotX : bitmap.width()/2;
    data->hy = hotY >= 0 ? hotY : bitmap.height()/2;
}

QCursor::QCursor( const QCursor &c )
{
    if ( !initialized )
	initialize();
    data = c.data;
    data->ref();
}

QCursor::~QCursor()
{
    if ( data && data->deref() )
	delete data;
}


QCursor &QCursor::operator=( const QCursor &c )
{
    if ( !initialized )
	initialize();
    if ( !initialized )
	initialize();
    c.data->ref();				// avoid c = c
    if ( data && data->deref() )
	delete data;
    data = c.data;
    return *this;
}


int QCursor::shape() const
{
    if ( !initialized )
	initialize();
    return data->cshape;
}

void QCursor::setShape( int shape )
{
    if ( !initialized )
	initialize();
    QCursor *c = find_cur( shape );		// find one of the global ones
    if ( !c )					// not found
	c = (QCursor *)&arrowCursor;	//   then use arrowCursor
    c->data->ref();
    if ( data->deref() )			// make shallow copy
	delete data;
    data = c->data;
}


const QBitmap *QCursor::bitmap() const
{
    if ( !initialized )
	initialize();
    return data->bm;
}

const QBitmap *QCursor::mask() const
{
    if ( !initialized )
	initialize();
    return data->bmm;
}

QPoint QCursor::hotSpot() const
{
    if ( !initialized )
	initialize();
    return QPoint( data->hx, data->hy );
}


HCURSOR QCursor::handle() const
{
    if ( !initialized )
	initialize();
    if ( !data->hcurs )
	update();
    return data->hcurs;
}

QCursor::QCursor( HCURSOR handle )
{
    data = new QCursorData;
    data->hcurs = handle;
}

QPoint QCursor::pos()
{
    POINT p;
    GetCursorPos( &p );
    return QPoint( p.x, p.y );
}

void QCursor::setPos( int x, int y )
{
    SetCursorPos( x, y );
}


void QCursor::update() const
{
    if ( !initialized )
	initialize();
    if ( data->hcurs )				// already loaded
	return;

    // Non-standard Windows cursors are created from bitmaps

    static const uchar vsplit_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
	0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
	0x00, 0x41, 0x82, 0x00, 0x80, 0x41, 0x82, 0x01, 0xc0, 0x7f, 0xfe, 0x03,
	0x80, 0x41, 0x82, 0x01, 0x00, 0x41, 0x82, 0x00, 0x00, 0x40, 0x02, 0x00,
	0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
	0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const uchar vsplitm_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00,
	0x00, 0xe0, 0x07, 0x00, 0x00, 0xe2, 0x47, 0x00, 0x00, 0xe3, 0xc7, 0x00,
	0x80, 0xe3, 0xc7, 0x01, 0xc0, 0xff, 0xff, 0x03, 0xe0, 0xff, 0xff, 0x07,
	0xc0, 0xff, 0xff, 0x03, 0x80, 0xe3, 0xc7, 0x01, 0x00, 0xe3, 0xc7, 0x00,
	0x00, 0xe2, 0x47, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00,
	0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const uchar hsplit_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x80, 0x00, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xe0, 0x03, 0x00,
	0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
	0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xff, 0x7f, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x7f, 0x00,
	0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
	0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x00,
	0x00, 0xc0, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const uchar hsplitm_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
	0x00, 0xc0, 0x01, 0x00, 0x00, 0xe0, 0x03, 0x00, 0x00, 0xf0, 0x07, 0x00,
	0x00, 0xf8, 0x0f, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00,
	0x00, 0xc0, 0x01, 0x00, 0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00,
	0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00,
	0x80, 0xff, 0xff, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00,
	0x00, 0xc0, 0x01, 0x00, 0x00, 0xf8, 0x0f, 0x00, 0x00, 0xf0, 0x07, 0x00,
	0x00, 0xe0, 0x03, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const uchar phand_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x80, 0x04, 0x00, 0x00,
	0x80, 0x04, 0x00, 0x00, 0x80, 0x04, 0x00, 0x00, 0x80, 0x04, 0x00, 0x00,
	0x80, 0x1c, 0x00, 0x00, 0x80, 0xe4, 0x00, 0x00, 0x80, 0x24, 0x03, 0x00,
	0x80, 0x24, 0x05, 0x00, 0xb8, 0x24, 0x09, 0x00, 0xc8, 0x00, 0x09, 0x00,
	0x88, 0x00, 0x08, 0x00, 0x90, 0x00, 0x08, 0x00, 0xa0, 0x00, 0x08, 0x00,
	0x20, 0x00, 0x08, 0x00, 0x40, 0x00, 0x08, 0x00, 0x40, 0x00, 0x04, 0x00,
	0x80, 0x00, 0x04, 0x00, 0x80, 0x00, 0x04, 0x00, 0x00, 0x01, 0x02, 0x00,
	0x00, 0x01, 0x02, 0x00, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

   static const uchar phandm_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x80, 0x07, 0x00, 0x00,
	0x80, 0x07, 0x00, 0x00, 0x80, 0x07, 0x00, 0x00, 0x80, 0x07, 0x00, 0x00,
	0x80, 0x1f, 0x00, 0x00, 0x80, 0xff, 0x00, 0x00, 0x80, 0xff, 0x03, 0x00,
	0x80, 0xff, 0x07, 0x00, 0xb8, 0xff, 0x0f, 0x00, 0xf8, 0xff, 0x0f, 0x00,
	0xf8, 0xff, 0x0f, 0x00, 0xf0, 0xff, 0x0f, 0x00, 0xe0, 0xff, 0x0f, 0x00,
	0xe0, 0xff, 0x0f, 0x00, 0xc0, 0xff, 0x0f, 0x00, 0xc0, 0xff, 0x07, 0x00,
	0x80, 0xff, 0x07, 0x00, 0x80, 0xff, 0x07, 0x00, 0x00, 0xff, 0x03, 0x00,
	0x00, 0xff, 0x03, 0x00, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    static const uchar * const cursor_bits32[] = {
	vsplit_bits, vsplitm_bits, hsplit_bits, hsplitm_bits,
        phand_bits, phandm_bits
    };

    unsigned short *sh;
    switch ( data->cshape ) {			// map to windows cursor
    case ArrowCursor:
	sh = (unsigned short*)IDC_ARROW;
	break;
    case UpArrowCursor:
	sh = (unsigned short*)IDC_UPARROW;
	break;
    case CrossCursor:
	sh = (unsigned short*)IDC_CROSS;
	break;
    case WaitCursor:
	sh = (unsigned short*)IDC_WAIT;
	break;
    case IbeamCursor:
	sh = (unsigned short*)IDC_IBEAM;
	break;
    case SizeVerCursor:
	sh = (unsigned short*)IDC_SIZENS;
	break;
    case SizeHorCursor:
	sh = (unsigned short*)IDC_SIZEWE;
	break;
    case SizeBDiagCursor:
	sh = (unsigned short*)IDC_SIZENESW;
	break;
    case SizeFDiagCursor:
	sh = (unsigned short*)IDC_SIZENWSE;
	break;
    case SizeAllCursor:
	sh = (unsigned short*)IDC_SIZEALL;
	break;
    case ForbiddenCursor:
	sh = (unsigned short*)IDC_NO;
	break;
    case WhatsThisCursor:
	sh = (unsigned short*)IDC_HELP;
	break;
    case PointingHandCursor:
	if ( qt_winver == Qt::WV_2000 || qt_winver == Qt::WV_XP ) {
	    sh = (unsigned short*)IDC_HAND;
	    break;
	}
    case BlankCursor:
    case SplitVCursor:
    case SplitHCursor:
    case BitmapCursor: {
	QImage bbits, mbits;
	bool invb, invm;
	if ( data->cshape == BlankCursor ) {
	    bbits.create( 32, 32, 1, 2, QImage::BigEndian );
	    bbits.fill( 0 );		// ignore color table
	    mbits = bbits.copy();
	    data->hx = data->hy = 16;
	    invb = invm = FALSE;
	} else if ( data->cshape != BitmapCursor ) {
	    int i = data->cshape - SplitVCursor;
	    QBitmap cb( 32, 32, cursor_bits32[i*2], TRUE );
	    QBitmap cm( 32, 32, cursor_bits32[i*2+1], TRUE );
	    bbits = cb;
	    mbits = cm;
	    if ( data->cshape == PointingHandCursor ) {
		data->hx = 7;
		data->hy = 0;
	    } else
		data->hx = data->hy = 16;
	    invb = invm = FALSE;
	} else {
	    bbits = *data->bm;
	    mbits = *data->bmm;
	    invb = bbits.numColors() > 1 &&
		   qGray(bbits.color(0)) < qGray(bbits.color(1));
	    invm = mbits.numColors() > 1 &&
		   qGray(mbits.color(0)) < qGray(mbits.color(1));
	}
	int n = QMAX( 1, bbits.width() / 8 );
	int h = bbits.height();
	uchar* xBits = new uchar[h*n];
	uchar* xMask = new uchar[h*n];
	int x = 0;
	for ( int i = 0; i < h; i++ ) {
	    uchar *bits = bbits.scanLine( i );
	    uchar *mask = mbits.scanLine( i );
	    for ( int j = 0; j < n; j++ ) {
		uchar b = bits[j];
		uchar m = mask[j];
		if ( invb )
		    b ^= 0xff;
		if ( invm )
		    m ^= 0xff;
		xBits[x] = ~m;
		xMask[x] = b ^ m;
		x++;
	    }
	}
#ifndef Q_OS_TEMP
	data->hcurs = CreateCursor( qWinAppInst(), data->hx, data->hy,
				    bbits.width(), bbits.height(),
				    xBits, xMask );
#endif
	delete xBits;
	delete xMask;
	return;
    }
    default:
#if defined(QT_CHECK_RANGE)
	qWarning( "QCursor::update: Invalid cursor shape %d",
		  data->cshape );
#endif
	return;
    }
#ifdef Q_OS_TEMP
	data->hcurs = LoadCursorW( 0, (const TCHAR *)sh );
#else
#if defined(UNICODE)
    if ( qt_winver & Qt::WV_NT_based )
	// ### From MSDN:
	// ### LoadCursor has been superseded by the LoadImage function.
	data->hcurs = LoadCursorW( 0, (const TCHAR *)sh );
    else
#endif
	data->hcurs = LoadCursorA( 0, (const char*)sh );
#endif
}
