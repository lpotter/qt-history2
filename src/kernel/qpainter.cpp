/****************************************************************************
**
** Implementation of QPainter, QPen and QBrush classes.
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qapplication.h"
#include "qatomic.h"
#include "qbitmap.h"
#include "qcleanuphandler.h"
#include "qdatastream.h"
#include "qimage.h"
#include "qpaintdevicemetrics.h"
#include "qpainter.h"
#include "qpainter_p.h"
#include "qmap.h"
#include "qlist.h"
#include "qregexp.h"
#include "qtextlayout_p.h"
#include "qwidget.h"
#ifdef Q_WS_QWS
#include "qgfx_qws.h"
#endif


#include <string.h>
#include <limits.h>

#ifndef QT_NO_TRANSFORMATIONS
typedef QList<QWMatrix*> QWMatrixStack;
#endif

// POSIX Large File Support redefines truncate -> truncate64
#if defined(truncate)
# undef truncate
#endif

/*!
    \class QPainter qpainter.h
    \brief The QPainter class does low-level painting e.g. on widgets.

    \ingroup graphics
    \ingroup images
    \mainclass

    The painter provides highly optimized functions to do most of the
    drawing GUI programs require. QPainter can draw everything from
    simple lines to complex shapes like pies and chords. It can also
    draw aligned text and pixmaps. Normally, it draws in a "natural"
    coordinate system, but it can also do view and world
    transformation.

    The typical use of a painter is:

    \list
    \i Construct a painter.
    \i Set a pen, a brush etc.
    \i Draw.
    \i Destroy the painter.
    \endlist

    Mostly, all this is done inside a paint event. (In fact, 99% of
    all QPainter use is in a reimplementation of
    QWidget::paintEvent(), and the painter is heavily optimized for
    such use.) Here's one very simple example:

    \code
    void SimpleExampleWidget::paintEvent()
    {
	QPainter paint( this );
	paint.setPen( Qt::blue );
	paint.drawText( rect(), AlignCenter, "The Text" );
    }
    \endcode

    \list

    \i font() is the currently set font. If you set a font that isn't
    available, Qt finds a close match. In fact font() returns what
    you set using setFont() and fontInfo() returns the font actually
    being used (which may be the same).

    \i brush() is the currently set brush; the color or pattern that's
    used for filling e.g. circles.

    \i pen() is the currently set pen; the color or stipple that's
    used for drawing lines or boundaries.

    \i backgroundMode() is \c Opaque or \c Transparent, i.e. whether
    backgroundColor() is used or not.

    \i backgroundColor() only applies when backgroundMode() is Opaque
    and pen() is a stipple. In that case, it describes the color of
    the background pixels in the stipple.

    \i rasterOp() is how pixels drawn interact with the pixels already
    there.

    \i brushOrigin() is the origin of the tiled brushes, normally the
    origin of the window.

    \i viewport(), window(), worldMatrix() and many more make up the
    painter's coordinate transformation system. See \link
    coordsys.html The Coordinate System \endlink for an explanation of
    this, or see below for a very brief overview of the functions.

    \i hasClipping() is whether the painter clips at all. (The paint
    device clips, too.) If the painter clips, it clips to clipRegion().

    \i pos() is the current position, set by moveTo() and used by
    lineTo().

    \endlist

    Note that some of these settings mirror settings in some paint
    devices, e.g. QWidget::font(). QPainter::begin() (or the QPainter
    constructor) copies these attributes from the paint device.
    Calling, for example, QWidget::setFont() doesn't take effect until
    the next time a painter begins painting on it.

    save() saves all of these settings on an internal stack, restore()
    pops them back. This is especially useful if you are
    reimplementing a drawing function that gives you a non-const
    QPainter reference since you can call save() before you use it and
    call restore() before leaving your function so that it remains in
    tact.

    The core functionality of QPainter is drawing, and there are
    functions to draw most primitives: drawPoint(), drawPoints(),
    drawLine(), drawRect(), drawWinFocusRect(), drawRoundRect(),
    drawEllipse(), drawArc(), drawPie(), drawChord(),
    drawLineSegments(), drawPolyline(), drawPolygon(),
    drawConvexPolygon() and drawCubicBezier(). All of these functions
    take integer coordinates; there are no floating-point versions
    since we want drawing to be as fast as possible.

    There are functions to draw pixmaps/images, namely drawPixmap(),
    drawImage() and drawTiledPixmap(). drawPixmap() and drawImage()
    produce the same result, except that drawPixmap() is faster
    on-screen and drawImage() faster and sometimes better on QPrinter
    and QPicture.

    Text drawing is done using drawText(), and when you need
    fine-grained positioning, boundingRect() tells you where a given
    drawText() command would draw.

    There is a drawPicture() function that draws the contents of an
    entire QPicture using this painter. drawPicture() is the only
    function that disregards all the painter's settings: the QPicture
    has its own settings.

    Normally, the QPainter operates on the device's own coordinate
    system (usually pixels), but QPainter has good support for
    coordinate transformation. See \link coordsys.html The Coordinate
    System \endlink for a more general overview and a simple example.

    The most common functions used are scale(), rotate(), translate()
    and shear(), all of which operate on the worldMatrix().
    setWorldMatrix() can replace or add to the currently set
    worldMatrix().

    setViewport() sets the rectangle on which QPainter operates. The
    default is the entire device, which is usually fine, except on
    printers. setWindow() sets the coordinate system, that is, the
    rectangle that maps to viewport(). What's drawn inside the
    window() ends up being inside the viewport(). The window's
    default is the same as the viewport, and if you don't use the
    transformations, they are optimized away, gaining another little
    bit of speed.

    After all the coordinate transformation is done, QPainter can clip
    the drawing to an arbitrary rectangle or region. hasClipping() is
    TRUE if QPainter clips, and clipRegion() returns the clip region.
    You can set it using either setClipRegion() or setClipRect().
    Note that the clipping can be slow. It's all system-dependent,
    but as a rule of thumb, you can assume that drawing speed is
    inversely proportional to the number of rectangles in the clip
    region.

    After QPainter's clipping, the paint device may also clip. For
    example, most widgets clip away the pixels used by child widgets,
    and most printers clip away an area near the edges of the paper.
    This additional clipping is not reflected by the return value of
    clipRegion() or hasClipping().

    QPainter also includes some less-used functions that are very
    useful on those occasions when they're needed.

    isActive() indicates whether the painter is active. begin() (and
    the most usual constructor) makes it active. end() (and the
    destructor) deactivates it. If the painter is active, device()
    returns the paint device on which the painter paints.

    setTabStops() and setTabArray() can change where the tab stops
    are, but these are very seldomly used.

    \warning Note that QPainter does not attempt to work around
    coordinate limitations in the underlying window system. Some
    platforms may behave incorrectly with coordinates as small as
    +/-4000.

    \headerfile qdrawutil.h

    \sa QPaintDevice QWidget QPixmap QPrinter QPicture
	\link simple-application.html Application Walkthrough \endlink
	\link coordsys.html Coordinate System Overview \endlink
*/

/*!
    \fn QGfx * QPainter::internalGfx()

    \internal
*/

/*!
    \enum QPainter::CoordinateMode
    \value CoordDevice
    \value CoordPainter

    \sa clipRegion()
*/
/*!
    \enum QPainter::TextDirection
    \value Auto
    \value RTL right to left
    \value LTR left to right

    \sa drawText()
*/

/*!
    \enum Qt::PaintUnit
    \value PixelUnit
    \value LoMetricUnit \e obsolete
    \value HiMetricUnit \e obsolete
    \value LoEnglishUnit \e obsolete
    \value HiEnglishUnit \e obsolete
    \value TwipsUnit \e obsolete
*/

/*!
    \enum Qt::BrushStyle

    \value NoBrush
    \value SolidPattern
    \value Dense1Pattern
    \value Dense2Pattern
    \value Dense3Pattern
    \value Dense4Pattern
    \value Dense5Pattern
    \value Dense6Pattern
    \value Dense7Pattern
    \value HorPattern
    \value VerPattern
    \value CrossPattern
    \value BDiagPattern
    \value FDiagPattern
    \value DiagCrossPattern
    \value CustomPattern

    \img brush-styles.png Brush Styles

*/

/*!
    \enum Qt::RasterOp

    \keyword raster operation
    \keyword raster op

    This enum type is used to describe the way things are written to
    the paint device. Each bit of the \e src (what you write)
    interacts with the corresponding bit of the \e dst pixel.

    \value CopyROP  dst = src
    \value OrROP   dst = src OR dst
    \value XorROP   dst = src XOR dst
    \value NotAndROP  dst = (NOT src) AND dst
    \value EraseROP  an alias for \c NotAndROP
    \value NotCopyROP  dst = NOT src
    \value NotOrROP  dst = (NOT src) OR dst
    \value NotXorROP  dst = (NOT src) XOR dst
    \value AndROP  dst = src AND dst
    \value NotEraseROP  an alias for \c AndROP
    \value NotROP  dst = NOT dst
    \value ClearROP  dst = 0
    \value SetROP  dst = 1
    \value NopROP  dst = dst
    \value AndNotROP  dst = src AND (NOT dst)
    \value OrNotROP  dst = src OR (NOT dst)
    \value NandROP  dst = NOT (src AND dst)
    \value NorROP  dst = NOT (src OR dst)

    By far the most useful ones are \c CopyROP and \c XorROP.

    On Qt/Mac, only \c CopyROP, \c OrROP, \c XorROP, \c NotAndROP,
    \c NotCopyROP, \c NotOrROP, \c NotXorROP, and \c AndROP are
    supported.

    On Qt/Embedded, only \c CopyROP, \c XorROP, and \c NotROP are supported.
*/

/*!
    \enum Qt::AlignmentFlags

    This enum type is used to describe alignment. It contains
    horizontal and vertical flags.

    The horizontal flags are:

    \value AlignAuto Aligns according to the language. Left for most,
	right for Arabic and Hebrew.
    \value AlignLeft Aligns with the left edge.
    \value AlignRight Aligns with the right edge.
    \value AlignHCenter Centers horizontally in the available space.
    \value AlignJustify Justifies the text in the available space.
	Does not work for everything and may be interpreted as
	AlignAuto in some cases.

    The vertical flags are:

    \value AlignTop Aligns with the top.
    \value AlignBottom Aligns with the bottom.
    \value AlignVCenter Centers vertically in the available space.

    You can use only one of the horizontal flags at a time. There is
    one two-dimensional flag:

    \value AlignCenter Centers in both dimensions.

    You can use at most one horizontal and one vertical flag at a time. \c
    AlignCenter counts as both horizontal and vertical.

    Masks:

    \value AlignHorizontal_Mask
    \value AlignVertical_Mask

    Conflicting combinations of flags have undefined meanings.
*/

/*!
    \enum Qt::TextFlags

    This enum type is used to define some modifier flags. Some of
    these flags only make sense in the context of printing:

    \value SingleLine Treats all whitespace as spaces and prints just
	one line.
    \value DontClip If it's impossible to stay within the given bounds,
	it prints outside.
    \value ExpandTabs Makes the U+0009 (ASCII tab) character move to
	the next tab stop.
    \value ShowPrefix Displays the string "\&P" as <u>P</u>
	(see QButton for an example). For an ampersand, use "\&\&".
    \value WordBreak Breaks lines at appropriate points, e.g. at word
	boundaries.
    \value BreakAnywhere Breaks lines anywhere, even within words.
    \value NoAccel Same as ShowPrefix but doesn't draw the underlines.

    You can use as many modifier flags as you want, except that \c
    SingleLine and \c WordBreak cannot be combined.

    Flags that are inappropriate for a given use (e.g. ShowPrefix to
    QGridLayout::addWidget()) are generally ignored.

*/

/*!
    \enum Qt::PenStyle

    This enum type defines the pen styles that can be drawn using
    QPainter. The styles are

    \value NoPen  no line at all. For example, QPainter::drawRect()
    fills but does not draw any boundary line.

    \value SolidLine  a simple line.

    \value DashLine  dashes separated by a few pixels.

    \value DotLine  dots separated by a few pixels.

    \value DashDotLine  alternate dots and dashes.

    \value DashDotDotLine  one dash, two dots, one dash, two dots.

    \value MPenStyle mask of the pen styles.

    \img pen-styles.png Pen Styles
*/

/*!
    \enum Qt::PenCapStyle

    This enum type defines the pen cap styles supported by Qt, i.e.
    the line end caps that can be drawn using QPainter.

    \value FlatCap  a square line end that does not cover the end
	point of the line.
    \value SquareCap  a square line end that covers the end point and
	extends beyond it with half the line width.
    \value RoundCap  a rounded line end.
    \value MPenCapStyle mask of the pen cap styles.

    \img pen-cap-styles.png Pen Cap Styles
*/

/*!
    \enum Qt::PenJoinStyle

    This enum type defines the pen join styles supported by Qt, i.e.
    which joins between two connected lines can be drawn using
    QPainter.

    \value MiterJoin  The outer edges of the lines are extended to
	meet at an angle, and this area is filled.
    \value BevelJoin  The triangular notch between the two lines is filled.
    \value RoundJoin  A circular arc between the two lines is filled.
    \value MPenJoinStyle mask of the pen join styles.

    \img pen-join-styles.png Pen Join Styles
*/

/*!
    \enum Qt::BGMode

    Background mode

    \value TransparentMode
    \value OpaqueMode
*/

/*!
    Constructs a painter.

    Notice that all painter settings (setPen, setBrush etc.) are reset
    to default values when begin() is called.

    \sa begin(), end()
*/

QPainter::QPainter()
{
    init();
}


/*!
    Constructs a painter that begins painting the paint device \a pd
    immediately. Depending on the underlying graphic system the
    painter will paint over children of the paintdevice if \a
    unclipped is TRUE.

    This constructor is convenient for short-lived painters, e.g. in a
    \link QWidget::paintEvent() paint event\endlink and should be used
    only once. The constructor calls begin() for you and the QPainter
    destructor automatically calls end().

    Here's an example using begin() and end():
    \code
	void MyWidget::paintEvent( QPaintEvent * )
	{
	    QPainter p;
	    p.begin( this );
	    p.drawLine( ... );	// drawing code
	    p.end();
	}
    \endcode

    The same example using this constructor:
    \code
	void MyWidget::paintEvent( QPaintEvent * )
	{
	    QPainter p( this );
	    p.drawLine( ... );	// drawing code
	}
    \endcode

    Since the constructor cannot provide feedback when the initialization
    of the painter failed you should rather use begin() and end() to paint
    on external devices, e.g. printers.

    \sa begin(), end()
*/

QPainter::QPainter( const QPaintDevice *pd, bool unclipped )
{
    init();
    if ( begin( pd, unclipped ) )
	flags |= CtorBegin;
}


/*!
    Constructs a painter that begins painting the paint device \a pd
    immediately, with the default arguments taken from \a
    copyAttributes. The painter will paint over children of the paint
    device if \a unclipped is TRUE (although this is not supported on
    all platforms).

    \sa begin()
*/

QPainter::QPainter( const QPaintDevice *pd,
		    const QWidget *copyAttributes, bool unclipped )
{
    init();
    if ( begin( pd, copyAttributes, unclipped ) )
	flags |= CtorBegin;
}


/*!
    Destroys the painter.
*/

QPainter::~QPainter()
{
    if ( isActive() )
	end();
    else
	killPStack();
    if ( tabarray )				// delete tab array
	delete [] tabarray;
#ifndef QT_NO_TRANSFORMATIONS
    if ( wm_stack )
	delete (QWMatrixStack *)wm_stack;
#endif
    destroy();
}


/*!
    \overload bool QPainter::begin( const QPaintDevice *pd, const QWidget *copyAttributes, bool unclipped )

    This version opens the painter on a paint device \a pd and sets
    the initial pen, background color and font from \a copyAttributes,
    painting over the paint device's children when \a unclipped is
    TRUE.

    This begin function is convenient for double buffering. When you
    draw in a pixmap instead of directly in a widget (to later bitBlt
    the pixmap into the widget) you will need to set the widget's
    font etc. This function does exactly that.

    Example:
    \code
	void MyWidget::paintEvent( QPaintEvent * )
	{
	    QPixmap pm(size());
	    QPainter p;
	    p.begin(&pm, this);
	    // ... potentially flickering paint operation ...
	    p.end();
	    bitBlt(this, 0, 0, &pm);
	}
    \endcode

    \sa end()
*/

bool QPainter::begin( const QPaintDevice *pd, const QWidget *copyAttributes, bool unclipped )
{
    if ( copyAttributes == 0 ) {
	qWarning( "QPainter::begin: The widget to copy attributes from cannot "
		 "be null" );
	return FALSE;
    }
    if ( begin( pd, unclipped ) ) {
	copyFrom(copyAttributes);
	updateBrush();
	updatePen();
#ifndef QT_NO_TRANSFORMATIONS
	updateXForm();
#endif
	return TRUE;
    }
    return FALSE;
}


/*!
  \internal
  Sets or clears a pointer flag.
*/

void QPainter::setf( uint b, bool v )
{
    if ( v )
	setf( b );
    else
	clearf( b );
}


/*!
    \fn bool QPainter::isActive() const

    Returns TRUE if the painter is active painting, i.e. begin() has
    been called and end() has not yet been called; otherwise returns
    FALSE.

    \sa QPaintDevice::paintingActive()
*/

/*!
    \fn QPaintDevice *QPainter::device() const

    Returns the paint device on which this painter is currently
    painting, or 0 if the painter is not active.

    \sa QPaintDevice::paintingActive()
*/


struct QPState {				// painter state
    QFont	font;
    QPen	pen;
    QPoint	curPt;
    QBrush	brush;
    QBrush	bgb;
    uchar	bgm;
    uchar	rop;
    QPoint	bro;
    QRect	wr, vr;
#ifndef QT_NO_TRANSFORMATIONS
    QWMatrix	wm;
#else
    int		xlatex;
    int		xlatey;
#endif
    bool	vxf;
    bool	wxf;
    QRegion	rgn;
    bool	clip;
    int		ts;
    int	       *ta;
    void* wm_stack;
};

//TODO lose the worldmatrix stack

typedef QList<QPState*> QPStateStack;


void QPainter::killPStack()
{
    if ( ps_stack && !((QPStateStack *)ps_stack)->isEmpty() )
	qWarning( "QPainter::killPStack: non-empty save/restore stack when "
		  "end() was called" );
    delete (QPStateStack *)ps_stack;
    ps_stack = 0;
}

/*!
    Saves the current painter state (pushes the state onto a stack). A
    save() must be followed by a corresponding restore(). end()
    unwinds the stack.

    \sa restore()
*/

void QPainter::save()
{
    if ( testf(ExtDev) ) {
	if ( testf(DirtyFont) )
	    updateFont();
	if ( testf(DirtyPen) )
	    updatePen();
	if ( testf(DirtyBrush) )
	    updateBrush();
	pdev->cmd( QPaintDevice::PdcSave, this, 0 );
    }
    QPStateStack *pss = (QPStateStack *)ps_stack;
    if ( pss == 0 ) {
	pss = new QPStateStack;
	pss->setAutoDelete( TRUE );
	ps_stack = pss;
    }
    QPState *ps = new QPState;
    ps->font  = cfont;
    ps->pen   = cpen;
    ps->curPt = pos();
    ps->brush = cbrush;
    ps->bgb   = bg_brush;
    ps->bgm   = bg_mode;
    ps->rop   = rop;
    ps->bro   = bro;
#ifndef QT_NO_TRANSFORMATIONS
    ps->wr    = QRect( wx, wy, ww, wh );
    ps->vr    = QRect( vx, vy, vw, vh );
    ps->wm    = wxmat;
    ps->vxf   = testf(VxF);
    ps->wxf   = testf(WxF);
#else
    ps->xlatex = xlatex;
    ps->xlatey = xlatey;
#endif
    ps->rgn   = crgn;
    ps->clip  = testf(ClipOn);
    ps->ts    = tabstops;
    ps->ta    = tabarray;
    ps->wm_stack = wm_stack;
    wm_stack = 0;
    pss->push_back( ps );
}

/*!
    Restores the current painter state (pops a saved state off the
    stack).

    \sa save()
*/

void QPainter::restore()
{
    if ( testf(ExtDev) ) {
	pdev->cmd( QPaintDevice::PdcRestore, this, 0 );
	if ( pdev->devType() == QInternal::Picture )
	    block_ext = TRUE;
    }
    QPStateStack *pss = (QPStateStack *)ps_stack;
    if ( pss == 0 || pss->isEmpty() ) {
	qWarning( "QPainter::restore: Empty stack error" );
	return;
    }
    QPState *ps = pss->back();
    pss->pop_back();
    bool hardRestore = testf(VolatileDC);

    if ( ps->font != cfont || hardRestore )
	setFont( ps->font );
    if ( ps->pen != cpen || hardRestore )
	setPen( ps->pen );
    if ( ps->brush != cbrush || hardRestore )
	setBrush( ps->brush );
    if ( ps->bgb != bg_brush || hardRestore )
	setBackgroundColor( ps->bgb );
    if ( ps->bgm != bg_mode || hardRestore )
	setBackgroundMode( (BGMode)ps->bgm );
    if ( ps->rop != rop || hardRestore )
	setRasterOp( (RasterOp)ps->rop );
    if ( ps->bro != bro || hardRestore )
	setBrushOrigin( ps->bro );
#ifndef QT_NO_TRANSFORMATIONS
    QRect wr( wx, wy, ww, wh );
    QRect vr( vx, vy, vw, vh );
    if ( ps->wr != wr || hardRestore )
	setWindow( ps->wr );
    if ( ps->vr != vr || hardRestore )
	setViewport( ps->vr );
    if ( ps->wm != wxmat || hardRestore )
	setWorldMatrix( ps->wm );
    if ( ps->vxf != testf(VxF) || hardRestore )
	setViewXForm( ps->vxf );
    if ( ps->wxf != testf(WxF) || hardRestore )
	setWorldXForm( ps->wxf );
#else
    xlatex = ps->xlatex;
    xlatey = ps->xlatey;
    setf( VxF, xlatex || xlatey );
#endif
    if ( ps->curPt != pos() || hardRestore )
	moveTo( ps->curPt );
    if ( ps->rgn != crgn || hardRestore )
	setClipRegion( ps->rgn );
    if ( ps->clip != testf(ClipOn) || hardRestore )
	setClipping( ps->clip );
    tabstops = ps->ts;
    tabarray = ps->ta;

#ifndef QT_NO_TRANSFORMATIONS
    if ( wm_stack )
	delete (QWMatrixStack *)wm_stack;
    wm_stack = ps->wm_stack;
#endif
    delete ps;
    block_ext = FALSE;
}

/*!
    Returns the font metrics for the painter, if the painter is
    active. It is not possible to obtain metrics for an inactive
    painter, so the return value is undefined if the painter is not
    active.

    \sa fontInfo(), isActive()
*/

QFontMetrics QPainter::fontMetrics() const
{
    if ( pdev && pdev->devType() == QInternal::Picture )
	return QFontMetrics( cfont );

    return QFontMetrics(this);
}

/*!
    Returns the font info for the painter, if the painter is active.
    It is not possible to obtain font information for an inactive
    painter, so the return value is undefined if the painter is not
    active.

    \sa fontMetrics(), isActive()
*/

QFontInfo QPainter::fontInfo() const
{
    if ( pdev && pdev->devType() == QInternal::Picture )
	return QFontInfo( cfont );

    return QFontInfo(this);
}


/*!
    \fn const QPen &QPainter::pen() const

    Returns the painter's current pen.

    \sa setPen()
*/

/*!
    Sets a new painter pen.

    The \a pen defines how to draw lines and outlines, and it also
    defines the text color.

    \sa pen()
*/

void QPainter::setPen( const QPen &pen )
{
    if ( !isActive() )
	qWarning( "QPainter::setPen: Will be reset by begin()" );
    if ( cpen == pen )
	return;
    cpen = pen;
    updatePen();
}

/*!
    \overload

    Sets the painter's pen to have style \a style, width 0 and black
    color.

    \sa pen(), QPen
*/

void QPainter::setPen( PenStyle style )
{
    if ( !isActive() )
	qWarning( "QPainter::setPen: Will be reset by begin()" );
    QPen::QPenData *d = cpen.d;
    if ( d->style == style && d->linest == style && !d->width && d->color == black )
	return;
    if ( d->ref != 1 ) {
	cpen.detach_helper();
	d = cpen.d;
    }
    d->style = style;
    d->width = 0;
    d->color = black;
    d->linest = style;
    updatePen();
}

/*!
    \overload

    Sets the painter's pen to have style \c SolidLine, width 0 and the
    specified \a color.

    \sa pen(), QPen
*/

void QPainter::setPen( const QColor &color )
{
    if ( !isActive() )
	qWarning( "QPainter::setPen: Will be reset by begin()" );
    QPen::QPenData *d = cpen.d;
    if ( d->color == color && !d->width && d->style == SolidLine && d->linest == SolidLine )
	return;
    if ( d->ref != 1 ) {
	cpen.detach_helper();
	d = cpen.d;
    }
    d->style = SolidLine;
    d->width = 0;
    d->color = color;
    d->linest = SolidLine;
    updatePen();
}

/*!
    \fn const QBrush &QPainter::brush() const

    Returns the painter's current brush.

    \sa QPainter::setBrush()
*/

/*!
    \overload

    Sets the painter's brush to \a brush.

    The \a brush defines how shapes are filled.

    \sa brush()
*/
void QPainter::setBrush( const QBrush &brush )
{
    if ( !isActive() )
	qWarning( "QPainter::setBrush: Will be reset by begin()" );
    if ( cbrush == brush )
	return;
    cbrush = brush;
    updateBrush();
}

/*!
    Sets the painter's brush to black color and the specified \a
    style.

    \sa brush(), QBrush
*/
void QPainter::setBrush( BrushStyle style )
{
    if ( !isActive() )
	qWarning( "QPainter::setBrush: Will be reset by begin()" );
    QBrush::QBrushData *d = cbrush.d;
    if ( d->style == style && d->color == Qt::black && !d->pixmap )
	return;
    if ( d->ref != 1 ) {
	cbrush.detach_helper();
	d = cbrush.d;
    }
    d->style = style;
    d->color = Qt::black;
    if ( d->pixmap ) {
	delete d->pixmap;
	d->pixmap = 0;
    }
    updateBrush();
}

/*!
    \fn const QColor &QPainter::backgroundColor() const

    Returns the current background color.

    \sa setBackgroundColor() QColor
*/

/*!
    \fn BGMode QPainter::backgroundMode() const

    Returns the current background mode.

    \sa setBackgroundMode() BGMode
*/

/*!
    \fn RasterOp QPainter::rasterOp() const

    Returns the current raster operation.

    \sa setRasterOp() RasterOp
*/

/*!
    \fn const QBrush &QPainter::background() const

    Returns the current background as brush.

    \sa eraseRect() backgroundOrigin() setBackgroundColor() QColor
*/

/*!
    \fn const QPoint &QPainter::backgroundOrigin() const

    Returns the origin of the background. This is only interesting
    when a painter operates on a widget that inherits a pixmap
    background. Using the background origin as brush origin makes it
    possible to paint with different brushes in the background's
    coordinate system:

    \code
	painter->setBrushOrigin(painter->backgroundOrigin);
	painter->setBrush(palette().dark());
	painter->drawRect(x, y, w, h);
    \endcode

    \sa eraseRect()
*/


/*!
    \fn const QPoint &QPainter::brushOrigin() const

    Returns the brush origin currently set.

    \sa setBrushOrigin()
*/


/*!
    \fn int QPainter::tabStops() const

    Returns the tab stop setting.

    \sa setTabStops()
*/

/*!
    Set the tab stop width to \a ts, i.e. locates tab stops at \a ts,
    2*\a ts, 3*\a ts and so on.

    Tab stops are used when drawing formatted text with \c ExpandTabs
    set. This fixed tab stop value is used only if no tab array is set
    (which is the default case).

    A value of 0 (the default) implies a tabstop setting of 8 times the width of the
    character 'x' in the font currently set on the painter.

    \sa tabStops(), setTabArray(), drawText(), fontMetrics()
*/

void QPainter::setTabStops( int ts )
{
    if ( !isActive() )
	qWarning( "QPainter::setTabStops: Will be reset by begin()" );
    tabstops = ts;
    if ( isActive() && testf(ExtDev) ) {	// tell extended device
	QPDevCmdParam param[1];
	param[0].ival = ts;
	pdev->cmd( QPaintDevice::PdcSetTabStops, this, param );
    }
}

/*!
    \fn int *QPainter::tabArray() const

    Returns the currently set tab stop array.

    \sa setTabArray()
*/

/*!
    Sets the tab stop array to \a ta. This puts tab stops at \a ta[0],
    \a ta[1] and so on. The array is null-terminated.

    If both a tab array and a tab top size is set, the tab array wins.

    \sa tabArray(), setTabStops(), drawText(), fontMetrics()
*/

void QPainter::setTabArray( int *ta )
{
    if ( !isActive() )
	qWarning( "QPainter::setTabArray: Will be reset by begin()" );
    if ( ta != tabarray ) {
	tabarraylen = 0;
	if ( tabarray )				// Avoid purify complaint
	    delete [] tabarray;			// delete old array
	if ( ta ) {				// tabarray = copy of 'ta'
	    while ( ta[tabarraylen] )
		tabarraylen++;
	    tabarraylen++; // and 0 terminator
	    tabarray = new int[tabarraylen];	// duplicate ta
	    memcpy( tabarray, ta, sizeof(int)*tabarraylen );
	} else {
	    tabarray = 0;
	}
    }
    if ( isActive() && testf(ExtDev) ) {	// tell extended device
	QPDevCmdParam param[2];
	param[0].ival = tabarraylen;
	param[1].ivec = tabarray;
	pdev->cmd( QPaintDevice::PdcSetTabArray, this, param );
    }
}


/*!
    \fn HANDLE QPainter::handle() const

    Returns the platform-dependent handle used for drawing. Using this
    function is not portable.
*/


/*****************************************************************************
  QPainter xform settings
 *****************************************************************************/

#ifndef QT_NO_TRANSFORMATIONS

/*!
    Enables view transformations if \a enable is TRUE, or disables
    view transformations if \a enable is FALSE.

    \sa hasViewXForm(), setWindow(), setViewport(), setWorldMatrix(),
    setWorldXForm(), xForm()
*/

void QPainter::setViewXForm( bool enable )
{
    if ( !isActive() )
	qWarning( "QPainter::setViewXForm: Will be reset by begin()" );
    if ( !isActive() || enable == testf(VxF) )
	return;
    setf( VxF, enable );
    if ( testf(ExtDev) ) {
	QPDevCmdParam param[1];
	param[0].ival = enable;
	pdev->cmd( QPaintDevice::PdcSetVXform, this, param );
    }
    mergeXForms();
}

/*!
    \fn bool QPainter::hasViewXForm() const

    Returns TRUE if view transformation is enabled; otherwise returns
    FALSE.

    \sa setViewXForm(), xForm()
*/

/*!
    Returns the window rectangle.

    \sa setWindow(), setViewXForm()
*/

QRect QPainter::window() const
{
    return QRect( wx, wy, ww, wh );
}

/*!
    Sets the window rectangle view transformation for the painter and
    enables view transformation.

    The window rectangle is part of the view transformation. The
    window specifies the logical coordinate system and is specified by
    the \a x, \a y, \a w width and \a h height parameters. Its sister,
    the viewport(), specifies the device coordinate system.

    The default window rectangle is the same as the device's
    rectangle. See the \link coordsys.html Coordinate System Overview
    \endlink for an overview of coordinate transformation.

    \sa window(), setViewport(), setViewXForm(), setWorldMatrix(),
    setWorldXForm()
*/

void QPainter::setWindow( int x, int y, int w, int h )
{
    if ( !isActive() )
	qWarning( "QPainter::setWindow: Will be reset by begin()" );
    wx = x;
    wy = y;
    ww = w;
    wh = h;
    if ( testf(ExtDev) ) {
	QRect r( x, y, w, h );
	QPDevCmdParam param[1];
	param[0].rect = (QRect*)&r;
	pdev->cmd( QPaintDevice::PdcSetWindow, this, param );
    }
    if ( testf(VxF) )
	mergeXForms();
    else
	setViewXForm( TRUE );
}

/*!
    Returns the viewport rectangle.

    \sa setViewport(), setViewXForm()
*/

QRect QPainter::viewport() const		// get viewport
{
    return QRect( vx, vy, vw, vh );
}

/*!
    Sets the viewport rectangle view transformation for the painter
    and enables view transformation.

    The viewport rectangle is part of the view transformation. The
    viewport specifies the device coordinate system and is specified
    by the \a x, \a y, \a w width and \a h height parameters. Its
    sister, the window(), specifies the logical coordinate system.

    The default viewport rectangle is the same as the device's
    rectangle. See the \link coordsys.html Coordinate System Overview
    \endlink for an overview of coordinate transformation.

    \sa viewport(), setWindow(), setViewXForm(), setWorldMatrix(),
    setWorldXForm(), xForm()
*/

void QPainter::setViewport( int x, int y, int w, int h )
{
    if ( !isActive() )
	qWarning( "QPainter::setViewport: Will be reset by begin()" );
    vx = x;
    vy = y;
    vw = w;
    vh = h;
    if ( testf(ExtDev) ) {
	QRect r( x, y, w, h );
	QPDevCmdParam param[1];
	param[0].rect = (QRect*)&r;
	pdev->cmd( QPaintDevice::PdcSetViewport, this, param );
    }
    if ( testf(VxF) )
	mergeXForms();
    else
	setViewXForm( TRUE );
}


/*!
    Enables world transformations if \a enable is TRUE, or disables
    world transformations if \a enable is FALSE. The world
    transformation matrix is not changed.

    \sa setWorldMatrix(), setWindow(), setViewport(), setViewXForm(),
    xForm()
*/

void QPainter::setWorldXForm( bool enable )
{
    if ( !isActive() )
	qWarning( "QPainter::setWorldXForm: Will be reset by begin()" );
    if ( !isActive() || enable == testf(WxF) )
	return;
    setf( WxF, enable );
    if ( testf(ExtDev) ) {
	QPDevCmdParam param[1];
	param[0].ival = enable;
	pdev->cmd( QPaintDevice::PdcSetWXform, this, param );
    }
    mergeXForms();
}

/*!
    \fn bool QPainter::hasWorldXForm() const

    Returns TRUE if world transformation is enabled; otherwise returns
    FALSE.

    \sa setWorldXForm()
*/

/*!
    Returns the world transformation matrix.

    \sa setWorldMatrix()
*/

const QWMatrix &QPainter::worldMatrix() const
{
    return wxmat;
}

/*!
    Sets the world transformation matrix to \a m and enables world
    transformation.

    If \a combine is TRUE, then \a m is combined with the current
    transformation matrix, otherwise \a m replaces the current
    transformation matrix.

    If \a m is the identity matrix and \a combine is FALSE, this
    function calls setWorldXForm(FALSE). (The identity matrix is the
    matrix where QWMatrix::m11() and QWMatrix::m22() are 1.0 and the
    rest are 0.0.)

    World transformations are applied after the view transformations
    (i.e. \link setWindow() window\endlink and \link setViewport()
    viewport\endlink).

    The following functions can transform the coordinate system without using
    a QWMatrix:
    \list
    \i translate()
    \i scale()
    \i shear()
    \i rotate()
    \endlist

    They operate on the painter's worldMatrix() and are implemented like this:

    \code
	void QPainter::rotate( double a )
	{
	    QWMatrix m;
	    m.rotate( a );
	    setWorldMatrix( m, TRUE );
	}
    \endcode

    Note that you should always use \a combine when you are drawing
    into a QPicture. Otherwise it may not be possible to replay the
    picture with additional transformations. Using translate(),
    scale(), etc., is safe.

    For a brief overview of coordinate transformation, see the \link
    coordsys.html Coordinate System Overview. \endlink

    \sa worldMatrix() setWorldXForm() setWindow() setViewport()
    setViewXForm() xForm() QWMatrix
*/

void QPainter::setWorldMatrix( const QWMatrix &m, bool combine )
{
    if ( !isActive() )
	qWarning( "QPainter::setWorldMatrix: Will be reset by begin()" );
    if ( combine )
	wxmat = m * wxmat;			// combines
    else
	wxmat = m;				// set new matrix
    bool identity = wxmat.m11() == 1.0F && wxmat.m22() == 1.0F &&
		    wxmat.m12() == 0.0F && wxmat.m21() == 0.0F &&
		    wxmat.dx()	== 0.0F && wxmat.dy()  == 0.0F;
    if ( testf(ExtDev) && !block_ext ) {
	QPDevCmdParam param[2];
	param[0].matrix = &m;
	param[1].ival = combine;
	pdev->cmd( QPaintDevice::PdcSetWMatrix, this, param );
    }
    if ( identity && pdev->devType() != QInternal::Picture )
	setWorldXForm( FALSE );
    else if ( !testf(WxF) )
	setWorldXForm( TRUE );
    else
	mergeXForms();
}

/*! \obsolete

  We recommend using save() instead.
*/

void QPainter::saveWorldMatrix()
{
    QWMatrixStack *stack = (QWMatrixStack *)wm_stack;
    if ( stack == 0 ) {
	stack  = new QList<QWMatrix*>;
	stack->setAutoDelete( TRUE );
	wm_stack = stack;
    }

    stack->push_back( new QWMatrix( wxmat ) );

}

/*! \obsolete
  We recommend using restore() instead.
*/

void QPainter::restoreWorldMatrix()
{
    QWMatrixStack *stack = (QWMatrixStack *)wm_stack;
    if ( stack == 0 || stack->isEmpty() ) {
	qWarning( "QPainter::restoreWorldMatrix: Empty stack error" );
	return;
    }
    QWMatrix* m = stack->back();
    stack->pop_back();
    setWorldMatrix( *m );
    delete m;
}

#endif // QT_NO_TRANSFORMATIONS

/*!
    Translates the coordinate system by \a (dx, dy). After this call,
    \a (dx, dy) is added to points.

    For example, the following code draws the same point twice:
    \code
	void MyWidget::paintEvent()
	{
	    QPainter paint( this );

	    paint.drawPoint( 0, 0 );

	    paint.translate( 100.0, 40.0 );
	    paint.drawPoint( -100, -40 );
	}
    \endcode

    \sa scale(), shear(), rotate(), resetXForm(), setWorldMatrix(), xForm()
*/

void QPainter::translate( double dx, double dy )
{
#ifndef QT_NO_TRANSFORMATIONS
    QWMatrix m;
    m.translate( dx, dy );
    setWorldMatrix( m, TRUE );
#else
    xlatex += (int)dx;
    xlatey += (int)dy;
    setf( VxF, xlatex || xlatey );
#endif
}


#ifndef QT_NO_TRANSFORMATIONS
/*!
    Scales the coordinate system by \a (sx, sy).

    \sa translate(), shear(), rotate(), resetXForm(), setWorldMatrix(),
    xForm()
*/

void QPainter::scale( double sx, double sy )
{
    QWMatrix m;
    m.scale( sx, sy );
    setWorldMatrix( m, TRUE );
}

/*!
    Shears the coordinate system by \a (sh, sv).

    \sa translate(), scale(), rotate(), resetXForm(), setWorldMatrix(),
    xForm()
*/

void QPainter::shear( double sh, double sv )
{
    QWMatrix m;
    m.shear( sv, sh );
    setWorldMatrix( m, TRUE );
}

/*!
    Rotates the coordinate system \a a degrees counterclockwise.

    \sa translate(), scale(), shear(), resetXForm(), setWorldMatrix(),
    xForm()
*/

void QPainter::rotate( double a )
{
    QWMatrix m;
    m.rotate( a );
    setWorldMatrix( m, TRUE );
}


/*!
    Resets any transformations that were made using translate(), scale(),
    shear(), rotate(), setWorldMatrix(), setViewport() and
    setWindow().

    \sa worldMatrix(), viewport(), window()
*/

void QPainter::resetXForm()
{
    if ( !isActive() )
	return;
    wx = wy = vx = vy = 0;			// default view origins
    ww = vw = pdev->metric( QPaintDeviceMetrics::PdmWidth );
    wh = vh = pdev->metric( QPaintDeviceMetrics::PdmHeight );
    wxmat = QWMatrix();
    setWorldXForm( FALSE );
    setViewXForm( FALSE );

    if (!redirection_offset.isNull()) {
	txop = TxTranslate;
	setf(WxF, true);
    }
}

/*!
  \internal
  Updates an internal integer transformation matrix.
*/

void QPainter::mergeXForms()
{
    QWMatrix m;
    if ( testf(VxF) ) {
	double scaleW = (double)vw/(double)ww;
	double scaleH = (double)vh/(double)wh;
	m.setMatrix( scaleW, 0,  0,  scaleH, vx - wx*scaleW, vy - wy*scaleH );
    }
    if ( testf(WxF) ) {
	if ( testf(VxF) )
	    m = wxmat * m;
	else
	    m = wxmat;
    }
    xmat = m;

    txinv = FALSE;				// no inverted matrix
    txop  = TxNone;
    if ( m12()==0.0 && m21()==0.0 && m11() >= 0.0 && m22() >= 0.0 ) {
	if ( m11()==1.0 && m22()==1.0 ) {
	    if ( dx()!=0.0 || dy()!=0.0 )
		txop = TxTranslate;
	} else {
	    txop = TxScale;
#if defined(Q_WS_WIN)
	    setf(DirtyFont);
#endif
	}
    } else {
	txop = TxRotShear;
#if defined(Q_WS_WIN)
	setf(DirtyFont);
#endif
    }
    setBrushOrigin( bro.x(), bro.y() );

    if (!redirection_offset.isNull()) {
	txop |= TxTranslate;
	setf(WxF, true);
    }
    updateXForm();
}


/*!
  \internal
  Updates an internal integer inverse transformation matrix.
*/

void QPainter::updateInvXForm()
{
    Q_ASSERT( txinv == FALSE );
    txinv = TRUE;				// creating inverted matrix
    bool invertible;
    QWMatrix m;
    if ( testf(VxF) ) {
	m.translate( vx, vy );
	m.scale( 1.0*vw/ww, 1.0*vh/wh );
	m.translate( -wx, -wy );
    }
    if ( testf(WxF) ) {
	if ( testf(VxF) )
	    m = wxmat * m;
	else
	    m = wxmat;
    }
    ixmat = m.invert( &invertible );		// invert matrix
}

#else
void QPainter::resetXForm()
{
    xlatex = 0;
    xlatey = 0;
    clearf( VxF );

    if (!redirection_offset.isNull()) {
	setf(WxF, true);
    }
}
#endif // QT_NO_TRANSFORMATIONS


extern bool qt_old_transformations;

/*!
  \internal
  Maps a point from logical coordinates to device coordinates.
*/

void QPainter::map( int x, int y, int *rx, int *ry ) const
{
#ifndef QT_NO_TRANSFORMATIONS
    if ( qt_old_transformations ) {
	switch ( txop ) {
	    case TxNone:
		*rx = x;  *ry = y;
		break;
	    case TxTranslate:
		// #### "Why no rounding here?", Warwick asked of Haavard.
		*rx = int(x + dx());
		*ry = int(y + dy());
		break;
	    case TxScale: {
		double tx = m11()*x + dx();
		double ty = m22()*y + dy();
		*rx = tx >= 0 ? int(tx + 0.5) : int(tx - 0.5);
		*ry = ty >= 0 ? int(ty + 0.5) : int(ty - 0.5);
	    } break;
	    default: {
		double tx = m11()*x + m21()*y+dx();
		double ty = m12()*x + m22()*y+dy();
		*rx = tx >= 0 ? int(tx + 0.5) : int(tx - 0.5);
		*ry = ty >= 0 ? int(ty + 0.5) : int(ty - 0.5);
	    } break;
	}
    } else {
	switch ( txop ) {
	    case TxNone:
		*rx = x;
		*ry = y;
		break;
	    case TxTranslate:
		*rx = qRound( x + dx() );
		*ry = qRound( y + dy() );
		break;
	    case TxScale:
		*rx = qRound( m11()*x + dx() );
		*ry = qRound( m22()*y + dy() );
		break;
	    default:
		*rx = qRound( m11()*x + m21()*y+dx() );
		*ry = qRound( m12()*x + m22()*y+dy() );
		break;
	}
    }
#else
    *rx = x + xlatex;
    *ry = y + xlatey;
#endif

    *rx -= redirection_offset.x();
    *ry -= redirection_offset.y();
}

/*!
  \internal
  Maps a rectangle from logical coordinates to device coordinates.
  This internal function does not handle rotation and/or shear.
*/

void QPainter::map( int x, int y, int w, int h,
		    int *rx, int *ry, int *rw, int *rh ) const
{
#ifndef QT_NO_TRANSFORMATIONS
    if ( qt_old_transformations ) {
	switch ( txop ) {
	    case TxNone:
		*rx = x;  *ry = y;
		*rw = w;  *rh = h;
		break;
	    case TxTranslate:
		// #### "Why no rounding here?", Warwick asked of Haavard.
		*rx = int(x + dx());
		*ry = int(y + dy());
		*rw = w;  *rh = h;
		break;
	    case TxScale: {
		double tx1 = m11()*x + dx();
		double ty1 = m22()*y + dy();
		double tx2 = m11()*(x + w - 1) + dx();
		double ty2 = m22()*(y + h - 1) + dy();
		*rx = qRound( tx1 );
		*ry = qRound( ty1 );
		*rw = qRound( tx2 ) - *rx + 1;
		*rh = qRound( ty2 ) - *ry + 1;
	    } break;
	    default:
		qWarning( "QPainter::map: Internal error" );
		break;
	}
    } else {
	switch ( txop ) {
	    case TxNone:
		*rx = x;  *ry = y;
		*rw = w;  *rh = h;
		break;
	    case TxTranslate:
		*rx = qRound(x + dx() );
		*ry = qRound(y + dy() );
		*rw = w;  *rh = h;
		break;
	    case TxScale:
		*rx = qRound( m11()*x + dx() );
		*ry = qRound( m22()*y + dy() );
		*rw = qRound( m11()*w );
		*rh = qRound( m22()*h );
		break;
	    default:
		qWarning( "QPainter::map: Internal error" );
		break;
	}
    }
#else
    *rx = x + xlatex;
    *ry = y + xlatey;
    *rw = w;  *rh = h;
#endif

    *rx -= redirection_offset.x();
    *ry -= redirection_offset.y();
}

/*!
  \internal
  Maps a point from device coordinates to logical coordinates.
*/

void QPainter::mapInv( int x, int y, int *rx, int *ry ) const
{
#ifndef QT_NO_TRANSFORMATIONS
    if ( !txinv )
	qWarning( "QPainter::mapInv: Internal error" );
    if ( qt_old_transformations ) {
	double tx = im11()*x + im21()*y+idx();
	double ty = im12()*x + im22()*y+idy();
	*rx = tx >= 0 ? int(tx + 0.5) : int(tx - 0.5);
	*ry = ty >= 0 ? int(ty + 0.5) : int(ty - 0.5);
    } else {
	*rx = qRound( im11()*x + im21()*y + idx() );
	*ry = qRound( im12()*x + im22()*y + idy() );
    }
#else
    *rx = x - xlatex;
    *ry = y - xlatey;
#endif
}

/*!
  \internal
  Maps a rectangle from device coordinates to logical coordinates.
  Cannot handle rotation and/or shear.
*/

void QPainter::mapInv( int x, int y, int w, int h,
		       int *rx, int *ry, int *rw, int *rh ) const
{
#ifndef QT_NO_TRANSFORMATIONS
    if ( !txinv || txop == TxRotShear )
	qWarning( "QPainter::mapInv: Internal error" );
    if ( qt_old_transformations ) {
	double tx = im11()*x + idx();
	double ty = im22()*y + idy();
	double tw = im11()*w;
	double th = im22()*h;
	*rx = tx >= 0 ? int(tx + 0.5) : int(tx - 0.5);
	*ry = ty >= 0 ? int(ty + 0.5) : int(ty - 0.5);
	*rw = tw >= 0 ? int(tw + 0.5) : int(tw - 0.5);
	*rh = th >= 0 ? int(th + 0.5) : int(th - 0.5);
    } else {
	*rx = qRound( im11()*x + idx() );
	*ry = qRound( im22()*y + idy() );
	*rw = qRound( im11()*w );
	*rh = qRound( im22()*h );
    }
#else
    *rx = x - xlatex;
    *ry = y - xlatey;
    *rw = w;
    *rh = h;
#endif
}


/*!
    Returns the point \a pv transformed from model coordinates to
    device coordinates.

    \sa xFormDev(), QWMatrix::map()
*/

QPoint QPainter::xForm( const QPoint &pv ) const
{
#ifndef QT_NO_TRANSFORMATIONS
    if ( txop == TxNone )
	return pv;
    int x=pv.x(), y=pv.y();
    map( x, y, &x, &y );
    return QPoint( x, y ) + redirection_offset;
#else
    return QPoint( pv.x()+xlatex, pv.y()+xlatey );
#endif
}

/*!
    \overload

    Returns the rectangle \a rv transformed from model coordinates to
    device coordinates.

    If world transformation is enabled and rotation or shearing has
    been specified, then the bounding rectangle is returned.

    \sa xFormDev(), QWMatrix::map()
*/

QRect QPainter::xForm( const QRect &rv ) const
{
#ifndef QT_NO_TRANSFORMATIONS
    if ( txop == TxNone )
	return rv;
    if ( txop == TxRotShear ) {			// rotation/shear
	return xmat.mapRect( rv );
    }
    // Just translation/scale
    int x, y, w, h;
    rv.rect( &x, &y, &w, &h );
    map( x, y, w, h, &x, &y, &w, &h );
    return QRect( x + redirection_offset.x(), y + redirection_offset.y(), w, h );
#else
    return QRect( rv.x()+xlatex, rv.y()+xlatey, rv.width(), rv.height() );
#endif
}

/*!
    \overload

    Returns the point array \a av transformed from model coordinates
    to device coordinates.

    \sa xFormDev(), QWMatrix::map()
*/

QPointArray QPainter::xForm( const QPointArray &av ) const
{
    QPointArray a = av;
#ifndef QT_NO_TRANSFORMATIONS
    if ( txop != TxNone )
    {
	return xmat * av;
    }
#else
    a.translate( xlatex, xlatey );
#endif
    return a;
}

/*!
    \overload

    Returns the point array \a av transformed from model coordinates
    to device coordinates. The \a index is the first point in the
    array and \a npoints denotes the number of points to be
    transformed. If \a npoints is negative, all points from \a
    av[index] until the last point in the array are transformed.

    The returned point array consists of the number of points that
    were transformed.

    Example:
    \code
	QPointArray a(10);
	QPointArray b;
	b = painter.xForm(a, 2, 4);  // b.size() == 4
	b = painter.xForm(a, 2, -1); // b.size() == 8
    \endcode

    \sa xFormDev(), QWMatrix::map()
*/

QPointArray QPainter::xForm( const QPointArray &av, int index,
			     int npoints ) const
{
    int lastPoint = npoints < 0 ? av.size() : index+npoints;
    QPointArray a( lastPoint-index );
    memcpy( a.data(), av.data()+index, (lastPoint-index)*sizeof( QPoint ) );
#ifndef QT_NO_TRANSFORMATIONS
    return xmat*a;
#else
    a.translate( xlatex, xlatey );
    return a;
#endif
}

/*!
    \overload

    Returns the point \a pd transformed from device coordinates to
    model coordinates.

    \sa xForm(), QWMatrix::map()
*/

QPoint QPainter::xFormDev( const QPoint &pd ) const
{
#ifndef QT_NO_TRANSFORMATIONS
    if ( txop == TxNone )
	return pd;
    if ( !txinv ) {
	QPainter *that = (QPainter*)this;	// mutable
	that->updateInvXForm();
    }
#endif
    int x=pd.x(), y=pd.y();
    mapInv( x, y, &x, &y );
    return QPoint( x, y );
}

/*!
    Returns the rectangle \a rd transformed from device coordinates to
    model coordinates.

    If world transformation is enabled and rotation or shearing is
    used, then the bounding rectangle is returned.

    \sa xForm(), QWMatrix::map()
*/

QRect QPainter::xFormDev( const QRect &rd ) const
{
#ifndef QT_NO_TRANSFORMATIONS
    if ( txop == TxNone )
	return rd;
    if ( !txinv ) {
	QPainter *that = (QPainter*)this;	// mutable
	that->updateInvXForm();
    }
    if ( txop == TxRotShear ) {			// rotation/shear
	return ixmat.mapRect( rd );
    }
#endif
    // Just translation/scale
    int x, y, w, h;
    rd.rect( &x, &y, &w, &h );
    mapInv( x, y, w, h, &x, &y, &w, &h );
    return QRect( x, y, w, h );
}

/*!
    \overload

    Returns the point array \a ad transformed from device coordinates
    to model coordinates.

    \sa xForm(), QWMatrix::map()
*/

QPointArray QPainter::xFormDev( const QPointArray &ad ) const
{
#ifndef QT_NO_TRANSFORMATIONS
    if ( txop == TxNone )
	return ad;
    if ( !txinv ) {
	QPainter *that = (QPainter*)this;	// mutable
	that->updateInvXForm();
    }
    return ixmat * ad;
#else
    // ###
    return ad;
#endif
}

/*!
    \overload

    Returns the point array \a ad transformed from device coordinates
    to model coordinates. The \a index is the first point in the array
    and \a npoints denotes the number of points to be transformed. If
    \a npoints is negative, all points from \a ad[index] until the
    last point in the array are transformed.

    The returned point array consists of the number of points that
    were transformed.

    Example:
    \code
	QPointArray a(10);
	QPointArray b;
	b = painter.xFormDev(a, 1, 3);  // b.size() == 3
	b = painter.xFormDev(a, 1, -1); // b.size() == 9
    \endcode

    \sa xForm(), QWMatrix::map()
*/

QPointArray QPainter::xFormDev( const QPointArray &ad, int index,
				int npoints ) const
{
    int lastPoint = npoints < 0 ? ad.size() : index+npoints;
    QPointArray a( lastPoint-index );
    memcpy( a.data(), ad.data()+index, (lastPoint-index)*sizeof( QPoint ) );
#ifndef QT_NO_TRANSFORMATIONS
    if ( txop == TxNone )
	return a;
    if ( !txinv ) {
	QPainter *that = (QPainter*)this;	// mutable
	that->updateInvXForm();
    }
    return ixmat * a;
#else
    // ###
    return a;
#endif
}


/*!
    Fills the rectangle \a (x, y, w, h) with the \a brush.

    You can specify a QColor as \a brush, since there is a QBrush
    constructor that takes a QColor argument and creates a solid
    pattern brush.

    \sa drawRect()
*/

void QPainter::fillRect( int x, int y, int w, int h, const QBrush &brush )
{
    QPen   oldPen   = pen();			// save pen
    QBrush oldBrush = this->brush();		// save brush
    setPen( NoPen );
    setBrush( brush );
    drawRect( x, y, w, h );			// draw filled rect
    setBrush( oldBrush );			// restore brush
    setPen( oldPen );				// restore pen
}


/*!
    \overload void QPainter::setBrushOrigin( const QPoint &p )

    Sets the brush origin to point \a p.
*/

/*!
    \overload void QPainter::setWindow( const QRect &r )

    Sets the painter's window to rectangle \a r.
*/


/*!
    \overload void QPainter::setViewport( const QRect &r )

    Sets the painter's viewport to rectangle \a r.
*/


/*!
    \fn bool QPainter::hasClipping() const

    Returns TRUE if clipping has been set; otherwise returns FALSE.

    \sa setClipping()
*/

/*!
    Returns the currently set clip region. Note that the clip region
    is given in physical device coordinates and \e not subject to any
    \link coordsys.html coordinate transformation \endlink if \a m is
    equal to \c CoordDevice (the default). If \a m equals \c
    CoordPainter the returned region is in model coordinates.

    \sa setClipRegion(), setClipRect(), setClipping() QPainter::CoordinateMode
*/
QRegion QPainter::clipRegion( CoordinateMode m ) const
{
    // ### FIXME in 4.0:
    // If the transformation mode is CoordPainter, we should transform the
    // clip region with painter transformations.

#ifndef QT_NO_TRANSFORMATIONS
    QRegion r;
    if ( m == CoordDevice ) {
	r = crgn;
    } else {
	if ( !txinv ) {
	    QPainter *that = (QPainter*)this;	// mutable
	    that->updateInvXForm();
	}

	r = ixmat * crgn;
    }
    return r;
#else
    return crgn;
#endif
}

/*!
    \fn void QPainter::setClipRect( int x, int y, int w, int h, CoordinateMode m)

    Sets the clip region to the rectangle \a x, \a y, \a w, \a h and
    enables clipping. The clip mode is set to \a m.

    If \a m is \c CoordDevice (the default), the coordinates given for
    the clip region are taken to be physical device coordinates and
    are \e not subject to any \link coordsys.html coordinate
    transformations\endlink. If \a m is \c CoordPainter, the
    coordinates given for the clip region are taken to be model
    coordinates.

    \sa setClipRegion(), clipRegion(), setClipping() QPainter::CoordinateMode
*/

/*!
    \overload void QPainter::drawPoint( const QPoint &p )

    Draws the point \a p.
*/


/*!
    \overload void QPainter::moveTo( const QPoint &p )

    Moves to the point \a p.
*/

/*!
    \overload void QPainter::lineTo( const QPoint &p )

    Draws a line to the point \a p.
*/

/*!
    \overload void QPainter::drawLine( const QPoint &p1, const QPoint &p2 )

    Draws a line from point \a p1 to point \a p2.
*/

/*!
    \overload void QPainter::drawRect( const QRect &r )

    Draws the rectangle \a r.
*/

/*!
    \overload void QPainter::drawWinFocusRect( const QRect &r )

    Draws rectangle \a r as a window focus rectangle.
*/

/*!
    \overload void QPainter::drawWinFocusRect( const QRect &r, const QColor &bgColor )

    Draws rectangle \a r as a window focus rectangle using background
    color \a bgColor.
*/


#if !defined(Q_WS_X11) && !defined(Q_WS_QWS) && !defined(Q_WS_MAC)
// The doc and X implementation of this functions is in qpainter_x11.cpp
void QPainter::drawWinFocusRect( int, int, int, int,
				 bool, const QColor & )
{
    // do nothing, only called from X11 specific functions
}
#endif


/*!
    \overload void QPainter::drawRoundRect( const QRect &r, int xRnd, int yRnd )

    Draws a rounded rectangle \a r, rounding to the x position \a xRnd
    and the y position \a yRnd on each corner.
*/

/*!
    \overload void QPainter::drawEllipse( const QRect &r )

    Draws the ellipse that fits inside rectangle \a r.
*/

/*!
    \overload void QPainter::drawArc( const QRect &r, int a, int alen )

    Draws the arc that fits inside the rectangle \a r with start angle
    \a a and arc length \a alen.
*/

/*!
    \overload void QPainter::drawPie( const QRect &r, int a, int alen )

    Draws a pie segment that fits inside the rectangle \a r with start
    angle \a a and arc length \a alen.
*/

/*!
    \overload void QPainter::drawChord( const QRect &r, int a, int alen )

    Draws a chord that fits inside the rectangle \a r with start angle
    \a a and arc length \a alen.
*/

/*!
    \overload void QPainter::drawPixmap( const QPoint &p, const QPixmap &pm, const QRect &sr )

    Draws the rectangle \a sr of pixmap \a pm with its origin at point
    \a p.
*/

/*!
    \overload void QPainter::drawPixmap( const QPoint &p, const QPixmap &pm )

    Draws the pixmap \a pm with its origin at point \a p.
*/

void QPainter::drawPixmap( const QPoint &p, const QPixmap &pm )
{
    drawPixmap( p.x(), p.y(), pm, 0, 0, pm.width(), pm.height() );
}

#if !defined(QT_NO_IMAGE_SMOOTHSCALE) || !defined(QT_NO_PIXMAP_TRANSFORMATION)

/*!
    \overload

    Draws the pixmap \a pm into the rectangle \a r. The pixmap is
    scaled to fit the rectangle, if image and rectangle size disagree.
*/
void QPainter::drawPixmap( const QRect &r, const QPixmap &pm )
{
    int rw = r.width();
    int rh = r.height();
    int iw= pm.width();
    int ih = pm.height();
    if ( rw <= 0 || rh <= 0 || iw <= 0 || ih <= 0 )
	return;
    bool scale = ( rw != iw || rh != ih );
    float scaleX = (float)rw/(float)iw;
    float scaleY = (float)rh/(float)ih;
    bool smooth = ( scaleX < 1.5 || scaleY < 1.5 );

    if ( testf(ExtDev) ) {
	QPDevCmdParam param[2];
	param[0].rect = &r;
	param[1].pixmap = &pm;
#if defined(Q_WS_WIN)
	if ( !pdev->cmd( QPaintDevice::PdcDrawPixmap, this, param ) || !hdc )
	    return;
#elif defined(Q_WS_QWS)
	pdev->cmd( QPaintDevice::PdcDrawPixmap, this, param );
	return;
#elif defined(Q_WS_MAC)
	if ( !pdev->cmd( QPaintDevice::PdcDrawPixmap, this, param ) || !pdev->handle())
	    return;
#else
	if ( !pdev->cmd( QPaintDevice::PdcDrawPixmap, this, param ) || !hd )
	    return;
#endif
    }

    QPixmap pixmap = pm;

    if ( scale ) {
#ifndef QT_NO_IMAGE_SMOOTHSCALE
# ifndef QT_NO_PIXMAP_TRANSFORMATION
	if ( smooth )
# endif
	{
	    QImage i = pm.convertToImage();
	    pixmap = QPixmap( i.smoothScale( rw, rh ) );
	}
# ifndef QT_NO_PIXMAP_TRANSFORMATION
	else
# endif
#endif
#ifndef QT_NO_PIXMAP_TRANSFORMATION
	{
	    pixmap = pm.xForm( QWMatrix( scaleX, 0, 0, scaleY, 0, 0 ) );
	}
#endif
    }
    drawPixmap( r.x(), r.y(), pixmap );
}

#endif

/*!
    \overload void QPainter::drawImage( const QPoint &, const QImage &, const QRect &sr, int conversionFlags = 0 );

    Draws the rectangle \a sr from the image at the given point.
*/

/*
    Draws at point \a p the \sr rect from image \a pm, using \a
    conversionFlags if the image needs to be converted to a pixmap.
    The default value for \a conversionFlags is 0; see
    convertFromImage() for information about what other values do.

  This function may convert \a image to a pixmap and then draw it, if
  device() is a QPixmap or a QWidget, or else draw it directly, if
  device() is a QPrinter or QPicture.
*/

/*!
    Draws at (\a x, \a y) the \a sw by \a sh area of pixels from (\a
    sx, \a sy) in \a image, using \a conversionFlags if the image
    needs to be converted to a pixmap. The default value for \a
    conversionFlags is 0; see convertFromImage() for information about
    what other values do.

    This function may convert \a image to a pixmap and then draw it,
    if device() is a QPixmap or a QWidget, or else draw it directly,
    if device() is a QPrinter or QPicture.

    Currently alpha masks of the image are ignored when painting on a QPrinter.

    \sa drawPixmap() QPixmap::convertFromImage()
*/
void QPainter::drawImage( int x, int y, const QImage & image,
			  int sx, int sy, int sw, int sh,
			  int conversionFlags )
{
#ifdef Q_WS_QWS
    //### Hackish
# ifndef QT_NO_TRANSFORMATIONS
    if ( !image.isNull() && gfx &&
	(txop==TxNone||txop==TxTranslate) && !testf(ExtDev) )
# else
    if ( !image.isNull() && gfx && !testf(ExtDev) )
# endif
    {
	if(sw<0)
	    sw=image.width();
	if(sh<0)
	    sh=image.height();

	QImage image2 = qt_screen->mapToDevice( image );

	// This is a bit dubious
	if(image2.depth()==1) {
	    image2.setNumColors( 2 );
	    image2.setColor( 0, qRgb(255,255,255) );
	    image2.setColor( 1, qRgb(0,0,0) );
	}
	if ( image2.hasAlphaBuffer() )
	    gfx->setAlphaType(QGfx::InlineAlpha);
	else
	    gfx->setAlphaType(QGfx::IgnoreAlpha);
	gfx->setSource(&image2);
	if ( testf(VxF|WxF) ) {
	    map( x, y, &x, &y );
	}
	gfx->blt(x,y,sw,sh,sx,sy);
	return;
    }
#endif

    if ( !isActive() || image.isNull() )
	return;

    // right/bottom
    if ( sw < 0 )
	sw = image.width()  - sx;
    if ( sh < 0 )
	sh = image.height() - sy;

    // Sanity-check clipping
    if ( sx < 0 ) {
	x -= sx;
	sw += sx;
	sx = 0;
    }
    if ( sw + sx > image.width() )
	sw = image.width() - sx;
    if ( sy < 0 ) {
	y -= sy;
	sh += sy;
	sy = 0;
    }
    if ( sh + sy > image.height() )
	sh = image.height() - sy;

    if ( sw <= 0 || sh <= 0 )
	return;

    bool all = image.rect().intersect(QRect(sx,sy,sw,sh)) == image.rect();
    QImage subimage = all ? image : image.copy(sx,sy,sw,sh);

    if ( testf(ExtDev) ) {
	QPDevCmdParam param[2];
	QRect r( x, y, subimage.width(), subimage.height() );
	param[0].rect = &r;
	param[1].image = &subimage;
#if defined(Q_WS_WIN)
	if ( !pdev->cmd( QPaintDevice::PdcDrawImage, this, param ) || !hdc )
	    return;
#elif defined (Q_WS_QWS)
	pdev->cmd( QPaintDevice::PdcDrawImage, this, param );
	return;
#elif defined(Q_WS_MAC)
	if(!pdev->cmd( QPaintDevice::PdcDrawImage, this, param ) || !pdev->handle() )
	    return;
#else
	if ( !pdev->cmd( QPaintDevice::PdcDrawImage, this, param ) || !hd )
	    return;
#endif
    }

    QPixmap pm;
    pm.convertFromImage( subimage, conversionFlags );
    drawPixmap( x, y, pm );
}

/*!
    \overload void QPainter::drawImage( const QPoint &p, const QImage &i, int conversion_flags )

    Draws the image \a i at point \a p.

    If the image needs to be modified to fit in a lower-resolution
    result (e.g. converting from 32-bit to 8-bit), use the \a
    conversion_flags to specify how you'd prefer this to happen.

    \sa Qt::ImageConversionFlags
*/
void QPainter::drawImage( const QPoint & p, const QImage & i,
			  int conversion_flags )
{
    drawImage(p, i, i.rect(), conversion_flags);
}

#if !defined(QT_NO_IMAGE_TRANSFORMATION) || !defined(QT_NO_IMAGE_SMOOTHSCALE)

/*!
    \overload

    Draws the image \a i into the rectangle \a r. The image will be
    scaled to fit the rectangle if image and rectangle dimensions
    differ.
*/
void QPainter::drawImage( const QRect &r, const QImage &i )
{
    int rw = r.width();
    int rh = r.height();
    int iw= i.width();
    int ih = i.height();
    if ( rw <= 0 || rh <= 0 || iw <= 0 || ih <= 0 )
	return;

    if ( testf(ExtDev) ) {
	QPDevCmdParam param[2];
	param[0].rect = &r;
	param[1].image = &i;
#if defined(Q_WS_WIN)
	if ( !pdev->cmd( QPaintDevice::PdcDrawImage, this, param ) || !hdc )
	    return;
#elif defined(Q_WS_QWS)
	pdev->cmd( QPaintDevice::PdcDrawImage, this, param );
	return;
#elif defined(Q_WS_MAC)
	if ( !pdev->cmd( QPaintDevice::PdcDrawImage, this, param ) || !pdev->handle() )
	    return;
#else
	if ( !pdev->cmd( QPaintDevice::PdcDrawImage, this, param ) || !hd )
	    return;
#endif
    }


    bool scale = ( rw != iw || rh != ih );
    float scaleX = (float)rw/(float)iw;
    float scaleY = (float)rh/(float)ih;
    bool smooth = ( scaleX < 1.5 || scaleY < 1.5 );

    QImage img = scale
	? (
#if defined(QT_NO_IMAGE_TRANSFORMATION)
		i.smoothScale( rw, rh )
#elif defined(QT_NO_IMAGE_SMOOTHSCALE)
		i.scale( rw, rh )
#else
		smooth ? i.smoothScale( rw, rh ) : i.scale( rw, rh )
#endif
	  )
	: i;

    drawImage( r.x(), r.y(), img );
}

#endif


void bitBlt( QPaintDevice *dst, int dx, int dy,
	     const QImage *src, int sx, int sy, int sw, int sh,
	     int conversion_flags )
{
    QPixmap tmp;
    if ( sx == 0 && sy == 0
	&& (sw<0 || sw==src->width()) && (sh<0 || sh==src->height()) )
    {
	tmp.convertFromImage( *src, conversion_flags );
    } else {
	tmp.convertFromImage( src->copy( sx, sy, sw, sh, conversion_flags),
			      conversion_flags );
    }
    bitBlt( dst, dx, dy, &tmp );
}


/*!
    \overload void QPainter::drawTiledPixmap( const QRect &r, const QPixmap &pm, const QPoint &sp )

    Draws a tiled pixmap, \a pm, inside rectangle \a r with its origin
    at point \a sp.
*/

/*!
    \overload void QPainter::drawTiledPixmap( const QRect &r, const QPixmap &pm )

    Draws a tiled pixmap, \a pm, inside rectangle \a r.
*/

/*!
    \overload void QPainter::fillRect( const QRect &r, const QBrush &brush )

    Fills the rectangle \a r using brush \a brush.
*/

/*!

    Erases the area inside \a x, \a y, \a w, \a h with the painter's
    background.
*/
void QPainter::eraseRect( int x, int y, int w, int h )
{
    if (bg_brush.pixmap())
	drawTiledPixmap(QRect(x, y, w, h), *bg_brush.pixmap(), -bg_origin);
    else
	fillRect(x, y, w, h, bg_brush);
}


/*!
    \overload void QPainter::eraseRect( const QRect &r )

    Erases the area inside the rectangle \a r.
*/

/*!
    \fn QPainter::drawText( int x, int y, const QString &, int len = -1, TextDirection dir = Auto )

    \overload

    Draws the given text at position \a x, \a y. If \a len is -1 (the
    default) all the text is drawn, otherwise the first \a len
    characters are drawn. The text's direction is given by \a dir.

    \sa QPainter::TextDirection
*/

/*!
    \fn void QPainter::drawText( int x, int y, int w, int h, int flags,
			         const QString&, int len = -1, QRect *br=0 )

    \overload

    Draws the given text within the rectangle starting at \a x, \a y,
    with width \a w and height \a h. If \a len is -1 (the default) all
    the text is drawn, otherwise the first \a len characters are
    drawn. The text's flags that are given in the \a flags parameter
    are \l{Qt::AlignmentFlags} and \l{Qt::TextFlags} OR'd together. \a
    br (if not null) is set to the actual bounding rectangle of the
    output. The \a internal parameter is for internal use only.
*/

/*!
    \fn void QPainter::drawText( const QPoint &, const QString &, int len = -1, TextDirection dir = Auto );

    \overload

    Draws the text at the given point.

    \sa QPainter::TextDirection
*/

/*
    Draws the text in \a s at point \a p. If \a len is -1 the entire
    string is drawn, otherwise just the first \a len characters. The
    text's direction is specified by \a dir.
*/


/*!
    \fn void QPainter::drawText( int x, int y, const QString &, int pos, int len, TextDirection dir = Auto );

    \overload

    Draws the text from position \a pos, at point \a (x, y). If \a len is
    -1 the entire string is drawn, otherwise just the first \a len
    characters. The text's direction is specified by \a dir.
*/

/*!
    \fn void QPainter::drawText( const QPoint &p, const QString &, int pos, int len, TextDirection dir = Auto );

    Draws the text from position \a pos, at point \a p. If \a len is
    -1 the entire string is drawn, otherwise just the first \a len
    characters. The text's direction is specified by \a dir.

    Note that the meaning of \e y is not the same for the two
    drawText() varieties. For overloads that take a simple \e x, \e y
    pair (or a point), the \e y value is the text's baseline; for
    overloads that take a rectangle, \e rect.y() is the top of the
    rectangle and the text is aligned within that rectangle in
    accordance with the alignment flags.

    \sa QPainter::TextDirection
*/

/*!
    \fn void QPainter::drawTextItem(const QPoint &, const QTextItem &, int)
    \internal
*/

static inline void fix_neg_rect( int *x, int *y, int *w, int *h )
{
    if ( *w < 0 ) {
	*w = -*w;
	*x -= *w - 1;
    }
    if ( *h < 0 ) {
	*h = -*h;
	*y -= *h - 1;
    }
}
void QPainter::fix_neg_rect( int *x, int *y, int *w, int *h )
{
    ::fix_neg_rect(x,y,w,h);
}

//
// The drawText function takes two special parameters; 'internal' and 'brect'.
//
// The 'internal' parameter contains a pointer to an array of encoded
// information that keeps internal geometry data.
// If the drawText function is called repeatedly to display the same text,
// it makes sense to calculate text width and linebreaks the first time,
// and use these parameters later to print the text because we save a lot of
// CPU time.
// The 'internal' parameter will not be used if it is a null pointer.
// The 'internal' parameter will be generated if it is not null, but points
// to a null pointer, i.e. internal != 0 && *internal == 0.
// The 'internal' parameter will be used if it contains a non-null pointer.
//
// If the 'brect parameter is a non-null pointer, then the bounding rectangle
// of the text will be returned in 'brect'.
//

/*!
    \overload

    Draws at most \a len characters from \a str in the rectangle \a r.

    This function draws formatted text. The \a tf text format is
    really of type \l Qt::AlignmentFlags and \l Qt::TextFlags OR'd
    together.

    Horizontal alignment defaults to AlignAuto and vertical alignment
    defaults to AlignTop.

    \a brect (if not null) is set to the actual bounding rectangle of
    the output. \a internal is, yes, internal.

    \sa boundingRect()
*/

void QPainter::drawText( const QRect &r, int tf,
			 const QString& str, int len, QRect *brect )
{
    if ( !isActive() )
	return;
    if ( len < 0 )
	len = str.length();
    if ( len == 0 )				// empty string
	return;

    if ( testf(DirtyFont|ExtDev) ) {
	if ( testf(DirtyFont) )
	    updateFont();
	if ( testf(ExtDev) && (tf & DontPrint) == 0 ) {
	    QPDevCmdParam param[3];
	    QString newstr = str;
	    newstr.truncate( len );
	    param[0].rect = &r;
	    param[1].ival = tf;
	    param[2].str = &newstr;
	    if ( pdev->devType() != QInternal::Printer ) {
#if defined(Q_WS_WIN)
		if ( !pdev->cmd( QPaintDevice::PdcDrawText2Formatted,
				 this, param) ||
		     !hdc )
		    return;			// QPrinter wants PdcDrawText2
#elif defined(Q_WS_QWS)
		pdev->cmd( QPaintDevice::PdcDrawText2Formatted, this, param);
		return;
#elif defined(Q_WS_MAC)
		if ( !pdev->cmd( QPaintDevice::PdcDrawText2Formatted, this, param) ||
		    !pdev->handle())
		    return;			// QPrinter wants PdcDrawText2
#else
		if ( !pdev->cmd( QPaintDevice::PdcDrawText2Formatted,
				 this, param) ||
		     !hd )
		    return;			// QPrinter wants PdcDrawText2
#endif
	    }
	}
    }

    qt_format_text(font(), r, tf, str, len, brect,
		   tabstops, tabarray, tabarraylen, this);
}

//#define QT_FORMAT_TEXT_DEBUG

#define QChar_linesep QChar(0x2028U)

void qt_format_text( const QFont& font, const QRect &_r,
		     int tf, const QString& str, int len, QRect *brect,
		     int tabstops, int* tabarray, int tabarraylen,
		     QPainter* painter )
{
    // we need to copy r here to protect against the case (&r == brect).
    QRect r( _r );

    bool dontclip  = (tf & Qt::DontClip)  == Qt::DontClip;
    bool wordbreak  = (tf & Qt::WordBreak)  == Qt::WordBreak;
    bool singleline = (tf & Qt::SingleLine) == Qt::SingleLine;
    bool showprefix = (tf & Qt::ShowPrefix) == Qt::ShowPrefix;
    bool noaccel = ( tf & Qt::NoAccel ) == Qt::NoAccel;

    bool isRightToLeft = str.isRightToLeft();
    if ( ( tf & Qt::AlignHorizontal_Mask ) == Qt::AlignAuto )
	tf |= isRightToLeft ? Qt::AlignRight : Qt::AlignLeft;

    bool expandtabs = ( (tf & Qt::ExpandTabs) &&
			( ( (tf & Qt::AlignLeft) && !isRightToLeft ) ||
			  ( (tf & Qt::AlignRight) && isRightToLeft ) ) );

    if ( !painter )
	tf |= Qt::DontPrint;

    int maxUnderlines = 0;
    int numUnderlines = 0;
    int underlinePositionStack[32];
    int *underlinePositions = underlinePositionStack;

    QFont fnt(painter ? (painter->pfont ? *painter->pfont : painter->cfont) : font);
    QFontMetrics fm( fnt );

    QString text = str;
    // str.setLength() always does a deep copy, so the replacement
    // code below is safe.
    text.setLength( len );
    // compatible behaviour to the old implementation. Replace
    // tabs by spaces
    QChar *chr = (QChar*)text.unicode();
    const QChar *end = chr + len;
    bool haveLineSep = FALSE;
    while ( chr != end ) {
	if ( *chr == '\r' || ( singleline && *chr == '\n' ) ) {
	    *chr = ' ';
	} else if ( *chr == '\n' ) {
	    *chr = QChar_linesep;
	    haveLineSep = TRUE;
	} else if ( *chr == '&' ) {
	    ++maxUnderlines;
	}
	++chr;
    }
    if ( !expandtabs ) {
	chr = (QChar*)text.unicode();
	while ( chr != end ) {
	    if ( *chr == '\t' )
		*chr = ' ';
	    ++chr;
	}
    } else if (!tabarraylen && !tabstops) {
	tabstops = fm.width('x')*8;
    }

    if ( noaccel || showprefix ) {
	if ( maxUnderlines > 32 )
	    underlinePositions = new int[maxUnderlines];
	QChar *cout = (QChar*)text.unicode();
	QChar *cin = cout;
	int l = len;
	while ( l ) {
	    if ( *cin == '&' ) {
		++cin;
		--l;
		if ( !l )
		    break;
		if ( *cin != '&' )
		    underlinePositions[numUnderlines++] = cout - text.unicode();
	    }
	    *cout = *cin;
	    ++cout;
	    ++cin;
	    --l;
	}
	int newlen = cout - text.unicode();
	if ( newlen != text.length())
	    text.setLength( newlen );
    }

    // no need to do extra work for underlines if we don't paint
    if ( tf & Qt::DontPrint )
	numUnderlines = 0;

    int height = 0;
    int left = r.width();
    int right = 0;

    QTextLayout textLayout( text, fnt );
    int rb = qMax( 0, -fm.minRightBearing() );
    int lb = qMax( 0, -fm.minLeftBearing() );

    if ( text.isEmpty() ) {
	height = fm.height();
	left = right = 0;
	tf |= QPainter::DontPrint;
    } else {
	textLayout.beginLayout((haveLineSep || expandtabs || wordbreak) ?
			       QTextLayout::MultiLine :
			       (tf & Qt::DontPrint) ? QTextLayout::NoBidi : QTextLayout::SingleLine );

	// break underline chars into items of their own
	for( int i = 0; i < numUnderlines; i++ ) {
	    textLayout.setBoundary( underlinePositions[i] );
	    textLayout.setBoundary( underlinePositions[i]+1 );
	}

	int lineWidth = wordbreak ? qMax(0, r.width()-rb-lb) : INT_MAX;
	if(!wordbreak)
	    tf |= Qt::IncludeTrailingSpaces;

	int add = 0;
	int leading = fm.leading();
	int asc = fm.ascent();
	int desc = fm.descent();
	height = -leading;

	//qDebug("\n\nbeginLayout: lw = %d, rectwidth=%d", lineWidth , r.width());
	while ( !textLayout.atEnd() ) {
	    height += leading;
	    textLayout.beginLine( lineWidth == INT_MAX ? lineWidth : lineWidth + add );
	    //qDebug("-----beginLine( %d )-----",  lineWidth+add );
	    bool linesep = FALSE;
	    while ( 1 ) {
		QTextItem ti = textLayout.currentItem();
 		//qDebug("item: from=%d, ch=%x", ti.from(), text.unicode()[ti.from()].unicode() );
		if ( expandtabs && ti.isTab() ) {
		    int tw = 0;
		    int x = textLayout.widthUsed();
		    if ( tabarraylen ) {
// 			qDebug("tabarraylen=%d", tabarraylen );
			int tab = 0;
			while ( tab < tabarraylen ) {
			    if ( tabarray[tab] > x ) {
				tw = tabarray[tab] - x;
				break;
			    }
			    ++tab;
			}
		    } else {
			tw = tabstops - (x % tabstops);
		    }
 		    //qDebug("tw = %d",  tw );
		    if ( tw )
			ti.setWidth( tw );
		}
		if ( ti.isObject() && text.unicode()[ti.from()] == QChar_linesep )
		    linesep = TRUE;

		if ( linesep || textLayout.addCurrentItem() != QTextLayout::Ok || textLayout.atEnd() )
		    break;
	    }

	    int ascent = asc, descent = desc, lineLeft, lineRight;
	    textLayout.setLineWidth( r.width()-rb-lb + add );

	    textLayout.endLine( 0, height, tf, &ascent, &descent, &lineLeft, &lineRight );

	    //qDebug("finalizing line: lw=%d ascent = %d, descent=%d lineleft=%d lineright=%d", lineWidth+add, ascent, descent,lineLeft, lineRight  );
	    left = qMin( left, lineLeft );
	    right = qMax( right, lineRight );
	    height += ascent + descent + 1;
	    add = 0;
	    if ( linesep )
		textLayout.nextItem();
	}
    }

    int yoff = 0;
    if ( tf & Qt::AlignBottom )
	yoff = r.height() - height;
    else if ( tf & Qt::AlignVCenter )
	yoff = (r.height() - height)/2;

    if ( brect ) {
	*brect = QRect( r.x() + left, r.y() + yoff, right-left + lb+rb, height );
	//qDebug("br = %d %d %d/%d, left=%d, right=%d", brect->x(), brect->y(), brect->width(), brect->height(), left, right);
    }

    if (!(tf & QPainter::DontPrint)) {
	bool restoreClipping = FALSE;
	bool painterHasClip = FALSE;
	QRegion painterClipRegion;
	if ( !dontclip ) {
#ifndef QT_NO_TRANSFORMATIONS
	    QRegion reg = painter->xmat * r;
#else
	    QRegion reg = r;
	    reg.translate( painter->xlatex, painter->xlatey );
#endif
	    if ( painter->hasClipping() )
		reg &= painter->clipRegion();

	    painterHasClip = painter->hasClipping();
	    painterClipRegion = painter->clipRegion();
	    restoreClipping = TRUE;
	    painter->setClipRegion( reg );
	} else {
	    if ( painter->hasClipping() ){
		painterHasClip = painter->hasClipping();
		painterClipRegion = painter->clipRegion();
		restoreClipping = TRUE;
		painter->setClipping( FALSE );
	    }
	}

	int cUlChar = 0;
	int _tf = 0;
	if (fnt.underline()) _tf |= Qt::Underline;
	if (fnt.overline()) _tf |= Qt::Overline;
	if (fnt.strikeOut()) _tf |= Qt::StrikeOut;

	//qDebug("have %d items",textLayout.numItems());
	for ( int i = 0; i < textLayout.numItems(); i++ ) {
	    QTextItem ti = textLayout.itemAt( i );
 	    //qDebug("Item %d: from=%d,  length=%d,  space=%d x=%d", i, ti.from(),  ti.length(), ti.isSpace(), ti.x() );
	    if ( ti.isTab() || ti.isObject() )
		continue;
	    int textFlags = _tf;
	    if ( !noaccel && numUnderlines > cUlChar && ti.from() == underlinePositions[cUlChar] ) {
		textFlags |= Qt::Underline;
		cUlChar++;
	    }
#if defined(Q_WS_X11) || defined(Q_WS_QWS)
	    if ( painter->bg_mode == Qt::OpaqueMode )
		qt_draw_background( painter, r.x()+lb + ti.x(), r.y() + yoff + ti.y() - ti.ascent(),
				    ti.width(), ti.ascent() + ti.descent() + 1);
#endif
	    painter->drawTextItem( r.x()+lb, r.y() + yoff, ti, textFlags );
	}

	if ( restoreClipping ) {
	    painter->setClipRegion( painterClipRegion );
	    painter->setClipping( painterHasClip );
	}
    }

    if ( underlinePositions != underlinePositionStack )
	delete [] underlinePositions;
}

/*!
    \overload

    Returns the bounding rectangle of the aligned text that would be
    printed with the corresponding drawText() function using the first
    \a len characters from \a str if \a len is > -1, or the whole of
    \a str if \a len is -1. The drawing, and hence the bounding
    rectangle, is constrained to the rectangle \a r, or to the
    rectangle required to draw the text, whichever is the larger.

    The \a internal parameter should not be used.

    \sa drawText(), fontMetrics(), QFontMetrics::boundingRect(), Qt::TextFlags
*/

QRect QPainter::boundingRect( const QRect &r, int flags,
			      const QString& str, int len )
{
    QRect brect;
    if ( str.isEmpty() )
	brect.setRect( r.x(),r.y(), 0,0 );
    else
	drawText( r, flags | DontPrint, str, len, &brect );
    return brect;
}

/*!
    \fn QRect QPainter::boundingRect( int x, int y, int w, int h, int flags, const QString&, int len = -1 );

    Returns the bounding rectangle of the aligned text that would be
    printed with the corresponding drawText() function using the first
    \a len characters of the string if \a len is > -1, or the whole of
    the string if \a len is -1. The drawing, and hence the bounding
    rectangle, is constrained to the rectangle that begins at point \a
    (x, y) with width \a w and hight \a h, or to the
    rectangle required to draw the text, whichever is the larger.

    The \a flags argument is
    the bitwise OR of the following flags:
    \table
    \header \i Flag \i Meaning
    \row \i \c AlignAuto \i aligns according to the language, usually left.
    \row \i \c AlignLeft \i aligns to the left border.
    \row \i \c AlignRight \i aligns to the right border.
    \row \i \c AlignHCenter \i aligns horizontally centered.
    \row \i \c AlignTop \i aligns to the top border.
    \row \i \c AlignBottom \i aligns to the bottom border.
    \row \i \c AlignVCenter \i aligns vertically centered.
    \row \i \c AlignCenter \i (== \c AlignHCenter | \c AlignVCenter).
    \row \i \c SingleLine \i ignores newline characters in the text.
    \row \i \c ExpandTabs \i expands tabs.
    \row \i \c ShowPrefix \i interprets "&x" as "<u>x</u>".
    \row \i \c WordBreak \i breaks the text to fit the rectangle.
    \endtable

    Horizontal alignment defaults to \c AlignLeft and vertical
    alignment defaults to \c AlignTop.

    If several of the horizontal or several of the vertical alignment flags
    are set, the resulting alignment is undefined.

    The \a intern parameter should not be used.

    \sa Qt::TextFlags
*/


struct QPaintDeviceRedirection
{
    QPaintDeviceRedirection():device(0), replacement(0){}
    QPaintDeviceRedirection(const QPaintDevice *device, const QPaintDevice *replacement,
			    const QPoint& offset)
	: device(device), replacement(replacement), offset(offset){}
    const QPaintDevice *device, *replacement;
    QPoint offset;
    bool operator==(const QPaintDevice *pdev) const { return device == pdev; }
    Q_DUMMY_COMPARISON_OPERATOR(QPaintDeviceRedirection)
};
static QList<QPaintDeviceRedirection> redirections;

/*!
  \internal

    Redirects all paint commands for a paint device, \a device, to
    another paint device, \a replacement. The optional point \a offset
    defines an offset within the replaced device. After painting you
    must call restoreRedirected().

    In general, you'll probably find calling QPixmap::grabWidget() or
    QPixmap::grabWindow() is an easier solution.

    \sa redirected()
*/
void QPainter::setRedirected(const QPaintDevice *device, const QPaintDevice *replacement,
			     const QPoint& offset)
{
    Q_ASSERT(device != 0);
    redirections.ensure_constructed();
    redirections += QPaintDeviceRedirection(device, replacement, offset);
}

/*!\internal

  Restores the previous redirection for \a device after a call to
  setRedirected().

  \sa redirected()
 */
void QPainter::restoreRedirected(const QPaintDevice *device)
{
    Q_ASSERT(device != 0);
    redirections.ensure_constructed();
    for (int i = redirections.size()-1; i >= 0; --i)
	if (redirections.at(i) == device ) {
	    redirections.removeAt(i);
	    return;
	}
}

/*!
    \internal

    Returns the replacement for \a device. The optional out parameter
    \a offset returns return the offset within the replaced device.
*/
const QPaintDevice *QPainter::redirected(const QPaintDevice *device, QPoint *offset)
{
    Q_ASSERT(device != 0);
    redirections.ensure_constructed();
    for (int i = redirections.size()-1; i >= 0; --i)
	if (redirections.at(i) == device ) {
	    if (offset)
		*offset = redirections.at(i).offset;
	    return redirections.at(i).replacement;
	}
    return 0;
}



/*****************************************************************************
  QPen member functions
 *****************************************************************************/

/*!
    \class QPen qpen.h
    \brief The QPen class defines how a QPainter should draw lines and outlines
    of shapes.

    \ingroup graphics
    \ingroup images
    \ingroup shared
    \mainclass

    A pen has a style, width, color, cap style and join style.

    The pen style defines the line type. The default pen style is \c
    Qt::SolidLine. Setting the style to \c NoPen tells the painter to
    not draw lines or outlines.

    When drawing 1 pixel wide diagonal lines you can either use a very
    fast algorithm (specified by a line width of 0, which is the
    default), or a slower but more accurate algorithm (specified by a
    line width of 1). For horizontal and vertical lines a line width
    of 0 is the same as a line width of 1. The cap and join style have
    no effect on 0-width lines.

    The pen color defines the color of lines and text. The default
    line color is black. The QColor documentation lists predefined
    colors.

    The cap style defines how the end points of lines are drawn. The
    join style defines how the joins between two lines are drawn when
    multiple connected lines are drawn (QPainter::drawPolyline()
    etc.). The cap and join styles only apply to wide lines, i.e. when
    the width is 1 or greater.

    Use the QBrush class to specify fill styles.

    Example:
    \code
    QPainter painter;
    QPen     pen( red, 2 );             // red solid line, 2 pixels wide
    painter.begin( &anyPaintDevice );   // paint something
    painter.setPen( pen );              // set the red, wide pen
    painter.drawRect( 40,30, 200,100 ); // draw a rectangle
    painter.setPen( blue );             // set blue pen, 0 pixel width
    painter.drawLine( 40,30, 240,130 ); // draw a diagonal in rectangle
    painter.end();                      // painting done
    \endcode

    See the \l Qt::PenStyle enum type for a complete list of pen
    styles.

    With reference to the end points of lines, for wide (non-0-width)
    pens it depends on the cap style whether the end point is drawn or
    not. QPainter will try to make sure that the end point is drawn
    for 0-width pens, but this cannot be absolutely guaranteed because
    the underlying drawing engine is free to use any (typically
    accelerated) algorithm for drawing 0-width lines. On all tested
    systems, however, the end point of at least all non-diagonal lines
    are drawn.

    A pen's color(), width(), style(), capStyle() and joinStyle() can
    be set in the constructor or later with setColor(), setWidth(),
    setStyle(), setCapStyle() and setJoinStyle(). Pens may also be
    compared and streamed.

    \img pen-styles.png Pen styles

    \sa QPainter, QPainter::setPen()
*/

/*!
  \internal
  Initializes the pen.
*/

void QPen::init(const QColor &color, int width, uint linestyle)
{
    d = new QPenData;
    d->ref = 1;
    d->style = (PenStyle)(linestyle & MPenStyle);
    d->width = width;
    d->color = color;
    d->linest = linestyle;
}

/*!
    Constructs a default black solid line pen with 0 width, which
    renders lines 1 pixel wide (fast diagonals).
*/

QPen::QPen()
{
    init(black, 0, SolidLine);
}

/*!
    Constructs a black pen with 0 width (fast diagonals) and style \a
    style.

    \sa setStyle()
*/

QPen::QPen(PenStyle style)
{
    init(black, 0, style);
}

/*!
    Constructs a pen with the specified \a color, \a width and \a
    style.

    \sa setWidth(), setStyle(), setColor()
*/

QPen::QPen(const QColor &color, int width, PenStyle style)
{
    init(color, width, style);
}

/*!
    Constructs a pen with the specified color \a cl and width \a
    width. The pen style is set to \a s, the pen cap style to \a c and
    the pen join style to \a j.

    A line width of 0 will produce a 1 pixel wide line using a fast
    algorithm for diagonals. A line width of 1 will also produce a 1
    pixel wide line, but uses a slower more accurate algorithm for
    diagonals. For horizontal and vertical lines a line width of 0 is
    the same as a line width of 1. The cap and join style have no
    effect on 0-width lines.

    \sa setWidth(), setStyle(), setColor()
*/

QPen::QPen(const QColor &cl, int width, PenStyle s, PenCapStyle c, PenJoinStyle j)
{
    init(cl, width, s | c | j);
}

/*!
    Constructs a pen that is a copy of \a p.
*/

QPen::QPen(const QPen &p)
{
    d = p.d;
    ++d->ref;
}

/*!
    Destroys the pen.
*/

QPen::~QPen()
{
    if (!--d->ref)
	delete d;
}

/*!
    \fn QPen::detach()
    Detaches from shared pen data to make sure that this pen is the
    only one referring the data.

    If multiple pens share common data, this pen dereferences the data
    and gets a copy of the data. Nothing is done if there is just a
    single reference.
*/

void QPen::detach_helper()
{
    QPenData *x = new QPenData;
    x->ref = 1;
    x->style = d->style;
    x->width = d->width;
    x->color = d->color;
    x->linest = d->linest;
    x = qAtomicSetPtr(&d, x);
    if (!--x->ref)
	delete x;
}


/*!
    Assigns \a p to this pen and returns a reference to this pen.
*/

QPen &QPen::operator=(const QPen &p)
{
    QPenData *x = p.d;
    ++x->ref;
    x = qAtomicSetPtr(&d, x);
    if (!--x->ref)
	delete x;
    return *this;
}


/*!
    \fn PenStyle QPen::style() const

    Returns the pen style.

    \sa setStyle()
*/

/*!
    Sets the pen style to \a s.

    See the \l Qt::PenStyle documentation for a list of all the
    styles.

    \warning On Windows 95/98 and Macintosh, the style setting (other
    than \c NoPen and \c SolidLine) has no effect for lines with width
    greater than 1.

    \sa style()
*/

void QPen::setStyle(PenStyle s)
{
    if (d->style == s)
	return;
    detach();
    d->style = s;
    d->linest = (d->linest & ~MPenStyle) | s;
}


/*!
    \fn int QPen::width() const

    Returns the pen width.

    \sa setWidth()
*/

/*!
    Sets the pen width to \a width.

    A line width of 0 will produce a 1 pixel wide line using a fast
    algorithm for diagonals. A line width of 1 will also produce a 1
    pixel wide line, but uses a slower more accurate algorithm for
    diagonals. For horizontal and vertical lines a line width of 0 is
    the same as a line width of 1. The cap and join style have no
    effect on 0-width lines.

    \sa width()
*/

void QPen::setWidth(int width)
{
    if (d->width == width)
	return;
    detach();
    d->width = width;
}


/*!
    Returns the pen's cap style.

    \sa setCapStyle()
*/
Qt::PenCapStyle QPen::capStyle() const
{
    return (PenCapStyle)(d->linest & MPenCapStyle);
}

/*!
    Sets the pen's cap style to \a c.

    The default value is \c FlatCap. The cap style has no effect on
    0-width pens.

    \img pen-cap-styles.png Pen Cap Styles

    \warning On Windows 95/98 and Macintosh, the cap style setting has
    no effect. Wide lines are rendered as if the cap style was \c
    SquareCap.

    \sa capStyle()
*/

void QPen::setCapStyle(PenCapStyle c)
{
    if ((d->linest & MPenCapStyle) == c)
	return;
    detach();
    d->linest = (d->linest & ~MPenCapStyle) | c;
}

/*!
    Returns the pen's join style.

    \sa setJoinStyle()
*/
Qt::PenJoinStyle QPen::joinStyle() const
{
    return (PenJoinStyle)(d->linest & MPenJoinStyle);
}

/*!
    Sets the pen's join style to \a j.

    The default value is \c MiterJoin. The join style has no effect on
    0-width pens.

    \img pen-join-styles.png Pen Join Styles

    \warning On Windows 95/98 and Macintosh, the join style setting
    has no effect. Wide lines are rendered as if the join style was \c
    BevelJoin.

    \sa joinStyle()
*/

void QPen::setJoinStyle(PenJoinStyle j)
{
    if ((d->linest & MPenJoinStyle) == j)
	return;
    detach();
    d->linest = (d->linest & ~MPenJoinStyle) | j;
}

/*!
    \fn const QColor &QPen::color() const

    Returns the pen color.

    \sa setColor()
*/

/*!
    Sets the pen color to \a c.

    \sa color()
*/

void QPen::setColor(const QColor &c)
{
    detach();
    d->color = c;
}


/*!
    \fn bool QPen::operator!=( const QPen &p ) const

    Returns TRUE if the pen is different from \a p; otherwise returns
    FALSE.

    Two pens are different if they have different styles, widths or
    colors.

    \sa operator==()
*/

/*!
    Returns TRUE if the pen is equal to \a p; otherwise returns FALSE.

    Two pens are equal if they have equal styles, widths and colors.

    \sa operator!=()
*/

bool QPen::operator==(const QPen &p) const
{
    return (p.d == d) || (p.d->linest == d->linest && p.d->width == d->width
			  && p.d->color == d->color);
}


/*****************************************************************************
  QPen stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
/*!
    \relates QPen

    Writes the pen \a p to the stream \a s and returns a reference to
    the stream.

    \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator<<(QDataStream &s, const QPen &p)
{
    if (s.version() < 3)
	s << (Q_UINT8)p.style();
    else
	s << (Q_UINT8)(p.style() | p.capStyle() | p.joinStyle());
    if (s.version() < 6)
	s << (Q_UINT8)p.width();
    else
	s << (Q_INT16)p.width();
    return s << p.color();
}

/*!
    \relates QPen

    Reads a pen from the stream \a s into \a p and returns a reference
    to the stream.

    \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator>>(QDataStream &s, QPen &p)
{
    Q_UINT8 style;
    Q_UINT8 width8 = 0;
    Q_INT16 width = 0;
    QColor color;
    s >> style;
    if (s.version() < 6)
	s >> width8;
    else
	s >> width;
    s >> color;
    p = QPen(color, width8 | width, (Qt::PenStyle)style);
    return s;
}
#endif //QT_NO_DATASTREAM

/*****************************************************************************
  QBrush member functions
 *****************************************************************************/

/*!
    \class QBrush qbrush.h

    \brief The QBrush class defines the fill pattern of shapes drawn by a QPainter.

    \ingroup graphics
    \ingroup images
    \ingroup shared

    A brush has a style and a color. One of the brush styles is a
    custom pattern, which is defined by a QPixmap.

    The brush style defines the fill pattern. The default brush style
    is \c NoBrush (depending on how you construct a brush). This style
    tells the painter to not fill shapes. The standard style for
    filling is \c SolidPattern.

    The brush color defines the color of the fill pattern. The QColor
    documentation lists the predefined colors.

    Use the QPen class for specifying line/outline styles.

    Example:
    \code
        QPainter painter;
        QBrush   brush( yellow );           // yellow solid pattern
        painter.begin( &anyPaintDevice );   // paint something
        painter.setBrush( brush );          // set the yellow brush
        painter.setPen( NoPen );            // do not draw outline
        painter.drawRect( 40,30, 200,100 ); // draw filled rectangle
        painter.setBrush( NoBrush );        // do not fill
        painter.setPen( black );            // set black pen, 0 pixel width
        painter.drawRect( 10,10, 30,20 );   // draw rectangle outline
        painter.end();                      // painting done
    \endcode

    See the setStyle() function for a complete list of brush styles.

    \img brush-styles.png Brush Styles

    \sa QPainter, QPainter::setBrush(), QPainter::setBrushOrigin()
*/


QBrush::QBrushData *QBrush::shared_default = 0;

/*! \internal Initializes the brush. */
void QBrush::init(const QColor &color, BrushStyle style)
{
    d = new QBrushData;
    d->ref = 1;
    d->style = style;
    d->color = color;
    d->pixmap = 0;
}

/*!
    Constructs a default black brush with the style \c NoBrush (will
    not fill shapes).
*/

QBrush::QBrush()
{
    if ( !shared_default ) {
	static QCleanupHandler<QBrush::QBrushData> shared_default_cleanup;
	shared_default = new QBrushData;
	shared_default->ref = 1;
	shared_default->style = (BrushStyle)0;
	shared_default->color = black;
	shared_default->pixmap = 0;
	shared_default_cleanup.add(&shared_default);
    }
    d = shared_default;
    ++d->ref;
}

/*!
    Constructs a brush with a black color and a pixmap set to \a pixmap.
*/

QBrush::QBrush(const QPixmap &pixmap)
{
// ## if pixmap was image, we could pick a nice color rather than
// assuming black.
    init(Qt::black, CustomPattern);
    setPixmap(pixmap);
}

/*!
    Constructs a black brush with the style \a style.

    \sa setStyle()
*/

QBrush::QBrush(BrushStyle style)
{
    init(black, style);
}

/*!
    Constructs a brush with the color \a color and the style \a style.

    \sa setColor(), setStyle()
*/

QBrush::QBrush(const QColor &color, BrushStyle style)
{
    init(color, style);
}

/*! \overload
    Constructs a brush with the color \a color and the style \a style.

    \sa setColor(), setStyle()
*/
QBrush::QBrush(Qt::GlobalColor color, BrushStyle style)
{
    init(color, style);
}

/*!
    Constructs a brush with the color \a color and a custom pattern
    stored in \a pixmap.

    The color will only have an effect for monochrome pixmaps, i.e.
    for QPixmap::depth() == 1.

    Pixmap brushes are currently not supported when printing on X11.

    \sa setColor(), setPixmap()
*/

QBrush::QBrush(const QColor &color, const QPixmap &pixmap)
{
    init(color, CustomPattern);
    setPixmap(pixmap);
}

/*! \overload
    Constructs a brush with the color \a color and a custom pattern
    stored in \a pixmap.

    The color will only have an effect for monochrome pixmaps, i.e.
    for QPixmap::depth() == 1.

    Pixmap brushes are currently not supported when printing on X11.

    \sa setColor(), setPixmap()
*/
QBrush::QBrush(Qt::GlobalColor color, const QPixmap &pixmap)
{
    init(color, CustomPattern);
    setPixmap(pixmap);
}

/*!
    Constructs a brush that is a \link shclass.html shallow
    copy\endlink of \a b.
*/

QBrush::QBrush( const QBrush &b )
{
    d = b.d;
    ++d->ref;
}

/*!
    Destroys the brush.
*/

QBrush::~QBrush()
{
    if (!--d->ref)
	cleanUp(d);
}

void QBrush::cleanUp(QBrush::QBrushData *x)
{
    delete x->pixmap;
    delete x;
}


void QBrush::detach_helper()
{
    QBrushData *x = new QBrushData;
    x->ref = 1;
    x->style = d->style;
    x->color = d->color;
    if (d->pixmap)
	x->pixmap = new QPixmap(*d->pixmap);
    else
	x->pixmap = 0;
    x = qAtomicSetPtr(&d, x);
    if (!--x->ref)
	cleanUp(x);
}


/*!
    Assigns \a b to this brush and returns a reference to this brush.
*/

QBrush &QBrush::operator=(const QBrush &b)
{
    QBrushData *x = b.d;
    ++x->ref;
    x = qAtomicSetPtr(&d, x);
    if (!--x->ref)
	cleanUp(x);
    return *this;
}


/*!
    \fn BrushStyle QBrush::style() const

    Returns the brush style.

    \sa setStyle()
*/

/*!
    Sets the brush style to \a s.

    The brush styles are:
    \table
    \header \i Pattern \i Meaning
    \row \i NoBrush \i will not fill shapes (default).
    \row \i SolidPattern  \i solid (100%) fill pattern.
    \row \i Dense1Pattern \i11 94% fill pattern.
    \row \i Dense2Pattern \i11 88% fill pattern.
    \row \i Dense3Pattern \i11 63% fill pattern.
    \row \i Dense4Pattern \i11 50% fill pattern.
    \row \i Dense5Pattern \i11 37% fill pattern.
    \row \i Dense6Pattern \i11 12% fill pattern.
    \row \i Dense7Pattern \i11 6% fill pattern.
    \row \i HorPattern \i horizontal lines pattern.
    \row \i VerPattern \i vertical lines pattern.
    \row \i CrossPattern \i crossing lines pattern.
    \row \i BDiagPattern \i diagonal lines (directed /) pattern.
    \row \i FDiagPattern \i diagonal lines (directed \) pattern.
    \row \i DiagCrossPattern \i diagonal crossing lines pattern.
    \row \i CustomPattern \i set when a pixmap pattern is being used.
    \endtable

    On Windows, dense and custom patterns cannot be transparent.

    See the \link #details Detailed Description\endlink for a picture
    of all the styles.

    \sa style()
*/

void QBrush::setStyle(BrushStyle s)
{
    if (d->style == s)
	return;
    if (s == CustomPattern)
	qWarning( "QBrush::setStyle: CustomPattern is for internal use" );
    detach();
    d->style = s;
}


/*!
    \fn const QColor &QBrush::color() const

    Returns the brush color.

    \sa setColor()
*/

/*!
    Sets the brush color to \a c.

    \sa color(), setStyle()
*/

void QBrush::setColor(const QColor &c)
{
    detach();
    d->color = c;
}


/*!
    \fn QPixmap *QBrush::pixmap() const

    Returns a pointer to the custom brush pattern, or 0 if no custom
    brush pattern has been set.

    \sa setPixmap()
*/

/*!
    Sets the brush pixmap to \a pixmap. The style is set to \c
    CustomPattern.

    The current brush color will only have an effect for monochrome
    pixmaps, i.e. for QPixmap::depth() == 1.

    Pixmap brushes are currently not supported when printing on X11.

    \sa pixmap(), color()
*/

void QBrush::setPixmap(const QPixmap &pixmap)
{
    detach();
    if (d->pixmap)
	delete d->pixmap;
    if (pixmap.isNull()) {
	d->style  = NoBrush;
	d->pixmap = 0;
    } else {
	d->style = CustomPattern;
	d->pixmap = new QPixmap(pixmap);
	if (d->pixmap->optimization() == QPixmap::MemoryOptim)
	    d->pixmap->setOptimization(QPixmap::NormalOptim);
    }
}


/*!
    \fn bool QBrush::operator!=( const QBrush &b ) const

    Returns TRUE if the brush is different from \a b; otherwise
    returns FALSE.

    Two brushes are different if they have different styles, colors or
    pixmaps.

    \sa operator==()
*/

/*!
    Returns TRUE if the brush is equal to \a b; otherwise returns
    FALSE.

    Two brushes are equal if they have equal styles, colors and
    pixmaps.

    \sa operator!=()
*/

bool QBrush::operator==(const QBrush &b) const
{
    return (b.d == d)
	   || (b.d->style == d->style && b.d->color == d->color && b.d->pixmap == d->pixmap);
}


/*!
    \fn inline double QPainter::translationX() const
    \internal
*/

/*!
    \fn inline double QPainter::translationY() const
    \internal
*/


/*****************************************************************************
  QBrush stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
/*!
    \relates QBrush

    Writes the brush \a b to the stream \a s and returns a reference
    to the stream.

    \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator<<(QDataStream &s, const QBrush &b)
{
    s << (Q_UINT8)b.style() << b.color();
    if (b.style() == Qt::CustomPattern)
#ifndef QT_NO_IMAGEIO
	s << *b.pixmap();
#else
	qWarning("No Image Brush I/O");
#endif
    return s;
}

/*!
    \relates QBrush

    Reads the brush \a b from the stream \a s and returns a reference
    to the stream.

    \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator>>(QDataStream &s, QBrush &b)
{
    Q_UINT8 style;
    QColor color;
    s >> style;
    s >> color;
    if (style == Qt::CustomPattern) {
#ifndef QT_NO_IMAGEIO
	QPixmap pm;
	s >> pm;
	b = QBrush(color, pm);
#else
	qWarning("No Image Brush I/O");
#endif
    } else
	b = QBrush(color, (Qt::BrushStyle)style);
    return s;
}
#endif // QT_NO_DATASTREAM

