/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qwidget.cpp#34 $
**
** Implementation of QWidget class
**
** Author  : Haavard Nord
** Created : 931031
**
** Copyright (C) 1993-1995 by Troll Tech AS.  All rights reserved.
**
** --------------------------------------------------------------------------
** IMPORTANT NOTE: Widget identifier should only be set with the set_id()
** function, otherwise widget mapping will not work.
*****************************************************************************/

#define	 NO_WARNINGS
#include "qobjcoll.h"
#include "qwidget.h"
#include "qwidcoll.h"
#include "qpalette.h"
#include "qapp.h"

#if defined(DEBUG)
static char ident[] = "$Id: //depot/qt/main/src/kernel/qwidget.cpp#34 $";
#endif

/*! \class QWidget qwidget.h

  \brief QWidget is the base class of all Qt classes that represent
  on-screen objects.

  Lots of verbiage here...

*/

// --------------------------------------------------------------------------
// Internal QWidgetMapper class
//
// The purpose of this class is to map widget identifiers to QWidget objects.
// All QWidget objects register themselves in the QWidgetMapper when they
// get an identifier. Widgets unregister themselves when they change ident-
// ifier or when they are destroyed. A widget identifier is really a window
// handle.
//
// The widget mapper is created and destroyed by the main application routines
// in the file qapp_xxx.cpp.
//

static const WDictSize = 101;

class QWidgetMapper : public QWidgetIntDict
{						// maps ids -> widgets
public:
    QWidgetMapper();
   ~QWidgetMapper();
    QWidget *find( WId id );			// find widget
    void     insert( const QWidget * );		// insert widget
    bool     remove( WId id );			// remove widget
private:
    WId	     cur_id;
    QWidget *cur_widget;
};

QWidgetMapper *QWidget::mapper = 0;		// app global widget mapper

QWidget *QWidget::activeWidget = 0;		// widget in focus


QWidgetMapper::QWidgetMapper() : QWidgetIntDict(WDictSize)
{
    cur_id = 0;
    cur_widget = 0;
}

QWidgetMapper::~QWidgetMapper()
{
    clear();
}

inline QWidget *QWidgetMapper::find( WId id )
{
    if ( id != cur_id ) {			// need to lookup
	cur_widget = QWidgetIntDict::find((long)id);
	if ( cur_widget )
	    cur_id = id;
	else
	    cur_id = 0;
    }
    return cur_widget;
}

inline void QWidgetMapper::insert( const QWidget *widget )
{
    QWidgetIntDict::insert((long)widget->id(),widget);
}

inline bool QWidgetMapper::remove( WId id )
{
    if ( cur_id == id ) {			// reset current widget
	cur_id = 0;
	cur_widget = 0;
    }
    return QWidgetIntDict::remove((long)id);
}


// --------------------------------------------------------------------------
// QWidget member functions
//

/*! Constructs a new QWidget inside \e parent, named \e name,
  optionally with widget flags \e f.

  The widget flags are strictly internal; don't use them unless you
  know what you're doing.

  If \e parent is NULL, the new widget will be a window of its own.

\todo explain widget name
*/

QWidget::QWidget( QWidget *parent, const char *name, WFlags f )
	: QObject( parent, name ),
	  pal( *qApp->palette() ),		// use application palette
          fnt( TRUE )                           // create a default font
{
    initMetaObject();				// initialize meta object
    isWidget = TRUE;				// is a widget
    ident = 0;					// default attributes
    flags = f;
    extra = 0;					// no extra widget info
    create();					// platform-dependent init
}

/*! Destroys the widget.  All children of this widget are deleted
  first.  The application exits if this widget is (was) the main
  widget. */

QWidget::~QWidget()
{
    if ( QApplication::main_widget == this )	// reset main widget
	QApplication::main_widget = 0;
    if ( childObjects ) {			// widget has children
	register QObject *obj = childObjects->first();
	while ( obj ) {				// delete all child objects
	    obj->parentObj = 0;			// object should not remove
	    delete obj;				//   itself from the list
	    obj = childObjects->next();
	}
	delete childObjects;
	childObjects = 0;
    }
    destroy();					// platform-dependent cleanup
    delete extra;
}


void QWidget::createMapper()			// create widget mapper
{
    mapper = new QWidgetMapper;
    CHECK_PTR( mapper );
}

void QWidget::destroyMapper()			// destroy widget mapper
{
    if ( !mapper )				// already gone
	return;
    register QWidget *w;
    QWidgetIntDictIt it( *((QWidgetIntDict*)mapper) );
    w = it.current();
    while ( w ) {				// remove child widgets first
	if ( !w->parentObj ) {			// widget is a parent
	    delete w;
	    w = it.current();			// w will be next widget
	}
	else					// skip child widgets now
	    w = ++it;
    }
    w = it.toFirst();
    while ( w ) {				// delete the rest
	delete w;
	w = ++it;
    }
    delete mapper;
    mapper = 0;
}

void QWidget::set_id( WId id )			// set widget identifier
{
    if ( !mapper )				// mapper destroyed
	return;
    if ( ident )
	mapper->remove( ident );
    ident = id;
#if defined(_WS_X11_)
    hd = id;					// X11: hd == ident
#endif
    if ( id )
	mapper->insert( this );
}

void QWidget::createExtra()			// create extra data
{
    if ( !extra ) {				// if not exists
	extra = new QWExtra;
	CHECK_PTR( extra );
	extra->guistyle = QApplication::style();// initialize
	extra->minw = extra->minh = -1;
	extra->maxw = extra->maxh = -1;
	extra->incw = extra->inch = -1;
    }
}

QWidget *QWidget::find( WId id )		// find widget with id
{
    return mapper ? mapper->find( id ) : 0;
}


/*! Returns the widgets's GUI style.  Provided for flexibility, not
  because it's a good idea to change behaviour based on how the screen
  happens to look.

  \sa GUIStyle, setStyle() and QApplication::style(). */

GUIStyle QWidget::style() const			// get widget GUI style
{
    return extra ? extra->guistyle : QApplication::style();
}

/*! Sets the widget's GUI style.  By default, all widgets share the
  global GUI style, but it looks so cool that we just \e had to allow
  individual widgets to have their own style.  Not recommended; you're
  better off setting just the global style.

  \sa GUIStyle, style() and QApplication::setStyle(). */

void QWidget::setStyle( GUIStyle gs )		// set widget GUI style
{
    createExtra();
    extra->guistyle = gs;
}


/*! Makes the widget accept events from the window system.  \sa
  disable(), isDisabled() and colorGroup(). */

void QWidget::enable()				// enable events
{
    clearFlag( WState_Disabled );
}

/*! Makes the widget refuse events from the window system (it's grayed
  out, in effect).  \sa enable() and colorGroup(). */

void QWidget::disable()				// disable events
{
    setFlag( WState_Disabled );
}

/*!
\fn bool QWidget::isDisabled() const

Returns TRUE if the widget is disabled (grayed out), FALSE if it is
enabled. \sa enable(), disable() and colorGroup().
*/

/*!
\fn QRect QWidget::frameGeometry() const

Returns the geometry of the widget, relative to its parent and
including any frame the window manager decides to decorate the window
with. \sa geometry(), QRect, size() and rect(). */

/*!
\fn QRect QWidget::geometry() const

Returns the geometry of the widget, relative to its parent widget and
excluding frames and other decorations.  \sa frameGeometry(), QRect,
size() and rect(). */

/*!
\fn QSize QWidget::size() const

Returns the size of the widget.  This is the internal size, it doens't
include any window frames.  \sa QSize, geometry(), width(), height(),
rect(), setMinimumSize() and setMaximumSize(). */

/*!
\fn int QWidget::width() const

Returns the width of the widget, excluding any window frames.  \sa
size(), height(), rect(), geometry(). */

/*!
\fn int QWidget::height() const

Returns the height of the widget, excluding any window frames.  \sa
size(), width(), rect(), geometry(). */

/*!
\fn QRect QWidget::rect() const

Returns the the internal geometry of the widget; a rectangle whose
opposite corners are (0,0) and (width(),height()).  \sa geometry(),
width(), height(), size(), setMinimumSize() and setMaximumSize(). */

/*!
\fn WId QWidget::id() const

Returns the window system ID of the widget.  Portable in principle,
but if you use it you're probably about to do something
non-portable: Be careful. */


/*! Returns the current color group the widget belongs to.  \sa
  QColor, enable(), disable(), isDisabled(), palette() and
  setPalette(). */

const QColorGroup &QWidget::colorGroup() const	// get current colors
{
    if ( testFlag(WState_Disabled) )
	return pal.disabled();
    else
	return pal.normal();
}

/*! Returns the palette of the widget.  \sa QPalette and setPalette(). */
const QPalette &QWidget::palette() const	// get widget palette
{
    return pal;
}

/*! Sets the widget's palette, and by extension, also its background
  color.  \sa QPalette and palette(). */
void QWidget::setPalette( const QPalette &p )	// set widget palette
{
    pal = p;
    setBackgroundColor( colorGroup().background() );
    update();
}

/*! \fn QFontMetrics QWidget::fontMetrics() const

  Returns the font metrics of the font currently in use by this
  widget.  Each widget gets a default font when it's created.  \sa
  QFont, QFontMetrics, font(), setFont() and fontInfo(). */

/*!  \fn QFontInfo QWidget::fontInfo() const

  Returns the font information for the font the widget is currently
  using, ie. the closest font the window system was able to offer
  QFont.  \sa QFont, QFontInfo, font() and setFont(). */

/*! \fn bool QWidget::setMouseTracking( bool enable )

  Returns TRUE if it's past midnight.  I called it just now, it
  returned TRUE, and I'm going home. \sa bed(), sleep().  */

#if !defined(_WS_X11_)
bool QWidget::setMouseTracking( bool enable )
{
    bool v = testFlag( WMouseTracking );
    if ( onOff )
	setFlag( WMouseTracking );
    else
	clearFlag( WMouseTracking );
    return v;
}
#endif // _WS_X11_


void QWidget::setFRect( const QRect &r )	// set frect, update crect
{
    crect.setLeft( crect.left() + r.left() - frect.left() );
    crect.setTop( crect.top() + r.top() - frect.top() );
    crect.setRight( crect.right() + r.right() - frect.right() );
    crect.setBottom( crect.bottom() + r.bottom() - frect.bottom() );
    frect = r;
}

void QWidget::setCRect( const QRect &r )	// set crect, update frect
{
    frect.setLeft( frect.left() + r.left() - crect.left() );
    frect.setTop( frect.top() + r.top() - crect.top() );
    frect.setRight( frect.right() + r.right() - crect.right() );
    frect.setBottom( frect.bottom() + r.bottom() - crect.bottom() );
    crect = r;
}

/*! Translates the coordinate \e pos from relative to absolute
  coordinates.  Absolute means relative to the root window, relative
  means relative to this widget. \sa mapFromGlobal() and
  mapToParent(). */

QPoint QWidget::mapToGlobal( const QPoint &pos ) const
{						// map to global coordinates
    register QWidget *w = (QWidget*)this;
    QPoint p = pos;
    while ( w ) {
	p += w->crect.topLeft();
	w = w->parentWidget();
    }
    return p;
}

/*! Translates the coordinate \e pos from absolute to relative
  coordinates.  Absolute means relative to the root window, relative
  means relative to this widget. \sa mapToGlobal() and
  mapFromParent(). */

QPoint QWidget::mapFromGlobal( const QPoint &pos ) const
{						// map from global coordinates
    register QWidget *w = (QWidget*)this;
    QPoint p = pos;
    while ( w ) {
	p -= w->crect.topLeft();
	w = w->parentWidget();
    }
    return p;
}

/*! Translates the coordinate \e pos from this widget's to its
  parent's coordinate systems. \sa mapToGlobal() and
  mapFromParent(). */

QPoint QWidget::mapToParent( const QPoint &p ) const
{						// map to parent coordinates
    return p + crect.topLeft();
}

/*! Translates the coordinate \e pos to this widget's from its
  parent's coordinate systems. \sa mapFromGlobal() and
  mapToParent(). */

QPoint QWidget::mapFromParent( const QPoint &p ) const
{						// map from parent coordinate
    return p - crect.topLeft();
}


bool QWidget::close( bool forceKill )		// close widget
{
    QCloseEvent event;
    QApplication::sendEvent( this, &event );
    if ( event.isAccepted() || forceKill )
	delete this;
    return event.isAccepted();
}


// --------------------------------------------------------------------------
// QWidget event handling
//

class QKeyEventFriend : public QKeyEvent {	// trick to use accel flag
public:
    QKeyEventFriend() : QKeyEvent(0,0,0,0) {}
    bool didAccel() const { return accel != 0; }
    void setAccel()	  { accel = TRUE; }
};

bool QWidget::event( QEvent *e )		// receive event
{
    if ( eventFilters ) {			// pass through event filters
	if ( activate_filters( e ) )		// stopped by a filter
	    return TRUE;
    }
    switch ( e->type() ) {

	case Event_Timer:
	    timerEvent( (QTimerEvent*)e );
	    break;

	case Event_MouseMove:
	    mouseMoveEvent( (QMouseEvent*)e );
	    break;

	case Event_MouseButtonPress:
	    mousePressEvent( (QMouseEvent*)e );
	    break;

	case Event_MouseButtonRelease:
	    mouseReleaseEvent( (QMouseEvent*)e );
	    break;

	case Event_MouseButtonDblClick:
	    mouseDoubleClickEvent( (QMouseEvent*)e );
	    break;

	case Event_KeyPress: {
	    QKeyEventFriend *k = (QKeyEventFriend*)e;
	    QWidget *w = this;
	    if ( !k->didAccel() ) {
		k->setAccel();			// flag that we tried accel
		while ( w ) {			// try all parents
		    bool has_accel = w->testFlag(WHasAccel);
		    if ( has_accel && w->children() ) {
			QObjectListIt it( *w->children() );
			QObject *obj;
			bool found_accel = FALSE;
			while ( (obj=it.current()) ) {
			    if ( !obj->highPriority() )
				break;
			    if ( !obj->inherits("QAccel") )
				break;
			    found_accel = TRUE;
			    if ( obj->event( k ) )
				return TRUE;	// accel wanted it
			    ++it;
			}
			if ( !found_accel )	// accel probably removed
			    w->clearFlag( WHasAccel );
		    }
		    else if ( has_accel )	// accel but not children???
			w->clearFlag( WHasAccel );
		    w = w->parentWidget();
		}
	    }
	    keyPressEvent( k );
#if defined(_WS_X11_)
	    if ( !k->isAccepted() && !testFlag(WType_Overlap) && parentObj )
		return parentObj->event( e );	// pass event to parent
#endif
	    }
	    break;

	case Event_KeyRelease: {
	    QKeyEvent *k = (QKeyEvent*)e;
	    keyReleaseEvent( k );
#if defined(_WS_X11_)
	    if ( !k->isAccepted() && !testFlag(WType_Overlap) && parentObj )
		return parentObj->event( e );	// pass event to parent
#endif
	    }
	    break;

	case Event_FocusIn:
	    focusChangeEvent( (QFocusEvent*)e );
	    break;

	case Event_FocusOut:
	    focusChangeEvent( (QFocusEvent*)e );
	    break;

	case Event_Paint:
	    paintEvent( (QPaintEvent*)e );
	    break;

	case Event_Move:
	    moveEvent( (QMoveEvent*)e );
	    break;

	case Event_Resize:
	    resizeEvent( (QResizeEvent*)e );
	    break;

	case Event_Close: {
	    QCloseEvent *c = (QCloseEvent *)e;
	    closeEvent( c );
	    if ( !c->isAccepted() )
		return FALSE;
	    }
	    break;

	case Event_AccelInserted:
	    setFlag( WHasAccel );
	    break;

	default:
	    return FALSE;
    }
    return TRUE;
}

void QWidget::timerEvent( QTimerEvent * )
{
}

void QWidget::mouseMoveEvent( QMouseEvent * )
{
}

void QWidget::mousePressEvent( QMouseEvent * )
{
}

void QWidget::mouseReleaseEvent( QMouseEvent * )
{
}

void QWidget::mouseDoubleClickEvent( QMouseEvent *e )
{
    mousePressEvent( e );			// try mouse press event
}

void QWidget::keyPressEvent( QKeyEvent *e )
{
    e->ignore();
}

void QWidget::keyReleaseEvent( QKeyEvent *e )
{
    e->ignore();
}

void QWidget::focusChangeEvent( QFocusEvent * )
{
}

void QWidget::paintEvent( QPaintEvent * )
{
}

void QWidget::moveEvent( QMoveEvent * )
{
}

void QWidget::resizeEvent( QResizeEvent * )
{
}

void QWidget::showEvent( QShowEvent *e )
{
}

void QWidget::hideEvent( QHideEvent *e )
{
}

void QWidget::closeEvent( QCloseEvent *e )
{
}


#if defined(_WS_MAC_)

bool QWidget::macEvent( MSG * )			// Macintosh event
{
    return FALSE;
}

#elif defined(_WS_WIN_)

bool QWidget::winEvent( MSG * )			// Windows (+NT) event
{
    return FALSE;
}

#elif defined(_WS_PM_)

bool QWidget::pmEvent( QMSG * )			// OS/2 PM event
{
    return FALSE;
}

#elif defined(_WS_X11_)

bool QWidget::x11Event( XEvent * )		// X11 event
{
    return FALSE;
}

#endif
