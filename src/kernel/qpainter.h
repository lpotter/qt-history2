/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qpainter.h#57 $
**
** Definition of QPainter class
**
** Author  : Haavard Nord
** Created : 940112
**
** Copyright (C) 1994,1995 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#ifndef QPAINTER_H
#define QPAINTER_H

#include "qpaintd.h"
#include "qcolor.h"
#include "qfontmet.h"
#include "qfontinf.h"
#include "qregion.h"
#include "qpen.h"
#include "qbrush.h"
#include "qpntarry.h"
#include "qwmatrix.h"


enum BGMode					// background mode
    { TransparentMode, OpaqueMode };

enum PaintUnit					// paint unit
    { PixelUnit, LoMetricUnit, HiMetricUnit, LoEnglishUnit, HiEnglishUnit,
      TwipsUnit };


class QPainter					// painter class
{
public:
    QPainter();
   ~QPainter();

    bool	begin( const QPaintDevice * );	// begin painting in device
    bool	end();				// end painting
    QPaintDevice *device() const { return pdev; }

    static void redirect( QPaintDevice *pdev, QPaintDevice *replacement );

    bool	isActive() const { return testf(IsActive); }

    void	save();				// save (push) current state
    void	restore();			// restore (pop) state

  // Drawing tools

    QFontMetrics fontMetrics()	const { return QFontMetrics(this); }
    QFontInfo	 fontInfo()	const { return QFontInfo(this); }

    const QFont &font()		const { return cfont; }
    void	setFont( const QFont & );
    const QPen &pen()		const { return cpen; }
    void	setPen( const QPen & );
    void	setPen( PenStyle );
    void	setPen( const QColor & );
    const QBrush &brush()	const { return cbrush; }
    void	setBrush( const QBrush & );
    void	setBrush( BrushStyle );
    void	setBrush( const QColor & );

  // Drawing attributes/modes

    const QColor &backgroundColor() const;
    void	setBackgroundColor( const QColor & );
    BGMode	backgroundMode() const;
    void	setBackgroundMode( BGMode );
    RasterOp	rasterOp()	const;
    void	setRasterOp( RasterOp );
    const QPoint &brushOrigin() const;
    void	setBrushOrigin( int x, int y );
    void	setBrushOrigin( const QPoint & );

  // Scaling and transformations

//    PaintUnit unit()	       const;		// get set painter unit
//    void	setUnit( PaintUnit );		// NOT IMPLEMENTED!!!

    void	setViewXForm( bool );		// set xform on/off
    bool	hasViewXForm() const { return testf(VxF); }
    QRect	window()       const;		// get window
    void	setWindow( const QRect & );	// set window
    void	setWindow( int x, int y, int w, int h );
    QRect	viewport()   const;		// get viewport
    void	setViewport( const QRect & );	// set viewport
    void	setViewport( int x, int y, int w, int h );

    void	setWorldXForm( bool );		// set world xform on/off
    bool	hasWorldXForm() const { return testf(WxF); }
    const QWMatrix &worldMatrix() const;	// get/set world xform matrix
    void	setWorldMatrix( const QWMatrix &, bool concat=FALSE );

    QPoint	xForm( const QPoint & ) const;	// map virtual -> device
    QRect	xForm( const QRect & )	const;
    QPointArray xForm( const QPointArray & ) const;
    QPoint	xFormDev( const QPoint & ) const; // map device -> virtual
    QRect	xFormDev( const QRect & )  const;
    QPointArray xFormDev( const QPointArray & ) const;

  // Clipping

    void	setClipping( bool );		// set clipping on/off
    bool	hasClipping() const { return testf(ClipOn); }
    void	setClipRect( const QRect & );	// set clip rectangle
    void	setClipRect( int x, int y, int w, int h );
    void	setClipRegion( const QRegion &);// set clip region

  // Graphics drawing functions

    void	drawPoint( int x, int y );
    void	drawPoint( const QPoint & );
    void	moveTo( int x, int y );
    void	moveTo( const QPoint & );
    void	lineTo( int x, int y );
    void	lineTo( const QPoint & );
    void	drawLine( int x1, int y1, int x2, int y2 );
    void	drawLine( const QPoint &, const QPoint & );
    void	drawRect( int x, int y, int w, int h );
    void	drawRect( const QRect & );
    void	drawRoundRect( int x, int y, int w, int h, int xRnd, int yRnd);
    void	drawRoundRect( const QRect &, int, int );
    void	drawEllipse( int x, int y, int w, int h );
    void	drawEllipse( const QRect & );
    void	drawArc( int x, int y, int w, int h, int a, int alen );
    void	drawArc( const QRect &, int a, int alen );
    void	drawPie( int x, int y, int w, int h, int a, int alen );
    void	drawPie( const QRect &, int a, int alen );
    void	drawChord( int x, int y, int w, int h, int a, int alen );
    void	drawChord( const QRect &, int a, int alen );
    void	drawLineSegments( const QPointArray &,
				  int index=0, int nlines=-1 );
    void	drawPolyline( const QPointArray &,
			      int index=0, int npoints=-1 );
    void	drawPolygon( const QPointArray &, bool winding=FALSE,
			     int index=0, int npoints=-1 );
    void	drawBezier( const QPointArray &,
			    int index=0, int npoints=-1 );
    void	drawPixmap( int x, int y, const QPixmap &,
			    int sx=0, int sy=0, int sw=-1, int sh=-1 );
    void	drawPixmap( const QPoint &, const QPixmap &,
			    const QRect &sr );
    void	drawPixmap( const QPoint &, const QPixmap & );
    void	drawPicture( const QPicture & );

    void	fillRect( int x, int y, int w, int h, const QColor & );
    void	fillRect( const QRect &, const QColor & );
    void	eraseRect( int x, int y, int w, int h );
    void	eraseRect( const QRect & );

  // Shade drawing functions

    void	drawShadeLine( int x1, int y1, int x2, int y2,
			       const QColor &tc, const QColor &bc, int lw=1,
			       const QColor &mc=black, int midlw=0 );
    void	drawShadeLine( const QPoint &, const QPoint &,
			       const QColor &tc, const QColor &bc, int lw=1,
			       const QColor &mc=black, int midlw=0 );
    void	drawShadeRect( int x, int y, int w, int h,
			       const QColor &tColor, const QColor &bColor,
			       int lw=1,
			       const QColor &fColor=black, int midlw=0 );
    void	drawShadeRect( const QRect &,
			       const QColor &tColor, const QColor &bColor,
			       int lw=1,
			       const QColor &fColor=black, int midlw=0 );
    void	drawShadePanel( int x, int y, int w, int h,
				const QColor &tColor, const QColor &bColor,
				int lw=1,
				const QColor &fColor=black, bool fill=FALSE );
    void	drawShadePanel( const QRect &,
				const QColor &tColor, const QColor &bColor,
				int lw=1,
				const QColor &fColor=black, bool fill=FALSE );

  // Text drawing functions

    void	drawText( int x, int y, const char *str, int len = -1 );
    void	drawText( const QPoint &, const char *str, int len = -1 );
    void	drawText( int x, int y, int w, int h, int flags,
			  const char *str, int len = -1, QRect *br=0,
			  char **internal=0 );
    void	drawText( const QRect &, int flags,
			  const char *str, int len = -1, QRect *br=0,
			  char **internal=0 );

  // Text drawing functions

    QRect	boundingRect( int x, int y, int w, int h, int flags,
			      const char *str, int len = -1, char **intern=0 );
    QRect	boundingRect( const QRect &, int flags,
			      const char *str, int len = -1, char **intern=0 );

    int		tabStops() const	{ return tabstops; }
    void	setTabStops( int );
    int	       *tabArray() const	{ return tabarray; }
    void	setTabArray( int * );

    HANDLE	handle() const;

    static void initialize();
    static void cleanup();

private:
    void	updateFont();
    void	updatePen();
    void	updateBrush();
    void	updateXForm();

    enum { IsActive=0x01, DirtyFont=0x02, DirtyPen=0x04, DirtyBrush=0x08,
	   VxF=0x10, WxF=0x20, ClipOn=0x40, ExtDev=0x80, SafePolygon=0x100,
	   FontMet=0x200, FontInf=0x400, IsStartingUp=0x800 };
    ushort	flags;
    bool	testf( ushort b ) const { return (flags&b)!=0; }
    void	setf( ushort b )	{ flags |= b; }
    void	setf( ushort b, bool v );
    void	clearf( ushort b )	{ flags &= (ushort)(~b); }

    QPaintDevice *pdev;
    QColor	bg_col;
    uchar	bg_mode;
    uchar	rop;
    uchar	pu;
    QPoint	bro;
    QFont	cfont;
    QPen	cpen;
    QBrush	cbrush;
    QRegion	crgn;
    int		tabstops;
    int	       *tabarray;
    int		tabarraylen;
    QCOORD	wx, wy, ww, wh;
    QCOORD	vx, vy, vw, vh;
    QWMatrix	wxmat;
#if defined(_WS_MAC_) || defined(_WS_WIN16_) || defined(_WS_X11_)
    long	wm11, wm12, wm21, wm22, wdx, wdy;
    long	im11, im12, im21, im22, idx, idy;
#endif
    void       *ps_stack;
    void	killPStack();

protected:
#if defined(_WS_WIN_)
    HDC		hdc;				// device context
    HANDLE	hpen;				// current pen
    HANDLE	hbrush;				// current brush
    HANDLE	hbrushbm;			// current brush bitmap
    uint	stockBrush	: 1;
    uint	pixmapBrush	: 1;
    uint	tmpHandle	: 1;
    uint	xfFont		: 1;
    void       *tm;
#elif defined(_WS_PM_)
    HPS		hps;				// presentation space
    int		dh;				// device height
    long	lctrl;				// outline/fill control
    void	updateCtrl();			// update lctrl
#elif defined(_WS_X11_)
    Display    *dpy;				// current display
    WId		hd;				// handle to drawable
    GC		gc;				// graphics context (standard)
    GC		gc_brush;			// graphics contect for brush
    QPoint	curPt;				// current point
#endif
    friend class QFontMetrics;
    friend class QFontInfo;
};


// --------------------------------------------------------------------------
// QPainter member functions
//

inline const QColor &QPainter::backgroundColor() const
{
    return bg_col;
}

inline BGMode QPainter::backgroundMode() const
{
    return (BGMode)bg_mode;
}

inline RasterOp QPainter::rasterOp() const
{
    return (RasterOp)rop;
}

inline const QPoint &QPainter::brushOrigin() const
{
    return bro;
}

/*
inline PaintUnit QPainter::unit() const
{
    return (PaintUnit)pu;
}
*/

inline HANDLE QPainter::handle() const
{
#if defined(_WS_WIN_)
    return hdc;
#elif defined(_WS_PM_)
    return hps;
#elif defined(_WS_X11_)
    return hd;
#endif
}


#if !(defined(QPAINTER_C) || defined(DEBUG))

inline void QPainter::setBrushOrigin( const QPoint &p )
{
    setBrushOrigin( p.x(), p.y() );
}

inline void QPainter::setWindow( const QRect &r )
{
    setWindow( r.x(), r.y(), r.width(), r.height() );
}

inline void QPainter::setViewport( const QRect &r )
{
    setViewport( r.x(), r.y(), r.width(), r.height() );
}

inline void QPainter::setClipRect( int x, int y, int w, int h )
{
    setClipRect( QRect(x,y,w,h) );
}

inline void QPainter::drawPoint( const QPoint &p )
{
    drawPoint( p.x(), p.y() );
}

inline void QPainter::moveTo( const QPoint &p )
{
    moveTo( p.x(), p.y() );
}

inline void QPainter::lineTo( const QPoint &p )
{
    lineTo( p.x(), p.y() );
}

inline void QPainter::drawLine( const QPoint &p1, const QPoint &p2 )
{
    drawLine( p1.x(), p1.y(), p2.x(), p2.y() );
}

inline void QPainter::drawRect( const QRect &r )
{
    drawRect( r.x(), r.y(), r.width(), r.height() );
}

inline void QPainter::drawRoundRect( const QRect &r, int xRnd, int yRnd )
{
    drawRoundRect( r.x(), r.y(), r.width(), r.height(), xRnd, yRnd );
}

inline void QPainter::drawEllipse( const QRect &r )
{
    drawEllipse( r.x(), r.y(), r.width(), r.height() );
}

inline void QPainter::drawArc( const QRect &r, int a, int alen )
{
    drawArc( r.x(), r.y(), r.width(), r.height(), a, alen );
}

inline void QPainter::drawPie( const QRect &r, int a, int alen )
{
    drawPie( r.x(), r.y(), r.width(), r.height(), a, alen );
}

inline void QPainter::drawChord( const QRect &r, int a, int alen )
{
    drawChord( r.x(), r.y(), r.width(), r.height(), a, alen );
}

inline void QPainter::drawPixmap( const QPoint &p, const QPixmap &pm,
				  const QRect &sr )
{
    drawPixmap( p.x(), p.y(), pm, sr.x(), sr.y(), sr.width(), sr.height() );
}

inline void QPainter::fillRect( const QRect &r, const QColor &c )
{
    fillRect( r.x(), r.y(), r.width(), r.height(), c );
}

inline void QPainter::eraseRect( int x, int y, int w, int h )
{
    fillRect( x, y, w, h, backgroundColor() );
}

inline void QPainter::eraseRect( const QRect &r )
{
    fillRect( r.x(), r.y(), r.width(), r.height(), backgroundColor() );
}

inline void QPainter::drawShadeLine( const QPoint &p1, const QPoint &p2,
				     const QColor &tc, const QColor &bc,
				     int lw, const QColor &mc, int mlw )
{
    drawShadeLine( p1.x(), p1.y(), p2.x(), p2.y(), tc, bc, lw, mc, mlw );
}

inline void QPainter::drawShadeRect( const QRect &r,
				     const QColor &tc, const QColor &bc,
				     int lw, const QColor &mc, int mlw )
{
    drawShadeRect( r.x(), r.y(), r.width(), r.height(), tc, bc, lw, mc, mlw );
}

inline void QPainter::drawShadePanel( const QRect &r,
				      const QColor &tc, const QColor &bc,
				      int lw, const QColor &fc, bool fill )
{
    drawShadePanel( r.x(), r.y(), r.width(), r.height(), tc, bc, lw, fc, fill);
}

inline void QPainter::drawText( const QPoint &p, const char *s, int len )
{
    drawText( p.x(), p.y(), s, len );
}

inline void QPainter::drawText( const QRect &r, int tf,
				const char *str, int len, QRect *br, char **i )
{
    drawText( r.x(), r.y(), r.width(), r.height(), tf, str, len, br, i );
}

inline QRect QPainter::boundingRect( const QRect &r, int tf,
				     const char *str, int len, char **i )
{
    return boundingRect( r.x(), r.y(), r.width(), r.height(), tf, str, len,
			 i );
}

#endif // inline functions


#endif // QPAINTER_H
