/****************************************************************************
** $Id: $
**
** Implementation of QWidget and QWindow classes for X11
**
** Created : 931031
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses for Unix/X11 may use this file in accordance with the Qt Commercial
** License Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "qapplication.h"
#include "qapplication_p.h"
#include "qnamespace.h"
#include "qpaintdevicemetrics.h"
#include "qpainter.h"
#include "qbitmap.h"
#include "qobjectlist.h"
#include "qlayout.h"
#include "qtextcodec.h"
#include "qcursor.h"
#include "qt_x11.h"

// NOT REVISED

// defined in qapplication_x11.cpp
void qt_insert_sip( QWidget*, int, int );
int  qt_sip_count( QWidget* );
bool qt_wstate_iconified( WId );
void qt_updated_rootinfo();

#ifndef QT_NO_XIM
#include "qinputcontext_p.h"

extern XIM qt_xim;
extern XIMStyle qt_xim_style;
#endif

// Paint event clipping magic
extern void qt_set_paintevent_clipping( QPaintDevice* dev, const QRegion& region);
extern void qt_clear_paintevent_clipping();

extern bool qt_dnd_enable( QWidget* w, bool on );
extern bool qt_nograb();

// defined in qapplication_x11.cpp
extern void qt_deferred_map_add( QWidget* );
extern void qt_deferred_map_take( QWidget* );

static QWidget *mouseGrb    = 0;
static QWidget *keyboardGrb = 0;

// defined in qapplication_x11.cpp
extern Time qt_x_time;
extern bool qt_use_xrender;

int qt_x11_create_desktop_on_screen = -1;

/*****************************************************************************
  QWidget member functions
 *****************************************************************************/

// defined in qapplication_x11.cpp
extern Atom qt_wm_delete_window;
extern Atom qt_wm_take_focus;
extern Atom qt_wm_client_leader;
extern Atom qt_window_role;
extern Atom qt_sm_client_id;
extern Atom qt_net_wm_context_help;
extern Atom qt_xa_motif_wm_hints;
extern Atom qt_net_wm_state;
extern Atom qt_net_wm_state_modal;
extern Atom qt_net_wm_state_max_v;
extern Atom qt_net_wm_state_max_h;
extern Atom qt_net_wm_state_stays_on_top;
extern Atom qt_net_wm_window_type;
extern Atom qt_net_wm_window_type_normal;
extern Atom qt_net_wm_window_type_dialog;
extern Atom qt_net_wm_window_type_toolbar;
extern Atom qt_net_wm_window_type_override;
extern Atom qt_enlightenment_desktop;
extern Atom qt_net_virtual_roots;
extern bool qt_broken_wm;

// defined in qapplication_x11.cpp
extern bool qt_net_supports(Atom);
extern unsigned long *qt_net_virtual_root_list;

#if defined (QT_TABLET_SUPPORT)
extern XDevice *dev;
extern XEventClass event_list[7];
extern int curr_xinput_events;
#endif

const uint stdWidgetEventMask =			// X event mask
	(uint)(
	    KeyPressMask | KeyReleaseMask |
	    ButtonPressMask | ButtonReleaseMask |
	    KeymapStateMask |
	    ButtonMotionMask |
	    EnterWindowMask | LeaveWindowMask |
	    FocusChangeMask |
	    ExposureMask |
	    PropertyChangeMask |
	    StructureNotifyMask | SubstructureRedirectMask
	);

const uint stdDesktopEventMask =			// X event mask
       (uint)(
           KeymapStateMask |
	   EnterWindowMask | LeaveWindowMask |
	   PropertyChangeMask
       );


/*
  The qt_ functions below are implemented in qwidgetcreate_x11.cpp.
*/

Window qt_XCreateWindow( const QWidget *creator,
			 Display *display, Window parent,
			 int x, int y, uint w, uint h,
			 int borderwidth, int depth,
			 uint windowclass, Visual *visual,
			 ulong valuemask, XSetWindowAttributes *attributes );
Window qt_XCreateSimpleWindow( const QWidget *creator,
			       Display *display, Window parent,
			       int x, int y, uint w, uint h, int borderwidth,
			       ulong border, ulong background );
void qt_XDestroyWindow( const QWidget *destroyer,
			Display *display, Window window );


/*!
  Creates a new widget window if \a window is null, otherwise sets the
  widget's window to \a window.

  Initializes the window (sets the geometry etc.) if \a initializeWindow
  is TRUE.  If \a initializeWindow is FALSE, no initialization is
  performed.  This parameter makes only sense if \a window is a valid
  window.

  Destroys the old window if \a destroyOldWindow is TRUE.  If \a
  destroyOldWindow is FALSE, you are responsible for destroying
  the window yourself (using platform native code).

  The QWidget constructor calls create(0,TRUE,TRUE) to create a window for
  this widget.
*/

void QWidget::create( WId window, bool initializeWindow, bool destroyOldWindow)
{
    if ( testWState(WState_Created) && window == 0 )
	return;

    // set created flag
    setWState( WState_Created );

    bool popup = testWFlags(WType_Popup);
    bool dialog = testWFlags(WType_Dialog);
    bool desktop = testWFlags(WType_Desktop);

    // top-level widget
    if ( !parentWidget() || parentWidget()->isDesktop() )
	setWFlags( WType_TopLevel );

    // these are top-level, too
    if ( dialog || popup || desktop )
	setWFlags( WType_TopLevel );

    // a popup stays on top
    if ( popup )
	setWFlags(WStyle_StaysOnTop);

    bool topLevel = testWFlags(WType_TopLevel);
    Window parentw, destroyw = 0;
    WId	   id;

    // always initialize
    if ( !window )
	initializeWindow = TRUE;

    if ( desktop &&
	 qt_x11_create_desktop_on_screen >= 0 &&
	 qt_x11_create_desktop_on_screen != x11Screen() ) {
	// desktop on a certain screen other than the default requested
	QPaintDeviceX11Data* xd = getX11Data( TRUE );
	xd->x_screen = qt_x11_create_desktop_on_screen;
	xd->x_depth = QPaintDevice::x11AppDepth( xd->x_screen );
	xd->x_cells = QPaintDevice::x11AppCells( xd->x_screen );
	xd->x_colormap = QPaintDevice::x11AppColormap( xd->x_screen );
	xd->x_defcolormap = QPaintDevice::x11AppDefaultColormap( xd->x_screen );
	xd->x_visual = QPaintDevice::x11AppVisual( xd->x_screen );
	xd->x_defvisual = QPaintDevice::x11AppDefaultVisual( xd->x_screen );
	setX11Data( xd );
    } else if ( parentWidget() &&  parentWidget()->x11Screen() != x11Screen() ) {
	// if we have a parent widget, move to its screen if necessary
	QPaintDeviceX11Data* xd = getX11Data( TRUE );
	xd->x_screen = parentWidget()->x11Screen();
	xd->x_depth = QPaintDevice::x11AppDepth( xd->x_screen );
	xd->x_cells = QPaintDevice::x11AppCells( xd->x_screen );
	xd->x_colormap = QPaintDevice::x11AppColormap( xd->x_screen );
	xd->x_defcolormap = QPaintDevice::x11AppDefaultColormap( xd->x_screen );
	xd->x_visual = QPaintDevice::x11AppVisual( xd->x_screen );
	xd->x_defvisual = QPaintDevice::x11AppDefaultVisual( xd->x_screen );
	setX11Data( xd );
    }

    //get display, screen number, root window and desktop geometry for
    //the current screen
    Display *dpy = x11Display();
    int scr = x11Screen();
    Window root_win = RootWindow( dpy, scr );
    int sw = DisplayWidth(dpy,scr);
    int sh = DisplayHeight(dpy,scr);

    if ( desktop ) {				// desktop widget
	dialog = popup = FALSE;			// force these flags off
	crect.setRect( 0, 0, sw, sh );
    } else if ( topLevel ) {			// calc pos/size from screen
	crect.setRect( sw/4, 3*sh/10, sw/2, 4*sh/10 );
    } else {					// child widget
	crect.setRect( 0, 0, 100, 30 );
    }

    parentw = topLevel ? root_win : parentWidget()->winId();

    XSetWindowAttributes wsa;

    if ( window ) {				// override the old window
	if ( destroyOldWindow )
	    destroyw = winid;
	id = window;
	setWinId( window );
    } else if ( desktop ) {			// desktop widget
	id = (WId)parentw;			// id = root window
	QWidget *otherDesktop = find( id );	// is there another desktop?
	if ( otherDesktop && otherDesktop->testWFlags(WPaintDesktop) ) {
	    otherDesktop->setWinId( 0 );	// remove id from widget mapper
	    setWinId( id );			// make sure otherDesktop is
	    otherDesktop->setWinId( id );	//   found first
	} else {
	    setWinId( id );
	}
    } else {
	if ( x11DefaultVisual() && x11DefaultColormap() ) {
	    id = (WId)qt_XCreateSimpleWindow( this, dpy, parentw,
					      crect.left(), crect.top(),
					      crect.width(), crect.height(),
					      0,
					      black.pixel(x11Screen()),
					      bg_col.pixel(x11Screen()) );
	} else {
	    wsa.background_pixel = bg_col.pixel(x11Screen());
	    wsa.border_pixel = black.pixel(x11Screen());
	    wsa.colormap = (Colormap)x11Colormap();
	    id = (WId)qt_XCreateWindow( this, dpy, parentw,
					crect.left(), crect.top(),
					crect.width(), crect.height(),
					0, x11Depth(), InputOutput,
					(Visual*)x11Visual(),
					CWBackPixel|CWBorderPixel|CWColormap,
					&wsa );
	}

	setWinId( id );				// set widget id/handle + hd
    }

#ifndef QT_NO_XRENDER
    if (rendhd) {
	XRenderFreePicture(dpy, rendhd);
	rendhd = 0;
    }

    if (qt_use_xrender) {
	XRenderPictFormat *format = XRenderFindVisualFormat(dpy, (Visual *) x11Visual());
	if (format)
	    rendhd = XRenderCreatePicture(dpy, id, format, 0, 0);
    }
#endif // QT_NO_XRENDER

    // NET window types
    long net_wintypes[4] = { 0, 0, 0, 0 };
    int curr_wintype = 0;

    // NET window states
    long net_winstates[4] = { 0, 0, 0, 0 };
    int curr_winstate = 0;

    if (topLevel && ! (desktop || popup)) {
	ulong wsa_mask = 0;
	if (testWFlags(WStyle_Customize)) {
	    if (testWFlags(WStyle_NoBorder)) {
		struct {
		    ulong flags, functions, decorations;
		    long input_mode;
		    ulong status;
		} hints;

		hints.decorations = 0;
		hints.flags = (1L << 1); // MWM_HINTS_DECORATIONS
		XChangeProperty(dpy, id, qt_xa_motif_wm_hints,
				qt_xa_motif_wm_hints, 32, PropModeReplace,
				(unsigned char *) &hints, 5);

		// override netwm type - quick and easy for KDE noborder
		net_wintypes[curr_wintype++] = qt_net_wm_window_type_override;
	    }

	    if (testWFlags(WStyle_Tool)) {
		wsa.save_under = True;
		wsa_mask |= CWSaveUnder;

		// toolbar netwm type
		net_wintypes[curr_wintype++] = qt_net_wm_window_type_toolbar;
	    }
	} else if (testWFlags(WType_Dialog)) {
	    setWFlags(WStyle_NormalBorder | WStyle_Title |
		      WStyle_SysMenu | WStyle_ContextHelp);

	    // dialog netwm type
	    // ### ENABLE ME AFTER KDE 2.1 RELASE - waiting for bug fix in
	    // kwin to propogate
	    // net_wintypes[curr_wintype++] = qt_net_wm_window_type_dialog;

	    if (testWFlags(WShowModal))
		net_winstates[curr_winstate++] = qt_net_wm_state_modal;
	} else {
	    setWFlags(WStyle_NormalBorder | WStyle_Title |
		      WStyle_MinMax | WStyle_SysMenu);

	    // maximized netwm state
	    if (testWFlags(WState_Maximized)) {
		net_winstates[curr_winstate++] = qt_net_wm_state_max_v;
		net_winstates[curr_winstate++] = qt_net_wm_state_max_h;
	    }
	}

	// normal netwm type - default
	net_wintypes[curr_wintype++] = qt_net_wm_window_type_normal;

	// stays on top
	if (testWFlags(WStyle_StaysOnTop))
	    net_winstates[curr_winstate++] = qt_net_wm_state_stays_on_top;

	if ( testWFlags( WX11BypassWM ) ) {
	    wsa.override_redirect = True;
	    wsa_mask |= CWOverrideRedirect;
	}

	if ( wsa_mask && initializeWindow )
	    XChangeWindowAttributes( dpy, id, wsa_mask, &wsa );
    } else {
	if (! testWFlags(WStyle_Customize))
	    setWFlags(WStyle_NormalBorder | WStyle_Title |
		      WStyle_MinMax | WStyle_SysMenu);
    }


    if ( !initializeWindow ) {
	// do no initialization
    } else if ( popup ) {			// popup widget
	wsa.override_redirect = True;
	wsa.save_under = True;
	XChangeWindowAttributes( dpy, id, CWOverrideRedirect | CWSaveUnder,
				 &wsa );
    } else if ( topLevel && !desktop ) {	// top-level widget
	QWidget *p = parentWidget();	// real parent
	if (p)
	    p = p->topLevelWidget();

	if ( testWFlags(WStyle_DialogBorder) ||
	     testWFlags(WStyle_StaysOnTop) ||
	     dialog ||
	     testWFlags(WStyle_Tool) ) {
	    if ( testWFlags( WStyle_StaysOnTop ) )
		XSetTransientForHint( dpy, id, None );
	    else  if ( p )
		XSetTransientForHint( dpy, id, p->winId() );
	    else				// application-modal
		XSetTransientForHint( dpy, id, root_win );
	}

	// find the real client leader, i.e. a toplevel without parent
	while ( p && p->parentWidget())
	    p = p->parentWidget()->topLevelWidget();

	XSizeHints size_hints;
	size_hints.flags = USSize | PSize | PWinGravity;
	size_hints.x = crect.left();
	size_hints.y = crect.top();
	size_hints.width = crect.width();
	size_hints.height = crect.height();
	size_hints.win_gravity = QApplication::reverseLayout() ?
				 NorthEastGravity : NorthWestGravity;
	const char *title = qAppName();
	XWMHints wm_hints;			// window manager hints
	wm_hints.input = True;
	wm_hints.initial_state = NormalState;
	wm_hints.flags = InputHint | StateHint;

	if (p) { // the real client leader (head of the group)
	    wm_hints.window_group = p->winId();
	    wm_hints.flags |= WindowGroupHint;
	}

	XClassHint class_hint;
	class_hint.res_class = (char *)title;	// app name and widget name
	class_hint.res_name = name() ? (char *)name() : (char *)title;
	XSetWMProperties( dpy, id, 0, 0, 0, 0, &size_hints, &wm_hints,
			  &class_hint );
	XResizeWindow( dpy, id, crect.width(), crect.height() );
	XStoreName( dpy, id, (char *)title );
	Atom protocols[3];
	int n = 0;
	protocols[n++] = qt_wm_delete_window;	// support del window protocol
	protocols[n++] = qt_wm_take_focus;	// support take focus window protocol
	if ( testWFlags( WStyle_ContextHelp ) )
	    protocols[n++] = qt_net_wm_context_help;
	XSetWMProtocols( dpy, id, protocols, n );

	// set _NET_WM_WINDOW_TYPE
	if (curr_wintype > 0)
	    XChangeProperty(dpy, id, qt_net_wm_window_type, XA_ATOM, 32, PropModeReplace,
			    (unsigned char *) net_wintypes, curr_wintype);

	// set _NET_WM_WINDOW_STATE
	if (curr_winstate > 0)
	    XChangeProperty(dpy, id, qt_net_wm_state, XA_ATOM, 32, PropModeReplace,
			    (unsigned char *) net_winstates, curr_winstate);

	// when we create a toplevel widget, the frame strut should be dirty
	fstrut_dirty = 1;

	// declare the widget's object name as window role
	XChangeProperty( dpy, id,
			 qt_window_role, XA_STRING, 8, PropModeReplace,
			 (unsigned char *)name(), qstrlen( name() ) );
	// If we are session managed, inform the window manager about it
	QCString session = qApp->sessionId().latin1();
	if ( !session.isEmpty() ) {
	    XChangeProperty( dpy, id,
			     qt_sm_client_id, XA_STRING, 8, PropModeReplace,
			     (unsigned char *)session.data(), session.length() );
	}
    } else
	// non-toplevel widgets don't have a frame, so no need to update the strut
	fstrut_dirty = 0;

    if ( initializeWindow ) {
	// don't erase when resizing
	wsa.bit_gravity =
	    QApplication::reverseLayout() ? NorthEastGravity : NorthWestGravity;
	XChangeWindowAttributes( dpy, id, CWBitGravity, &wsa );
    }

    setWState( WState_MouseTracking );
    setMouseTracking( FALSE );			// also sets event mask
    if ( desktop ) {
	setWState( WState_Visible );
    } else if ( topLevel ) {			// set X cursor
	QCursor *oc = QApplication::overrideCursor();
	if ( initializeWindow )
	    XDefineCursor( dpy, winid, oc ? oc->handle() : cursor().handle() );
	setWState( WState_OwnCursor );
    }

    if ( window ) {				// got window from outside
	XWindowAttributes a;
	XGetWindowAttributes( dpy, window, &a );
	crect.setRect( a.x, a.y, a.width, a.height );

	if ( a.map_state == IsUnmapped )
	    clearWState( WState_Visible );
	else
	    setWState( WState_Visible );

	QPaintDeviceX11Data* xd = getX11Data( TRUE );

	// find which screen the window is on...
	xd->x_screen = QPaintDevice::x11AppScreen(); // by default, use the default :)
	int i;
	for ( i = 0; i < ScreenCount( dpy ); i++ ) {
	    if ( RootWindow( dpy, i ) == a.root ) {
		xd->x_screen = i;
		break;
	    }
	}

	xd->x_depth = a.depth;
	xd->x_cells = DisplayCells( dpy, xd->x_screen );
	xd->x_visual = a.visual;
	xd->x_defvisual = ( XVisualIDFromVisual( a.visual ) ==
			    XVisualIDFromVisual( (Visual*)x11AppVisual(x11Screen()) ) );
	xd->x_colormap = a.colormap;
	xd->x_defcolormap = ( a.colormap == x11AppColormap( x11Screen() ) );
	setX11Data( xd );
    }

    if ( destroyw )
	qt_XDestroyWindow( this, dpy, destroyw );
}


/*!
  Frees up window system resources.
  Destroys the widget window if \a destroyWindow is TRUE.

  destroy() calls itself recursively for all the child widgets,
  passing \a destroySubWindows for the \a destroyWindow parameter.
  To have more control over destruction of subwidgets,
  destroy subwidgets selectively first.

  This function is usually called from the QWidget destructor.
*/

void QWidget::destroy( bool destroyWindow, bool destroySubWindows )
{
    deactivateWidgetCleanup();
    if ( testWState(WState_Created) ) {
	clearWState( WState_Created );
	if ( children() ) {
	    QObjectListIt it(*children());
	    register QObject *obj;
	    while ( (obj=it.current()) ) {	// destroy all widget children
		++it;
		if ( obj->isWidgetType() )
		    ((QWidget*)obj)->destroy(destroySubWindows,
					     destroySubWindows);
	    }
	}
	if ( mouseGrb == this )
	    releaseMouse();
	if ( keyboardGrb == this )
	    releaseKeyboard();
	if ( isTopLevel() )
	    qt_deferred_map_take( this );
	if ( testWFlags(WShowModal) )		// just be sure we leave modal
	    qt_leave_modal( this );
	else if ( testWFlags(WType_Popup) )
	    qApp->closePopup( this );

#ifndef QT_NO_XRENDER
	if (rendhd) {
	    XRenderFreePicture(x11Display(), rendhd);
	    rendhd = 0;
	}
#endif // QT_NO_XRENDER

	if ( testWFlags(WType_Desktop) ) {
	    if ( acceptDrops() )
		qt_dnd_enable( this, FALSE );
	} else {
	    if ( destroyWindow )
		qt_XDestroyWindow( this, x11Display(), winid );
	}
	setWinId( 0 );

	extern void qPRCleanup( QWidget *widget ); // from qapplication_x11.cpp
	if ( testWState(WState_Reparented) )
	    qPRCleanup(this);
    }
}

void QWidget::reparentSys( QWidget *parent, WFlags f, const QPoint &p, bool showIt )
{
    extern void qPRCreate( const QWidget *, Window );

    Display *dpy = x11Display();
    QCursor oldcurs;
    bool setcurs = testWState(WState_OwnCursor);
    if ( setcurs ) {
	oldcurs = cursor();
	unsetCursor();
    }

    // dnd unregister (we will register again below)
    bool accept_drops = acceptDrops();
    setAcceptDrops( FALSE );

    // clear mouse tracking, re-enabled below
    bool mouse_tracking = hasMouseTracking();
    clearWState(WState_MouseTracking);

    QWidget* oldtlw = topLevelWidget();
    QWidget *oldparent = parentWidget();
    WId old_winid = winid;
    if ( testWFlags(WType_Desktop) )
	old_winid = 0;
    setWinId( 0 );

    // hide and reparent our own window away. Otherwise we might get
    // destroyed when emitting the child remove event below. See QWorkspace.
    XUnmapWindow( x11Display(), old_winid );
    XReparentWindow( x11Display(), old_winid,
		     RootWindow( x11Display(), x11Screen() ), 0, 0 );

    if ( isTopLevel() )
	topData()->parentWinId = 0;

    if ( parentObj ) {				// remove from parent
	parentObj->removeChild( this );
    }
    if ( parent ) {				// insert into new parent
	parentObj = parent;			// avoid insertChild warning
	parent->insertChild( this );
    }
    bool     enable = isEnabled();		// remember status
    FocusPolicy fp = focusPolicy();
    QSize    s	    = size();
    QPixmap *bgp    = (QPixmap *)backgroundPixmap();
    QColor   bgc    = bg_col;			// save colors
    QString capt= caption();
    widget_flags = f;
    clearWState( WState_Created | WState_Visible | WState_ForceHide );
    if ( isTopLevel() || (!parent || parent->isVisibleTo( 0 ) ) )
	setWState( WState_ForceHide );	// new widgets do not show up in already visible parents
    create();

    const QObjectList *chlist = children();
    if ( chlist ) {				// reparent children
	QObjectListIt it( *chlist );
	QObject *obj;
	while ( (obj=it.current()) ) {
	    if ( obj->isWidgetType() ) {
		QWidget *w = (QWidget *)obj;
		if ( !w->isTopLevel() ) {
		    XReparentWindow( x11Display(), w->winId(), winId(),
				     w->geometry().x(), w->geometry().y() );
		} else if ( w->isPopup()
			    || w->testWFlags(WStyle_DialogBorder)
			    || w->testWFlags(WType_Dialog)
			    || w->testWFlags(WStyle_Tool) ) {
		    XSetTransientForHint( x11Display(), w->winId(), winId() );
		}
	    }
	    ++it;
	}
    }
    qPRCreate( this, old_winid );
    if ( bgp )
	XSetWindowBackgroundPixmap( dpy, winid, bgp->handle() );
    else
	XSetWindowBackground( dpy, winid, bgc.pixel(x11Screen()) );
    if ( p.isNull() )
	resize( s );
    else
	setGeometry( p.x(), p.y(), s.width(), s.height() );

    setEnabled( enable );
    setFocusPolicy( fp );
    if ( !capt.isNull() ) {
	extra->topextra->caption = QString::null;
	setCaption( capt );
    }
    if ( showIt )
	show();
    if ( old_winid )
	qt_XDestroyWindow( this, dpy, old_winid );
    if ( setcurs )
	setCursor(oldcurs);

    reparentFocusWidgets( oldtlw );

    // re-register dnd
    if (oldparent)
	oldparent->checkChildrenDnd();

    if ( accept_drops )
	setAcceptDrops( TRUE );
    else {
	checkChildrenDnd();
	topData()->dnd = 0;
	qt_dnd_enable(this, (extra && extra->children_use_dnd));
    }

    // re-enable mouse tracking
    if (mouse_tracking)
	setMouseTracking(mouse_tracking);
}


/*!
  Translates the widget coordinate \a pos to global screen coordinates.
  For example, \code mapToGlobal(QPoint(0,0))\endcode would give the
  global coordinates of the top-left pixel of the widget.
  \sa mapFromGlobal() mapTo() mapToParent()
*/

QPoint QWidget::mapToGlobal( const QPoint &pos ) const
{
    int	   x, y;
    Window child;
    XTranslateCoordinates( x11Display(), winId(),
			   QApplication::desktop()->screen(x11Screen())->winId(),
			   pos.x(), pos.y(), &x, &y, &child );
    return QPoint( x, y );
}

/*!
  Translates the global screen coordinate \a pos to widget coordinates.
  \sa mapToGlobal() mapFrom() mapFromParent()
*/

QPoint QWidget::mapFromGlobal( const QPoint &pos ) const
{
    int	   x, y;
    Window child;
    XTranslateCoordinates( x11Display(),
			   QApplication::desktop()->screen(x11Screen())->winId(),
			   winId(), pos.x(), pos.y(), &x, &y, &child );
    return QPoint( x, y );
}

/*!
  When a widget gets focus, it should call setMicroFocusHint for some
  appropriate position and size - \a x, \a y and \a width by \a height.  This
  has no \e visual effect, it just provides hints to any
  system-specific input handling tools.

  The \a text argument should be TRUE if this is a position for text
  input.

  In the Windows version of Qt, this method sets the system caret, which is
  used for user Accessibility focus handling.  If \a text is TRUE, it also
  sets the IME composition window in Far East Asian language input systems.

  In the X11 version of Qt, if \a text is TRUE, this method sets the
  XIM "spot" point for complex language input handling.

  The font parameter, \a f, is currently unused.

  \sa microFocusHint()
*/
void QWidget::setMicroFocusHint(int x, int y, int width, int height,
				bool text, QFont *f )
{
#ifndef QT_NO_XIM
    if ( text ) {
	QWidget* tlw = topLevelWidget();
	QTLWExtra *topdata = tlw->topData();

	// trigger input context creation if it hasn't happened already
	createInputContext();
	QInputContext *qic = (QInputContext *) topdata->xic;

	if ( qt_xim && qic ) {
	    QPoint p( x, y );
	    p = mapTo( topLevelWidget(), p);
	    qic->setXFontSet(f);
	    qic->setComposePosition(p.x(), p.y() + height);
	    qic->setComposeArea(p.x(), p.y(), width, height);
	}
    }
#endif

    if ( QRect( x, y, width, height ) != microFocusHint() )
	extraData()->micro_focus_hint.setRect( x, y, width, height );
}


void QWidget::setFontSys( QFont * )
{
    // Nothing
}


void QWidget::setBackgroundColorDirect( const QColor &color )
{
    bg_col = color;
    if ( extra && extra->bg_pix ) {		// kill the background pixmap
	delete extra->bg_pix;
	extra->bg_pix = 0;
    }
    XSetWindowBackground( x11Display(), winId(), bg_col.pixel(x11Screen()) );
}

static int allow_null_pixmaps = 0;


void QWidget::setBackgroundPixmapDirect( const QPixmap &pixmap )
{
    QPixmap old;
    if ( extra && extra->bg_pix )
	old = *extra->bg_pix;
    if ( !allow_null_pixmaps && pixmap.isNull() ) {
	XSetWindowBackground( x11Display(), winId(), bg_col.pixel(x11Screen()) );
	if ( extra && extra->bg_pix ) {
	    delete extra->bg_pix;
	    extra->bg_pix = 0;
	}
    } else {
	QPixmap pm = pixmap;
	if (!pm.isNull()) {
	    if ( pm.depth() == 1 && QPixmap::defaultDepth() > 1 ) {
		pm = QPixmap( pixmap.size() );
		bitBlt( &pm, 0, 0, &pixmap, 0, 0, pm.width(), pm.height() );
	    }
	}
	if ( extra && extra->bg_pix )
	    delete extra->bg_pix;
	else
	    createExtra();
	extra->bg_pix = new QPixmap( pm );
	XSetWindowBackgroundPixmap( x11Display(), winId(), pm.handle() );
	if ( testWFlags(WType_Desktop) )	// save rootinfo later
	    qt_updated_rootinfo();
    }
}


/*!
  Sets the window-system background of the widget to nothing.

  Note that `nothing' is actually a pixmap that isNull(), thus you
  can check for an empty background by checking backgroundPixmap().

  \sa setBackgroundPixmap(), setBackgroundColor()

  This class should \e NOT be made virtual - it is an alternate usage
  of setBackgroundPixmap().
*/
void QWidget::setBackgroundEmpty()
{
    allow_null_pixmaps++;
    setErasePixmap(QPixmap());
    allow_null_pixmaps--;
}


void QWidget::setBackgroundX11Relative()
{
    XSetWindowBackgroundPixmap( x11Display(), winId(), ParentRelative );
}

void QWidget::setCursor( const QCursor &cursor )
{
    if ( cursor.handle() != arrowCursor.handle()
	 || (extra && extra->curs) ) {
	createExtra();
	delete extra->curs;
	extra->curs = new QCursor(cursor);
    }
    setWState( WState_OwnCursor );
    QCursor *oc = QApplication::overrideCursor();
    XDefineCursor( x11Display(), winId(),
		   oc ? oc->handle() : cursor.handle() );
    XFlush( x11Display() );
}

void QWidget::unsetCursor()
{
    if ( !isTopLevel() ) {
	if (extra ) {
	    delete extra->curs;
	    extra->curs = 0;
	}
	clearWState( WState_OwnCursor );
	XDefineCursor( x11Display(), winId(), None );
	XFlush( x11Display() );
    }
}

static XTextProperty*
qstring_to_xtp( const QString& s )
{
    static XTextProperty tp = { 0, 0, 0, 0 };
    static bool free_prop = TRUE; // we can't free tp.value in case it references
    // the data of the static QCString below.
    if ( tp.value ) {
	if ( free_prop )
	    XFree( tp.value );
	tp.value = 0;
	free_prop = TRUE;
    }

    static const QTextCodec* mapper = QTextCodec::codecForLocale();
    int errCode = 0;
    if ( mapper ) {
	QCString mapped = mapper->fromUnicode(s);
	char* tl[2];
	tl[0] = mapped.data();
	tl[1] = 0;
	errCode = XmbTextListToTextProperty( QPaintDevice::x11AppDisplay(),
					     tl, 1, XStdICCTextStyle, &tp );
#if defined(QT_DEBUG)
	if ( errCode < 0 )
	    qDebug( "qstring_to_xtp result code %d", errCode );
#endif
    }
    if ( !mapper || errCode < 0 ) {
	static QCString qcs;
	qcs = s.ascii();
	tp.value = (uchar*)qcs.data();
	tp.encoding = XA_STRING;
	tp.format = 8;
	tp.nitems = qcs.length();
	free_prop = FALSE;
    }

    // ### If we knew WM could understand unicode, we could use
    // ### a much simpler, cheaper encoding...
    /*
	tp.value = (XChar2b*)s.unicode();
	tp.encoding = XA_UNICODE; // wish
	tp.format = 16;
	tp.nitems = s.length();
    */

    return &tp;
}

void QWidget::setCaption( const QString &caption )
{
    if ( QWidget::caption() == caption )
	return;

    topData()->caption = caption;
    XSetWMName( x11Display(), winId(), qstring_to_xtp(caption) );
    QEvent e( QEvent::CaptionChange );
    QApplication::sendEvent( this, &e );
}

void QWidget::setIcon( const QPixmap &pixmap )
{
    if ( extra && extra->topextra ) {
	delete extra->topextra->icon;
	extra->topextra->icon = 0;
    } else {
	createTLExtra();
    }
    Pixmap icon_pixmap = 0;
    Pixmap mask_pixmap = 0;
    if ( !pixmap.isNull() ) {
	QPixmap* pm = new QPixmap( pixmap );
	extra->topextra->icon = pm;
	if ( !pm->mask() )
	    pm->setMask( pm->createHeuristicMask() ); // may do detach()
	icon_pixmap = pm->handle();
	if ( pm->mask() )
	    mask_pixmap = pm->mask()->handle();
    }
    XWMHints *h = XGetWMHints( x11Display(), winId() );
    XWMHints  wm_hints;
    bool got_hints = h != 0;
    if ( !got_hints ) {
	h = &wm_hints;
	h->flags = 0;
    }
    h->icon_pixmap = icon_pixmap;
    h->icon_mask = mask_pixmap;
    h->flags |= IconPixmapHint | IconMaskHint;
    XSetWMHints( x11Display(), winId(), h );
    if ( got_hints )
	XFree( (char *)h );
    QEvent e( QEvent::IconChange );
    QApplication::sendEvent( this, &e );
}

void QWidget::setIconText( const QString &iconText )
{
    createTLExtra();
    extra->topextra->iconText = iconText;
    XSetIconName( x11Display(), winId(), iconText.utf8() );
    XSetWMIconName( x11Display(), winId(), qstring_to_xtp(iconText) );
}

void QWidget::setMouseTracking( bool enable )
{
    bool gmt = QApplication::hasGlobalMouseTracking();
    if ( !enable == !testWState(WState_MouseTracking) && !gmt )
	return;
    uint m = (enable || gmt) ? (uint)PointerMotionMask : 0;
    if ( enable )
	setWState( WState_MouseTracking );
    else
	clearWState( WState_MouseTracking );
    if ( testWFlags(WType_Desktop) ) {		// desktop widget?
	QWidget* main_desktop = find( winId() );
	if ( main_desktop->testWFlags(WPaintDesktop) )
	    XSelectInput( x11Display(), winId(),
			  stdDesktopEventMask | ExposureMask );
	else
	    XSelectInput( x11Display(), winId(), stdDesktopEventMask );
    } else {
	XSelectInput( x11Display(), winId(),
		      m | stdWidgetEventMask );
#if defined (QT_TABLET_SUPPORT)
	if ( dev != NULL ) {
	    XSelectExtensionEvent( x11Display(), winId(), event_list,
				   curr_xinput_events );
	}
#endif
    }
}


/*!
  Grabs the mouse input.

  This widget receives all mouse events and other widgets none until
  releaseMouse() is called. Keyboard events are not affected. Use
  grabKeyboard() if you want to grab that.

  \warning Bugs in mouse-grabbing applications very often lock the
  terminal. Use this function with extreme caution, and consider using
  the -nograb command line option while debugging.

  It is almost never necessary to grab the mouse when using Qt, as Qt
  grabs and releases it sensibly.  In particular, Qt grabs the mouse
  when a mouse button is pressed and keeps it until the last button is
  released.

  Note that only visible widgets can grab mouse input. If isVisible()
  returns FALSE for a widget, that widget can not call grabMouse().

  \sa releaseMouse() grabKeyboard() releaseKeyboard()
  grabKeyboard() focusWidget()
*/

void QWidget::grabMouse()
{
    if ( isVisible() && !qt_nograb() ) {
	if ( mouseGrb )
	    mouseGrb->releaseMouse();
#if defined(QT_CHECK_STATE)
	int status =
#endif
	    XGrabPointer( x11Display(), winId(), False,
			  (uint)( ButtonPressMask | ButtonReleaseMask |
				  PointerMotionMask | EnterWindowMask |
				  LeaveWindowMask ),
			  GrabModeAsync, GrabModeAsync,
			  None, None, qt_x_time );
#if defined(QT_CHECK_STATE)
	if ( status ) {
	    const char *s =
		status == GrabNotViewable ? "\"GrabNotViewable\"" :
		status == AlreadyGrabbed  ? "\"AlreadyGrabbed\"" :
		status == GrabFrozen      ? "\"GrabFrozen\"" :
		status == GrabInvalidTime ? "\"GrabInvalidTime\"" :
		"<?>";
	    qWarning( "Grabbing the mouse failed with %s", s );
	}
#endif
	mouseGrb = this;
    }
}

/*!
    \overload
  Grabs the mouse input and changes the cursor shape.

  The cursor will assume shape \a cursor (for as long as the mouse focus is
  grabbed) and this widget will be the only one to receive mouse events
  until releaseMouse() is called().

  \warning Grabbing the mouse might lock the terminal.

  \sa releaseMouse(), grabKeyboard(), releaseKeyboard(), setCursor()
*/

void QWidget::grabMouse( const QCursor &cursor )
{
    if ( !qt_nograb() ) {
	if ( mouseGrb )
	    mouseGrb->releaseMouse();
#if defined(QT_CHECK_STATE)
	int status =
#endif
	XGrabPointer( x11Display(), winId(), False,
		      (uint)(ButtonPressMask | ButtonReleaseMask |
			     PointerMotionMask | EnterWindowMask | LeaveWindowMask),
		      GrabModeAsync, GrabModeAsync,
		      None, cursor.handle(), qt_x_time );
#if defined(QT_CHECK_STATE)
	if ( status ) {
	    const char *s =
		status == GrabNotViewable ? "\"GrabNotViewable\"" :
		status == AlreadyGrabbed  ? "\"AlreadyGrabbed\"" :
		status == GrabFrozen      ? "\"GrabFrozen\"" :
		status == GrabInvalidTime ? "\"GrabInvalidTime\"" :
					    "<?>";
	    qWarning( "Grabbing the mouse failed with %s", s );
	}
#endif
	mouseGrb = this;
    }
}

/*!
  Releases the mouse grab.

  \sa grabMouse(), grabKeyboard(), releaseKeyboard()
*/

void QWidget::releaseMouse()
{
    if ( !qt_nograb() && mouseGrb == this ) {
	XUngrabPointer( x11Display(),  qt_x_time );
	XFlush( x11Display() );
	mouseGrb = 0;
    }
}

/*!  Grabs the keyboard input.

  This widget reveives all keyboard events and other widgets none
  until releaseKeyboard() is called. Mouse events are not
  affected. Use grabMouse() if you want to grab that.

  The focus widget is not affected, except that it doesn't receive any
  keyboard events. setFocus() moves the focus as usual, but the new
  focus widget receives keyboard events only after releaseKeyboard()
  is called.

  If a different widget is currently grabbing keyboard input, that
  widget's grab is released first.

  \sa releaseKeyboard() grabMouse() releaseMouse() focusWidget()
*/

void QWidget::grabKeyboard()
{
    if ( !qt_nograb() ) {
	if ( keyboardGrb )
	    keyboardGrb->releaseKeyboard();
	XGrabKeyboard( x11Display(), winid, False, GrabModeAsync, GrabModeAsync,
		       qt_x_time );
	keyboardGrb = this;
    }
}

/*!
  Releases the keyboard grab.

  \sa grabKeyboard(), grabMouse(), releaseMouse()
*/

void QWidget::releaseKeyboard()
{
    if ( !qt_nograb() && keyboardGrb == this ) {
	XUngrabKeyboard( x11Display(), qt_x_time );
	keyboardGrb = 0;
    }
}


/*!
  Returns a pointer to the widget that is currently grabbing the
  mouse input.

  If no widget in this application is currently grabbing the mouse, 0 is
  returned.

  \sa grabMouse(), keyboardGrabber()
*/

QWidget *QWidget::mouseGrabber()
{
    return mouseGrb;
}

/*!
  Returns a pointer to the widget that is currently grabbing the
  keyboard input.

  If no widget in this application is currently grabbing the keyboard, 0
  is returned.

  \sa grabMouse(), mouseGrabber()
*/

QWidget *QWidget::keyboardGrabber()
{
    return keyboardGrb;
}

/*!
  Sets the top-level widget containing this widget to be the active
  window.

  An active window is a visible top-level window that has the keyboard input
  focus.

  This function performs the same operation as clicking the mouse on
  the title bar of a top-level window. On X11, the result depends on
  the Window Manager. If you want to ensure that the window is stacked
  on top as well, call raise() in addition. Note that the window has be
  to visible, otherwise setActiveWindow() has no effect.

  On Windows, if you are calling this when the application is not
  currently the active one then it will not make it the active window.  It
  will flash the task bar entry blue to indicate that the window has done
  something.  This is due to Microsoft not allowing an application to
  interrupt what the user is currently doing in another application.

  \sa isActiveWindow(), topLevelWidget(), show()
*/

void QWidget::setActiveWindow()
{
    QWidget *tlw = topLevelWidget();
    if ( tlw->isVisible() && !tlw->topData()->embedded ) {
	XSetInputFocus( x11Display(), tlw->winId(), RevertToNone, qt_x_time);

#ifndef QT_NO_XIM
	// trigger input context creation if it hasn't happened already
	createInputContext();

	if (tlw->topData()->xic) {
	    QInputContext *qic = (QInputContext *) tlw->topData()->xic;
	    qic->setFocus();
	}
#endif
    }
}


/*!
  Updates the widget unless updates are disabled or the widget is hidden.

  This function does not cause an immediate repaint - rather, it
  schedules a paint event for processing when Qt returns to the main
  event loop. This permits Qt to optimize for more speed and less
  flicker and a call to repaint() does.

  Calling update() several times normally results in just one
  paintEvent() call.

  Qt normally erases the widget's area before the paintEvent() call.
  Only if the WRepaintNoErase widget flag is set, the widget itself is
  responsible for painting all its pixels.

  \sa repaint(), paintEvent(), setUpdatesEnabled(), erase(), setWFlags()
*/

void QWidget::update()
{
    if ( (widget_state & (WState_Visible|WState_BlockUpdates)) ==
	 WState_Visible )
	QApplication::postEvent( this, new QPaintEvent( visibleRect(), !testWFlags(WRepaintNoErase) ) );
}

/*!
    \overload
  Updates a rectangle (\a x, \a y, \a w, \a h) inside the widget
  unless updates are disabled or the widget is hidden.

  This function does not cause an immediate repaint - rather, it
  schedules a paint event for processing when Qt returns to the main
  event loop. This permits Qt to optimize for more speed and less
  flicker and a call to repaint() does.

  Calling update() several times normally results in just one
  paintEvent() call.

  If \a w is negative, it is replaced with \c{width() - x}.
  If \a h is negative, it is replaced width \c{height() - y}.

  Qt normally erases the specified area before the paintEvent() call.
  Only if the WRepaintNoErase widget flag is set, the widget itself is
  responsible for painting all its pixels.

  \sa repaint(), paintEvent(), setUpdatesEnabled(), erase()
*/

void QWidget::update( int x, int y, int w, int h )
{
    if ( w && h &&
	 (widget_state & (WState_Visible|WState_BlockUpdates)) == WState_Visible ) {
	if ( w < 0 )
	    w = crect.width()  - x;
	if ( h < 0 )
	    h = crect.height() - y;
	if ( w != 0 && h != 0 )
	    QApplication::postEvent( this,
		new QPaintEvent( visibleRect().intersect(QRect(x,y,w,h)),
				 !testWFlags( WRepaintNoErase ) ) );
    }
}

/*!
  \overload void QWidget::update( const QRect &r )
  Updates a rectangle \a r inside the widget
  unless updates are disabled or the widget is hidden.

  This function does not cause an immediate repaint - rather, it
  schedules a paint event for processing when Qt returns to the main
  event loop. This permits Qt to optimize for more speed and less
  flicker and a call to repaint() does.

  Calling update() several times normally results in just one
  paintEvent() call.
*/

/*!
  \overload void QWidget::repaint( bool erase )

  This version repaints the entire widget.
*/

/*!
  \overload void QWidget::repaint()

  This version erases and repaints the entire widget.
*/

/*!
  Repaints the widget directly by calling paintEvent() immediately,
  unless updates are disabled or the widget is hidden.

  If \a erase is TRUE, Qt erases the area \a (x,y,w,h) before the
  paintEvent() call.

  If \a w is negative, it is replaced with <code>width() - x</code>,
  and if \a h is negative, it is replaced width <code>height() -
  y</code>.

  We suggest using repaint() if you need an immediate repaint, for
  example during animation. In almost all circumstances update() is
  better, as it permits Qt to optimize for speed and against flicker.

  \warning If you call repaint() in a function which may itself be
  called from paintEvent(), you may see infinite recursion. The
  update() function never generates recursion.

  \sa update(), paintEvent(), setUpdatesEnabled(), erase()
*/

void QWidget::repaint( int x, int y, int w, int h, bool erase )
{
    if ( (widget_state & (WState_Visible|WState_BlockUpdates)) == WState_Visible ) {
	if ( x > crect.width() || y > crect.height() )
	    return;
	if ( w < 0 )
	    w = crect.width()  - x;
	if ( h < 0 )
	    h = crect.height() - y;
	QRect r(x,y,w,h);
	if ( r.isEmpty() )
	    return; // nothing to do
	QPaintEvent e( r, erase );
	if ( r != rect() )
	    qt_set_paintevent_clipping( this, r );
	if ( erase && w != 0 && h != 0 ) {
	    if ( backgroundOrigin() == WidgetOrigin )
		XClearArea( x11Display(), winId(), x, y, w, h, False );
	    else
		this->erase( x, y, w, h);
	}
	QApplication::sendEvent( this, &e );
	qt_clear_paintevent_clipping();
    }
}

/*!
    \overload
  Repaints the widget directly by calling paintEvent() directly,
  unless updates are disabled or the widget is hidden.

  Erases the widget region  \a reg if \a erase is TRUE.

  Use repaint if your widget needs to be repainted immediately, for
  example when doing some animation. In all other cases, update() is
  to be preferred. Calling update() many times in a row will generate
  a single paint event.

  \warning If you call repaint() in a function which may itself be called
  from paintEvent(), you may see infinite recursion. The update() function
  never generates recursion.

  \sa update(), paintEvent(), setUpdatesEnabled(), erase()
*/

void QWidget::repaint( const QRegion& reg, bool erase )
{
    if ( (widget_state & (WState_Visible|WState_BlockUpdates)) == WState_Visible ) {
	QPaintEvent e( reg );
	qt_set_paintevent_clipping( this, reg );
	if ( erase )
	    this->erase(reg);
	QApplication::sendEvent( this, &e );
	qt_clear_paintevent_clipping();
    }
}

/*!
  \overload void QWidget::repaint( const QRect &r, bool erase )

  Repaints the widget directly by calling paintEvent() directly,
  unless updates are disabled or the widget is hidden.

  Erases the widget region \a r if \a erase is TRUE.
*/


/*!
  \internal
  Platform-specific part of QWidget::show().
*/

void QWidget::showWindow()
{

    if ( isTopLevel()  ) {
	int sm = topData()->showMode;
	if ( sm ) { // handles minimize and reset
	    XWMHints *h = XGetWMHints( x11Display(), winId() );
	    XWMHints  wm_hints;
	    bool got_hints = h != 0;
	    if ( !got_hints ) {
		h = &wm_hints;
		h->flags = 0;
	    }
	    h->initial_state = sm == 1? IconicState : NormalState;
	    h->flags |= StateHint;
	    XSetWMHints( x11Display(), winId(), h );
	    if ( got_hints )
		XFree( (char *)h );
	    topData()->showMode = sm == 1?3:0; // trigger reset to normal state next time
	}
	if ( topData()->parentWinId &&
	     topData()->parentWinId != QPaintDevice::x11AppRootWindow(x11Screen()) &&
	     !isMinimized() ) {
	    qt_deferred_map_add( this );
	    return;
	}
    }
    XMapWindow( x11Display(), winId() );
}


/*!
  \internal
  Platform-specific part of QWidget::hide().
*/

void QWidget::hideWindow()
{
    clearWState( WState_Exposed );
    deactivateWidgetCleanup();
    if ( isTopLevel() ) {
	qt_deferred_map_take( this );
	if ( winId() ) // in nsplugin, may be 0
	    XWithdrawWindow( x11Display(), winId(), x11Screen() );

	QTLWExtra *top = topData();
	crect.moveTopLeft( QPoint(crect.x() - top->fleft, crect.y() - top->ftop ) );

	// zero the frame strut and mark it dirty
	top->fleft = top->fright = top->ftop = top->fbottom = 0;
	fstrut_dirty = TRUE;

	XFlush( x11Display() );
    } else {
	if ( winId() ) // in nsplugin, may be 0
	    XUnmapWindow( x11Display(), winId() );
    }
}


/*!
  Shows the widget minimized, as an icon.

  Calling this function has no effect for other than \link isTopLevel()
  top-level widgets\endlink.

  \sa showNormal(), showMaximized(), show(), hide(), isVisible(), isMinimized()
*/

void QWidget::showMinimized()
{
    if ( isTopLevel() ) {
	if ( isVisible() )
	    XIconifyWindow( x11Display(), winId(), x11Screen() );
	else {
	    topData()->showMode = 1;
	    show();
	    clearWState( WState_Visible );
	    sendHideEventsToChildren(TRUE);
	}
    }
    QEvent e( QEvent::ShowMinimized );
    QApplication::sendEvent( this, &e );
    clearWState( WState_Maximized );
    setWState( WState_Minimized );
}

/*! \property QWidget::minimized
    \brief whether this widget is minimized (iconified)

  This property is relevant only for top-level widgets.

  \sa showMinimized(), isVisible(), show(), hide(), showNormal()
 */
bool QWidget::isMinimized() const
{
    // true for non-toplevels that have the minimized flag, e.g. MDI children
    return qt_wstate_iconified( winId() ) || ( !isTopLevel() && testWState( WState_Minimized ) );
}

bool QWidget::isMaximized() const
{
    return testWState(WState_Maximized);
}

// ### ### this really needs to wait for a maximum of 0.N seconds
void qt_wait_for_window_manager( QWidget* w )
{
    QApplication::flushX();
    XEvent ev;
    while (!XCheckTypedWindowEvent( w->x11Display(), w->winId(), ReparentNotify, &ev )) {
	if ( XCheckTypedWindowEvent( w->x11Display(), w->winId(), MapNotify, &ev ) )
	    break;
    }
    qApp->x11ProcessEvent( &ev );
    if ( XCheckTypedWindowEvent( w->x11Display(), w->winId(), ConfigureNotify, &ev ) )
	qApp->x11ProcessEvent( &ev );
}

/*!
  Shows the widget maximized.

  Calling this function has no effect for other than \link isTopLevel()
  top-level widgets\endlink.

  On X11, this function may not work properly with certain window
  managers. See the \link geometry.html Window Geometry
  documentation\endlink for details on why.

  \sa showNormal(), showMinimized(), show(), hide(), isVisible()
*/

void QWidget::showMaximized()
{
    if ( isTopLevel() ) {
	Display *dpy = x11Display();

	if ( qt_net_supports(qt_net_wm_state_max_h) &&
	    qt_net_supports(qt_net_wm_state_max_v)) {
	    // we have a NET supporting window manager
	    long net_winstates[3] = { 0, 0, 0 };
	    int curr_winstate = 0;

	    // keep modal state if it is set
	    if (testWFlags(WShowModal))
		net_winstates[curr_winstate++] = qt_net_wm_state_modal;

	    // set maximized states
	    net_winstates[curr_winstate++] = qt_net_wm_state_max_v;
	    net_winstates[curr_winstate++] = qt_net_wm_state_max_h;

	    XChangeProperty(dpy, winId(), qt_net_wm_state, XA_ATOM, 32, PropModeReplace,
			    (unsigned char *) net_winstates, curr_winstate);
	} else {
	    int scr = x11Screen();
	    int sw = DisplayWidth(dpy,scr);
	    int sh = DisplayHeight(dpy,scr);
	    if ( topData()->normalGeometry.width() < 0 )
		topData()->normalGeometry = geometry();
	    if ( !topData()->parentWinId && !isVisible() ) {
		setGeometry(0, 0, sw, sh );
		show();
		qt_wait_for_window_manager( this );
		updateFrameStrut();
	    }
	    if ( width() == sw && height() == sh ) {
		// the wm was not smart enough to adjust our size, do that manually
		sw -= frameGeometry().width() - width();
		sh -= frameGeometry().height() - height();
		resize( sw, sh );
	    }
	}
    }
    show();
    QEvent e( QEvent::ShowMaximized );
    QApplication::sendEvent( this, &e );
    clearWState( WState_Minimized );
    setWState(WState_Maximized);
}

/*!
  Restores the widget after it has been maximized or minimized.

  Calling this function has no effect for other than \link isTopLevel()
  top-level widgets\endlink.

  \sa showMinimized(), showMaximized(), show(), hide(), isVisible()
*/

void QWidget::showNormal()
{
    if ( isTopLevel() ) {
	if ( topData()->fullscreen ) {
	    // when reparenting, preserve some widget flags
	    reparent( 0, WType_TopLevel | (getWFlags() & 0xffff0000), QPoint(0,0) );
	    topData()->fullscreen = 0;
	}
	QRect r = topData()->normalGeometry;
	if ( r.width() >= 0 ) {
	    // the widget has been maximized
	    topData()->normalGeometry = QRect(0,0,-1,-1);
	    resize( r.size() );
	    move( r.topLeft() );
	}
    }
    show();
    QEvent e( QEvent::ShowNormal );
    QApplication::sendEvent( this, &e );
    clearWState( WState_Minimized | WState_Maximized );
}


/*!
  Raises this widget to the top of the parent widget's stack.

  If there are any siblings of this widget that overlap it on the screen,
  this widget will be visually in front of its siblings afterwards.

  \sa lower(), stackUnder()
*/

void QWidget::raise()
{
    QWidget *p = parentWidget();
    if ( p && p->childObjects && p->childObjects->findRef(this) >= 0 )
	p->childObjects->append( p->childObjects->take() );
    XRaiseWindow( x11Display(), winId() );
}

/*!
  Lowers the widget to the bottom of the parent widget's stack.

  If there are siblings of this widget that overlap it on the screen, this
  widget will be obscured by its siblings afterwards.

  \sa raise(), stackUnder()
*/

void QWidget::lower()
{
    QWidget *p = parentWidget();
    if ( p && p->childObjects && p->childObjects->findRef(this) >= 0 )
	p->childObjects->insert( 0, p->childObjects->take() );
    XLowerWindow( x11Display(), winId() );
}


/*!
  Places the widget under \a w in the parent widget's stack.

  To make this work, the widget itself and \a w have to be siblings.

  \sa raise(), lower()
*/
void QWidget::stackUnder( QWidget* w)
{
    QWidget *p = parentWidget();
    if ( !w || isTopLevel() || p != w->parentWidget() || this == w )
	return;
    if ( p && p->childObjects && p->childObjects->findRef(w) >= 0 && p->childObjects->findRef(this) >= 0 ) {
	p->childObjects->take();
	p->childObjects->insert( p->childObjects->findRef(w), this );
    }
    Window stack[2];
    stack[0] = w->winId();;
    stack[1] = winId();
    XRestackWindows( x11Display(), stack, 2 );
}



/*
  The global variable qt_widget_tlw_gravity defines the window gravity of
  the next top level window to be created. We do this when setting the
  main widget's geometry and the "-geometry" command line option contains
  a negative position.
*/

// ### this should be set to NorthEast for reversed layout!
int qt_widget_tlw_gravity = NorthWestGravity;

static void do_size_hints( QWidget* widget, QWExtra *x )
{
    XSizeHints s;
    s.flags = 0;
    if ( x ) {
	s.x = widget->x();
	s.y = widget->y();
	s.width = widget->width();
	s.height = widget->height();
	if ( x->minw > 0 || x->minh > 0 ) {	// add minimum size hints
	    s.flags |= PMinSize;
	    s.min_width  = x->minw;
	    s.min_height = x->minh;
	}
	if ( x->maxw < QWIDGETSIZE_MAX || x->maxh < QWIDGETSIZE_MAX ) {
	    s.flags |= PMaxSize;		// add maximum size hints
	    s.max_width  = x->maxw;
	    s.max_height = x->maxh;
	}
	if ( x->topextra &&
	   (x->topextra->incw > 0 || x->topextra->inch > 0) )
	{					// add resize increment hints
	    s.flags |= PResizeInc | PBaseSize;
	    s.width_inc = x->topextra->incw;
	    s.height_inc = x->topextra->inch;
	    s.base_width = x->topextra->basew;
	    s.base_height = x->topextra->baseh;
	}

	if ( x->topextra && x->topextra->uspos) {
	    s.flags |= USPosition;
	    s.flags |= PPosition;
	}
	if ( x->topextra && x->topextra->ussize) {
	    s.flags |= USSize;
	    s.flags |= PSize;
	}
    }
    s.flags |= PWinGravity;
    s.win_gravity = qt_widget_tlw_gravity;	// usually NorthWest
    // ### should this be set to NE for right to left languages.
    qt_widget_tlw_gravity = NorthWestGravity;	// reset in case it was set
    XSetWMNormalHints( widget->x11Display(), widget->winId(), &s );
}


void QWidget::internalSetGeometry( int x, int y, int w, int h, bool isMove )
{
    Display *dpy = x11Display();

    if ( testWFlags(WType_Desktop) )
	return;
    clearWState(WState_Maximized);
    if ( extra ) {				// any size restrictions?
	w = QMIN(w,extra->maxw);
	h = QMIN(h,extra->maxh);
	w = QMAX(w,extra->minw);
	h = QMAX(h,extra->minh);
    }
    if ( w < 1 )				// invalid size
	w = 1;
    if ( h < 1 )
	h = 1;
    QPoint oldPos( pos() );
    QSize oldSize( size() );
    QRect oldGeom( crect );
    QRect  r( x, y, w, h );

    // We only care about stuff that changes the geometry, or may
    // cause the window manager to change its state
    if ( !isTopLevel() && oldGeom == r )
	return;

    crect = r;
    bool isResize = size() != oldSize;

    if ( isTopLevel() ) {
	if ( isMove )
	    topData()->uspos = 1;
	if ( isResize )
	    topData()->ussize = 1;
	do_size_hints( this, extra );
    }

    if ( isMove ) {
	if (! qt_broken_wm)
	    // pos() is right according to ICCCM 4.1.5
	    XMoveResizeWindow( dpy, winid, pos().x(), pos().y(), w, h );
	else
	    // work around 4Dwm's incompliance with ICCCM 4.1.5
	    XMoveResizeWindow( dpy, winid, x, y, w, h );
    } else if ( isResize )
	XResizeWindow( dpy, winid, w, h );

    if ( isVisible() ) {
	if ( isMove && pos() != oldPos ) {
	    if ( ! qt_broken_wm ) {
		// pos() is right according to ICCCM 4.1.5
		QMoveEvent e( pos(), oldPos );
		QApplication::sendEvent( this, &e );
	    } else {
		// work around 4Dwm's incompliance with ICCCM 4.1.5
		QMoveEvent e( crect.topLeft(), oldGeom.topLeft() );
		QApplication::sendEvent( this, &e );
	    }
	}
	if ( isResize ) {

	    // set config pending only on resize, see qapplication_x11.cpp, translateConfigEvent()
	    setWState( WState_ConfigPending );

	    QResizeEvent e( size(), oldSize );
	    QApplication::sendEvent( this, &e );
	}
    } else {
	if ( isMove && pos() != oldPos ) {
	    if ( ! qt_broken_wm )
		// pos() is right according to ICCCM 4.1.5
		QApplication::postEvent( this, new QMoveEvent( pos(), oldPos ) );
	    else
		// work around 4Dwm's incompliance with ICCCM 4.1.5
		QApplication::postEvent( this, new QMoveEvent( crect.topLeft(),
							       oldGeom.topLeft() ) );
	}
	if ( isResize )
	    QApplication::postEvent( this,
				     new QResizeEvent( size(), oldSize ) );
    }
}


/*!
  \overload

  This function corresponds to setMinimumSize( QSize(minw, minh) ).
  Sets the minimum width to \a minw and the minimum height to \a minh.
*/

void QWidget::setMinimumSize( int minw, int minh )
{
#if defined(QT_CHECK_RANGE)
    if ( minw < 0 || minh < 0 )
	qWarning("QWidget::setMinimumSize: The smallest allowed size is (0,0)");
#endif
    createExtra();
    if ( extra->minw == minw && extra->minh == minh )
	return;
    extra->minw = minw;
    extra->minh = minh;
    if ( minw > width() || minh > height() ) {
	bool resized = testWState( WState_Resized );
	resize( QMAX(minw,width()), QMAX(minh,height()) );
	if ( !resized )
	    clearWState( WState_Resized ); // not a user resize
    }
    if ( testWFlags(WType_TopLevel) )
	do_size_hints( this, extra );
    updateGeometry();
}

/*!
  \overload

  This function corresponds to setMaximumSize( QSize(\a maxw, \a maxh) ).
  Sets the maximum width to \a maxw and the maximum height to \a maxh.
*/
void QWidget::setMaximumSize( int maxw, int maxh )
{
#if defined(QT_CHECK_RANGE)
    if ( maxw > QWIDGETSIZE_MAX || maxh > QWIDGETSIZE_MAX ) {
	qWarning("QWidget::setMaximumSize: (%s/%s) "
		"The largest allowed size is (%d,%d)",
		 name( "unnamed" ), className(), QWIDGETSIZE_MAX,
		QWIDGETSIZE_MAX );
	maxw = QMIN( maxw, QWIDGETSIZE_MAX );
	maxh = QMIN( maxh, QWIDGETSIZE_MAX );
    }
    if ( maxw < 0 || maxh < 0 ) {
	qWarning("QWidget::setMaximumSize: (%s/%s) Negative sizes (%d,%d) "
		"are not possible",
		name( "unnamed" ), className(), maxw, maxh );
	maxw = QMAX( maxw, 0 );
	maxh = QMAX( maxh, 0 );
    }
#endif
    createExtra();
    if ( extra->maxw == maxw && extra->maxh == maxh )
	return;
    extra->maxw = maxw;
    extra->maxh = maxh;
    if ( maxw < width() || maxh < height() ) {
	bool resized = testWState( WState_Resized );
	resize( QMIN(maxw,width()), QMIN(maxh,height()) );
	if ( !resized )
	    clearWState( WState_Resized ); // not a user resize
    }
    if ( testWFlags(WType_TopLevel) )
	do_size_hints( this, extra );
    updateGeometry();
}

/*! \overload

    Sets the x (width) size increment to \a w and the y (height) size
    increment to \a h.
*/
void QWidget::setSizeIncrement( int w, int h )
{
    QTLWExtra* x = topData();
    if ( x->incw == w && x->inch == h )
	return;
    x->incw = w;
    x->inch = h;
    if ( testWFlags(WType_TopLevel) )
	do_size_hints( this, extra );
}

/*!
  \overload

  This corresponds to setBaseSize( QSize(\a basew, \a baseh) ).
  Sets the widgets base size to width \a basew and height \a baseh.
*/
void QWidget::setBaseSize( int basew, int baseh )
{
    createTLExtra();
    QTLWExtra* x = topData();
    if ( x->basew == basew && x->baseh == baseh )
	return;
    x->basew = basew;
    x->baseh = baseh;
    if ( testWFlags(WType_TopLevel) )
	do_size_hints( this, extra );
}

/*!
  \overload void QWidget::erase()
  This version erases the entire widget.
*/

/*!
  \overload void QWidget::erase( const QRect &r )
  Erases the specified area \a r in the widget without generating
  a \link paintEvent() paint event\endlink.
*/

/*!
  Erases the specified area \a (x, y, w, h) in the widget without generating
  a \link paintEvent() paint event\endlink.

  If \a w is negative, it is replaced with \c{width() - x}.
  If \a h is negative, it is replaced width <code>height() - y</code>.

  Child widgets are not affected.

  \sa repaint()
*/

void QWidget::erase( int x, int y, int w, int h )
{
    extern void qt_erase_rect( QWidget*, const QRect& ); // in qpainer_x11.cpp
    if ( w < 0 )
	w = crect.width()  - x;
    if ( h < 0 )
	h = crect.height() - y;
    if ( w != 0 && h != 0 )
	qt_erase_rect( this, QRect(x, y, w, h ) );
}

/*!
    \overload
  Erases the area defined by \a reg, without generating a
  \link paintEvent() paint event\endlink.

  Child widgets are not affected.
*/

void QWidget::erase( const QRegion& reg )
{
    extern void qt_erase_region( QWidget*, const QRegion& ); // in qpainer_x11.cpp
    qt_erase_region( this, reg );
}

/*! Scrolls the widget including its children \a dx pixels to the right and \a dy
  downwards.  Both \a dx and \a dy may be negative.

  After scrolling, scroll() sends a paint event for the the part that is read but
  not written.  For example, when scrolling 10 pixels rightwards, the leftmost
  ten pixels of the widget need repainting. The paint event may be delivered
  immediately or later, depending on some heuristics.

  \sa QScrollView erase() bitBlt()
*/

void QWidget::scroll( int dx, int dy )
{
    scroll( dx, dy, QRect() );
}

/*! \overload

  This version only scrolls \a r and does not move the children of the widget.

  If \a r is empty or invalid, the result is undefined.

  \sa QScrollView erase() bitBlt()
*/
void QWidget::scroll( int dx, int dy, const QRect& r )
{
    if ( testWState( WState_BlockUpdates ) )
	return;
    bool valid_rect = r.isValid();
    bool just_update = QABS( dx ) > width() || QABS( dy ) > height();
    if ( just_update )
	update();
    QRect sr = valid_rect?r:visibleRect();
    int x1, y1, x2, y2, w=sr.width(), h=sr.height();
    if ( dx > 0 ) {
	x1 = sr.x();
	x2 = x1+dx;
	w -= dx;
    } else {
	x2 = sr.x();
	x1 = x2-dx;
	w += dx;
    }
    if ( dy > 0 ) {
	y1 = sr.y();
	y2 = y1+dy;
	h -= dy;
    } else {
	y2 = sr.y();
	y1 = y2-dy;
	h += dy;
    }

    if ( dx == 0 && dy == 0 )
	return;

    Display *dpy = x11Display();
    GC gc = qt_xget_readonly_gc( x11Screen(), FALSE );
    // Want expose events
    if ( w > 0 && h > 0 && !just_update ) {
	XSetGraphicsExposures( dpy, gc, True );
	XCopyArea( dpy, winId(), winId(), gc, x1, y1, w, h, x2, y2);
	XSetGraphicsExposures( dpy, gc, False );
    }

    if ( !valid_rect && children() ) {	// scroll children
	QPoint pd( dx, dy );
	QObjectListIt it(*children());
	register QObject *object;
	while ( it ) {				// move all children
	    object = it.current();
	    if ( object->isWidgetType() ) {
		QWidget *w = (QWidget *)object;
		w->move( w->pos() + pd );
	    }
	    ++it;
	}
    }

    if ( just_update )
	return;

    // Don't let the server be bogged-down with repaint events
    bool repaint_immediately = qt_sip_count( this ) < 3;

    if ( dx ) {
	int x = x2 == sr.x() ? sr.x()+w : sr.x();
	if ( repaint_immediately )
	    repaint( x, sr.y(), QABS(dx), sr.height(), !testWFlags(WRepaintNoErase) );
	else
	    XClearArea( dpy, winid, x, sr.y(), QABS(dx), sr.height(), True );
    }
    if ( dy ) {
	int y = y2 == sr.y() ? sr.y()+h : sr.y();
	if ( repaint_immediately )
	    repaint( sr.x(), y, sr.width(), QABS(dy), !testWFlags(WRepaintNoErase) );
	else
	    XClearArea( dpy, winid, sr.x(), y, sr.width(), QABS(dy), True );
    }

    qt_insert_sip( this, dx, dy ); // #### ignores r
}


/*!
  \overload void QWidget::drawText( const QPoint &pos, const QString& str )
  Draws the string \a str at position \a pos.
*/

/*!
  Draws the string \a str at position \a(x, y).

  The \a y position is the base line position of the text.  The text is
  drawn using the default font and the default foreground color.

  This function is provided for convenience.  You will generally get
  more flexible results and often higher speed by using a a \link
  QPainter painter\endlink instead.

  \sa setFont(), foregroundColor(), QPainter::drawText()
*/

void QWidget::drawText( int x, int y, const QString &str )
{
    if ( testWState(WState_Visible) ) {
	QPainter paint;
	paint.begin( this );
	paint.drawText( x, y, str );
	paint.end();
    }
}


/*!
  Internal implementation of the virtual QPaintDevice::metric() function.

  Use the QPaintDeviceMetrics class instead.

  \a m is the metric to get.
*/

int QWidget::metric( int m ) const
{
    int val;
    if ( m == QPaintDeviceMetrics::PdmWidth ) {
	val = crect.width();
    } else if ( m == QPaintDeviceMetrics::PdmHeight ) {
	val = crect.height();
    } else {
	Display *dpy = x11Display();
	int scr = x11Screen();
	switch ( m ) {
	    case QPaintDeviceMetrics::PdmDpiX:
	    case QPaintDeviceMetrics::PdmPhysicalDpiX:
		val = QPaintDevice::x11AppDpiX();
		break;
	    case QPaintDeviceMetrics::PdmDpiY:
	    case QPaintDeviceMetrics::PdmPhysicalDpiY:
		val = QPaintDevice::x11AppDpiY();
		break;
	    case QPaintDeviceMetrics::PdmWidthMM:
		val = (DisplayWidthMM(dpy,scr)*crect.width())/
		      DisplayWidth(dpy,scr);
		break;
	    case QPaintDeviceMetrics::PdmHeightMM:
		val = (DisplayHeightMM(dpy,scr)*crect.height())/
		      DisplayHeight(dpy,scr);
		break;
	    case QPaintDeviceMetrics::PdmNumColors:
		val = x11Cells();
		break;
	    case QPaintDeviceMetrics::PdmDepth:
		val = x11Depth();
		break;
	    default:
		val = 0;
#if defined(QT_CHECK_RANGE)
		qWarning( "QWidget::metric: Invalid metric command" );
#endif
	}
    }
    return val;
}

void QWidget::createSysExtra()
{
    extra->xDndProxy = 0;
    extra->children_use_dnd = FALSE;
}

void QWidget::deleteSysExtra()
{
}

void QWidget::createTLSysExtra()
{
    // created lazily
    extra->topextra->xic = 0;
}

void QWidget::deleteTLSysExtra()
{
    destroyInputContext();
}

/*
   examine the children of our parent up the tree and set the
   children_use_dnd extra data appropriately... this is used to keep DND enabled
   for widgets that are reparented and don't have DND enabled, BUT *DO* have
   children (or children of children ...) with DND enabled...
*/
void QWidget::checkChildrenDnd()
{
    QWidget *widget = this;
    const QObjectList *children;
    const QObject *object;
    const QWidget *child;
    while (widget && ! widget->isDesktop()) {
	// note: this isn't done for the desktop widget

	bool children_use_dnd = FALSE;
	children = widget->children();
	if ( children ) {
	    QObjectListIt it(*children);
	    while ( (object = it.current()) ) {
		++it;
		if ( object->isWidgetType() ) {
		    child = (const QWidget *) object;
		    children_use_dnd = (children_use_dnd ||
					child->acceptDrops() ||
					(child->extra &&
					 child->extra->children_use_dnd));
		}
	    }
	}

	widget->createExtra();
	widget->extra->children_use_dnd = children_use_dnd;

	widget = widget->parentWidget();
    }
}

/*! \property QWidget::acceptDrops
    \brief whether drop events are enabled for this widget

  Setting this property to TRUE announces to the system that this
  widget \e may be able to accept drop events.

  If the widgets is the desktop (QWidget::isDesktop()), this may fail
  if another application is using the desktop; you can call
  acceptDrops() to test if this occurs.
*/
bool QWidget::acceptDrops() const
{
    return testWState( WState_DND );
}

void QWidget::setAcceptDrops( bool on )
{
    if ( testWState(WState_DND) != on ) {
	if ( qt_dnd_enable( this, on ) ) {
	    if ( on )
		setWState( WState_DND );
	    else
		clearWState( WState_DND );
	}

	checkChildrenDnd();
    }
}

/*!
    \overload
  Causes only the parts of the widget which overlap \a region
  to be visible.  If the region includes pixels outside the
  rect() of the widget, window system controls in that area
  may or may not be visible, depending on the platform.

  Note that this effect can be slow if the region is particularly
  complex.

  \sa setMask(), clearMask()
*/

void QWidget::setMask( const QRegion& region )
{
    XShapeCombineRegion( x11Display(), winId(), ShapeBounding, 0, 0,
			 region.handle(), ShapeSet );
}

/*!
  Causes only the pixels of the widget for which \a bitmap
  has a corresponding 1 bit
  to be visible.  If the region includes pixels outside the
  rect() of the widget, window system controls in that area
  may or may not be visible, depending on the platform.

  Note that this effect can be slow if the region is particularly
  complex.

  \sa setMask(), clearMask()
*/

void QWidget::setMask( const QBitmap &bitmap )
{
    QBitmap bm = bitmap;
    if ( bm.x11Screen() != x11Screen() )
	bm.x11SetScreen( x11Screen() );
    XShapeCombineMask( x11Display(), winId(), ShapeBounding, 0, 0,
		       bm.handle(), ShapeSet );
}

/*!
  Removes any mask set by setMask().

  \sa setMask()
*/

void QWidget::clearMask()
{
    XShapeCombineMask( x11Display(), winId(), ShapeBounding, 0, 0,
		       None, ShapeSet );
}

/*!\reimp
 */
void QWidget::setName( const char *name )
{
    QObject::setName( name );
    if ( isTopLevel() ) {
	XChangeProperty( x11Display(), winId(),
			 qt_window_role, XA_STRING, 8, PropModeReplace,
			 (unsigned char *)name, qstrlen( name ) );
    }
}


/*!
  \internal

  Computes the frame rectangle when needed.  This is an internal function, you
  should never call this.
*/

void QWidget::updateFrameStrut() const
{
    QWidget *that = (QWidget *) this;

    if (! isVisible() || isDesktop()) {
	that->fstrut_dirty = (! isVisible());
	return;
    }

    Atom type_ret;
    Window l = winId(), w = winId(), p, r; // target window, it's parent, root
    Window *c;
    int i_unused;
    unsigned int nc;
    unsigned char *data_ret;
    unsigned long l_unused;

    while (XQueryTree(QPaintDevice::x11AppDisplay(), w, &r, &p, &c, &nc)) {
	if (c && nc > 0)
	    XFree(c);

	if (! p) {
	    qWarning("QWidget::updateFrameStrut(): ERROR - no parent");
	    return;
	}

	// if the parent window is the root window, an Enlightenment virtual root or
	// a NET WM virtual root window, stop here
	data_ret = 0;
	if (p == r ||
	    (XGetWindowProperty(QPaintDevice::x11AppDisplay(), p,
				qt_enlightenment_desktop, 0, 1, False, XA_CARDINAL,
				&type_ret, &i_unused, &l_unused, &l_unused,
				&data_ret) == Success &&
	     type_ret == XA_CARDINAL)) {
	    if (data_ret)
		XFree(data_ret);

	    break;
	} else if (qt_net_supports(qt_net_virtual_roots) && qt_net_virtual_root_list) {
	    int i = 0;
	    while (qt_net_virtual_root_list[i] != 0) {
		if (qt_net_virtual_root_list[i++] == p)
		    break;
	    }
	}

	l = w;
	w = p;
    }

    // we have our window
    int transx, transy;
    XWindowAttributes wattr;
    if (XTranslateCoordinates(QPaintDevice::x11AppDisplay(), l, w,
			      0, 0, &transx, &transy, &p) &&
	XGetWindowAttributes(QPaintDevice::x11AppDisplay(), w, &wattr)) {
	QTLWExtra *top = that->topData();
	top->fleft = transx;
	top->ftop = transy;
	top->fright = wattr.width - crect.width() - top->fleft;
	top->fbottom = wattr.height - crect.height() - top->ftop;

	// add the border_width for the window managers frame... some window managers
	// do not use a border_width of zero for their frames, and if we the left and
	// top strut, we ensure that pos() is absolutely correct.  frameGeometry()
	// will still be incorrect though... perhaps i should have foffset as well, to
	// indicate the frame offset (equal to the border_width on X).
	// - Brad
	top->fleft += wattr.border_width;
	top->fright += wattr.border_width;
	top->ftop += wattr.border_width;
	top->fbottom += wattr.border_width;
    }

    that->fstrut_dirty = 0;
}


void QWidget::createInputContext()
{
    QWidget *tlw = topLevelWidget();
    QTLWExtra *topdata = tlw->topData();

#ifndef QT_NO_XIM
    if (qt_xim) {
	if (! topdata->xic) {
	    QInputContext *qic = new QInputContext(tlw);
	    topdata->xic = (void *) qic;
	}
    } else
#endif // QT_NO_XIM
	{
	    // qDebug("QWidget::createInputContext: no xim");
	    topdata->xic = 0;
	}
}


void QWidget::destroyInputContext()
{
#ifndef QT_NO_XIM
    QInputContext *qic = (QInputContext *) extra->topextra->xic;
    delete qic;
#endif // QT_NO_XIM
    extra->topextra->xic = 0;
}


/*!
   This function is called when the user finishes input composition,
   e.g. changes focus to another widget, moves the cursor, etc.
*/
void QWidget::resetInputContext()
{
#ifndef QT_NO_XIM
    if (qt_xim_style & XIMPreeditCallbacks) {
	QWidget *tlw = topLevelWidget();
	QTLWExtra *topdata = tlw->topData();

	// trigger input context creation if it hasn't happened already
	createInputContext();

	if (topdata->xic) {
	    QInputContext *qic = (QInputContext *) topdata->xic;
	    qic->reset();
	}
    }
#endif // QT_NO_XIM
}


void QWidget::focusInputContext()
{
#ifndef QT_NO_XIM
    QWidget *tlw = topLevelWidget();
    QTLWExtra *topdata = tlw->topData();

    // trigger input context creation if it hasn't happened already
    createInputContext();

    if (topdata->xic) {
	QInputContext *qic = (QInputContext *) topdata->xic;
	qic->setFocus();
    }
#endif // QT_NO_XIM
}
