/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qpaintd.h#30 $
**
** Definition of QPaintDevice class
**
** Author  : Haavard Nord
** Created : 940721
**
** Copyright (C) 1994,1995 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#ifndef QPAINTD_H
#define QPAINTD_H

#include "qwindefs.h"
#include "qrect.h"


// Painter device types (is-A)

#define PDT_UNDEF	0x00
#define PDT_WIDGET	0x01
#define PDT_PIXMAP	0x02
#define PDT_PRINTER	0x03
#define PDT_PICTURE	0x04
#define PDT_MASK	0x0f


// Painter device flags

#define PDF_EXTDEV	0x10
#define PDF_PAINTACTIVE 0x20


// Painter device command param (defined in qpaintdc.h)

union QPDevCmdParam;


class QPaintDevice				// device for QPainter
{
public:
    virtual ~QPaintDevice();

    int	     devType()	      const { return devFlags & PDT_MASK; }
    bool     isExtDev()	      const { return devFlags & PDF_EXTDEV; }
    bool     paintingActive() const { return (devFlags & PDF_PAINTACTIVE) ==
					     PDF_PAINTACTIVE; }

#if defined(_WS_WIN_)
    HDC	     handle()  const { return hdc; }	// get device context
#elif defined(_WS_PM_)
    HPS	     handle()  const { return hps; }	// get presentation space
#elif defined(_WS_X11_)
    Display *display() const { return dpy; }	// get display
    WId	     handle()  const { return hd; }	// get drawable
#endif

protected:
    QPaintDevice( uint devflags );

#if defined(_WS_WIN_)
    HDC	     hdc;				// device context
#elif defined(_WS_PM_)
    HPS	     hps;				// presentation space
#elif defined(_WS_X11_)
    Display *dpy;				// display
    HANDLE   hd;				// handle to drawable
#endif

    virtual bool cmd( int, QPainter *, QPDevCmdParam * );
    virtual long metric( int ) const;

    uint     devFlags;				// device flags

    friend class QPainter;
    friend class QPaintDeviceMetrics;
    friend void bitBlt( QPaintDevice *, int, int, const QPaintDevice *,
			int, int, int, int, RasterOp );
};

void bitBlt( QPaintDevice *dst, int dx, int dy,
	     const QPaintDevice *src, int sx=0, int sy=0, int sw=-1, int sh=-1,
	     RasterOp = CopyROP );

inline
void bitBlt( QPaintDevice *dst, const QPoint &dp,
	     const QPaintDevice *src, const QRect &sr = QRect(0,0,-1,-1),
	     RasterOp rop = CopyROP )
{
    bitBlt( dst, dp.x(), dp.y(), src, sr.x(), sr.y(), sr.width(), sr.height(),
	    rop );
}


#endif // QPAINTD_H
