/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qwidget_x11.cpp#56 $
**
** Implementation of QWidget and QView classes for X11
**
** Author  : Haavard Nord
** Created : 931031
**
** Copyright (C) 1993-1995 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#include "qview.h"
#include "qpalette.h"
#include "qapp.h"
#include "qpaintdc.h"
#include "qpainter.h"
#include "qpixmap.h"
#include "qwidcoll.h"
#include "qobjcoll.h"
#define	 GC GC_QQQ
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#if defined(DEBUG)
static char ident[] = "$Id: //depot/qt/main/src/kernel/qwidget_x11.cpp#56 $";
#endif


void qt_enter_modal( QWidget * );		// defined in qapp_x11.cpp
void qt_leave_modal( QWidget * );		// --- "" ---
bool qt_modal_state();				// --- "" ---
void qt_open_popup( QWidget * );		// --- "" ---
void qt_close_popup( QWidget * );		// --- "" ---


// --------------------------------------------------------------------------
// QWidget member functions
//

extern Atom q_wm_delete_window;			// defined in qapp_x11.cpp

const ulong stdWidgetEventMask =		// X event mask
	KeyPressMask | KeyReleaseMask |
	ButtonPressMask | ButtonReleaseMask |
	KeymapStateMask |
	ButtonMotionMask |
	EnterWindowMask | LeaveWindowMask |
	FocusChangeMask |
	ExposureMask |
	StructureNotifyMask | SubstructureRedirectMask;


bool QWidget::create()				// create widget
{
    if ( testFlag(WState_Created) )		// already created
	return FALSE;
    setFlag( WState_Created );			// set created flag

    if ( !parentWidget() )
	setFlag( WType_Overlap );		// overlapping widget

    int	   screen = qt_xscreen();		// X11 screen
    int	   sw = DisplayWidth( dpy, screen );	// screen width
    int	   sh = DisplayHeight( dpy, screen );	// screen height
    bool   overlap = testFlag( WType_Overlap );
    bool   popup   = testFlag( WType_Popup );
    bool   modal   = testFlag( WType_Modal );
    bool   desktop = testFlag( WType_Desktop );
    Window parentwin;
    int	   border = 0;
    WId	   id;

    bg_col = pal.normal().background();		// set default background color

    if ( modal ) {				// modal windows overlap
	overlap = TRUE;
	setFlag( WType_Overlap );
    }

    if ( desktop ) {				// desktop widget
	frect.setRect( 0, 0, sw, sh );
	overlap = popup = FALSE;		// force these flags off
    }
    else if ( overlap || popup )		// parentless widget
	frect.setRect( sw/2 - sw/4, sh/2 - sh/5, sw/2, 2*sh/5 );
    else					// child widget
	frect.setRect( 10, 10, 100, 30 );
    crect = frect;				// default client rect

    if ( overlap || popup || desktop )		// overlapping widget
	parentwin = RootWindow( dpy, screen );
    else {					// child widget
	parentwin = parentWidget()->id();
	if ( testFlag(WStyle_Border) )		// has a border
	    border = 1;
    }

    if ( desktop ) {				// desktop widget
	id = parentwin;				// id = root window
	QWidget *otherDesktop = find( id );	// is there another desktop?
	if ( otherDesktop && otherDesktop->testFlag(WPaintDesktop) ) {
	    otherDesktop->set_id( 0 );		// remove id from widget mapper
	    set_id( id );			// make sure otherDesktop is
	    otherDesktop->set_id( id );		//   found first
	}
	else
	    set_id( id );
    }
    else {
	id = XCreateSimpleWindow( dpy, parentwin,
				  frect.left(), frect.top(),
				  frect.width(), frect.height(),
				  border,
				  black.pixel(),
				  bg_col.pixel() );
	set_id( id );				// set widget id/handle + hd
    }

    if ( popup ) {				// popup widget
	XSetTransientForHint( dpy, parentwin, id );
	XSetWindowAttributes v;
	v.override_redirect = TRUE;
	v.save_under = TRUE;
	XChangeWindowAttributes( dpy, id,
				 CWOverrideRedirect | CWSaveUnder,
				 &v );
    }
    else if ( overlap ) {			// top level widget
	if ( modal ) {
	    if ( parentWidget() )		// modal to one widget
		XSetTransientForHint( dpy, id, parentWidget()->id() );
	    else				// application-modal
		XSetTransientForHint( dpy, id, qt_xrootwin() );
	}
	XSizeHints size_hints;
	size_hints.flags = PPosition | PSize | PWinGravity;
	size_hints.x = crect.left();
	size_hints.y = crect.top();
	size_hints.width = crect.width();
	size_hints.height = crect.height();
	size_hints.win_gravity = 1;		// NortWest
	char *title = qAppName();
	XWMHints wm_hints;			// window manager hints
	wm_hints.input = True;
	wm_hints.initial_state = NormalState;
	wm_hints.flags = InputHint | StateHint;
	XClassHint class_hint;
	class_hint.res_name = title;		// app name and widget name
	class_hint.res_class = name() ? (char *)name() : title;
	XSetWMProperties( dpy, id, 0, 0, 0, 0, &size_hints, &wm_hints,
			  &class_hint );
	XStoreName( dpy, id, title );
	Atom protocols[1];
	protocols[0] = q_wm_delete_window;	// support del window protocol
	XSetWMProtocols( dpy, id, protocols, 1 );
    }
    if ( testFlag(WResizeNoErase) ) {
	XSetWindowAttributes v;
	v.bit_gravity = NorthWestGravity;	// don't erase when resizing
	XChangeWindowAttributes( dpy, id,
				 CWBitGravity,
				 &v );
    }
    setMouseTracking( FALSE );			// also sets event mask
    if ( overlap ) {				// set X cursor
	QCursor *appc = QApplication::cursor();
	XDefineCursor( dpy, ident, appc ? appc->handle() : curs.handle() );
	setFlag( WCursorSet );
    }
    return TRUE;
}


bool QWidget::destroy()				// destroy widget
{
    if ( this == activeWidget )
	activeWidget = 0;
    if ( testFlag(WState_Created) ) {
	clearFlag( WState_Created );
	if ( children() ) {
	    QObjectListIt it(*children());
	    register QObject *object;
	    while ( it ) {			// destroy all widget children
		object = it.current();
		if ( object->isWidgetType() )
		    ((QWidget*)object)->destroy();
		++it;
	    }
	}
	if ( testFlag(WType_Modal) )		// just be sure we leave modal
	    qt_leave_modal( this );
	else if ( testFlag(WType_Popup) )
	    qt_close_popup( this );
	if ( !testFlag(WType_Desktop) )
	    XDestroyWindow( dpy, ident );
	set_id( 0 );
	emit destroyed();			// send out destroyed signal
    }
    return TRUE;
}


void QWidget::recreate( QWidget *parent, WFlags f, const QPoint &p,
			bool showIt )
{
    extern void qPRCreate( const QWidget *, Window );
    WId old_ident = ident;
    if ( testFlag(WType_Desktop) )
	old_ident = 0;
    set_id( 0 );
    if ( parentObj )				// remove from parent
	parentObj->removeChild( this );
    if ( (parentObj = parent) )
	parentObj->insertChild( this );
    bool was_disabled = isDisabled();
    QSize s = size();				// save size
    QColor bgc = bg_col;			// save colors
    flags = f;
    clearFlag( WState_Created );
    clearFlag( WState_Visible );
    create();
    const QObjectList *chlist = children();
    if ( chlist ) {				// reparent children
	QObjectListIt it( *chlist );
	QObject *obj;
	while ( (obj=it.current()) ) {
	    if ( obj->isWidgetType() ) {
		QWidget *w = (QWidget *)obj;
		XReparentWindow( dpy, w->id(), id(), w->geometry().x(),
				 w->geometry().y() );
	    }
	    ++it;
	}
    }
    qPRCreate( this, old_ident );
    setBackgroundColor( bgc );			// restore colors
    setGeometry( p.x(), p.y(), s.width(), s.height() );
    if ( was_disabled )
	disable();
    if ( showIt )
	show();
    if ( old_ident )
	XDestroyWindow( dpy, old_ident );
}


bool QWidget::setMouseTracking( bool enable )
{
    bool v = testFlag( WMouseTracking );
    ulong m;
    if ( enable ) {
	m = PointerMotionMask;
	setFlag( WMouseTracking );
    }
    else {
	m = 0;
	clearFlag( WMouseTracking );
    }
    if ( testFlag(WType_Desktop) ) {		// desktop widget?
	if ( testFlag(WPaintDesktop) )		// get desktop paint events
	    XSelectInput( dpy, ident, ExposureMask );
    }
    else
	XSelectInput( dpy, ident,		// specify events
		      m | stdWidgetEventMask );
    return v;
}

/*! Returns the backround color of this widget.  The background color
  is independent of the color group.  \sa colorGroup(), QColor,
  setBackgroundColor() and foregroundColor(). */

QColor QWidget::backgroundColor() const		// get background color
{
    return bg_col;
}

/*! Returns the foreground color of this widget.  The foreground color
  depends on the color group of the widget.  \sa colorGroup(), QColor
  and backgroundColor(). */
QColor QWidget::foregroundColor() const		// get foreground color
{
    return colorGroup().foreground();
}

/*! Sets the background color of this widget.  The background color is
  independent of the widget's color group.  \sa QColor and
  backgroundColor(). */
void QWidget::setBackgroundColor( const QColor &c )
{						// set background color
    bg_col = c;
    XSetWindowBackground( dpy, ident, bg_col.pixel() );
    update();
}

void QWidget::setBackgroundPixmap( const QPixmap &pm )
{
    if ( !pm.isNull() )
	XSetWindowBackgroundPixmap( dpy, ident, pm.handle() );
}


/*! Returns the font the widget is currently using (wanting to use,
  rather, QFontInfo will tell you what's actually on the screen).

  \sa QFont, setFont(), fontMetrics() and fontInfo(). */

QFont &QWidget::font()
{
    return fnt;
}

/*! Sets the font of the widget.  You may not get the exact font you
  specify; use fontInfo() to see what you got, if you can do anything
  about it.  This code fragment switches to a bold version of whatever
  font is being used:

  \code
  QFont f = font();
  f.setWeight( QFont::Bold );
  setFont(f);
  \endcode

  Here's how to copy the font from the parent widget:

  \code
  if (parentWidget)
      setFont( parentWidget->font() );
  \endcode

  \sa font() and fontInfo(). */

void QWidget::setFont( const QFont &font )	// set font
{
    fnt = font;
    update();
}


/*! Returns the cursor in use; the QCursor class lists the
  cursors. \sa QCursor and setcursor().*/
QCursor QWidget::cursor() const			// get cursor
{
    return curs;
}

/*! Set the cursor shape.  The mouse cursor will assume this shape
  when it's over this widget.  The available shapes are listed in the
  QCursor documentation.

  \code
  setcursor( QCursor::hourGlassCursor );
  \endcode

  \sa QCursor, cursor().  */
void QWidget::setCursor( const QCursor &cursor )// set cursor
{
    curs = cursor;
    QCursor *appc = QApplication::cursor();
    XDefineCursor( dpy, ident, appc ? appc->handle() : curs.handle() );
    setFlag( WCursorSet );
    XFlush( dpy );
}


void QWidget::grabMouse()
{
    if ( !testFlag(WState_MGrab) ) {
	setFlag( WState_MGrab );
	XGrabPointer( dpy, ident, TRUE,
		      ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
		      EnterWindowMask | LeaveWindowMask,
		      GrabModeAsync, GrabModeAsync,
		      None, None, CurrentTime );
    }
}

void QWidget::grabMouse( const QCursor &cursor )
{
    if ( !testFlag(WState_MGrab) ) {
	setFlag( WState_MGrab );
	XGrabPointer( dpy, ident, TRUE,
		      ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
		      EnterWindowMask | LeaveWindowMask,
		      GrabModeAsync, GrabModeAsync,
		      None, cursor.handle(), CurrentTime );
    }
}

void QWidget::releaseMouse()
{
    if ( testFlag(WState_MGrab) ) {
	clearFlag( WState_MGrab );
	XUngrabPointer( dpy, CurrentTime );
    }
}


void QWidget::grabKeyboard()
{
    if ( !testFlag(WState_KGrab) ) {
	setFlag( WState_KGrab );
	XGrabKeyboard( dpy, ident, TRUE, GrabModeSync, GrabModeSync,
		       CurrentTime );
    }
}

void QWidget::releaseKeyboard()
{
    if ( testFlag(WState_KGrab) ) {
	clearFlag( WState_KGrab );
	XUngrabKeyboard( dpy, CurrentTime );
    }
}


void QWidget::setFocus()			// set keyboard focus
{
/*
    if ( activeWidget == this ) {		// is active widget
	if ( !testFlag(WState_FocusA) ) {
	    QFocusEvent e( Event_FocusOut );
	    if ( QApplication::sendEvent( this, &e ) )
		setFlag( (WState_FocusA | WState_FocusP) );
	}
	return;
    }
    else if ( activeWidget ) {			// send focus-out
	activeWidget->clearFlag( WState_FocusA );
	if ( activeWidget->parent && activeWidget->parent == parent )
	    activeWidget->clearFlag( WState_FocusP );
	QFocusEvent e( Event_FocusOut );
	QApplication::sendEvent( activeWidget, &e );
    }
    setFlag( WState_FocusA );
    activeWidget = this;
*/
}

QWidget *QWidget::widgetInFocus()		// get focus widget
{
    return activeWidget;
}


bool QWidget::enableUpdates( bool enable )	// enable widget update/repaint
{
    bool last = !testFlag( WNoUpdates );
    if ( enable )
	clearFlag( WNoUpdates );
    else
	setFlag( WNoUpdates );
    return last;
}

void QWidget::update()				// update widget
{
    if ( !testFlag(WNoUpdates) )
	XClearArea( dpy, ident, 0, 0, 0, 0, TRUE );
}

void QWidget::update( int x, int y, int w, int h )
{						// update part of widget
    if ( !testFlag(WNoUpdates) )
	XClearArea( dpy, ident, x, y, w, h, TRUE );
}

void QWidget::repaint( const QRect &r, bool eraseArea )
{
    if ( !isVisible() || testFlag(WNoUpdates) )	// ignore repaint
	return;
    QPaintEvent e( r );				// send fake paint event
    if ( eraseArea )
	XClearArea( dpy, ident, r.x(), r.y(), r.width(), r.height(), FALSE );
    QApplication::sendEvent( this, &e );
}


void QWidget::show()				// show widget
{
    if ( testFlag(WState_Visible) )
	return;
    if ( children() ) {
	QObjectListIt it(*children());
	register QObject *object;
	QWidget *widget;
	while ( it ) {				// show all widget children
	    object = it.current();		//   (except popups)
	    if ( object->isWidgetType() ) {
		widget = (QWidget*)object;
		if ( !widget->testFlag(WExplicitHide) )
		    widget->show();
	    }
	    ++it;
	}
    }
    XMapWindow( dpy, ident );
    setFlag( WState_Visible );
    clearFlag( WExplicitHide );
    if ( testFlag(WType_Modal) )
	qt_enter_modal( this );
    else if ( testFlag(WType_Popup) )
	qt_open_popup( this );
}

void QWidget::hide()				// hide widget
{
    if ( !testFlag(WState_Visible) ) {		// not visible
	setFlag( WExplicitHide );
	return;
    }
    if ( testFlag(WType_Modal) )
	qt_leave_modal( this );
    else if ( testFlag(WType_Popup) )
	qt_close_popup( this );
    XUnmapWindow( dpy, ident );
    clearFlag( WState_Visible );
}


void QWidget::raise()				// raise widget
{
    XRaiseWindow( dpy, ident );
}

void QWidget::lower()				// lower widget
{
    XLowerWindow( dpy, ident );
}


static void do_size_hints( Display *dpy, WId ident, QWExtra *x, XSizeHints *s )
{
    if ( x ) {
	if ( x->minw >= 0 && x->minh >= 0 ) {	// add minimum size hints
	    s->flags |= PMinSize;
	    s->min_width = x->minw;
	    s->min_height = x->minh;
	}
	if ( x->maxw >= 0 && x->maxw >= 0 ) {	// add maximum size hints
	    s->flags |= PMaxSize;
	    s->max_width = x->maxw;
	    s->max_height = x->maxh;
	}
	if ( x->incw >= 0 && x->inch >= 0 ) {	// add resize increment hints
	    s->flags |= PResizeInc | PBaseSize;
	    s->width_inc = x->incw;
	    s->height_inc = x->inch;
	    s->base_width = 0;
	    s->base_height = 0;
	}
    }
    s->flags |= PWinGravity;
    s->win_gravity = 1;				// NorthWest
    XSetNormalHints( dpy, ident, s );
}

void QWidget::move( int x, int y )		// move widget
{
    QPoint p(x,y);
    QRect r = frect;
    if ( testFlag(WType_Desktop) )
	return;
    r.setTopLeft( p );
    setFRect( r );
    if ( testFlag(WType_Overlap) ) {
	XSizeHints size_hints;			// tell window manager
	size_hints.flags = PPosition;
	size_hints.x = x;
	size_hints.y = y;
	do_size_hints( dpy, ident, extra, &size_hints );
    }
    XMoveWindow( dpy, ident, x, y );
    QMoveEvent e( r.topLeft() );
    QApplication::sendEvent( this, &e );	// send move event immediatly
}

void QWidget::resize( int w, int h )		// resize widget
{
    if ( w < 1 )				// invalid size
	w = 1;
    if ( h < 1 )
	h = 1;
    QRect r = crect;
    QSize s(w,h);
    if ( testFlag(WType_Desktop) )
	return;
    r.setSize( s );
    setCRect( r );
    if ( testFlag(WType_Overlap) ) {
	XSizeHints size_hints;			// tell window manager
	size_hints.flags = PSize;
	size_hints.width = w;
	size_hints.height = h;
	do_size_hints( dpy, ident, extra, &size_hints );
    }
    XResizeWindow( dpy, ident, w, h );
    QResizeEvent e( s );
    QApplication::sendEvent( this, &e );	// send resize event immediatly
}

void QWidget::setGeometry( int x, int y, int w, int h )
{						// move and resize widget
    if ( w < 1 )				// invalid size
	w = 1;
    if ( h < 1 )
	h = 1;
    QRect  r( x, y, w, h );
    if ( testFlag(WType_Desktop) )
	return;
    setCRect( r );
    if ( testFlag(WType_Overlap) ) {
	XSizeHints size_hints;			// tell window manager
	size_hints.flags = USPosition | USSize;
	size_hints.x = x;
	size_hints.y = y;
	size_hints.width = w;
	size_hints.height = h;
	do_size_hints( dpy, ident, extra, &size_hints );
    }
    XMoveResizeWindow( dpy, ident, x, y, w, h );
    QResizeEvent e1( r.size() );
    QApplication::sendEvent( this, &e1 );	// send resize event
    QMoveEvent e2( r.topLeft() );
    QApplication::sendEvent( this, &e2 );	// send move event
}

/*! Sets the minimum size of the widget to \e w pixels wide and \e h
  pixels high.  The user will not be able to resize the widget to a
  smaller size.  \sa setMaximumSize(), setSizeIncrement(), size() and
  geometry(). */

void QWidget::setMinimumSize( int w, int h )	// set minimum size
{
    if ( testFlag(WType_Overlap) ) {
	createExtra();
	extra->minw = w;
	extra->minh = h;
	XSizeHints size_hints;
	size_hints.flags = 0;
	do_size_hints( dpy, ident, extra, &size_hints );
    }
}

/*! Sets the maximum size of the widget to \e w pixels wide and \e h
  pixels high.  The user will not be able to resize the widget to a
  larger size.  \sa setMinimumSize(), setSizeIncrement(), size() and
  geometry(). */

void QWidget::setMaximumSize( int w, int h )	// set maximum size
{
    if ( testFlag(WType_Overlap) ) {
	createExtra();
	extra->maxw = w;
	extra->maxh = h;
	XSizeHints size_hints;
	size_hints.flags = 0;
	do_size_hints( dpy, ident, extra, &size_hints );
    }
}

/*! Sets the size increment of the widget.  When the user resizes the
  widget, the size will move in steps of \e w horizontally and \e h
  vertically.  Both default to 1. \sa setMaximumSize() and
  setSizeIncrement(). */

void QWidget::setSizeIncrement( int w, int h )
{						// set size increment
    if ( testFlag(WType_Overlap) ) {
	createExtra();
	extra->incw = w;
	extra->inch = h;
	XSizeHints size_hints;
	size_hints.flags = 0;
	do_size_hints( dpy, ident, extra, &size_hints );
    }
}


void QWidget::erase()				// erase widget contents
{
    XClearArea( dpy, ident, 0, 0, 0, 0, FALSE );
}

void QWidget::scroll( int dx, int dy )		// scroll widget contents
{
    QSize sz = size();
    int x1, y1, x2, y2, w=sz.width(), h=sz.height();
    if ( dx > 0 ) {
	x1 = 0;
	x2 = dx;
	w -= dx;
    }
    else {
	x1 = -dx;
	x2 = 0;
	w += dx;
    }
    if ( dy > 0 ) {
	y1 = 0;
	y2 = dy;
	h -= dy;
    }
    else {
	y1 = -dy;
	y2 = 0;
	h += dy;
    }
    XCopyArea( dpy, ident, ident, qt_xget_readonly_gc(), x1, y1, w, h, x2, y2);
    if ( children() ) {				// scroll children
	QPoint pd( dx, dy );
	QObjectListIt it(*children());
	register QObject *object;
	while ( it ) {				// move all children
	    object = it.current();
	    if ( object->isWidgetType() ) {
		QWidget *w = (QWidget *)object;
		w->move( w->geometry().topLeft()+pd );
	    }
	    ++it;
	}
    }
    if ( dx ) {
	x1 = x2 == 0 ? w : 0;
	XClearArea( dpy, ident, x1, 0, sz.width()-w, sz.height(), TRUE );
    }
    if ( dy ) {
	y1 = y2 == 0 ? h : 0;
	XClearArea( dpy, ident, 0, y1, sz.width(), sz.height()-h, TRUE );
    }
}


void QWidget::drawText( int x, int y, const char *str )
{						// draw text in widget
    if ( testFlag( WState_Visible ) ) {
	QPainter paint;
	paint.begin( this );
	paint.drawText( x, y, str );
	paint.end();
    }
}


long QWidget::metric( int m ) const		// get metric information
{
    long val;
    if ( m == PDM_WIDTH || m == PDM_HEIGHT ) {
	if ( m == PDM_WIDTH )
	    val = crect.width();
	else
	    val = crect.height();
    }
    else {
	int scr = qt_xscreen();
	switch ( m ) {
	    case PDM_WIDTHMM:
	        val = ((long)DisplayWidthMM(dpy,scr)*crect.width())/
		      DisplayWidth(dpy,scr);
		break;
	    case PDM_HEIGHTMM:
	        val = ((long)DisplayHeightMM(dpy,scr)*crect.height())/
		      DisplayHeight(dpy,scr);
		break;
	    case PDM_NUMCOLORS:
		val = DisplayCells(dpy,scr);
		break;
	    case PDM_NUMPLANES:
		val = DisplayPlanes(dpy,scr);
		break;
	    default:
		val = 0;
#if defined(CHECK_RANGE)
		warning( "QWidget::metric: Invalid metric command" );
#endif
	}
    }
    return val;
}


// --------------------------------------------------------------------------
// QView member functions
//

void QView::setCaption( const char *s )		// set caption text
{
    ctext = s;
    XStoreName( dpy, id(), (const char *)ctext );
}

void QView::setIconText( const char *s )	// set icon text
{
    itext = s;
    XSetIconName( dpy, id(), (const char *)itext );
}

void QView::setIcon( QPixmap *pm )		// set icon pixmap
{
    if ( ipm != pm ) {
	delete ipm;
	ipm = pm;
    }
    XWMHints wm_hints;				// window manager hints
    wm_hints.input = True;
    wm_hints.icon_pixmap = ipm->handle();
    wm_hints.flags = IconPixmapHint;
    XSetWMHints( display(), id(), &wm_hints );
}
