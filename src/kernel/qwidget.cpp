/****************************************************************************
**
** Implementation of QWidget class.
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


#include "qwidget.h"
#include "qdesktopwidget.h"
#include "qevent.h"
#include "qhash.h"
#include "qptrdict.h"
#include "qcursor.h"
#include "qpixmap.h"
#include "qapplication.h"
#include "qapplication_p.h"
#include "qbrush.h"
#include "qlayout.h"
#include "qstylefactory.h"
#include "qcleanuphandler.h"
#include "qstyle.h"
#include "qmetaobject.h"
#include "qguardedptr.h"
#if defined(QT_ACCESSIBILITY_SUPPORT)
#include "qaccessible.h"
#endif
#if defined(Q_WS_WIN)
#include "qt_windows.h"
#include "qinputcontext_p.h"
#endif
#if defined(Q_WS_QWS)
#include "qwsmanager_qws.h"
#endif
#include "qfontdata_p.h"
#include "qpainter.h"

#include "qwidget_p.h"


QWidgetPrivate::~QWidgetPrivate()
{
    if ( extra )
	deleteExtra();
}

void QWidgetPrivate::propagatePaletteChange()
{
    QEvent pc(QEvent::PaletteChange);
    QApplication::sendEvent(q, &pc);
    if(!children.isEmpty()) {
	QEvent ppc(QEvent::ParentPaletteChange);
	for(int i = 0; i < children.size(); ++i) {
	    QObject *o = children.at(i);
	    if(o->isWidgetType())
		QApplication::sendEvent(o, &ppc);
	}
    }
    q->paletteChange(q->palette()); // compatibility
}

/*!
    \class QWidget qwidget.h
    \brief The QWidget class is the base class of all user interface objects.

    \ingroup abstractwidgets
    \mainclass

    The widget is the atom of the user interface: it receives mouse,
    keyboard and other events from the window system, and paints a
    representation of itself on the screen. Every widget is
    rectangular, and they are sorted in a Z-order. A widget is
    clipped by its parent and by the widgets in front of it.

    A widget that isn't embedded in a parent widget is called a
    top-level widget. Usually, top-level widgets are windows with a
    frame and a title bar (although it is also possible to create
    top-level widgets without such decoration if suitable widget flags
    are used). In Qt, QMainWindow and the various subclasses of
    QDialog are the most common top-level windows.

    A widget without a parent widget is always a top-level widget.

    Non-top-level widgets are child widgets. These are child windows
    in their parent widgets. You cannot usually distinguish a child
    widget from its parent visually. Most other widgets in Qt are
    useful only as child widgets. (It is possible to make, say, a
    button into a top-level widget, but most people prefer to put
    their buttons inside other widgets, e.g. QDialog.)

    If you want to use a QWidget to hold child widgets you will
    probably want to add a layout to the parent QWidget. (See \link
    layout.html Layouts\endlink.)

    QWidget has many member functions, but some of them have little
    direct functionality: for example, QWidget has a font property,
    but never uses this itself. There are many subclasses which
    provide real functionality, such as QPushButton, QListBox and
    QTabDialog, etc.

    \section1 Groups of functions:

    \table
    \header \i Context \i Functions

    \row \i Window functions \i
	show(),
	hide(),
	raise(),
	lower(),
	close().

    \row \i Top level windows \i
	caption(),
	setCaption(),
	icon(),
	setIcon(),
	iconText(),
	setIconText(),
	isActiveWindow(),
	setActiveWindow(),
	showMinimized().
	showMaximized(),
	showFullScreen(),
	showNormal().

    \row \i Window contents \i
	update(),
	repaint(),
	erase(),
	scroll(),
	updateMask().

    \row \i Geometry \i
	pos(),
	size(),
	rect(),
	x(),
	y(),
	width(),
	height(),
	sizePolicy(),
	setSizePolicy(),
	sizeHint(),
	updateGeometry(),
	layout(),
	move(),
	resize(),
	setGeometry(),
	frameGeometry(),
	geometry(),
	childrenRect(),
	adjustSize(),
	mapFromGlobal(),
	mapFromParent()
	mapToGlobal(),
	mapToParent(),
	maximumSize(),
	minimumSize(),
	sizeIncrement(),
	setMaximumSize(),
	setMinimumSize(),
	setSizeIncrement(),
	setBaseSize(),
	setFixedSize()

    \row \i Mode \i
	isVisible(),
	isVisibleTo(),
	isMinimized(),
	isDesktop(),
	isEnabled(),
	isEnabledTo(),
	isModal(),
	isPopup(),
	isTopLevel(),
	setEnabled(),
	hasMouseTracking(),
	setMouseTracking(),
	isUpdatesEnabled(),
	setUpdatesEnabled(),
	clipRegion().

    \row \i Look and feel \i
	style(),
	setStyle(),
	cursor(),
	setCursor()
	font(),
	setFont(),
	palette(),
	setPalette(),
	backgroundRole(),
	setBackgroundRole(),
	fontMetrics(),
	fontInfo().

    \row \i Keyboard focus<br>functions \i
	isFocusEnabled(),
	setFocusPolicy(),
	focusPolicy(),
	hasFocus(),
	setFocus(),
	clearFocus(),
	setTabOrder(),
	setFocusProxy().

    \row \i Mouse and<br>keyboard grabbing \i
	grabMouse(),
	releaseMouse(),
	grabKeyboard(),
	releaseKeyboard(),
	mouseGrabber(),
	keyboardGrabber().

    \row \i Event handlers \i
	event(),
	mousePressEvent(),
	mouseReleaseEvent(),
	mouseDoubleClickEvent(),
	mouseMoveEvent(),
	keyPressEvent(),
	keyReleaseEvent(),
	focusInEvent(),
	focusOutEvent(),
	wheelEvent(),
	enterEvent(),
	leaveEvent(),
	paintEvent(),
	moveEvent(),
	resizeEvent(),
	closeEvent(),
	dragEnterEvent(),
	dragMoveEvent(),
	dragLeaveEvent(),
	dropEvent(),
	childEvent(),
	showEvent(),
	hideEvent(),
	customEvent().

    \row \i Change handlers \i
	enabledChange(),
	fontChange(),
	paletteChange(),
	styleChange(),
	windowActivationChange().

    \row \i System functions \i
	parentWidget(),
	topLevelWidget(),
	reparent(),
	winId(),
	find(),
	metric().

    \row \i What's this help \i
	customWhatsThis()

    \row \i Internal kernel<br>functions \i
	focusNextPrevChild(),
	wmapper(),
	clearWFlags(),
	getWFlags(),
	setWFlags(),
	testWFlags().

    \endtable

    Every widget's constructor accepts two or three standard arguments:
    \list 1
    \i \c{QWidget *parent = 0} is the parent of the new widget.
    If it is 0 (the default), the new widget will be a top-level window.
    If not, it will be a child of \e parent, and be constrained by \e
    parent's geometry (unless you specify \c WType_TopLevel as
    widget flag).
    \i \c{const char *name = 0} is the widget name of the new
    widget. You can access it using name(). The widget name is little
    used by programmers but is quite useful with GUI builders such as
    \e{Qt Designer} (you can name a widget in \e{Qt Designer}, and
    connect() to it using the name in your code). The dumpObjectTree()
    debugging function also uses it.
    \i \c{WFlags f = 0} (where available) sets the widget flags; the
    default is suitable for almost all widgets, but to get, for
    example, a top-level widget without a window system frame, you
    must use special flags.
    \endlist

    The tictac/tictac.cpp example program is good example of a simple
    widget. It contains a few event handlers (as all widgets must), a
    few custom routines that are specific to it (as all useful widgets
    do), and has a few children and connections. Everything it does
    is done in response to an event: this is by far the most common way
    to design GUI applications.

    You will need to supply the content for your widgets yourself, but
    here is a brief run-down of the events, starting with the most common
    ones:

    \list

    \i paintEvent() - called whenever the widget needs to be
    repainted. Every widget which displays output must implement it,
    and it is wise \e not to paint on the screen outside
    paintEvent().

    \i resizeEvent() - called when the widget has been resized.

    \i mousePressEvent() - called when a mouse button is pressed.
    There are six mouse-related events, but the mouse press and mouse
    release events are by far the most important. A widget receives
    mouse press events when the mouse is inside it, or when it has
    grabbed the mouse using grabMouse().

    \i mouseReleaseEvent() - called when a mouse button is released.
    A widget receives mouse release events when it has received the
    corresponding mouse press event. This means that if the user
    presses the mouse inside \e your widget, then drags the mouse to
    somewhere else, then releases, \e your widget receives the release
    event. There is one exception: if a popup menu appears while the
    mouse button is held down, this popup immediately steals the mouse
    events.

    \i mouseDoubleClickEvent() - not quite as obvious as it might seem.
    If the user double-clicks, the widget receives a mouse press event
    (perhaps a mouse move event or two if they don't hold the mouse
    quite steady), a mouse release event and finally this event. It is
    \e{not possible} to distinguish a click from a double click until you've
    seen whether the second click arrives. (This is one reason why most GUI
    books recommend that double clicks be an extension of single clicks,
    rather than trigger a different action.)

    \endlist

    If your widget only contains child widgets, you probably do not need to
    implement any event handlers. If you want to detect a mouse click in
    a child widget call the child's underMouse() function inside the
    parent widget's mousePressEvent().

    Widgets that accept keyboard input need to reimplement a few more
    event handlers:

    \list

    \i keyPressEvent() - called whenever a key is pressed, and again
    when a key has been held down long enough for it to auto-repeat.
    Note that the Tab and Shift+Tab keys are only passed to the widget
    if they are not used by the focus-change mechanisms. To force those
    keys to be processed by your widget, you must reimplement
    QWidget::event().

    \i focusInEvent() - called when the widget gains keyboard focus
    (assuming you have called setFocusPolicy()). Well written widgets
    indicate that they own the keyboard focus in a clear but discreet
    way.

    \i focusOutEvent() - called when the widget loses keyboard focus.

    \endlist

    Some widgets will also need to reimplement some of the less common
    event handlers:

    \list

    \i mouseMoveEvent() - called whenever the mouse moves while a
    button is held down. This is useful for, for example, dragging. If
    you call setMouseTracking(TRUE), you get mouse move events even
    when no buttons are held down. (See also the \link dnd.html drag
    and drop\endlink information.)

    \i keyReleaseEvent() - called whenever a key is released, and also
    while it is held down if the key is auto-repeating. In that case
    the widget receives a key release event and immediately a key press
    event for every repeat. Note that the Tab and Shift+Tab keys are
    only passed to the widget if they are not used by the focus-change
    mechanisms. To force those keys to be processed by your widget, you
    must reimplement QWidget::event().

    \i wheelEvent() -- called whenever the user turns the mouse wheel
    while the widget has the focus.

    \i enterEvent() - called when the mouse enters the widget's screen
    space. (This excludes screen space owned by any children of the
    widget.)

    \i leaveEvent() - called when the mouse leaves the widget's screen
    space.

    \i moveEvent() - called when the widget has been moved relative to its
    parent.

    \i closeEvent() - called when the user closes the widget (or when
    close() is called).

    \endlist

    There are also some rather obscure events. They are listed in
    \c qevent.h and you need to reimplement event() to handle them.
    The default implementation of event() handles Tab and Shift+Tab
    (to move the keyboard focus), and passes on most other events to
    one of the more specialized handlers above.

    When implementing a widget, there are a few more things to
    consider.

    \list

    \i In the constructor, be sure to set up your member variables
    early on, before there's any chance that you might receive an event.

    \i It is almost always useful to reimplement sizeHint() and to set
    the correct size policy with setSizePolicy(), so users of your class
    can set up layout management more easily. A size policy lets you
    supply good defaults for the layout management handling, so that
    other widgets can contain and manage yours easily. sizeHint()
    indicates a "good" size for the widget.

    \i If your widget is a top-level window, setCaption() and setIcon() set
    the title bar and icon respectively.

    \endlist

    \sa QEvent, QPainter, QGridLayout, QBoxLayout
*/


QWidgetMapper *QWidget::mapper = 0;		// app global widget mapper


/*****************************************************************************
  QWidget utility functions
 *****************************************************************************/

static QFont qt_naturalWidgetFont( QWidget* w ) {
    QFont naturalfont = QApplication::font( w );
    if ( ! w->isTopLevel() ) {
	if ( ! naturalfont.isCopyOf( QApplication::font() ) )
	    naturalfont = naturalfont.resolve( w->parentWidget()->font() );
	else
	    naturalfont = w->parentWidget()->font();
    }
    return naturalfont;
}

#ifndef QT_NO_PALETTE
static QPalette qt_naturalWidgetPalette( QWidget* w ) {
    QPalette naturalpalette = QApplication::palette( w );
    if ( !w->isTopLevel() && naturalpalette.isCopyOf( QApplication::palette() ) )
	naturalpalette = w->parentWidget()->palette();
    return naturalpalette;
}
#endif


/*****************************************************************************
  QWidget member functions
 *****************************************************************************/

/*
    Widget state flags:
  \list
  \i WState_Created The widget has a valid winId().
  \i WState_Visible The widget is currently visible.
  \i WState_Hidden The widget is hidden, i.e. it won't
  become visible unless you call show() on it. WState_Hidden
  implies !WState_Visible.
  \i WState_MouseTracking Mouse tracking is enabled.
  \i WState_CompressKeys Compress keyboard events.
  \i WState_BlockUpdates Repaints and updates are disabled.
  \i WState_InPaintEvent Currently processing a paint event.
  \i WState_Reparented The widget has been reparented.
  \i WState_ConfigPending A configuration (resize/move) event is pending.
  \i WState_Resized The widget has been resized.
  \i WState_AutoMask The widget has an automatic mask, see setAutoMask().
  \i WState_DND The widget supports drag and drop, see setAcceptDrops().
  \i WState_Exposed the widget was finally exposed (X11 only,
      helps avoid paint event doubling).
  \endlist
*/

/*! \enum Qt::WFlags
    \internal */
/*! \enum Qt::WState
    \internal */

/*!
    \enum Qt::WidgetFlags

    \keyword widget flag

    This enum type is used to specify various window-system properties
    for the widget. They are fairly unusual but necessary in a few
    cases. Some of these flags depend on whether the underlying window
    manager supports them. (See the \link toplevel-example.html
    toplevel example\endlink for an explanation and example of their
    use.)

    The main types are

    \value WType_TopLevel  indicates that this widget is a top-level
    widget, usually with a window-system frame and so on.

    \value WType_Dialog  indicates that this widget is a top-level
    window that should be decorated as a dialog (i.e. typically no
    maximize or minimize buttons in the title bar). If you want to use
    it as a modal dialog it should be launched from another window, or
    have a parent and this flag should be combined with \c WShowModal.
    If you make it modal, the dialog will prevent other top-level
    windows in the application from getting any input. \c WType_Dialog
    implies \c WType_TopLevel. We refer to a top-level window that has
    a parent as a \e secondary window. (See also \c WGroupLeader.)

    \value WType_Popup  indicates that this widget is a popup
    top-level window, i.e. that it is modal, but has a window system
    frame appropriate for popup menus. \c WType_Popup implies
    WType_TopLevel.

    \value WType_Desktop  indicates that this widget is the desktop.
    See also \c WPaintDesktop below. \c WType_Desktop implies \c
    WType_TopLevel.

    There are also a number of flags which you can use to customize
    the appearance of top-level windows. These have no effect on other
    windows:

    \value WStyle_Customize  indicates that the \c WStyle_* flags
    should be used to build the window instead of the default flags.

    \value WStyle_NormalBorder  gives the window a normal border.
    This cannot be combined with \c WStyle_DialogBorder or \c
    WStyle_NoBorder.

    \value WStyle_DialogBorder  gives the window a thin dialog border.
    This cannot be combined with \c WStyle_NormalBorder or \c
    WStyle_NoBorder.

    \value WStyle_NoBorder  produces a borderless window. Note that
    the user cannot move or resize a borderless window via the window
    system. This cannot be combined with \c WStyle_NormalBorder or \c
    WStyle_DialogBorder. On Windows, the flag works fine. On X11, the
    result of the flag is dependent on the window manager and its
    ability to understand MOTIF and/or NETWM hints: most existing
    modern window managers can handle this. With \c WX11BypassWM, you
    can bypass the window manager completely. This results in a
    borderless window that is not managed at all (i.e. no keyboard
    input unless you call setActiveWindow() manually).

    \value WStyle_NoBorderEx  this value is obsolete. It has the same
    effect as using \c WStyle_NoBorder.

    \value WStyle_Title  gives the window a title bar.

    \value WStyle_SysMenu  adds a window system menu.

    \value WStyle_Minimize  adds a minimize button. Note that on
    Windows this has to be combined with \c WStyle_SysMenu for it to
    work.

    \value WStyle_Maximize  adds a maximize button. Note that on
    Windows this has to be combined with \c WStyle_SysMenu for it to work.

    \value WStyle_MinMax  is equal to \c
    WStyle_Minimize|WStyle_Maximize. Note that on Windows this has to
    be combined with \c WStyle_SysMenu to work.

    \value WStyle_ContextHelp  adds a context help button to dialogs.

    \value WStyle_Tool  makes the window a tool window. A tool window
    is often a small window with a smaller than usual title bar and
    decoration, typically used for collections of tool buttons. It
    there is a parent, the tool window will always be kept on top of
    it. If there isn't a parent, you may consider passing \c
    WStyle_StaysOnTop as well. If the window system supports it, a
    tool window can be decorated with a somewhat lighter frame. It can
    also be combined with \c WStyle_NoBorder.

    \value WStyle_StaysOnTop  informs the window system that the
    window should stay on top of all other windows. Note that on some
    window managers on X11 you also have to pass \c WX11BypassWM for
    this flag to work correctly.

    \value WStyle_Dialog  indicates that the window is a logical
    subwindow of its parent (i.e. a dialog). The window will not get
    its own taskbar entry and will be kept on top of its parent by the
    window system. Usually it will also be minimized when the parent
    is minimized. If not customized, the window is decorated with a
    slightly simpler title bar. This is the flag QDialog uses.

    \value WStyle_Splash  indicates that the window is a splash screen.
    This is the same as \c WStyle_NoBorder|WStyle_StaysOnTop|WX11BypassWM.

    Modifier flags:

    \value WDestructiveClose  makes Qt delete this widget when the
    widget has accepted closeEvent(), or when the widget tried to
    ignore closeEvent() but could not.

    \value WPaintDesktop  gives this widget paint events for the
    desktop.

    \value WPaintUnclipped  makes all painters operating on this
    widget unclipped. Children of this widget or other widgets in
    front of it do not clip the area the painter can paint on.

    \value WPaintClever  indicates that Qt should \e not try to
    optimize repainting for the widget, but instead pass on window
    system repaint events directly. (This tends to produce more events
    and smaller repaint regions.)

    \value WMouseNoMask  indicates that even if the widget has a mask,
    it wants mouse events for its entire rectangle.

    \value WStaticContents  indicates that the widget contents are
    north-west aligned and static. On resize, such a widget will
    receive paint events only for the newly visible part of itself.

    \value WNoAutoErase indicates that the widget paints all its
    pixels. Updating, resizing, scrolling and focus changes should
    therefore not erase the widget. This allows smart-repainting to
    avoid flicker.

    \value WResizeNoErase  \obsolete Use WNoAutoErase instead.
    \value WRepaintNoErase  \obsolete Use WNoAutoErase instead.
    \value WGroupLeader  makes this window a group leader. A group
    leader should \e not have a parent (i.e. it should be a top-level
    window). Any decendant windows (direct or indirect) of a group
    leader are in its group; other windows are not. If you show a
    secondary window from the group (i.e. show a window whose top-most
    parent is a group leader), that window will be modal with respect
    to the other windows in the group, but modeless with respect to
    windows in other groups.

    Miscellaneous flags

    \value WShowModal see WType_Dialog

    Internal flags.

    \value WNoMousePropagation
    \value WStaticContents
    \value WStyle_Reserved
    \value WSubWindow
    \value WType_Modal
    \value WWinOwnDC
    \value WX11BypassWM
    \value WMacNoSheet
    \value WMacDrawer
    \value WStyle_Mask
    \value WType_Mask

*/

/*!
    \enum Qt::WidgetState

    Internal flags.

    \value WState_Created
    \value WState_Visible
    \value WState_Hidden
    \value WState_MouseTracking
    \value WState_CompressKeys
    \value WState_BlockUpdates
    \value WState_InPaintEvent
    \value WState_Reparented
    \value WState_ConfigPending
    \value WState_Resized
    \value WState_AutoMask
    \value WState_DND
    \value WState_Reserved0
    \value WState_Reserved1
    \value WState_Maximized
    \value WState_Minimized
    \value WState_ForceDisabled
    \value WState_Exposed
    \value WState_OwnSizePolicy
*/


/*!
    Constructs a widget which is a child of \a parent, with the name
    \a name and widget flags set to \a f.

    If \a parent is 0, the new widget becomes a top-level window. If
    \a parent is another widget, this widget becomes a child window
    inside \a parent. The new widget is deleted when its \a parent is
    deleted.

    The \a name is sent to the QObject constructor.

    The widget flags argument, \a f, is normally 0, but it can be set
    to customize the window frame of a top-level widget (i.e. \a
    parent must be 0). To customize the frame, set the \c
    WStyle_Customize flag OR'ed with any of the \l Qt::WidgetFlags.

    If you add a child widget to an already visible widget you must
    explicitly show the child to make it visible.

    Note that the X11 version of Qt may not be able to deliver all
    combinations of style flags on all systems. This is because on
    X11, Qt can only ask the window manager, and the window manager
    can override the application's settings. On Windows, Qt can set
    whatever flags you want.

    Example:
    \code
    QLabel *splashScreen = new QLabel( 0, "mySplashScreen",
				WStyle_Customize | WStyle_Splash );
    \endcode
*/

QWidget::QWidget( QWidget *parent, const char *name, WFlags f )
    : QObject( new QWidgetPrivate, parent, name ), QPaintDevice( QInternal::Widget )
{
    if ( qApp->type() == QApplication::Tty )
	qWarning( "QWidget: Cannot create a QWidget when no GUI is being used" );

    fstrut_dirty = 1;

    isWidget = TRUE;				// is a widget
    winid = 0;					// default attributes
    widget_attributes = 0;
    widget_state = 0;
    widget_flags = f;

    focus_policy = 0;

    sizehint_forced = 0;
    is_closing = 0;
    in_show = 0;
    in_show_maximized = 0;
    im_enabled = FALSE;
    create();					// platform-dependent init
    pal = isTopLevel() ? QApplication::palette() : parentWidget()->palette();
    if ( ! isTopLevel() )
	fnt = parentWidget()->font();
#if defined(Q_WS_X11)
    fnt.x11SetScreen( x11Screen() );
#endif // Q_WS_X11

    if ( !isDesktop() )
	d->updateSystemBackground();
    if ( isTopLevel() ) {
	setWState(WState_Hidden);
	d->createTLExtra();
    } else {
	// propagate enabled state
	if ( !parentWidget()->isEnabled() )
	    setAttribute( WA_Disabled, true );
	// new widgets do not show up in already visible parents
	if (parentWidget()->isVisible())
	    setWState(WState_Hidden);
    }

    if (isTopLevel()) {
	d->focus_next = this;
    } else {
	// insert at the end of the focus chain
	QWidget *focus_handler = topLevelWidget();
	QWidget *w = focus_handler;
	while (w->d->focus_next != focus_handler)
	    w = w->d->focus_next;
	w->d->focus_next = this;
	d->focus_next = focus_handler;
    }

    if ( ++instanceCounter > maxInstances )
    	maxInstances = instanceCounter;

    // send and post remaining QObject events
    QEvent e( QEvent::Create );
    QApplication::sendEvent( this, &e );
    QApplication::postEvent(this, new QEvent(QEvent::PolishRequest));
}

/*! \internal
  ######## remove code duplication
*/
QWidget::QWidget( QWidgetPrivate *dd, QWidget* parent, const char* name, WFlags f)
    : QObject( dd, parent, name ), QPaintDevice( QInternal::Widget )
{
    if ( qApp->type() == QApplication::Tty )
	qWarning( "QWidget: Cannot create a QWidget when no GUI is being used" );

    fstrut_dirty = 1;

    isWidget = TRUE;				// is a widget
    winid = 0;					// default attributes
    widget_attributes = 0;
    widget_state = 0;
    widget_flags = f;

    focus_policy = 0;

    sizehint_forced = 0;
    is_closing = 0;
    in_show = 0;
    in_show_maximized = 0;
    im_enabled = FALSE;
    create();					// platform-dependent init
#ifndef QT_NO_PALETTE
    pal = isTopLevel() ? QApplication::palette() : parentWidget()->palette();
#endif
    if ( ! isTopLevel() )
	fnt = parentWidget()->font();
#if defined(Q_WS_X11)
    fnt.x11SetScreen( x11Screen() );
#endif // Q_WS_X11

    if ( !isDesktop() )
	d->updateSystemBackground();
    if ( isTopLevel() ) {
	setWState(WState_Hidden);
	d->createTLExtra();
    } else {
	// propagate enabled state
	if ( !parentWidget()->isEnabled() )
	    setAttribute( WA_Disabled, true );
	// new widgets do not show up in already visible parents
	if (parentWidget()->isVisible())
	    setWState(WState_Hidden);
    }

    if (isTopLevel()) {
	d->focus_next = this;
    } else {
	// insert at the end of the focus chain
	QWidget *focus_handler = topLevelWidget();
	QWidget *w = focus_handler;
	while (w->d->focus_next != focus_handler)
	    w = w->d->focus_next;
	w->d->focus_next = this;
	d->focus_next = focus_handler;
    }

    if ( ++instanceCounter > maxInstances )
    	maxInstances = instanceCounter;

    // send and post remaining QObject events
    QEvent e( QEvent::Create );
    QApplication::sendEvent( this, &e );
    QApplication::postEvent(this, new QEvent(QEvent::PolishRequest));

}


/*!
    Destroys the widget.

    All this widget's children are deleted first. The application
    exits if this widget is the main widget.
*/

QWidget::~QWidget()
{
#if defined (QT_CHECK_STATE)
    if ( paintingActive() )
	qWarning( "%s (%s): deleted while being painted", className(), name() );
#endif

    // delete layout while we still are a valid widget
    delete d->layout;

    // Remove myself focus list
    // ### Focus: maybe remove children aswell?
    QWidget *w = this;
    while (w->d->focus_next != this)
	w = w->d->focus_next;
    w->d->focus_next = d->focus_next;
    d->focus_next = 0;

    if ( QApplication::main_widget == this ) {	// reset main widget
	QApplication::main_widget = 0;
	if (qApp)
	    qApp->quit();
    }

    clearFocus();

    if ( isTopLevel() && isShown() && winId() )
	hide();

    // A parent widget must destroy all its children before destroying itself
    d->children.setAutoDelete(true);
    d->children.clear();

    QApplication::removePostedEvents( this );

    destroy();					// platform-dependent cleanup

    --instanceCounter;
}

int QWidget::instanceCounter = 0;  // Current number of widget instances
int QWidget::maxInstances = 0;     // Maximum number of widget instances


void QWidget::setWinId( WId id )		// set widget identifier
{
    if ( !mapper )				// mapper destroyed
	return;
    if ( winid )
	mapper->remove( winid );

    winid = id;
#if defined(Q_WS_X11)
    hd = id;					// X11: hd == ident
#endif
    if ( id )
	mapper->insert(winid, this);
}

void QWidgetPrivate::createTLExtra()
{
    if ( !extra )
	createExtra();
    if ( !extra->topextra ) {
	QTLWExtra* x = extra->topextra = new QTLWExtra;
#ifndef QT_NO_WIDGET_TOPEXTRA
	x->icon = 0;
#endif
	x->fleft = x->fright = x->ftop = x->fbottom = 0;
	x->incw = x->inch = 0;
	x->basew = x->baseh = 0;
	x->iconic = 0;
	x->fullscreen = 0;
	x->showMode = 0;
	x->normalGeometry = QRect(0,0,-1,-1);
#if defined(Q_WS_X11)
	x->embedded = 0;
	x->parentWinId = 0;
	x->spont_unmapped = 0;
	x->dnd = 0;
	x->uspos = 0;
	x->ussize = 0;
	x->savedFlags = 0;
#endif
#if defined(Q_WS_QWS) && !defined(QT_NO_QWS_MANAGER)
	x->decor_allocated_region = QRegion();
	x->qwsManager = 0;
#endif
	createTLSysExtra();
    }
}

/*!
  \internal
  Creates the widget extra data.
*/

void QWidgetPrivate::createExtra()
{
    if ( !extra ) {				// if not exists
	extra = new QWExtra;
	extra->minw = extra->minh = 0;
	extra->maxw = extra->maxh = QWIDGETSIZE_MAX;
#ifndef QT_NO_CURSOR
	extra->curs = 0;
#endif
	extra->topextra = 0;
	extra->bg_origin = QWidget::WidgetOrigin;
#ifndef QT_NO_STYLE
	extra->style = 0;
#endif
	extra->size_policy = QSizePolicy( QSizePolicy::Preferred,
					  QSizePolicy::Preferred );
	createSysExtra();
    }
}


/*!
  \internal
  Deletes the widget extra data.
*/

void QWidgetPrivate::deleteExtra()
{
    if ( extra ) {				// if exists
#ifndef QT_NO_CURSOR
	delete extra->curs;
#endif
	deleteSysExtra();
	if ( extra->topextra ) {
	    deleteTLSysExtra();
#ifndef QT_NO_WIDGET_TOPEXTRA
	    delete extra->topextra->icon;
#endif
#if defined(Q_WS_QWS) && !defined(QT_NO_QWS_MANAGER)
	    delete extra->topextra->qwsManager;
#endif
	    delete extra->topextra;
	}
	delete extra;
	// extra->xic destroyed in QWidget::destroy()
	extra = 0;
    }
}

/*
  Returns true if the foreground is inherited; otherwise returns
  false.

  A widget does not inherit its parents foreground if setPalette() or
  setForegroundRole() was called, unless the WA_ForegroundInherited
  attribute is set explicitely.
*/

bool QWidgetPrivate::isForegroundInherited() const
{
    return (q->testWFlags(Qt::WType_TopLevel|Qt::WSubWindow) == 0
	    && (q->testAttribute(QWidget::WA_ForegroundInherited)
		|| (!q->testAttribute(QWidget::WA_SetPalette)
		    && !q->testAttribute(QWidget::WA_SetForegroundRole)
		    && !q->testAttribute(QWidget::WA_SetBackgroundRole))));
}


/*
  Returns true if the background is inherited; otherwise returns
  false.

  A widget does not inherit its parents background if setPalette() or
  setBackgroundRole() was called, unless the WA_BackgroundInherited
  attribute is set explicitely.
*/

bool QWidgetPrivate::isBackgroundInherited() const
{
    return (q->testWFlags(Qt::WType_TopLevel|Qt::WSubWindow) == 0
	    && (q->testAttribute(QWidget::WA_BackgroundInherited)
		|| (!q->testAttribute(QWidget::WA_SetPalette)
		    && !q->testAttribute(QWidget::WA_SetBackgroundRole))));
}



/*!
  \internal
  This function is called when a widget is hidden or destroyed.
  It resets some application global pointers that should only refer active,
  visible widgets.
*/

void QWidget::deactivateWidgetCleanup()
{
    // If this was the active application window, reset it
    if ( this == QApplication::active_window )
	qApp->setActiveWindow( 0 );
    // If the is the active mouse press widget, reset it
#ifdef Q_WS_MAC
    extern QGuardedPtr<QWidget> qt_button_down;
#else
    extern QWidget *qt_button_down;
#endif
    if ( this == qt_button_down )
	qt_button_down = 0;
}


/*!
    Returns a pointer to the widget with window identifer/handle \a
    id.

    The window identifier type depends on the underlying window
    system, see \c qwindowdefs.h for the actual definition. If there
    is no widget with this identifier, 0 is returned.
*/

QWidget *QWidget::find( WId id )
{
    return mapper ? mapper->value(id, 0) : 0;
}

/*!
  \fn QWidgetMapper *QWidget::wmapper()
  \internal
  Returns a pointer to the widget mapper.

  The widget mapper is an internal dictionary that is used to map from
  window identifiers/handles to widget pointers.
  \sa find(), id()
*/

/*!
    \fn WFlags QWidget::getWFlags() const

    Returns the widget flags for this this widget.

    Widget flags are a combination of \l{Qt::WidgetFlags}.

    \sa testWFlags(), setWFlags(), clearWFlags()
*/

/*!
    \fn void QWidget::setWFlags( WFlags f )

    Sets the widget flags \a f.

    Widget flags are a combination of \l{Qt::WidgetFlags}.

    \sa testWFlags(), getWFlags(), clearWFlags()
*/

/*!
    \fn void QWidget::clearWFlags( WFlags f )

    Clears the widget flags \a f.

    Widget flags are a combination of \l{Qt::WidgetFlags}.

    \sa testWFlags(), getWFlags(), setWFlags()
*/



/*!
    \fn WId QWidget::winId() const

    Returns the window system identifier of the widget.

    Portable in principle, but if you use it you are probably about to
    do something non-portable. Be careful.

    \sa find()
*/

#ifndef QT_NO_STYLE
/*!
    Returns the GUI style for this widget

    \sa QWidget::setStyle(), QApplication::setStyle(), QApplication::style()
*/

QStyle& QWidget::style() const
{
    if ( d->extra && d->extra->style )
	return *d->extra->style;
    QStyle &ret = qApp->style();
    return ret;
}

/*!
    Sets the widget's GUI style to \a style. Ownership of the style
    object is not transferred.

    If no style is set, the widget uses the application's style,
    QApplication::style() instead.

    Setting a widget's style has no effect on existing or future child
    widgets.

    \warning This function is particularly useful for demonstration
    purposes, where you want to show Qt's styling capabilities. Real
    applications should avoid it and use one consistent GUI style
    instead.

    \sa style(), QStyle, QApplication::style(), QApplication::setStyle()
*/

void QWidget::setStyle( QStyle *style )
{
    QStyle& old  = QWidget::style();
    d->createExtra();
    d->extra->style = style;
    if ( !testWFlags(WType_Desktop) // (except desktop)
	 && d->polished) { // (and have been polished)
	old.unPolish( this );
	QWidget::style().polish( this );
    }
    styleChange( old );
}

/*!
    \overload

    Sets the widget's GUI style to \a style using the QStyleFactory.
*/
QStyle* QWidget::setStyle( const QString &style )
{
    QStyle *s = QStyleFactory::create( style );
    setStyle( s );
    return s;
}

/*!
    This virtual function is called when the style of the widgets
    changes. \a oldStyle is the previous GUI style; you can get the
    new style from style().

    Reimplement this function if your widget needs to know when its
    GUI style changes. You will almost certainly need to update the
    widget using update().

    The default implementation updates the widget including its
    geometry.

    \sa QApplication::setStyle(), style(), update(), updateGeometry()
*/

void QWidget::styleChange( QStyle& /* oldStyle */ )
{
    update();
    updateGeometry();
}

#endif

/*!
    \property QWidget::isTopLevel
    \brief whether the widget is a top-level widget

    A top-level widget is a widget which usually has a frame and a
    \link QWidget::caption caption (title)\endlink. \link
    QWidget::isPopup() Popup\endlink and \link QWidget::isDesktop()
    desktop\endlink widgets are also top-level widgets.

    A top-level widget can have a \link QWidget::parentWidget() parent
    widget\endlink. It will then be grouped with its parent and deleted
    when the parent is deleted, minimized when the parent is minimized
    etc. If supported by the window manager, it will also have a
    common taskbar entry with its parent.

    QDialog and QMainWindow widgets are by default top-level, even if
    a parent widget is specified in the constructor. This behavior is
    specified by the \c WType_TopLevel widget flag.

    \sa topLevelWidget(), isDialog(), isModal(), isPopup(), isDesktop(), parentWidget()
*/

/*!
    \property QWidget::isDialog
    \brief whether the widget is a dialog widget

    A dialog widget is a secondary top-level widget, i.e. a top-level
    widget with a parent.

    \sa isTopLevel(), QDialog
*/

/*!
    \property QWidget::isPopup
    \brief whether the widget is a popup widget

    A popup widget is created by specifying the widget flag \c
    WType_Popup to the widget constructor. A popup widget is also a
    top-level widget.

    \sa isTopLevel()
*/

/*!
    \property QWidget::isDesktop
    \brief whether the widget is a desktop widget, i.e. represents the desktop

    A desktop widget is also a top-level widget.

    \sa isTopLevel(), QApplication::desktop()
*/

/*!
    \property QWidget::isModal
    \brief whether the widget is a modal widget

    This property only makes sense for top-level widgets. A modal
    widget prevents widgets in all other top-level widgets from
    getting any input.

    \sa isTopLevel(), isDialog(), QDialog
*/

/*!
    \fn QWidget::underMouse()

    Returns true if the widget is under the mouse cursor; otherwise
    returns false.

    This value is not updated properly during drag and drop
    operations.

    \sa QEvent::Enter, QEvent::Leave
*/

/*!
    \property QWidget::minimized
    \brief whether this widget is minimized (iconified)

    This property is only relevant for top-level widgets.

    \sa showMinimized(), visible, show(), hide(), showNormal(), maximized
*/

/*!
    \property QWidget::maximized
    \brief whether this widget is maximized

    This property is only relevant for top-level widgets.

    Note that due to limitations in some window-systems, this does not
    always report the expected results (e.g. if the user on X11
    maximizes the window via the window manager, Qt has no way of
    distinguishing this from any other resize). This is expected to
    improve as window manager protocols evolve.

    \sa showMaximized(), visible, show(), hide(), showNormal(), minimized
*/

/*!
    Returns TRUE if this widget would become enabled if \a ancestor is
    enabled; otherwise returns FALSE.

    This is the case if neither the widget itself nor every parent up
    to but excluding \a ancestor has been explicitly disabled.

    isEnabledTo(0) is equivalent to isEnabled().

    \sa setEnabled() enabled
*/

bool QWidget::isEnabledTo( QWidget* ancestor ) const
{
    const QWidget * w = this;
    while ( w && !w->testAttribute(WA_ForceDisabled)
	    && !w->isTopLevel()
	    && w->parentWidget()
	    && w->parentWidget() != ancestor )
	w = w->parentWidget();
    return !w->testAttribute(WA_ForceDisabled);
}


/*!
  \fn bool QWidget::isEnabledToTLW() const
  \obsolete

  This function is deprecated. It is equivalent to isEnabled()
*/

/*!
    \property QWidget::enabled
    \brief whether the widget is enabled

    An enabled widget receives keyboard and mouse events; a disabled
    widget does not. In fact, an enabled widget only receives keyboard
    events when it is in focus.

    Some widgets display themselves differently when they are
    disabled. For example a button might draw its label grayed out. If
    your widget needs to know when it becomes enabled or disabled, you
    can reimplement the enabledChange() function.

    Disabling a widget implicitly disables all its children. Enabling
    respectively enables all child widgets unless they have been
    explicitly disabled.

    \sa isEnabled(), isEnabledTo(), QKeyEvent, QMouseEvent, enabledChange()
*/
void QWidget::setEnabled( bool enable )
{
    setAttribute(WA_ForceDisabled, !enable);
    setEnabled_helper(enable);
}

void QWidget::setEnabled_helper(bool enable)
{
    if (enable && !isTopLevel() && parentWidget() && !parentWidget()->isEnabled())
	return; // nothing we can do
    if (enable != testAttribute(WA_Disabled))
	return; // nothing to do

    setAttribute(WA_Disabled, !enable);
    d->updateSystemBackground();
    enabledChange(!enable);

    if (!enable && focusWidget() == this && (!parentWidget()||parentWidget()->isEnabled()))
	if (!focusNextPrevChild(true))
	    clearFocus();

    WidgetAttribute attribute = enable ? WA_ForceDisabled : WA_Disabled;
    for (int i = 0; i < d->children.size(); ++i) {
	QWidget *w = static_cast<QWidget *>(d->children.at(i));
	if (w->isWidgetType() && !w->testAttribute(attribute))
	    w->setEnabled_helper(enable);
    }
#if defined(Q_WS_X11)
    if ( testAttribute( WA_SetCursor ) ) {
	// enforce the windows behavior of clearing the cursor on
	// disabled widgets
	extern void qt_x11_enforce_cursor( QWidget * w ); // defined in qwidget_x11.cpp
	qt_x11_enforce_cursor( this );
    }
#endif
#ifdef Q_WS_WIN
    QInputContext::enable(this, im_enabled && enable);
#endif
}

/*!
    Disables widget input events if \a disable is TRUE; otherwise
    enables input events.

    See the \l enabled documentation for more information.

    \sa isEnabledTo(), QKeyEvent, QMouseEvent, enabledChange()
*/
void QWidget::setDisabled( bool disable )
{
    setEnabled( !disable );
}

/*!
    \fn void QWidget::enabledChange( bool oldEnabled )

    This virtual function is called from setEnabled(). \a oldEnabled
    is the previous setting; you can get the new setting from
    isEnabled().

    Reimplement this function if your widget needs to know when it
    becomes enabled or disabled. You will almost certainly need to
    update the widget using update().

    The default implementation repaints the visible part of the
    widget.

    \sa setEnabled(), isEnabled(), repaint(), update(), clipRegion()
*/

void QWidget::enabledChange( bool )
{
    update();
#if defined(QT_ACCESSIBILITY_SUPPORT)
    QAccessible::updateAccessibility( this, 0, QAccessible::StateChanged );
#endif
}

/*!
    \fn void QWidget::windowActivationChange( bool oldActive )

    This virtual function is called for a widget when its window is
    activated or deactivated by the window system. \a oldActive is the
    previous state; you can get the new setting from isActiveWindow().

    Reimplement this function if your widget needs to know when its
    window becomes activated or deactivated.

    The default implementation updates the visible part of the widget
    if the inactive and the active roles in the palette are different for colors
    other than the highlight and link colors.

    \sa setActiveWindow(), isActiveWindow(), update(), palette()
*/

void QWidget::windowActivationChange( bool )
{
#ifndef QT_NO_PALETTE
    if ( !isVisible() )
	return;

    for(int role=0; role < (int)QPalette::NColorRoles; role++) {
	if(pal.brush(QPalette::Active, (QPalette::ColorRole)role) !=
	   pal.brush(QPalette::Inactive, (QPalette::ColorRole)role)) {
	    QPalette::ColorRole bg_role = backgroundRole();
	    if ( !testAttribute(WA_NoErase) && bg_role < QPalette::NColorRoles &&
		 (role == bg_role || (role < bg_role && pal.brush(QPalette::Active, bg_role) !=
					 pal.brush(QPalette::Inactive, bg_role ))))
		d->updateSystemBackground();
	    else if(role <= QPalette::Shadow)
		update();
	    break;
	}
    }
#endif
}

/*!
    This virtual function is called when the application language has
    changed.

    Reimplement this function if your widget needs to know when the
    language changes.  You will typically need to retranslate all user
    visible strings in your reimplementation.

    The default implementation does nothing.

    \sa QTranslator
*/
void QWidget::languageChange()
{
}

/*!
    \property QWidget::frameGeometry
    \brief geometry of the widget relative to its parent including any
    window frame

    See the \link geometry.html Window Geometry documentation\endlink
    for an overview of geometry issues with top-level widgets.

    \sa geometry() x() y() pos()
*/
QRect QWidget::frameGeometry() const
{
    if (isTopLevel() && ! isPopup()) {
	if (fstrut_dirty)
	    updateFrameStrut();
	QTLWExtra *top = d->topData();
	return QRect(crect.x() - top->fleft,
		     crect.y() - top->ftop,
		     crect.width() + top->fleft + top->fright,
		     crect.height() + top->ftop + top->fbottom);
    }
    return crect;
}

/*! \property QWidget::x
    \brief the x coordinate of the widget relative to its parent including
    any window frame

    See the \link geometry.html Window Geometry documentation\endlink
    for an overview of top-level widget geometry.

    \sa frameGeometry, y, pos
*/
int QWidget::x() const
{
    if (isTopLevel() && ! isPopup()) {
	if (fstrut_dirty)
	    updateFrameStrut();
	return crect.x() - d->topData()->fleft;
    }
    return crect.x();
}

/*!
    \property QWidget::y
    \brief the y coordinate of the widget relative to its parent and
    including any window frame

    See the \link geometry.html Window Geometry documentation\endlink
    for an overview of top-level widget geometry.

    \sa frameGeometry, x, pos
*/
int QWidget::y() const
{
    if (isTopLevel() && ! isPopup()) {
	if (fstrut_dirty)
	    updateFrameStrut();
	return crect.y() - d->topData()->ftop;
    }
    return crect.y();
}

/*!
    \property QWidget::pos
    \brief the position of the widget within its parent widget

    If the widget is a top-level widget, the position is that of the
    widget on the desktop, including its frame.

    When changing the position, the widget, if visible, receives a
    move event (moveEvent()) immediately. If the widget is not
    currently visible, it is guaranteed to receive an event before it
    is shown.

    move() is virtual, and all other overloaded move() implementations
    in Qt call it.

    \warning Calling move() or setGeometry() inside moveEvent() can
    lead to infinite recursion.

    See the \link geometry.html Window Geometry documentation\endlink
    for an overview of top-level widget geometry.

    \sa frameGeometry, size x(), y()
*/
QPoint QWidget::pos() const
{
    if (isTopLevel() && ! isPopup()) {
	if (fstrut_dirty)
	    updateFrameStrut();
	QTLWExtra *top = d->topData();
	return QPoint(crect.x() - top->fleft, crect.y() - top->ftop);
    }
    return crect.topLeft();
}

/*!
    \property QWidget::geometry
    \brief the geometry of the widget relative to its parent and
    excluding the window frame

    When changing the geometry, the widget, if visible, receives a
    move event (moveEvent()) and/or a resize event (resizeEvent())
    immediately. If the widget is not currently visible, it is
    guaranteed to receive appropriate events before it is shown.

    The size component is adjusted if it lies outside the range
    defined by minimumSize() and maximumSize().

    setGeometry() is virtual, and all other overloaded setGeometry()
    implementations in Qt call it.

    \warning Calling setGeometry() inside resizeEvent() or moveEvent()
    can lead to infinite recursion.

    See the \link geometry.html Window Geometry documentation\endlink
    for an overview of top-level widget geometry.

    \sa frameGeometry(), rect(), move(), resize(), moveEvent(),
	resizeEvent(), minimumSize(), maximumSize()
*/

/*!
    \property QWidget::size
    \brief the size of the widget excluding any window frame

    When resizing, the widget, if visible, receives a resize event
    (resizeEvent()) immediately. If the widget is not currently
    visible, it is guaranteed to receive an event before it is shown.

    The size is adjusted if it lies outside the range defined by
    minimumSize() and maximumSize(). Furthermore, the size is always
    at least QSize(1, 1). For toplevel widgets, the minimum size
    might be larger, depending on the window manager.

    If you want a top-level window to have a fixed size, call
    setResizeMode( QLayout::FreeResize ) on its layout.

    resize() is virtual, and all other overloaded resize()
    implementations in Qt call it.

    \warning Calling resize() or setGeometry() inside resizeEvent() can
    lead to infinite recursion.

    \sa pos, geometry, minimumSize, maximumSize, resizeEvent()
*/

/*!
    \property QWidget::width
    \brief the width of the widget excluding any window frame

    See the \link geometry.html Window Geometry documentation\endlink
    for an overview of top-level widget geometry.

    \sa geometry, height, size
*/

/*!
    \property QWidget::height
    \brief the height of the widget excluding any window frame

    See the \link geometry.html Window Geometry documentation\endlink
    for an overview of top-level widget geometry.

    \sa geometry, width, size
*/

/*!
    \property QWidget::rect
    \brief the internal geometry of the widget excluding any window
    frame

    The rect property equals QRect(0, 0, width(), height()).

    See the \link geometry.html Window Geometry documentation\endlink
    for an overview of top-level widget geometry.

    \sa size
*/

/*!
    \property QWidget::childrenRect
    \brief the bounding rectangle of the widget's children

    Hidden children are excluded.

    \sa childrenRegion() geometry()
*/

QRect QWidget::childrenRect() const
{
    QRect r( 0, 0, 0, 0 );
    for (int i = 0; i < d->children.size(); ++i) {
	QObject *obj = d->children.at(i);
	if ( obj->isWidgetType() && !((QWidget*)obj)->isHidden() )
	    r = r.unite( ((QWidget*)obj)->geometry() );
    }
    return r;
}

/*!
    \property QWidget::childrenRegion
    \brief the combined region occupied by the widget's children

    Hidden children are excluded.

    \sa childrenRect() geometry()
*/

QRegion QWidget::childrenRegion() const
{
    QRegion r;
    for (int i = 0; i < d->children.size(); ++i) {
	QObject *obj = d->children.at(i);
	if ( obj->isWidgetType() && !((QWidget*)obj)->isHidden() )
	    r = r.unite( ((QWidget*)obj)->geometry() );
    }
    return r;
}


/*!
    \property QWidget::minimumSize
    \brief the widget's minimum size

    The widget cannot be resized to a smaller size than the minimum
    widget size. The widget's size is forced to the minimum size if
    the current size is smaller.

    If you use a layout inside the widget, the minimum size will be
    set by the layout and not by setMinimumSize(), unless you set the
    layout's resize mode to QLayout::FreeResize.

    \sa minimumWidth, minimumHeight, maximumSize, sizeIncrement
	QLayout::setResizeMode()
*/

QSize QWidget::minimumSize() const
{
    return d->extra ? QSize( d->extra->minw, d->extra->minh ) : QSize( 0, 0 );
}

/*!
    \property QWidget::maximumSize
    \brief the widget's maximum size

    The widget cannot be resized to a larger size than the maximum
    widget size.

    \sa maximumWidth(), maximumHeight(), setMaximumSize(),
    minimumSize(), sizeIncrement()
*/

QSize QWidget::maximumSize() const
{
    return d->extra ? QSize( d->extra->maxw, d->extra->maxh )
		 : QSize( QWIDGETSIZE_MAX, QWIDGETSIZE_MAX );
}


/*!
    \property QWidget::minimumWidth
    \brief the widget's minimum width

    This property corresponds to minimumSize().width().

    \sa minimumSize, minimumHeight
*/

/*!
    \property QWidget::minimumHeight
    \brief the widget's minimum height

    This property corresponds to minimumSize().height().

    \sa minimumSize, minimumWidth
*/

/*!
    \property QWidget::maximumWidth
    \brief the widget's maximum width

    This property corresponds to maximumSize().width().

    \sa maximumSize, maximumHeight
*/

/*!
    \property QWidget::maximumHeight
    \brief the widget's maximum height

    This property corresponds to maximumSize().height().

    \sa maximumSize, maximumWidth
*/

/*!
    \property QWidget::sizeIncrement
    \brief the size increment of the widget

    When the user resizes the window, the size will move in steps of
    sizeIncrement().width() pixels horizontally and
    sizeIncrement.height() pixels vertically, with baseSize() as the
    basis. Preferred widget sizes are for non-negative integers \e i
    and \e j:
    \code
	width = baseSize().width() + i * sizeIncrement().width();
	height = baseSize().height() + j * sizeIncrement().height();
    \endcode

    Note that while you can set the size increment for all widgets, it
    only affects top-level widgets.

    \warning The size increment has no effect under Windows, and may
    be disregarded by the window manager on X.

    \sa size, minimumSize, maximumSize
*/
QSize QWidget::sizeIncrement() const
{
    return ( d->extra && d->extra->topextra )
	? QSize( d->extra->topextra->incw, d->extra->topextra->inch )
	: QSize( 0, 0 );
}

/*!
    \property QWidget::baseSize
    \brief the base size of the widget

    The base size is used to calculate a proper widget size if the
    widget defines sizeIncrement().

    \sa setSizeIncrement()
*/

QSize QWidget::baseSize() const
{
    return ( d->extra != 0 && d->extra->topextra != 0 )
	? QSize( d->extra->topextra->basew, d->extra->topextra->baseh )
	: QSize( 0, 0 );
}

/*!
    Sets both the minimum and maximum sizes of the widget to \a s,
    thereby preventing it from ever growing or shrinking.

    \sa setMaximumSize() setMinimumSize()
*/

void QWidget::setFixedSize( const QSize & s)
{
    setMinimumSize( s );
    setMaximumSize( s );
    resize( s );
}


/*!
    \overload void QWidget::setFixedSize( int w, int h )

    Sets the width of the widget to \a w and the height to \a h.
*/

void QWidget::setFixedSize( int w, int h )
{
    setMinimumSize( w, h );
    setMaximumSize( w, h );
    resize( w, h );
}

void QWidget::setMinimumWidth( int w )
{
    setMinimumSize( w, minimumSize().height() );
}

void QWidget::setMinimumHeight( int h )
{
    setMinimumSize( minimumSize().width(), h );
}

void QWidget::setMaximumWidth( int w )
{
    setMaximumSize( w, maximumSize().height() );
}

void QWidget::setMaximumHeight( int h )
{
    setMaximumSize( maximumSize().width(), h );
}

/*!
    Sets both the minimum and maximum width of the widget to \a w
    without changing the heights. Provided for convenience.

    \sa sizeHint() minimumSize() maximumSize() setFixedSize()
*/

void QWidget::setFixedWidth( int w )
{
    setMinimumSize( w, minimumSize().height() );
    setMaximumSize( w, maximumSize().height() );
}


/*!
    Sets both the minimum and maximum heights of the widget to \a h
    without changing the widths. Provided for convenience.

    \sa sizeHint() minimumSize() maximumSize() setFixedSize()
*/

void QWidget::setFixedHeight( int h )
{
    setMinimumSize( minimumSize().width(), h );
    setMaximumSize( maximumSize().width(), h );
}


/*!
    Translates the widget coordinate \a pos to the coordinate system
    of \a parent. The \a parent must not be 0 and must be a parent
    of the calling widget.

    \sa mapFrom() mapToParent() mapToGlobal() underMouse()
*/

QPoint QWidget::mapTo( QWidget * parent, const QPoint & pos ) const
{
    QPoint p = pos;
    if ( parent ) {
	const QWidget * w = this;
	while ( w != parent ) {
	    p = w->mapToParent( p );
	    w = w->parentWidget();
	}
    }
    return p;
}


/*!
    Translates the widget coordinate \a pos from the coordinate system
    of \a parent to this widget's coordinate system. The \a parent
    must not be 0 and must be a parent of the calling widget.

    \sa mapTo() mapFromParent() mapFromGlobal() underMouse()
*/

QPoint QWidget::mapFrom( QWidget * parent, const QPoint & pos ) const
{
    QPoint p( pos );
    if ( parent ) {
	const QWidget * w = this;
	while ( w != parent ) {
	    p = w->mapFromParent( p );
	    w = w->parentWidget();
	}
    }
    return p;
}


/*!
    Translates the widget coordinate \a pos to a coordinate in the
    parent widget.

    Same as mapToGlobal() if the widget has no parent.

    \sa mapFromParent() mapTo() mapToGlobal() underMouse()
*/

QPoint QWidget::mapToParent( const QPoint &pos ) const
{
    return pos + crect.topLeft();
}

/*!
    Translates the parent widget coordinate \a pos to widget
    coordinates.

    Same as mapFromGlobal() if the widget has no parent.

    \sa mapToParent() mapFrom() mapFromGlobal() underMouse()
*/

QPoint QWidget::mapFromParent( const QPoint &pos ) const
{
    return pos - crect.topLeft();
}


/*!
    Returns the top-level widget for this widget, i.e. the next
    ancestor widget that has (or could have) a window-system frame.

    If the widget is a top-level, the widget itself is returned.

    Typical usage is changing the window caption:

    \code
	aWidget->topLevelWidget()->setCaption( "New Caption" );
    \endcode

    \sa isTopLevel()
*/

QWidget *QWidget::topLevelWidget() const
{
    QWidget *w = (QWidget *)this;
    QWidget *p = w->parentWidget();
    while ( !w->testWFlags(WType_TopLevel) && p ) {
	w = p;
	p = p->parentWidget();
    }
    return w;
}

#ifndef QT_NO_COMPAT
Qt::BackgroundMode QWidget::backgroundMode() const
{
    if (testAttribute(WA_NoErase))
	return NoBackground;
    switch(backgroundRole()) {
    case QPalette::Foreground:
	return PaletteForeground;
    case QPalette::Button:
	return PaletteButton;
    case QPalette::Light:
	return PaletteLight;
    case QPalette::Midlight:
	return PaletteMidlight;
    case QPalette::Dark:
	return PaletteDark;
    case QPalette::Mid:
	return PaletteMid;
    case QPalette::Text:
	return PaletteText;
    case QPalette::BrightText:
	return PaletteBrightText;
    case QPalette::Base:
	return PaletteBase;
    case QPalette::Background:
	return PaletteBackground;
    case QPalette::Shadow:
	return PaletteShadow;
    case QPalette::Highlight:
	return PaletteHighlight;
    case QPalette::HighlightedText:
	return PaletteHighlightedText;
    case QPalette::ButtonText:
	return PaletteButtonText;
    case QPalette::Link:
	return PaletteLink;
    case QPalette::LinkVisited:
	return PaletteLinkVisited;
    default:
	break;
    }
    return NoBackground;
}

void QWidget::setBackgroundMode( BackgroundMode m )
{
    if(m == NoBackground) {
	setAttribute(WA_NoErase, true);
	d->updateSystemBackground();
	return;
    }
    setAttribute(WA_NoErase, false);
    QPalette::ColorRole role;
    switch(m) {
    case FixedColor:
    case FixedPixmap:
	break;
    case PaletteForeground:
	role = QPalette::Foreground;
	break;
    case PaletteButton:
	role = QPalette::ButtonText;
	break;
    case PaletteLight:
	role = QPalette::Light;
	break;
    case PaletteMidlight:
	role = QPalette::Midlight;
	break;
    case PaletteDark:
	role = QPalette::Dark;
	break;
    case PaletteMid:
	role = QPalette::Mid;
	break;
    case PaletteText:
	role = QPalette::Text;
	break;
    case PaletteBrightText:
	role = QPalette::BrightText;
	break;
    case PaletteBase:
	role = QPalette::Base;
	break;
    case PaletteBackground:
	role = QPalette::Background;
	break;
    case PaletteShadow:
	role = QPalette::Shadow;
	break;
    case PaletteHighlight:
	role = QPalette::Highlight;
	break;
    case PaletteHighlightedText:
	role = QPalette::HighlightedText;
	break;
    case PaletteButtonText:
	role = QPalette::ButtonText;
	break;
    case PaletteLink:
	role = QPalette::Link;
	break;
    case PaletteLinkVisited:
	role = QPalette::LinkVisited;
	break;
    default:
	break;
    }
    setAttribute(WA_SetForegroundRole, false);
    setBackgroundRole(role);
}
#endif


/*!
  Returns the background role.

  \sa setBackgroundRole(), foregroundRole()
 */
QPalette::ColorRole QWidget::backgroundRole() const
{
    const QWidget *w = this;
    while (w->d->isBackgroundInherited())
	w = w->parentWidget();
    return w->d->bg_role;
}

/*!

  Sets the background role of the widget to \a role.

  \sa backgroundRole(), foregroundRole()
 */
void QWidget::setBackgroundRole(QPalette::ColorRole role)
{
    d->bg_role = role;
    setAttribute(WA_SetBackgroundRole);
    if (!testAttribute(WA_SetForegroundRole))
	switch (role) {
	case QPalette::Button:
	    d->fg_role = QPalette::ButtonText;
	    break;
	case QPalette::Base:
	    d->fg_role = QPalette::Text;
	    break;
	case QPalette::Dark:
	case QPalette::Shadow:
	    d->fg_role = QPalette::Light;
	    break;
	case QPalette::Highlight:
	    d->fg_role = QPalette::HighlightedText;
	    break;
	default:
	    d->fg_role = QPalette::Foreground;
	    break;
	}
    d->updateSystemBackground();
    d->propagatePaletteChange();
}

/*!
  Returns the foreground role.

  If no explicit foreground role is set, returns a role that contrasts
  with the backgroundRole().

  \sa setForegroundRole(), backgroundRole()
 */
QPalette::ColorRole QWidget::foregroundRole() const
{
    const QWidget *w = this;
    while (w->d->isForegroundInherited())
	w = w->parentWidget();
    return w->d->fg_role;
}

/*!
  Sets the foreground role of the widget to \a role \sa
  backgroundRole(), setForegroundRole()
 */
void QWidget::setForegroundRole(QPalette::ColorRole role)
{
    d->bg_role = role;
    setAttribute(WA_SetForegroundRole);
    d->updateSystemBackground();
    d->propagatePaletteChange();
}


#ifndef QT_NO_PALETTE
/*!
    \property QWidget::palette
    \brief the widget's palette

    As long as no special palette has been set, or after unsetPalette()
    has been called, this is either a special palette for the widget
    class, the parent's palette or (if this widget is a top level
    widget), the default application palette.

    Instead of defining an entirely new palette, you can also use the
    \link QWidget::paletteBackgroundColor paletteBackgroundColor\endlink,
    \link QWidget::paletteBackgroundPixmap paletteBackgroundPixmap\endlink and
    \link QWidget::paletteForegroundColor paletteForegroundColor\endlink
    convenience properties to change a widget's
    background and foreground appearance only.

    \sa ownPalette, QApplication::palette()
*/
const QPalette &QWidget::palette() const
{
    if ( !isEnabled() )
	pal.setCurrentColorGroup(QPalette::Disabled);
    else if ( !isVisible() || isActiveWindow() )
	pal.setCurrentColorGroup(QPalette::Active);
    else
	pal.setCurrentColorGroup(QPalette::Inactive);
    return pal;
}

void QWidget::setPalette( const QPalette &palette )
{
    setAttribute(WA_SetPalette, true);
    setPalette_helper(palette);
}

void QWidget::setPalette_helper( const QPalette &palette )
{
    if ( pal == palette )
	return;
    QPalette old = pal;
    pal = palette;
    d->updateSystemBackground();
    d->propagatePaletteChange();
}

void QWidget::unsetPalette()
{
    setAttribute(WA_SetPalette, false);
    setPalette_helper( qt_naturalWidgetPalette( this ) );
}

/*!
  \fn void QWidget::setPalette( const QPalette&, bool )
  \obsolete

  Use setPalette( const QPalette& p ) instead.
*/

/*!
    \fn void QWidget::paletteChange( const QPalette &oldPalette )

    This virtual function is called from setPalette(). \a oldPalette
    is the previous palette; you can get the new palette from
    palette().

    Reimplement this function if your widget needs to know when its
    palette changes.

    \sa setPalette(), palette()
*/

void QWidget::paletteChange( const QPalette & )
{
}
#endif // QT_NO_PALETTE

/*!
    \property QWidget::font
    \brief the font currently set for the widget

    The fontInfo() function reports the actual font that is being used
    by the widget.

    As long as no special font has been set, or after unsetFont() is
    called, this is either a special font for the widget class, the
    parent's font or (if this widget is a top level widget), the
    default application font.

    This code fragment sets a 12 point helvetica bold font:
    \code
    QFont f( "Helvetica", 12, QFont::Bold );
    setFont( f );
    \endcode

    In addition to setting the font, setFont() informs all children
    about the change.

    \sa fontChange() fontInfo() fontMetrics()
*/

void QWidget::setFont( const QFont &font )
{
    setAttribute(WA_SetFont, true);
    setFont_helper(font);
}

void QWidget::setFont_helper( const QFont &font )
{
    if ( fnt == font && fnt.d->mask == font.d->mask )
	return;
    QFont old = fnt;
    fnt = font.resolve( qt_naturalWidgetFont( this ) );
#if defined(Q_WS_X11)
    // make sure the font set on this widget is associated with the correct screen
    fnt.x11SetScreen( x11Screen() );
#endif
    if ( !d->children.isEmpty() ) {
	QEvent e( QEvent::ParentFontChange );
	for (int i = 0; i < d->children.size(); ++i) {
	    QObject *o = d->children.at(i);
	    if ( o->isWidgetType() )
		QApplication::sendEvent( o, &e );
	}
    }
    if ( hasFocus() )
	setFontSys();
    fontChange(old); // compatibility
}

void QWidget::unsetFont()
{
    setAttribute(WA_SetFont, false);
    setFont_helper( qt_naturalWidgetFont( this ) );
}

/*!
  \fn void QWidget::setFont( const QFont&, bool )
  \obsolete

  Use setFont(const QFont& font) instead.
*/

/*!
    \fn void QWidget::fontChange( const QFont &oldFont )

    This virtual function is called from setFont(). \a oldFont is the
    previous font; you can get the new font from font().

    Reimplement this function if your widget needs to know when its
    font changes. You will almost certainly need to update the widget
    using update().

    The default implementation updates the widget including its
    geometry.

    \sa setFont(), font(), update(), updateGeometry()
*/

void QWidget::fontChange( const QFont & )
{
    update();
    updateGeometry();
}


/*!
    \fn QFontMetrics QWidget::fontMetrics() const

    Returns the font metrics for the widget's current font.
    Equivalent to QFontMetrics(widget->font()).

    \sa font(), fontInfo(), setFont()
*/

/*!
    \fn QFontInfo QWidget::fontInfo() const

    Returns the font info for the widget's current font.
    Equivalent to QFontInto(widget->font()).

    \sa font(), fontMetrics(), setFont()
*/


/*!
    \property QWidget::cursor
    \brief the cursor shape for this widget

    The mouse cursor will assume this shape when it's over this
    widget. See the \link Qt::CursorShape list of predefined cursor
    objects\endlink for a range of useful shapes.

    An editor widget might use an I-beam cursor:
    \code
	setCursor( IbeamCursor );
    \endcode

    If no cursor has been set, or after a call to unsetCursor(), the
    parent's cursor is used. The function unsetCursor() has no effect
    on top-level widgets.

    \sa QApplication::setOverrideCursor()
*/

#ifndef QT_NO_CURSOR
const QCursor &QWidget::cursor() const
{
    if ( testAttribute(WA_SetCursor) )
	return (d->extra && d->extra->curs)
	    ? *d->extra->curs
	    : arrowCursor;
    else
	return (isTopLevel() || !parentWidget()) ? arrowCursor : parentWidget()->cursor();
}
#endif
#ifndef QT_NO_WIDGET_TOPEXTRA
/*!
    \property QWidget::caption
    \brief the window caption (title)

    This property only makes sense for top-level widgets. If no
    caption has been set, the caption is QString::null.

    \sa icon() iconText()
*/
QString QWidget::caption() const
{
    return d->extra && d->extra->topextra
	? d->extra->topextra->caption
	: QString();
}

/*!
    \property QWidget::icon
    \brief the widget's icon

    This property only makes sense for top-level widgets. If no icon
    has been set, icon() returns 0.

    \sa iconText, caption,
      \link appicon.html Setting the Application Icon\endlink
*/
const QPixmap *QWidget::icon() const
{
    return ( d->extra && d->extra->topextra ) ? d->extra->topextra->icon : 0;
}

/*!
    \property QWidget::iconText
    \brief the widget's icon text

    This property only makes sense for top-level widgets. If no icon
    text has been set, this functions returns QString::null.

    \sa icon, caption
*/

QString QWidget::iconText() const
{
    return ( d->extra && d->extra->topextra ) ? d->extra->topextra->iconText
	: QString();
}
#endif //QT_NO_WIDGET_TOPEXTRA

/*!
    \property QWidget::mouseTracking
    \brief whether mouse tracking is enabled for the widget

    If mouse tracking is disabled (the default), the widget only
    receives mouse move events when at least one mouse button is
    pressed while the mouse is being moved.

    If mouse tracking is enabled, the widget receives mouse move
    events even if no buttons are pressed.

    \sa mouseMoveEvent(), QApplication::setGlobalMouseTracking()
*/


/*!
    Sets the widget's focus proxy to widget \a w. If \a w is 0, the
    function resets this widget to have no focus proxy.

    Some widgets, such as QComboBox, can "have focus", but create a
    child widget to actually handle the focus. QComboBox, for example,
    creates a QLineEdit which handles the focus.

    setFocusProxy() sets the widget which will actually get focus when
    "this widget" gets it. If there is a focus proxy, focusPolicy(),
    setFocusPolicy(), setFocus() and hasFocus() all operate on the
    focus proxy.

    \sa focusProxy()
*/

void QWidget::setFocusProxy( QWidget * w )
{
    if ( !w && !d->extra )
	return;

    for ( QWidget* fp  = w; fp; fp = fp->focusProxy() ) {
	if ( fp == this ) {
	    qWarning( "%s (%s): already in focus proxy chain", className(), name() );
	    return;
	}
    }

    d->createExtra();
    d->extra->focus_proxy = w;
}


/*!
    Returns the focus proxy, or 0 if there is no focus proxy.

    \sa setFocusProxy()
*/

QWidget * QWidget::focusProxy() const
{
    return d->extra ? (QWidget *)d->extra->focus_proxy : 0;
}


/*!
    \property QWidget::focus
    \brief whether this widget (or its focus proxy) has the keyboard
    input focus

    Effectively equivalent to \c {qApp->focusWidget() == this}.

    \sa setFocus(), clearFocus(), setFocusPolicy(), QApplication::focusWidget()
*/
bool QWidget::hasFocus() const
{
    const QWidget* w = this;
    while ( w->d->extra && w->d->extra->focus_proxy )
	w = w->d->extra->focus_proxy;
    return qApp->focus_widget == w;
}

/*!
    Gives the keyboard input focus to this widget (or its focus
    proxy) if this widget or one of its parents is the \link
    isActiveWindow() active window\endlink.

    First, a focus out event is sent to the focus widget (if any) to
    tell it that it is about to lose the focus. Then a focus in event
    is sent to this widget to tell it that it just received the focus.
    (Nothing happens if the focus in and focus out widgets are the
    same.)

    setFocus() gives focus to a widget regardless of its focus policy,
    but does not clear any keyboard grab (see grabKeyboard()).

    Be aware that if the widget is hidden, it will not accept focus.

    \warning If you call setFocus() in a function which may itself be
    called from focusOutEvent() or focusInEvent(), you may get an
    infinite recursion.

    \sa hasFocus() clearFocus() focusInEvent() focusOutEvent()
    setFocusPolicy() QApplication::focusWidget() grabKeyboard()
    grabMouse()
*/

void QWidget::setFocus()
{
    if ( !isEnabled() )
	return;

    QWidget *f = this;
    while (f->d->extra && f->d->extra->focus_proxy)
	f = f->d->extra->focus_proxy;

    if (qApp->focus_widget == f
#if defined(Q_WS_WIN)
	&& GetFocus() == f->winId()
#endif
	)
	return;


    QWidget *w = f;
    if (isHidden()) {
	while (w && w->isHidden()) {
	    w->d->focus_child = f;
	    w = w->parentWidget(true);
	}
    } else {
	while (w) {
	    w->d->focus_child = f;
	    w = w->parentWidget(true);
	}
    }

    if ( f->isActiveWindow() ) {
	QWidget *prev = qApp->focus_widget;
	if ( prev ) {
	    if ( prev != f )
		prev->resetInputContext();
	}
#if defined(Q_WS_WIN)
	else {
	    QInputContext::endComposition();
	}
#endif
	qApp->focus_widget = f;
#if defined(Q_WS_X11)
	f->d->focusInputContext();
#endif

#if defined(Q_WS_WIN)
	if ( !f->topLevelWidget()->isPopup() )
	    SetFocus( f->winId() );
	else {
#endif
#if defined(QT_ACCESSIBILITY_SUPPORT)
	    QAccessible::updateAccessibility( f, 0, QAccessible::Focus );
#endif
#if defined(Q_WS_WIN)
	}
#endif

	if ( prev != f ) {
	    if ( prev ) {
		QFocusEvent out( QEvent::FocusOut );
		QApplication::sendEvent( prev, &out );
	    }

	    if ( qApp->focus_widget == f ) {
		QFocusEvent in( QEvent::FocusIn );
		QApplication::sendEvent( f, &in );
	    }
	}
    }
}

/*!
    Takes keyboard input focus from the widget.

    If the widget has active focus, a \link focusOutEvent() focus out
    event\endlink is sent to this widget to tell it that it is about
    to lose the focus.

    This widget must enable focus setting in order to get the keyboard
    input focus, i.e. it must call setFocusPolicy().

    \sa hasFocus(), setFocus(), focusInEvent(), focusOutEvent(),
    setFocusPolicy(), QApplication::focusWidget()
*/

void QWidget::clearFocus()
{
    QWidget *w = this;
    while (w && w->d->focus_child == this) {
	w->d->focus_child = 0;
	w = w->parentWidget(true);
    }
    if (hasFocus()) {
	QWidget* w = qApp->focus_widget;
	// clear active focus
	qApp->focus_widget = 0;
	QFocusEvent out( QEvent::FocusOut );
	QApplication::sendEvent( w, &out );
#if defined(Q_WS_WIN)
	if ( !isPopup() && GetFocus() == w->winId() )
	    SetFocus( 0 );
	else {
#endif
#if defined(QT_ACCESSIBILITY_SUPPORT)
	    QAccessible::updateAccessibility( w, 0, QAccessible::Focus );
#endif
#if defined(Q_WS_WIN)
	}
#endif
    }
}


/*!
    Finds a new widget to give the keyboard focus to, as appropriate
    for Tab and Shift+Tab, and returns TRUE if is can find a new
    widget and FALSE if it can't,

    If \a next is TRUE, this function searches "forwards", if \a next
    is FALSE, it searches "backwards".

    Sometimes, you will want to reimplement this function. For
    example, a web browser might reimplement it to move its "current
    active link" forwards or backwards, and call
    QWidget::focusNextPrevChild() only when it reaches the last or
    first link on the "page".

    Child widgets call focusNextPrevChild() on their parent widgets,
    but only the top-level widget decides where to redirect focus. By
    overriding this method for an object, you thus gain control of
    focus traversal for all child widgets.
*/

bool QWidget::focusNextPrevChild( bool next )
{
    QWidget* p = parentWidget();
    if (!isTopLevel() && p)
	return p->focusNextPrevChild(next);

    extern bool qt_tab_all_widgets;
    uint focus_flag = qt_tab_all_widgets ? TabFocus : StrongFocus;

    QWidget *f = focusWidget();
    if (!f)
	f = this;

    QWidget *w = f;
    QWidget *test = f->d->focus_next;
    while (test != f) {
	if ((test->focusPolicy() & focus_flag) == focus_flag
	    && !(test->d->extra && test->d->extra->focus_proxy)
	    && test->isVisibleTo(this) && test->isEnabled()) {
	    w = test;
	    if (next)
		break;
	}
	test = test->d->focus_next;
    }
    if (w == f)
	return false;
    w->setFocus();
    return true;
}

/*!
    Returns the last child of this widget that setFocus had been
    called on.  For top level widgets this is the widget that will get
    focus in case this window gets activated

    This is not the same as QApplication::focusWidget(), which returns
    the focus widget in the currently active window.
*/

QWidget *QWidget::focusWidget() const
{
    return const_cast<QWidget *>(d->focus_child);
}

QWidget *QWidget::nextInFocusChain() const
{
    return const_cast<QWidget *>(d->focus_next);
}

/*!
    \property QWidget::inputMethodEnabled
    \brief enables or disables the use of input methods for this widget.

    Most Widgets (as eg. buttons) that do not handle text input should have
    the input method disabled if they have focus. This is the default.

    If a widget handles text input it should set this property to TRUE.
*/

void QWidget::setInputMethodEnabled( bool b )
{
    im_enabled = b;
#ifdef Q_WS_WIN
    QInputContext::enable( this, im_enabled && isEnabled() );
#endif
}


/*!
    \property QWidget::isActiveWindow
    \brief whether this widget is the active window

    The active window is the window (or child of the window) that has
    keyboard focus.

    When popup windows are visible, this property is TRUE for both the
    active window \e and for the popup.

    \sa setActiveWindow(), QApplication::activeWindow()
*/
bool QWidget::isActiveWindow() const
{
    QWidget *tlw = topLevelWidget();
    if(testWFlags(WSubWindow) && parentWidget())
	tlw = parentWidget()->topLevelWidget();
    if(tlw == qApp->activeWindow() || ( isVisible() && tlw->isPopup() ))
	return TRUE;
#ifndef QT_NO_STYLE
    if(style().styleHint(QStyle::SH_Widget_ShareActivation, this )) {
	if(tlw->isDialog() && !tlw->testWFlags(WShowModal) &&
	   tlw->parentWidget() && tlw->parentWidget()->isActiveWindow())
	   return TRUE;
	QWidget *w = qApp->activeWindow();
	if( !testWFlags(WSubWindow) && w && w->testWFlags(WSubWindow) &&
	    w->parentWidget()->topLevelWidget() == tlw)
	    return TRUE;
	while( w && w->isDialog() && !w->testWFlags(WShowModal) && w->parentWidget() ) {
	    w = w->parentWidget()->topLevelWidget();
	    if( w == tlw )
		return TRUE;
	}
    }
#endif
#if defined(Q_WS_WIN32)
    HWND parent = tlw->winId();
    HWND topparent = GetActiveWindow();
    while ( parent ) {
	parent = ::GetParent( parent );
	if ( parent && parent == topparent )
	    return TRUE;
    }
#endif

    return FALSE;
}

/*!
    Moves the \a second widget around the ring of focus widgets so
    that keyboard focus moves from the \a first widget to the \a
    second widget when the Tab key is pressed.

    Note that since the tab order of the \a second widget is changed,
    you should order a chain like this:

    \code
	setTabOrder( a, b ); // a to b
	setTabOrder( b, c ); // a to b to c
	setTabOrder( c, d ); // a to b to c to d
    \endcode

    \e not like this:

    \code
	setTabOrder( c, d ); // c to d   WRONG
	setTabOrder( a, b ); // a to b AND c to d
	setTabOrder( b, c ); // a to b to c, but not c to d
    \endcode

    If \a first or \a second has a focus proxy, setTabOrder()
    correctly substitutes the proxy.

    \sa setFocusPolicy(), setFocusProxy()
*/
void QWidget::setTabOrder( QWidget* first, QWidget *second )
{
    if ( !first || !second ||
	first->focusPolicy() == NoFocus || second->focusPolicy() == NoFocus )
	return;

    // If first is redirected, set first to the last child of first
    // so that second is inserted after that last child, and the
    // focus order within first is (more likely to be) preserved.
    if ( first->focusProxy() ) {
	QObjectList l = first->queryList( "QWidget" );
	if ( l.size() )
	    first = static_cast<QWidget*>(l.last());
    }
    while ( first->focusProxy() )
	first = first->focusProxy();
    while ( second->focusProxy() )
	second = second->focusProxy();

    QWidget *p = second;
    while (p->d->focus_next != second)
	p = p->d->focus_next;
    p->d->focus_next = second->d->focus_next;

    second->d->focus_next = first->d->focus_next;
    first->d->focus_next = second;
}

/*!\internal

  Moves the relevant subwidgets of this widget from the \a oldtlw's
  tab chain to that of the new parent, if there's anything to move and
  we're really moving

  This function is called from QWidget::reparent() *after* the widget
  has been reparented.

  \sa reparent()
*/

void QWidget::reparentFocusWidgets( QWidget * oldtlw )
{
    if ( oldtlw == topLevelWidget() )
	return; // nothing to do

    if(d->focus_child)
	d->focus_child->clearFocus();

    // seperate the focus chain
    QWidget *topLevel = topLevelWidget();
    QWidget *w = this;
    QWidget *firstOld = 0;
    QWidget *firstNew = 0;
    QWidget *o = 0;
    QWidget *n = 0;
    do {
	if (w == this || isAncestorOf(w)) {
	    if (!firstNew)
		firstNew = w;
	    if (n)
		n->d->focus_next = w;
	    n = w;
	} else {
	    if (!firstOld)
		firstOld = w;
	    if (o)
		o->d->focus_next = w;
	    o = w;
	}
    } while ((w = w->d->focus_next) != this);
    if(o)
	o->d->focus_next = firstOld;
    if(n)
	n->d->focus_next = firstNew;

    if (!isTopLevel()) {
	//insert chain
	w = topLevel;
	while (w->d->focus_next != topLevel)
	    w = w->d->focus_next;
	w->d->focus_next = this;
	n->d->focus_next = topLevel;
    } else {
	n->d->focus_next = this;
    }
}

/*!
  \fn void QWidget::recreate( QWidget *parent, WFlags f, const QPoint & p, bool showIt )

  \obsolete

  This method is provided to aid porting from Qt 1.0 to 2.0. It has
  been renamed reparent() in Qt 2.0.
*/

/*!
    \property QWidget::frameSize
    \brief the size of the widget including any window frame
*/
QSize QWidget::frameSize() const
{
    if ( isTopLevel() && !isPopup() ) {
	if ( fstrut_dirty )
	    updateFrameStrut();
	QWidget *that = (QWidget *) this;
	QTLWExtra *top = that->d->topData();
	return QSize( crect.width() + top->fleft + top->fright,
		      crect.height() + top->ftop + top->fbottom );
    }
    return crect.size();
}

/*!
    \internal

    Recursive function that updates \a widget and all its children,
    if they have some parent background origin.
*/
static void qt_update_bg_recursive( QWidget *widget )
{
    if ( !widget || widget->isHidden() || widget->backgroundOrigin() == QWidget::WidgetOrigin || !widget->backgroundPixmap() )
	return;

    QObjectList lst = widget->children();

    for (int i = 0; i < lst.size(); ++i) {
	QWidget *widget = static_cast<QWidget *>(lst.at(i));
	if (widget->isWidgetType() && !widget->isHidden() && !widget->isTopLevel()
	    && !widget->testWFlags(Qt::WSubWindow))
	    qt_update_bg_recursive( widget );
    }
    QApplication::postEvent( widget, new QPaintEvent( widget->clipRegion(), !widget->testWFlags(Qt::WRepaintNoErase) ) );
}

/*!
    \overload

    This corresponds to move( QSize(\a x, \a y) ).
*/

void QWidget::move( int x, int y )
{
    QPoint oldp(pos());
    setGeometry_helper( x + geometry().x() - QWidget::x(),
			 y + geometry().y() - QWidget::y(),
			 width(), height(), TRUE );
    if ( isVisible() && oldp != pos() )
	qt_update_bg_recursive( this );
}

/*!
    \overload

    This corresponds to resize( QSize(\a w, \a h) ).
*/
void QWidget::resize( int w, int h )
{
    setGeometry_helper( geometry().x(), geometry().y(), w, h, FALSE );
    setWState( WState_Resized );
}

/*!
    \overload

    This corresponds to setGeometry( QRect(\a x, \a y, \a w, \a h) ).
*/
void QWidget::setGeometry( int x, int y, int w, int h )
{
    QPoint oldp( pos( ));
    setGeometry_helper( x, y, w, h, TRUE );
    setWState( WState_Resized );
    if ( isVisible() && oldp != pos() )
	qt_update_bg_recursive( this );
}

/*!
    \property QWidget::focusEnabled
    \brief whether the widget accepts keyboard focus

    Keyboard focus is initially disabled (i.e. focusPolicy() ==
    \c QWidget::NoFocus).

    You must enable keyboard focus for a widget if it processes
    keyboard events. This is normally done from the widget's
    constructor. For instance, the QLineEdit constructor calls
    setFocusPolicy(QWidget::StrongFocus).

    \sa setFocusPolicy(), focusInEvent(), focusOutEvent(), keyPressEvent(),
      keyReleaseEvent(), isEnabled()
*/

/*!
    \enum QWidget::FocusPolicy

    This enum type defines the various policies a widget can have with
    respect to acquiring keyboard focus.

    \value TabFocus  the widget accepts focus by tabbing.
    \value ClickFocus  the widget accepts focus by clicking.
    \value StrongFocus  the widget accepts focus by both tabbing
			and clicking. On Mac OS X this will also
			be indicate that the widget accepts tab focus
			when in 'Text/List focus mode'.
    \value WheelFocus  like StrongFocus plus the widget accepts
			focus by using the mouse wheel.
    \value NoFocus  the widget does not accept focus.

*/

/*!
    \property QWidget::focusPolicy
    \brief the way the widget accepts keyboard focus

    The policy is \c QWidget::TabFocus if the widget accepts keyboard
    focus by tabbing, \c QWidget::ClickFocus if the widget accepts
    focus by clicking, \c QWidget::StrongFocus if it accepts both, and
    \c QWidget::NoFocus (the default) if it does not accept focus at
    all.

    You must enable keyboard focus for a widget if it processes
    keyboard events. This is normally done from the widget's
    constructor. For instance, the QLineEdit constructor calls
    setFocusPolicy(QWidget::StrongFocus).

    \sa focusEnabled, focusInEvent(), focusOutEvent(), keyPressEvent(),
      keyReleaseEvent(), enabled
*/

QWidget::FocusPolicy QWidget::focusPolicy() const
{
    const QWidget *w = this;
    while ( w->d->extra && w->d->extra->focus_proxy )
	w = w->d->extra->focus_proxy;
    return (FocusPolicy)w->focus_policy;
}

void QWidget::setFocusPolicy( FocusPolicy policy )
{
    QWidget *w = this;
    while ( w->d->extra && w->d->extra->focus_proxy )
	w = w->d->extra->focus_proxy;
    w->focus_policy = (uint) policy;
}

/*!
    \property QWidget::updatesEnabled
    \brief whether updates are enabled

    Calling update() and repaint() has no effect if updates are
    disabled. Paint events from the window system are processed
    normally even if updates are disabled.

    setUpdatesEnabled() is normally used to disable updates for a
    short period of time, for instance to avoid screen flicker during
    large changes.

    Example:
    \code
	setUpdatesEnabled( FALSE );
	bigVisualChanges();
	setUpdatesEnabled( TRUE );
	repaint();
    \endcode

    \sa update(), repaint(), paintEvent()
*/
void QWidget::setUpdatesEnabled( bool enable )
{
    if ( enable )
	clearWState( WState_BlockUpdates );
    else
	setWState( WState_BlockUpdates );
}


static inline QSize qt_initial_size(QWidget *w) {
    QSize s = w->sizeHint();
    QSizePolicy::ExpandData exp;
#ifndef QT_NO_LAYOUT
    QLayout *layout = w->layout();
    if (layout) {
	if ( layout->hasHeightForWidth() )
	    s.setHeight( layout->totalHeightForWidth( s.width() ) );
	exp = layout->expanding();
    } else
#endif
    {
	if ( w->sizePolicy().hasHeightForWidth() )
	    s.setHeight( w->heightForWidth( s.width() ) );
	exp = w->sizePolicy().expanding();
    }
    if ( exp & QSizePolicy::Horizontally )
	s.setWidth( QMAX( s.width(), 200 ) );
    if ( exp & QSizePolicy::Vertically )
	s.setHeight( QMAX( s.height(), 150 ) );
#if defined(Q_WS_X11)
    QRect screen = QApplication::desktop()->screenGeometry( w->x11Screen() );
#else // all others
    QRect screen = QApplication::desktop()->screenGeometry( w->pos() );
#endif
    s.setWidth( QMIN( s.width(), screen.width()*2/3 ) );
    s.setHeight( QMIN( s.height(), screen.height()*2/3 ) );
    return s;
}

/*!
    Shows the widget and its child widgets.

    If its size or position has changed, Qt guarantees that a widget
    gets move and resize events just before it is shown.

    You almost never have to reimplement this function. If you need to
    change some settings before a widget is shown, use showEvent()
    instead. If you need to do some delayed initialization use
    polishEvent().

    \sa showEvent(), hide(), showMinimized(), showMaximized(),
    showNormal(), isVisible(), polish()
*/

void QWidget::show()
{
    if (testWState(WState_ExplicitShowHide|WState_Hidden) == WState_ExplicitShowHide)
	return;

    // polish if necessary
    ensurePolished();

    // remember that show was called explicitly
    setWState(WState_ExplicitShowHide);
    // whether we need to inform the parent widget immediately
    bool needUpdateGeometry = !isTopLevel() && testWState(WState_Hidden);
    // we are no longer hidden
    clearWState(WState_Hidden);

    if (needUpdateGeometry)
	updateGeometry();

    if ( isTopLevel() || parentWidget()->isVisible() )
	show_helper();

    QEvent showToParentEvent( QEvent::ShowToParent );
    QApplication::sendEvent( this, &showToParentEvent );
}

/*! \internal

   Makes the widget visible in the isVisible() meaning of the word.
   It is only called for toplevels or widgets with visible parents.
 */
void QWidget::show_helper()
{
    in_show = true; // qws optimization

    // polish if necessary
    ensurePolished();

#ifndef QT_NO_COMPAT
    QApplication::sendPostedEvents( this, QEvent::ChildInserted );
#endif

    if (!isTopLevel() && parentWidget()->d->layout)
	parentWidget()->d->layout->activate();

    // activate our layout before we and our children become visible
    if (d->layout)
	d->layout->activate();

    // make sure we receive pending move and resize events
    if (testAttribute(WA_PendingMoveEvent)) {
	QMoveEvent e(crect.topLeft(), crect.topLeft());
	QApplication::sendEvent(this, &e);
	setAttribute(WA_PendingMoveEvent, false);
    }
    if (testAttribute(WA_PendingResizeEvent)) {
	QResizeEvent e(crect.size(), crect.size());
	QApplication::sendEvent(this, &e);
	setAttribute(WA_PendingResizeEvent, false);
    }

    // become visible before showing all children
    setWState(WState_Visible);

    // finally show all children recursively
    showChildren(false);

#ifndef QT_NO_COMPAT
    if ( parentWidget() )
	QApplication::sendPostedEvents( parentWidget(),
					QEvent::ChildInserted );
#endif

    if ( isTopLevel() && !testWState( WState_Resized ) )  {
#ifndef Q_OS_TEMP
	// toplevels with layout may need a initial size
	QSize s = qt_initial_size(this);
	// do this before sending the posted resize events. Otherwise
	// the layout would catch the resize event and may expand the
	// minimum size.
	if (!s.isEmpty())
	    resize(s);
#endif // Q_OS_TEMP
    }

    // popup handling: new popups and tools need to be raised, and
    // exisiting popups must be closed.
    if ( testWFlags(WStyle_Tool|testWFlags(WType_Popup))) {
	raise();
    } else if (isTopLevel()) {
	while ( QApplication::activePopupWidget() ) {
	    if ( !QApplication::activePopupWidget()->close() )
		break;
	}
    }

    // On Windows, show the popup now so that our own focus handling
    // stores the correct old focus widget even if it's stolen in the
    // showevent
#if defined(Q_WS_WIN)
    if ( testWFlags(WType_Popup) )
	qApp->openPopup( this );
#endif

    // send the show event before showing the window
    QShowEvent showEvent;
    QApplication::sendEvent( this, &showEvent );


#ifndef QT_NO_WIDGET_TOPEXTRA
    // make sure toplevels have an icon
    if ( isTopLevel() ) {
	const QPixmap *pm = icon();
	if ( !pm || pm->isNull() ) {
	    QWidget *mw = (QWidget *)parent();
	    pm = mw ? mw->icon() : 0;
	    if ( pm && !pm->isNull() )
		setIcon( *pm );
	    else {
		mw = mw ? mw->topLevelWidget() : 0;
		pm = mw ? mw->icon() : 0;
		if ( pm && !pm->isNull() )
		    setIcon( *pm );
		else {
		    mw = qApp ? qApp->mainWidget() : 0;
		    pm = mw ? mw->icon() : 0;
		    if ( pm && !pm->isNull() )
			setIcon( *pm );
		}
	    }
	}
    }
#endif

    if ( testWFlags(WShowModal) )
	// qt_enter_modal *before* show, otherwise the initial
	// stacking might be wrong
	qt_enter_modal( this );

    showWindow();

#if !defined(Q_WS_WIN)
    if ( testWFlags(WType_Popup) )
	qApp->openPopup( this );
#endif

#if defined(QT_ACCESSIBILITY_SUPPORT)
    QAccessible::updateAccessibility( this, 0, QAccessible::ObjectShow );
#endif

    in_show = false;  // reset qws optimization
}

/*! \fn void QWidget::iconify()
    \obsolete
*/

/*!
    Hides the widget.

    You almost never have to reimplement this function. If you need to
    do something after a widget is hidden, use hideEvent() instead.

    \sa hideEvent(), isHidden(), show(), showMinimized(), isVisible(), close()
*/

void QWidget::hide()
{
    if (testWState(WState_ExplicitShowHide|WState_Hidden) == (WState_ExplicitShowHide|WState_Hidden))
	return;
    setWState(WState_Hidden);
    if (testWState(WState_ExplicitShowHide))
	hide_helper();
    else
	setWState(WState_ExplicitShowHide);
    QEvent hideToParentEvent( QEvent::HideToParent );
    QApplication::sendEvent( this, &hideToParentEvent );
}

/*!\internal
 */
void QWidget::hide_helper()
{
    if ( testWFlags(WType_Popup) )
	qApp->closePopup( this );

    // Move test modal here.  Otherwise, a modal dialog could get
    // destroyed and we lose all access to its parent because we haven't
    // left modality.  (Eg. modal Progress Dialog)
    if ( testWFlags(WShowModal) )
	qt_leave_modal( this );

#if defined(Q_WS_WIN)
    if ( isTopLevel() && !isPopup() && parentWidget() && isActiveWindow() )
	parentWidget()->setActiveWindow();	// Activate parent
#endif

    hideWindow();

    bool wasVisible = testWState(WState_Visible);

    if (wasVisible) {
	clearWState( WState_Visible );
	// move the focus if we were the focus widget
	if ( qApp && qApp->focusWidget() == this )
	    focusNextPrevChild( TRUE );
    }

    QHideEvent hideEvent;
    QApplication::sendEvent(this, &hideEvent);
    hideChildren(false);

#if defined(QT_ACCESSIBILITY_SUPPORT)
    if (wasVisible)
	QAccessible::updateAccessibility( this, 0, QAccessible::ObjectHide );
#endif

    // invalidate layout similar to updateGeometry()
    if (!isTopLevel() && parentWidget() && parentWidget()->d->layout) {
	parentWidget()->d->layout->update();
	if (wasVisible)
	    QApplication::postEvent(parentWidget(),
				    new QEvent( QEvent::LayoutRequest));
    }
}

void QWidget::setShown( bool show )
{
    if ( show )
	this->show();
    else
	hide();
}

void QWidget::setHidden( bool hide )
{
    if ( hide )
	this->hide();
    else
	show();
}

void QWidget::showChildren(bool spontaneous)
{
    for (int i = 0; i < d->children.size(); ++i) {
	register QObject *object = d->children.at(i);
	if (!object->isWidgetType())
	    continue;
	QWidget *widget = static_cast<QWidget*>(object);
	if (widget->isTopLevel() || widget->testWState(WState_Hidden))
	    continue;
	if (spontaneous) {
	    widget->showChildren(true);
	    QShowEvent e;
	    QApplication::sendSpontaneousEvent(widget, &e);
	} else {
	    if (widget->testWState(WState_ExplicitShowHide))
		widget->show_helper();
	    else
		widget->show();
	}
    }
}

void QWidget::hideChildren(bool spontaneous)
{
    for (int i = 0; i < d->children.size(); ++i) {
	register QObject *object = d->children.at(i);
	if (!object->isWidgetType())
	    continue;
	QWidget *widget = static_cast<QWidget*>(object);
	if (widget->isTopLevel() || widget->testWState(WState_Hidden))
	    continue;
	if (!spontaneous)
	    widget->clearWState(WState_Visible);
	widget->hideChildren(spontaneous);
	QHideEvent e;
	if (spontaneous)
	    QApplication::sendSpontaneousEvent(widget, &e);
	else
	    QApplication::sendEvent(widget, &e);
    }
}

/*!
    \overload

    Closes this widget. Returns TRUE if the widget was closed;
    otherwise returns FALSE.

    If \a alsoDelete is TRUE or the widget has the \c
    WDestructiveClose widget flag, the widget is also deleted. The
    widget can prevent itself from being closed by rejecting the
    \l QCloseEvent it gets. A close events is delivered to the widget
    no matter if the widget is visible or not.

    The QApplication::lastWindowClosed() signal is emitted when the
    last visible top level widget is closed.

    Note that closing the \l QApplication::mainWidget() terminates the
    application.

    \sa closeEvent(), QCloseEvent, hide(), QApplication::quit(),
    QApplication::setMainWidget(), QApplication::lastWindowClosed()
*/

bool QWidget::close( bool alsoDelete )
{
    if ( is_closing )
	return TRUE;
    is_closing = 1;
    WId id	= winId();
    bool isMain = qApp->mainWidget() == this;
    bool checkLastWindowClosed = isTopLevel() && !isPopup();
    bool deleted = FALSE;
    QCloseEvent e;
    QApplication::sendEvent( this, &e );
    deleted = !QWidget::find(id);
    if ( !deleted && !e.isAccepted() ) {
	is_closing = 0;
	return FALSE;
    }
    if ( !deleted && !isHidden() )
	hide();
    if ( checkLastWindowClosed
	 && qApp->receivers(SIGNAL(lastWindowClosed())) ) {
	/* if there is no non-withdrawn top level window left (except
	   the desktop, popups, or dialogs with parents), we emit the
	   lastWindowClosed signal */
	QWidgetList list = qApp->topLevelWidgets();
	QWidget *widget = 0;
	for (int i = 0; !widget && i < list.size(); ++i) {
	    QWidget *w = list.at(i);
	    if ( !w->isHidden()
		 && !w->isDesktop()
		 && !w->isPopup()
		 && (!w->isDialog() || !w->parentWidget()))
		widget = w;
	}
	if ( widget == 0 )
	    emit qApp->lastWindowClosed();
    }
    if ( isMain )
	qApp->quit();
    if ( deleted )
	return TRUE;
    is_closing = 0;
    if ( alsoDelete )
	delete this;
    else if ( testWFlags(WDestructiveClose) ) {
	clearWFlags(WDestructiveClose);
	deleteLater();
    }
    return TRUE;
}


/*!
    \fn bool QWidget::close()

    Closes this widget. Returns TRUE if the widget was closed;
    otherwise returns FALSE.

    First it sends the widget a QCloseEvent. The widget is \link
    hide() hidden\endlink if it \link QCloseEvent::accept()
    accepts\endlink the close event. The default implementation of
    QWidget::closeEvent() accepts the close event.

    The \l QApplication::lastWindowClosed() signal is emitted when the
    last visible top level widget is closed.

*/

/*!
    \property QWidget::visible
    \brief whether the widget is visible

    Calling show() sets the widget to visible status if all its parent
    widgets up to the top-level widget are visible. If an ancestor is
    not visible, the widget won't become visible until all its
    ancestors are shown.

    Calling hide() hides a widget explicitly. An explicitly hidden
    widget will never become visible, even if all its ancestors become
    visible, unless you show it.

    A widget receives show and hide events when its visibility status
    changes. Between a hide and a show event, there is no need to
    waste CPU cycles preparing or displaying information to the user.
    A video application, for example, might simply stop generating new
    frames.

    A widget that happens to be obscured by other windows on the
    screen is considered to be visible. The same applies to iconified
    top-level widgets and windows that exist on another virtual
    desktop (on platforms that support this concept). A widget
    receives spontaneous show and hide events when its mapping status
    is changed by the window system, e.g. a spontaneous hide event
    when the user minimizes the window, and a spontaneous show event
    when the window is restored again.

    \sa show(), hide(), isHidden(), isVisibleTo(), isMinimized(),
    showEvent(), hideEvent()
*/


/*!
    Returns TRUE if this widget would become visible if \a ancestor is
    shown; otherwise returns FALSE.

    The TRUE case occurs if neither the widget itself nor any parent
    up to but excluding \a ancestor has been explicitly hidden.

    This function will still return TRUE if the widget is obscured by
    other windows on the screen, but could be physically visible if it
    or they were to be moved.

    isVisibleTo(0) is identical to isVisible().

    \sa show() hide() isVisible()
*/

bool QWidget::isVisibleTo(QWidget* ancestor) const
{
    if ( !ancestor )
	return isVisible();
    const QWidget * w = this;
    while ( w
	    && w->isShown()
	    && !w->isTopLevel()
	    && w->parentWidget()
	    && w->parentWidget() != ancestor )
	w = w->parentWidget();
    return w->isShown();
}


/*!
  \fn bool QWidget::isVisibleToTLW() const
  \obsolete

  This function is deprecated. It is equivalent to isVisible()
*/

/*!
    \property QWidget::hidden
    \brief whether the widget is explicitly hidden

    If FALSE, the widget is visible or would become visible if all its
    ancestors became visible.

    \sa hide(), show(), isVisible(), isVisibleTo(), shown
*/

/*!
    \property QWidget::shown
    \brief whether the widget is shown

    If TRUE, the widget is visible or would become visible if all its
    ancestors became visible.

    \sa hide(), show(), isVisible(), isVisibleTo(), hidden
*/

/*!
    \property QWidget::visibleRect
    \brief the visible rectangle

    \obsolete

    No longer necessary, you can simply call repaint(). If you do not
    need the rectangle for repaint(), use clipRegion() instead.
*/
QRect QWidget::visibleRect() const
{
    QRect r = rect();
    const QWidget * w = this;
    int ox = 0;
    int oy = 0;
    while ( w
	    && w->isVisible()
	    && !w->isTopLevel()
	    && w->parentWidget() ) {
	ox -= w->x();
	oy -= w->y();
	w = w->parentWidget();
	r = r.intersect( QRect( ox, oy, w->width(), w->height() ) );
    }
    if ( !w->isVisible() )
	return QRect();
    return r;
}

/*!
    Returns the unobscured region where paint events can occur.

    For visible widgets, this is an approximation of the area not
    covered by other widgets; otherwise, this is an empty region.

    The repaint() function calls this function if necessary, so in
    general you do not need to call it.

*/
QRegion QWidget::clipRegion() const
{
    return visibleRect();
}


/*!
    Adjusts the size of the widget to fit the contents.

    Uses sizeHint() if valid (i.e if the size hint's width and height
    are \>= 0), otherwise sets the size to the children rectangle (the
    union of all child widget geometries).

    \sa sizeHint(), childrenRect()
*/

void QWidget::adjustSize()
{
    ensurePolished();
    QSize s = sizeHint();
    if ( s.isValid() ) {
	resize( s );
	return;
    }
    QRect r = childrenRect();			// get children rectangle
    if ( r.isNull() )				// probably no widgets
	return;
    resize( r.width() + 2 * r.x(), r.height() + 2 * r.y() );
}


/*!
    \property QWidget::sizeHint
    \brief the recommended size for the widget

    If the value of this property is an invalid size, no size is
    recommended.

    The default implementation of sizeHint() returns an invalid size
    if there is no layout for this widget, and returns the layout's
    preferred size otherwise.

    \sa QSize::isValid(), minimumSizeHint(), sizePolicy(),
    setMinimumSize(), updateGeometry()
*/

QSize QWidget::sizeHint() const
{
#ifndef QT_NO_LAYOUT
    if ( d->layout )
	return d->layout->totalSizeHint();
#endif
    return QSize( -1, -1 );
}

/*!
    \property QWidget::minimumSizeHint
    \brief the recommended minimum size for the widget

    If the value of this property is an invalid size, no minimum size
    is recommended.

    The default implementation of minimumSizeHint() returns an invalid
    size if there is no layout for this widget, and returns the
    layout's minimum size otherwise. Most built-in widgets reimplement
    minimumSizeHint().

    \l QLayout will never resize a widget to a size smaller than
    minimumSizeHint.

    \sa QSize::isValid(), resize(), setMinimumSize(), sizePolicy()
*/
QSize QWidget::minimumSizeHint() const
{
#ifndef QT_NO_LAYOUT
    if ( d->layout )
	return d->layout->totalMinimumSize();
#endif
    return QSize( -1, -1 );
}


/*!
    \fn QWidget *QWidget::parentWidget() const

    Returns the parent of this widget, or 0 if it does not have any
    parent widget.
*/

/*!
    \fn WFlags QWidget::testWFlags( WFlags f ) const

    Returns the bitwise AND of the widget flags and \a f.

    Widget flags are a combination of \l{Qt::WidgetFlags}.

    \sa getWFlags(), setWFlags(), clearWFlags()
*/

/*!
  \fn WState QWidget::testWState( WState s ) const
  \internal

  Returns the bitwise AND of the widget states and \a s.
*/

/*!
  \fn uint QWidget::getWState() const

  \internal

  Returns the current widget state.
*/
/*!
  \fn void QWidget::clearWState( uint n )

  \internal

  Clears the widgets states \a n.
*/
/*!
  \fn void QWidget::setWState( uint n )

  \internal

  Sets the widgets states \a n.
*/



/*****************************************************************************
  QWidget event handling
 *****************************************************************************/

/*!
    This is the main event handler; it handles event \a e. You can
    reimplement this function in a subclass, but we recommend using
    one of the specialized event handlers instead.

    The main event handler first passes an event through all \link
    QObject::installEventFilter() event filters\endlink that have been
    installed. If none of the filters intercept the event, it calls
    one of the specialized event handlers.

    Key press and release events are treated differently from other
    events. event() checks for Tab and Shift+Tab and tries to move the
    focus appropriately. If there is no widget to move the focus to
    (or the key press is not Tab or Shift+Tab), event() calls
    keyPressEvent().

    This function returns TRUE if it is able to pass the event over to
    someone (i.e. someone wanted the event); otherwise returns FALSE.

    \sa closeEvent(), focusInEvent(), focusOutEvent(), enterEvent(),
    keyPressEvent(), keyReleaseEvent(), leaveEvent(),
    mouseDoubleClickEvent(), mouseMoveEvent(), mousePressEvent(),
    mouseReleaseEvent(), moveEvent(), paintEvent(), resizeEvent(),
    QObject::event(), QObject::timerEvent()
*/

bool QWidget::event( QEvent *e )
{
    if ( QObject::event( e ) )
	return TRUE;

    switch ( e->type() ) {
	case QEvent::MouseMove:
	    mouseMoveEvent( (QMouseEvent*)e );
	    if ( ! ((QMouseEvent*)e)->isAccepted() )
		return FALSE;
	    break;

	case QEvent::MouseButtonPress:
	    resetInputContext();
	    mousePressEvent( (QMouseEvent*)e );
	    if ( ! ((QMouseEvent*)e)->isAccepted() )
		return FALSE;
	    break;

	case QEvent::MouseButtonRelease:
	    mouseReleaseEvent( (QMouseEvent*)e );
	    if ( ! ((QMouseEvent*)e)->isAccepted() )
		return FALSE;
	    break;

	case QEvent::MouseButtonDblClick:
	    mouseDoubleClickEvent( (QMouseEvent*)e );
	    if ( ! ((QMouseEvent*)e)->isAccepted() )
		return FALSE;
	    break;
#ifndef QT_NO_WHEELEVENT
	case QEvent::Wheel:
	    wheelEvent( (QWheelEvent*)e );
	    if ( ! ((QWheelEvent*)e)->isAccepted() )
		return FALSE;
	    break;
#endif
	case QEvent::TabletMove:
	case QEvent::TabletPress:
	case QEvent::TabletRelease:
	    tabletEvent( (QTabletEvent*)e );
	    if ( ! ((QTabletEvent*)e)->isAccepted() )
		return FALSE;
	    break;
	case QEvent::Accel:
	    ((QKeyEvent*)e)->ignore();
	    return FALSE;
	case QEvent::KeyPress: {
	    QKeyEvent *k = (QKeyEvent *)e;
	    bool res = FALSE;
	    if ( !(k->state() & ControlButton || k->state() & AltButton) ) {
		if ( k->key() == Key_Backtab ||
		     (k->key() == Key_Tab &&
		      (k->state() & ShiftButton)) ) {
		    QFocusEvent::setReason( QFocusEvent::Backtab );
		    res = focusNextPrevChild( FALSE );
		    QFocusEvent::resetReason();

		} else if ( k->key() == Key_Tab ) {
		    QFocusEvent::setReason( QFocusEvent::Tab );
		    res = focusNextPrevChild( TRUE );
		    QFocusEvent::resetReason();
		}
		if ( res )
		    break;
	    }
	    keyPressEvent( k );
	    if ( !k->isAccepted() )
		return FALSE;
	    }
	    break;

	case QEvent::KeyRelease:
	    keyReleaseEvent( (QKeyEvent*)e );
	    if ( ! ((QKeyEvent*)e)->isAccepted() )
		return FALSE;
	    break;

	case QEvent::IMStart: {
	    QIMEvent *i = (QIMEvent *) e;
	    imStartEvent(i);
	    if (! i->isAccepted())
		return FALSE;
	    }
	    break;

	case QEvent::IMCompose: {
	    QIMEvent *i = (QIMEvent *) e;
	    imComposeEvent(i);
	    if (! i->isAccepted())
		return FALSE;
	    }
	    break;

	case QEvent::IMEnd: {
	    QIMEvent *i = (QIMEvent *) e;
	    imEndEvent(i);
	    if (! i->isAccepted())
		return FALSE;
	    }
	    break;

	case QEvent::FocusIn:
	    focusInEvent( (QFocusEvent*)e );
	    setFontSys();
	    break;

	case QEvent::FocusOut:
	    focusOutEvent( (QFocusEvent*)e );
	    break;

	case QEvent::Enter:
	    enterEvent( e );
	    break;

	case QEvent::Leave:
	     leaveEvent( e );
	    break;

	case QEvent::Paint:
	    // At this point the event has to be delivered, regardless
	    // whether the widget isVisible() or not because it
	    // already went through the filters
	    paintEvent( (QPaintEvent*)e );
	    break;

	case QEvent::Move:
	    moveEvent( (QMoveEvent*)e );
	    break;

	case QEvent::Resize:
	    resizeEvent( (QResizeEvent*)e );
	    break;

	case QEvent::Close: {
	    QCloseEvent *c = (QCloseEvent *)e;
	    closeEvent( c );
	    if ( !c->isAccepted() )
		return FALSE;
	    }
	    break;

	case QEvent::ContextMenu: {
	    QContextMenuEvent *c = (QContextMenuEvent *)e;
	    contextMenuEvent( c );
	    if ( !c->isAccepted() )
		return FALSE;
	    }
	    break;

#ifndef QT_NO_DRAGANDDROP
	case QEvent::Drop:
	    dropEvent( (QDropEvent*) e);
	    break;

	case QEvent::DragEnter:
	    dragEnterEvent( (QDragEnterEvent*) e);
	    break;

	case QEvent::DragMove:
	    dragMoveEvent( (QDragMoveEvent*) e);
	    break;

	case QEvent::DragLeave:
	    dragLeaveEvent( (QDragLeaveEvent*) e);
	    break;
#endif

	case QEvent::Show:
	    showEvent( (QShowEvent*) e);
	    break;

	case QEvent::Hide:
	    hideEvent( (QHideEvent*) e);
	    break;

	case QEvent::ShowWindowRequest:
	    if ( isShown() )
		showWindow();
	    break;

	case QEvent::ParentFontChange:
	    if ( isTopLevel() )
		break;
	    // fall through
	case QEvent::ApplicationFontChange:
	    if (testAttribute(WA_SetFont))
		setFont_helper(fnt.resolve(qt_naturalWidgetFont(this)));
	    else
		unsetFont();
	    break;

	case QEvent::ParentPaletteChange:
	    if(isTopLevel())
		break;
	    if(!testAttribute(WA_SetPalette))
		pal = qt_naturalWidgetPalette(this);
	    d->updateSystemBackground();
	    d->propagatePaletteChange();
	    break;

	case QEvent::ApplicationPaletteChange:
	    if (!testAttribute(WA_SetPalette) && !isDesktop())
		unsetPalette();
	    break;

	case QEvent::PaletteChange:
	    update();
	    break;

	case QEvent::WindowActivate:
	case QEvent::WindowDeactivate:
	    windowActivationChange( e->type() != QEvent::WindowActivate );
	    for (int i = 0; i < d->children.size(); ++i) {
		QObject *o = d->children.at(i);
		if (o->isWidgetType()
		    && static_cast<QWidget*>(o)->isVisible()
		    && !static_cast<QWidget*>(o)->isTopLevel())
		    QApplication::sendEvent( o, e );
	    }
	    break;

	case QEvent::LanguageChange:
	    languageChange();
	    // fall through
	case QEvent::LocaleChange:
	    for (int i = 0; i < d->children.size(); ++i) {
		QObject *o = d->children.at(i);
		QApplication::sendEvent( o, e );
	    }
	    update();
	    break;

#ifndef QT_NO_LAYOUT
	case QEvent::LayoutDirectionChange:
	    if ( d->layout ) {
		QHBoxLayout *hbox = ::qt_cast<QHBoxLayout*>(d->layout);
		if ( hbox )
		    hbox->setDirection( qApp->reverseLayout() ? QBoxLayout::RightToLeft : QBoxLayout::LeftToRight );
		d->layout->activate();
	    } else {
		QObjectList llist = queryList( "QLayout", 0, TRUE, TRUE );
		for (int i = 0; i < llist.size(); ++i) {
		    QLayout *lay = static_cast<QLayout *>(llist.at(i));
		    QHBoxLayout *hbox = ::qt_cast<QHBoxLayout*>(lay);
		    if ( hbox )
			hbox->setDirection( qApp->reverseLayout() ? QBoxLayout::RightToLeft : QBoxLayout::LeftToRight );
		    lay->activate();
		}
	    }
	    update();
	    break;
#endif
	default:
	    return FALSE;
    }
    return TRUE;
}

/*!
    This event handler, for event \a e, can be reimplemented in a
    subclass to receive mouse move events for the widget.

    If mouse tracking is switched off, mouse move events only occur if
    a mouse button is pressed while the mouse is being moved. If mouse
    tracking is switched on, mouse move events occur even if no mouse
    button is pressed.

    QMouseEvent::pos() reports the position of the mouse cursor,
    relative to this widget. For press and release events, the
    position is usually the same as the position of the last mouse
    move event, but it might be different if the user's hand shakes.
    This is a feature of the underlying window system, not Qt.

    \sa setMouseTracking(), mousePressEvent(), mouseReleaseEvent(),
    mouseDoubleClickEvent(), event(), QMouseEvent
*/

void QWidget::mouseMoveEvent( QMouseEvent * e)
{
    e->ignore();
}

/*!
    This event handler, for event \a e, can be reimplemented in a
    subclass to receive mouse press events for the widget.

    If you create new widgets in the mousePressEvent() the
    mouseReleaseEvent() may not end up where you expect, depending on
    the underlying window system (or X11 window manager), the widgets'
    location and maybe more.

    The default implementation implements the closing of popup widgets
    when you click outside the window. For other widget types it does
    nothing.

    \sa mouseReleaseEvent(), mouseDoubleClickEvent(),
    mouseMoveEvent(), event(), QMouseEvent
*/

void QWidget::mousePressEvent( QMouseEvent *e )
{
    e->ignore();
    if ( isPopup() ) {
	e->accept();
	QWidget* w;
	while ( (w = qApp->activePopupWidget() ) && w != this ){
	    w->close();
	    if (qApp->activePopupWidget() == w) // widget does not want to dissappear
		w->hide(); // hide at least
	}
	if (!rect().contains(e->pos()) ){
	    close();
	}
    }
}

/*!
    This event handler, for event \a e, can be reimplemented in a
    subclass to receive mouse release events for the widget.

    \sa mouseReleaseEvent(), mouseDoubleClickEvent(),
    mouseMoveEvent(), event(),  QMouseEvent
*/

void QWidget::mouseReleaseEvent( QMouseEvent * e )
{
    e->ignore();
}

/*!
    This event handler, for event \a e, can be reimplemented in a
    subclass to receive mouse double click events for the widget.

    The default implementation generates a normal mouse press event.

    Note that the widgets gets a mousePressEvent() and a
    mouseReleaseEvent() before the mouseDoubleClickEvent().

    \sa mousePressEvent(), mouseReleaseEvent() mouseMoveEvent(),
    event(), QMouseEvent
*/

void QWidget::mouseDoubleClickEvent( QMouseEvent *e )
{
    mousePressEvent( e );			// try mouse press event
}

#ifndef QT_NO_WHEELEVENT
/*!
    This event handler, for event \a e, can be reimplemented in a
    subclass to receive wheel events for the widget.

    If you reimplement this handler, it is very important that you
    \link QWheelEvent ignore()\endlink the event if you do not handle
    it, so that the widget's parent can interpret it.

    The default implementation ignores the event.

    \sa QWheelEvent::ignore(), QWheelEvent::accept(), event(),
    QWheelEvent
*/

void QWidget::wheelEvent( QWheelEvent *e )
{
    e->ignore();
}
#endif

/*!
    This event handler, for event \a e, can be reimplemented in a
    subclass to receive tablet events for the widget.

    If you reimplement this handler, it is very important that you
    \link QTabletEvent ignore()\endlink the event if you do not handle
    it, so that the widget's parent can interpret it.

    The default implementation ignores the event.

    \sa QTabletEvent::ignore(), QTabletEvent::accept(), event(),
    QTabletEvent
*/

void QWidget::tabletEvent( QTabletEvent *e )
{
    e->ignore();
}

/*!
    This event handler, for event \a e, can be reimplemented in a
    subclass to receive key press events for the widget.

    A widget must call setFocusPolicy() to accept focus initially and
    have focus in order to receive a key press event.

    If you reimplement this handler, it is very important that you
    \link QKeyEvent ignore()\endlink the event if you do not
    understand it, so that the widget's parent can interpret it.

    The default implementation closes popup widgets if the user
    presses Esc. Otherwise the event is ignored.

    \sa keyReleaseEvent(), QKeyEvent::ignore(), setFocusPolicy(),
    focusInEvent(), focusOutEvent(), event(), QKeyEvent
*/

void QWidget::keyPressEvent( QKeyEvent *e )
{
    if ( isPopup() && e->key() == Key_Escape ) {
	e->accept();
	close();
    } else {
	e->ignore();
    }
}

/*!
    This event handler, for event \a e, can be reimplemented in a
    subclass to receive key release events for the widget.

    A widget must \link setFocusPolicy() accept focus\endlink
    initially and \link hasFocus() have focus\endlink in order to
    receive a key release event.

    If you reimplement this handler, it is very important that you
    \link QKeyEvent ignore()\endlink the release if you do not
    understand it, so that the widget's parent can interpret it.

    The default implementation ignores the event.

    \sa keyPressEvent(), QKeyEvent::ignore(), setFocusPolicy(),
    focusInEvent(), focusOutEvent(), event(), QKeyEvent
*/

void QWidget::keyReleaseEvent( QKeyEvent *e )
{
    e->ignore();
}

/*!
    This event handler can be reimplemented in a subclass to receive
    keyboard focus events (focus received) for the widget.

    A widget normally must setFocusPolicy() to something other than
    \c NoFocus in order to receive focus events. (Note that the
    application programmer can call setFocus() on any widget, even
    those that do not normally accept focus.)

    The default implementation updates the widget (except for toplevel
    widgets that do not specify a focusPolicy() ). It also calls
    setMicroFocusHint(), hinting any system-specific input tools about
    the focus of the user's attention.

    \sa focusOutEvent(), setFocusPolicy(), keyPressEvent(),
    keyReleaseEvent(), event(), QFocusEvent
*/

void QWidget::focusInEvent( QFocusEvent * )
{
    if ( focusPolicy() != NoFocus || !isTopLevel() ) {
	update();
	if ( testWState(WState_AutoMask) )
	    updateMask();
	setMicroFocusHint(width()/2, 0, 1, height(), FALSE);
    }
}

/*!
    This event handler can be reimplemented in a subclass to receive
    keyboard focus events (focus lost) for the widget.

    A widget normally must setFocusPolicy() to something other than
    \c NoFocus in order to receive focus events. (Note that the
    application programmer can call setFocus() on any widget, even
    those that do not normally accept focus.)

    The default implementation updates the widget (except for toplevel
    widgets that do not specify a focusPolicy() ). It also calls
    setMicroFocusHint(), hinting any system-specific input tools about
    the focus of the user's attention.

    \sa focusInEvent(), setFocusPolicy(), keyPressEvent(),
    keyReleaseEvent(), event(), QFocusEvent
*/

void QWidget::focusOutEvent( QFocusEvent * )
{
    if ( focusPolicy() != NoFocus || !isTopLevel() ){
	update();
	if ( testWState(WState_AutoMask) )
	    updateMask();
    }
}

/*!
    \property QWidget::microFocusHint
    \brief the currently set micro focus hint for this widget.

    See the documentation of setMicroFocusHint() for more information.
*/
QRect QWidget::microFocusHint() const
{
    if ( !d->extra || d->extra->micro_focus_hint.isEmpty() )
	return QRect(width()/2, 0, 1, height() );
    else
	return d->extra->micro_focus_hint;
}

/*!
    This event handler can be reimplemented in a subclass to receive
    widget enter events.

    An event is sent to the widget when the mouse cursor enters the
    widget.

    \sa leaveEvent(), mouseMoveEvent(), event()
*/

void QWidget::enterEvent( QEvent * )
{
}

/*!
    This event handler can be reimplemented in a subclass to receive
    widget leave events.

    A leave event is sent to the widget when the mouse cursor leaves
    the widget.

    \sa enterEvent(), mouseMoveEvent(), event()
*/

void QWidget::leaveEvent( QEvent * )
{
}

/*!
    This event handler can be reimplemented in a subclass to receive
    paint events.

    A paint event is a request to repaint all or part of the widget.
    It can happen as a result of repaint() or update(), or because the
    widget was obscured and has now been uncovered, or for many other
    reasons.

    Many widgets can simply repaint their entire surface when asked
    to, but some slow widgets need to optimize by painting only the
    requested region: QPaintEvent::region(). This speed optimization
    does not change the result, as painting is clipped to that region
    during event processing. QListView and QCanvas do this, for
    example.

    Qt also tries to speed up painting by merging multiple paint
    events into one. When update() is called several times or the
    window system sends several paint events, Qt merges these events
    into one event with a larger region (see QRegion::unite()).
    repaint() does not permit this optimization, so we suggest using
    update() when possible.

    When the paint event occurs, the update region has normally been
    erased, so that you're painting on the widget's background. There
    are a couple of exceptions and QPaintEvent::erased() tells you
    whether the widget has been erased or not.

    The background can be set using setBackground() or
    setBackgroundRole().

    \sa event(), repaint(), update(), QPainter, QPixmap, QPaintEvent
*/

void QWidget::paintEvent( QPaintEvent * )
{
}


/*!
    This event handler can be reimplemented in a subclass to receive
    widget move events. When the widget receives this event, it is
    already at the new position.

    The old position is accessible through QMoveEvent::oldPos().

    \sa resizeEvent(), event(), move(), QMoveEvent
*/

void QWidget::moveEvent( QMoveEvent * )
{
}


/*!
    This event handler can be reimplemented in a subclass to receive
    widget resize events. When resizeEvent() is called, the widget
    already has its new geometry. The old size is accessible through
    QResizeEvent::oldSize().

    The widget will be erased and receive a paint event immediately
    after processing the resize event. No drawing need be (or should
    be) done inside this handler.

    Widgets that have been created with the \c WNoAutoErase flag
    will not be erased. Nevertheless, they will receive a paint event
    for their entire area afterwards. Again, no drawing needs to be
    done inside this handler.

    The default implementation calls updateMask() if the widget has
    \link QWidget::setAutoMask() automatic masking\endlink enabled.

    \sa moveEvent(), event(), resize(), QResizeEvent, paintEvent()
*/

void QWidget::resizeEvent( QResizeEvent * )
{
    if ( testWState(WState_AutoMask) )
	updateMask();
}

/*!
    This event handler, for event \a e, can be reimplemented in a
    subclass to receive widget close events.

    The default implementation calls e->accept(), which hides this
    widget. See the \l QCloseEvent documentation for more details.

    \sa event(), hide(), close(), QCloseEvent
*/

void QWidget::closeEvent( QCloseEvent *e )
{
    e->accept();
}


/*!
    This event handler, for event \a e, can be reimplemented in a
    subclass to receive widget context menu events.

    The default implementation calls e->ignore(), which rejects the
    context event. See the \l QContextMenuEvent documentation for
    more details.

    \sa event(), QContextMenuEvent
*/

void QWidget::contextMenuEvent( QContextMenuEvent *e )
{
    e->ignore();
}


/*!
    This event handler, for event \a e, can be reimplemented in a
    subclass to receive Input Method composition events. This handler
    is called when the user begins entering text using an Input Method.

    The default implementation calls e->ignore(), which rejects the
    Input Method event. See the \l QIMEvent documentation for more
    details.

    \sa event(), QIMEvent
*/
void QWidget::imStartEvent( QIMEvent *e )
{
    e->ignore();
}

/*!
    This event handler, for event \a e, can be reimplemented in a
    subclass to receive Input Method composition events. This handler
    is called when the user has entered some text using an Input Method.

    The default implementation calls e->ignore(), which rejects the
    Input Method event. See the \l QIMEvent documentation for more
    details.

    \sa event(), QIMEvent
*/
void QWidget::imComposeEvent( QIMEvent *e )
{
    e->ignore();
}


/*!
    This event handler, for event \a e, can be reimplemented in a
    subclass to receive Input Method composition events. This handler
    is called when the user has finished inputting text via an Input
    Method.

    The default implementation calls e->ignore(), which rejects the
    Input Method event. See the \l QIMEvent documentation for more
    details.

    \sa event(), QIMEvent
*/
void QWidget::imEndEvent( QIMEvent *e )
{
    e->ignore();
}


#ifndef QT_NO_DRAGANDDROP

/*!
    This event handler is called when a drag is in progress and the
    mouse enters this widget.

    See the \link dnd.html Drag-and-drop documentation\endlink for an
    overview of how to provide drag-and-drop in your application.

    \sa QTextDrag, QImageDrag, QDragEnterEvent
*/
void QWidget::dragEnterEvent( QDragEnterEvent * )
{
}

/*!
    This event handler is called when a drag is in progress and the
    mouse enters this widget, and whenever it moves within the widget.

    See the \link dnd.html Drag-and-drop documentation\endlink for an
    overview of how to provide drag-and-drop in your application.

    \sa QTextDrag, QImageDrag, QDragMoveEvent
*/
void QWidget::dragMoveEvent( QDragMoveEvent * )
{
}

/*!
    This event handler is called when a drag is in progress and the
    mouse leaves this widget.

    See the \link dnd.html Drag-and-drop documentation\endlink for an
    overview of how to provide drag-and-drop in your application.

    \sa QTextDrag, QImageDrag, QDragLeaveEvent
*/
void QWidget::dragLeaveEvent( QDragLeaveEvent * )
{
}

/*!
    This event handler is called when the drag is dropped on this
    widget.

    See the \link dnd.html Drag-and-drop documentation\endlink for an
    overview of how to provide drag-and-drop in your application.

    \sa QTextDrag, QImageDrag, QDropEvent
*/
void QWidget::dropEvent( QDropEvent * )
{
}

#endif // QT_NO_DRAGANDDROP

/*!
    This event handler can be reimplemented in a subclass to receive
    widget show events.

    Non-spontaneous show events are sent to widgets immediately before
    they are shown. The spontaneous show events of top-level widgets
    are delivered afterwards.

    \sa event(), QShowEvent
*/
void QWidget::showEvent( QShowEvent * )
{
}

/*!
    This event handler can be reimplemented in a subclass to receive
    widget hide events.

    Hide events are sent to widgets immediately after they have been
    hidden.

    \sa event(), QHideEvent
*/
void QWidget::hideEvent( QHideEvent * )
{
}

/*
    \fn QWidget::x11Event( MSG * )

    This special event handler can be reimplemented in a subclass to
    receive native X11 events.

    In your reimplementation of this function, if you want to stop the
    event being handled by Qt, return TRUE. If you return FALSE, this
    native event is passed back to Qt, which translates the event into
    a Qt event and sends it to the widget.

    \warning This function is not portable.

    \sa QApplication::x11EventFilter()
*/


#if defined(Q_WS_MAC)

/*!
    This special event handler can be reimplemented in a subclass to
    receive native Macintosh events.

    In your reimplementation of this function, if you want to stop the
    event being handled by Qt, return TRUE. If you return FALSE, this
    native event is passed back to Qt, which translates the event into
    a Qt event and sends it to the widget.

    \warning This function is not portable.

    \sa QApplication::macEventFilter()
*/

bool QWidget::macEvent( MSG * )
{
    return FALSE;
}

#endif
#if defined(Q_WS_WIN)

/*!
    This special event handler can be reimplemented in a subclass to
    receive native Windows events.

    In your reimplementation of this function, if you want to stop the
    event being handled by Qt, return TRUE. If you return FALSE, this
    native event is passed back to Qt, which translates the event into
    a Qt event and sends it to the widget.

    \warning This function is not portable.

    \sa QApplication::winEventFilter()
*/
bool QWidget::winEvent( MSG * )
{
    return FALSE;
}

#endif
#if defined(Q_WS_X11)

/*!
    This special event handler can be reimplemented in a subclass to
    receive native X11 events.

    In your reimplementation of this function, if you want to stop the
    event being handled by Qt, return TRUE. If you return FALSE, this
    native event is passed back to Qt, which translates the event into
    a Qt event and sends it to the widget.

    \warning This function is not portable.

    \sa QApplication::x11EventFilter()
*/
bool QWidget::x11Event( XEvent * )
{
    return FALSE;
}

#endif
#if defined(Q_WS_QWS)

/*!
    This special event handler can be reimplemented in a subclass to
    receive native Qt/Embedded events.

    In your reimplementation of this function, if you want to stop the
    event being handled by Qt, return TRUE. If you return FALSE, this
    native event is passed back to Qt, which translates the event into
    a Qt event and sends it to the widget.

    \warning This function is not portable.

    \sa QApplication::qwsEventFilter()
*/
bool QWidget::qwsEvent( QWSEvent * )
{
    return FALSE;
}

#endif

/*!
    \property QWidget::autoMask
    \brief whether the auto mask feature is enabled for the widget

    Transparent widgets use a mask to define their visible region.
    QWidget has some built-in support to make the task of
    recalculating the mask easier. When setting auto mask to TRUE,
    updateMask() will be called whenever the widget is resized or
    changes its focus state. Note that you must reimplement
    updateMask() (which should include a call to setMask()) or nothing
    will happen.

    Note: when you re-implement resizeEvent(), focusInEvent() or
    focusOutEvent() in your custom widgets and still want to ensure
    that the auto mask calculation works, you should add:

    \code
	if ( autoMask() )
	    updateMask();
    \endcode

    at the end of your event handlers. This is true for all member
    functions that change the appearance of the widget in a way that
    requires a recalculation of the mask.

    While being a technically appealing concept, masks have a big
    drawback: when using complex masks that cannot be expressed easily
    with relatively simple regions, they can be very slow on some
    window systems. The classic example is a transparent label. The
    complex shape of its contents makes it necessary to represent its
    mask by a bitmap, which consumes both memory and time. If all you
    want is to blend the background of several neighboring widgets
    together seamlessly, you will probably want to use
    setBackgroundOrigin() rather than a mask.

    \sa autoMask() updateMask() setMask() clearMask() setBackgroundOrigin()
*/

bool QWidget::autoMask() const
{
    return testWState(WState_AutoMask);
}

void QWidget::setAutoMask( bool enable )
{
    if ( enable == autoMask() )
	return;

    if ( enable ) {
	setWState(WState_AutoMask);
	updateMask();
    } else {
	clearWState(WState_AutoMask);
	clearMask();
    }
}

/*!
    \enum QWidget::BackgroundOrigin

    This enum defines the origin used to draw a widget's background
    pixmap.

    The pixmap is drawn using the:
    \value WidgetOrigin  widget's coordinate system.
    \value ParentOrigin  parent's coordinate system.
    \value WindowOrigin  top-level window's coordinate system.
    \value AncestorOrigin  same origin as the parent uses.
*/

/*!
    \property QWidget::backgroundOrigin
    \brief the origin of the widget's background

    The origin is either WidgetOrigin (the default), ParentOrigin,
    WindowOrigin or AncestorOrigin.

    This only makes a difference if the widget has a background
    pixmap, in which case positioning matters. Using \c WindowOrigin
    for several neighboring widgets makes the background blend
    together seamlessly. \c AncestorOrigin allows blending backgrounds
    seamlessly when an ancestor of the widget has an origin other than
    \c QWindowOrigin.

    \sa background()
*/
QWidget::BackgroundOrigin QWidget::backgroundOrigin() const
{
    return d->extra ? (BackgroundOrigin)d->extra->bg_origin : WidgetOrigin;
}

void QWidget::setBackgroundOrigin( BackgroundOrigin origin )
{
    if ( origin == backgroundOrigin() )
	return;
    d->createExtra();
    d->extra->bg_origin = origin;
    update();
}

/*!
    This function can be reimplemented in a subclass to support
    transparent widgets. It should be called whenever a widget changes
    state in a way that means that the shape mask must be recalculated.

    \sa setAutoMask(), updateMask(), setMask(), clearMask()
*/
void QWidget::updateMask()
{
}

/*!
  \internal
  Returns the offset of the widget from the backgroundOrigin.
*/
QPoint QWidget::backgroundOffset() const
{
    if (!isTopLevel()) {
	switch(backgroundOrigin()) {
	    case WidgetOrigin:
		break;
	    case ParentOrigin:
		return pos();
	    case WindowOrigin:
		{
		    const QWidget *topl = this;
		    while(topl && !topl->isTopLevel() && !topl->testWFlags(Qt::WSubWindow))
			topl = topl->parentWidget(TRUE);
		    return mapTo((QWidget *)topl, QPoint(0, 0) );
		}
	    case AncestorOrigin:
		{
		    const QWidget *topl = this;
		    bool ancestorIsWindowOrigin = FALSE;
		    while(topl && !topl->isTopLevel() && !topl->testWFlags(Qt::WSubWindow))
		    {
			if (!ancestorIsWindowOrigin) {
			    if (topl->backgroundOrigin() == QWidget::WidgetOrigin)
				break;
			    if (topl->backgroundOrigin() == QWidget::ParentOrigin)
			    {
				topl = topl->parentWidget(TRUE);
				break;
			    }
			    if (topl->backgroundOrigin() == QWidget::WindowOrigin)
				ancestorIsWindowOrigin = TRUE;
			}
			topl = topl->parentWidget(TRUE);
		    }

		    return mapTo((QWidget *) topl, QPoint(0,0) );
		}
	}
    }
    // fall back
    return QPoint(0,0);
}

/*!
    Returns the layout engine that manages the geometry of this
    widget's children.

    If the widget does not have a layout, layout() returns 0.

    \sa  sizePolicy()
*/
QLayout* QWidget::layout() const
{
    return d->layout;
}



/*!
    \property QWidget::sizePolicy
    \brief the default layout behavior of the widget

    If there is a QLayout that manages this widget's children, the
    size policy specified by that layout is used. If there is no such
    QLayout, the result of this function is used.

    The default policy is Preferred/Preferred, which means that the
    widget can be freely resized, but prefers to be the size
    sizeHint() returns. Button-like widgets set the size policy to
    specify that they may stretch horizontally, but are fixed
    vertically. The same applies to lineedit controls (such as
    QLineEdit, QSpinBox or an editable QComboBox) and other
    horizontally orientated widgets (such as QProgressBar).
    QToolButton's are normally square, so they allow growth in both
    directions. Widgets that support different directions (such as
    QSlider, QScrollBar or QHeader) specify stretching in the
    respective direction only. Widgets that can provide scrollbars
    (usually subclasses of QScrollView) tend to specify that they can
    use additional space, and that they can make do with less than
    sizeHint().

    \sa sizeHint() QLayout QSizePolicy updateGeometry()
*/
QSizePolicy QWidget::sizePolicy() const
{
    return d->extra ? d->extra->size_policy
	: QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
}

void QWidget::setSizePolicy( QSizePolicy policy )
{
    setWState( WState_OwnSizePolicy );
    if ( policy == sizePolicy() )
	return;
    d->createExtra();
    d->extra->size_policy = policy;
    updateGeometry();
}

/*!
    \overload void QWidget::setSizePolicy( QSizePolicy::SizeType hor, QSizePolicy::SizeType ver, bool hfw )

    Sets the size policy of the widget to \a hor, \a ver and \a hfw
    (height for width).

    \sa QSizePolicy::QSizePolicy()
*/

/*!
    Returns the preferred height for this widget, given the width \a
    w. The default implementation returns 0, indicating that the
    preferred height does not depend on the width.

    \warning Does not look at the widget's layout.
*/

int QWidget::heightForWidth( int w ) const
{
    (void)w;
    return 0;
}

/*!
    \property QWidget::customWhatsThis
    \brief whether the widget wants to handle What's This help manually

    The default implementation of customWhatsThis() returns FALSE,
    which means the widget will not receive any events in Whats This
    mode.

    The widget may leave What's This mode by calling
    QWhatsThis::leaveWhatsThisMode(), with or without actually
    displaying any help text.

    You can also reimplement customWhatsThis() if your widget is a
    "passive interactor" supposed to work under all circumstances.
    Simply don't call QWhatsThis::leaveWhatsThisMode() in that case.

    \sa QWhatsThis::inWhatsThisMode() QWhatsThis::leaveWhatsThisMode()
*/
bool QWidget::customWhatsThis() const
{
    return FALSE;
}

/*!
    Returns the visible child widget at pixel position \a (x, y) in
    the widget's own coordinate system.

    If \a includeThis is TRUE, and there is no child visible at \a (x,
    y), the widget itself is returned.
*/
QWidget  *QWidget::childAt( int x, int y, bool includeThis ) const
{
    if ( !rect().contains( x, y ) )
	return 0;
    for (int i = d->children.size(); i > 0 ; ) {
	--i;
	QWidget *w = static_cast<QWidget *>(d->children.at(i));
	QWidget *t;
	if ( w->isWidgetType() && !w->isTopLevel() && !w->isHidden() ) {
	    if ( ( t = w->childAt( x - w->x(), y - w->y(), TRUE ) ) )
		return t;
	}
    }
    if ( includeThis )
	return const_cast<QWidget*>(this);
    return 0;
}

/*!
    \overload

    Returns the visible child widget at point \a p in the widget's own
    coordinate system.

    If \a includeThis is TRUE, and there is no child visible at \a p,
    the widget itself is returned.

*/
QWidget  *QWidget::childAt( const QPoint & p, bool includeThis ) const
{
    return childAt( p.x(), p.y(), includeThis );
}

/*!
    Notifies the layout system that this widget has changed and may
    need to change geometry.

    Call this function if the sizeHint() or sizePolicy() have changed.

    For explicitly hidden widgets, updateGeometry() is a no-op. The
    layout system will be notified as soon as the widget is shown.
*/

void QWidget::updateGeometry()
{
    if (!isTopLevel() && isShown() && parentWidget() && parentWidget()->d->layout)
	parentWidget()->d->layout->update();
}


/*!
    Reparents the widget. The widget gets a new \a parent, new widget
    flags (\a f, but as usual, use 0) and is moved to position (0,0)
    in its new parent.

    If the new parent widget is in a different top-level widget, the
    reparented widget and its children are appended to the end of the
    \link setFocusPolicy() tab chain \endlink of the new parent
    widget, in the same internal order as before. If one of the moved
    widgets had keyboard focus, reparent() calls clearFocus() for that
    widget.

    If the new parent widget is in the same top-level widget as the
    old parent, reparent doesn't change the tab order or keyboard
    focus.

    \warning It is extremely unlikely that you will ever need this
    function. If you have a widget that changes its content
    dynamically, it is far easier to use \l QWidgetStack or \l
    QWizard.

    \sa getWFlags()
*/

void QWidget::reparent(QWidget *parent, WFlags f)
{
    reparent_helper( parent, f, QPoint(0,0), false);
    QEvent e( QEvent::Reparent );
    QApplication::sendEvent( this, &e );
    if (!testAttribute(WA_SetFont))
	unsetFont();
    else
	setFont_helper( fnt.resolve( qt_naturalWidgetFont( this ) ) );
    if (!testAttribute(WA_SetPalette))
	unsetPalette();
}

/*!
    \overload

    A convenience version of reparent that does not take widget flags
    as argument.

    Calls reparent(\a parent, getWFlags() \& ~\l WType_Mask).
*/
void QWidget::reparent( QWidget *parent)
{
    reparent( parent, getWFlags() & ~WType_Mask);
}


/*!
    Shows the widget in full-screen mode.

    Calling this function only affects top-level widgets.

    To return from full-screen mode, call showNormal().

    Full-screen mode works fine under Windows, but has certain
    problems under X. These problems are due to limitations of the
    ICCCM protocol that specifies the communication between X11
    clients and the window manager. ICCCM simply does not understand
    the concept of non-decorated full-screen windows. Therefore, the
    best we can do is to request a borderless window and place and
    resize it to fill the entire screen. Depending on the window
    manager, this may or may not work. The borderless window is
    requested using MOTIF hints, which are at least partially
    supported by virtually all modern window managers.

    An alternative would be to bypass the window manager entirely and
    create a window with the WX11BypassWM flag. This has other severe
    problems though, like totally broken keyboard focus and very
    strange effects on desktop changes or when the user raises other
    windows.

    Future X11 window managers that follow modern post-ICCCM
    specifications may support full-screen mode properly.

    \sa showNormal(), showMaximized(), show(), hide(), isVisible()
*/

#ifdef Q_OS_TEMP
# if UNDER_CE < 310
#  include <aygshell.h>
# else
#  define SHFS_HIDETASKBAR            0x0002
#  define SHFS_HIDESIPBUTTON          0x0008
   extern "C" BOOL __cdecl SHFullScreen(HWND hwndRequester, DWORD dwState);
# endif
#endif

void QWidget::showFullScreen()
{
    if ( !isTopLevel() )
	return;
    if ( d->topData()->fullscreen ) {
	show();
	raise();
	return;
    }
    if ( d->topData()->normalGeometry.width() < 0 )
	d->topData()->normalGeometry = QRect( pos(), size() );
    d->topData()->savedFlags = getWFlags();
    reparent( 0, WType_TopLevel | WStyle_Customize | WStyle_NoBorder |
	      // preserve some widget flags
	      (getWFlags() & 0xffff0000),
	      mapToGlobal( QPoint( 0, 0) ));
    d->topData()->fullscreen = 1;
    const QRect screen = qApp->desktop()->screenGeometry( qApp->desktop()->screenNumber( this ) );
    move( screen.topLeft() );
    resize( screen.size() );
    raise();
    show();
#ifdef Q_OS_TEMP
    SHFullScreen( winId(), SHFS_HIDETASKBAR | SHFS_HIDESIPBUTTON );
#endif
    QEvent e( QEvent::ShowFullScreen );
    QApplication::sendEvent( this, &e );
#if defined(Q_WS_X11)
    extern void qt_wait_for_window_manager( QWidget* w ); // defined in qwidget_x11.cpp
    qt_wait_for_window_manager( this );
#endif

    setActiveWindow();
}

/*!
    \property QWidget::fullScreen
    \brief whether the widget is full screen

    \sa minimized, maximized
*/
bool QWidget::isFullScreen() const
{
    QWidget* that = (QWidget*)this;
    return isTopLevel() && that->d->topData()->fullscreen;
}

void QWidget::repaint( bool erase )
{
    repaint( visibleRect(), erase );
}


/*!
    \overload void QWidget::drawText( int x, int y, const QString& str )

    Draws the string \a str at position \a pos.
*/

/*!
    Draws the string \a str at position \a(x, y).

    The \a y position is the base line position of the text. The text
    is drawn using the default font and the default foreground color.

    This function is provided for convenience. You will generally get
    more flexible results and often higher speed by using a a \link
    QPainter painter\endlink instead.

    \sa setFont(), foregroundColor(), QPainter::drawText()
*/

void QWidget::drawText(const QPoint &p, const QString &str)
{
    if(!testWState(WState_Visible))
	return;
    QPainter paint(this);
    paint.drawText(p.x(), p.y(), str);
}

/*!
    \enum QWidget::WidgetAttribute

    \keyword widget attributes

    This enum type is used to specify various widget
    attributes. Attributes are set and cleared with
    QWidget::setAttribute(), and queried with QWidget::hasAttribute().

    \table
    \header \i Attribute \i Meaning \i Set/cleared by

    \row \i WA_KeyCompression \i Enables key event compression if set,
    and disables it if not set. By default key compression is off, so
    widgets receive one key press event for each key press (or more,
    since autorepeat is usually on). If you turn it on and your
    program doesn't keep up with key input, Qt may try to compress key
    events so that more than one character can be processed in each
    event.

    For example, a word processor widget might receive 2, 3 or more
    characters in each QKeyEvent::text(), if the layout recalculation
    takes too long for the CPU.

    If a widget supports multiple character unicode input, it is
    always safe to turn the compression on.

    Qt performs key event compression only for printable characters.
    Modifier keys, cursor movement keys, function keys and
    miscellaneous action keys (e.g. Escape, Enter, Backspace,
    PrintScreen) will stop key event compression, even if there are
    more compressible key events available.

    Not all platforms support this compression, in which case turning
    it on will have no effect.

    \i widget author

    \row \i WA_PendingMoveEvent \i Indicates that a move event is
    pending, e.g. when a hidden widget was moved. \i Qt kernel

    \row \i WA_PendingResizeEvent \i Indicates that a resize event is
    pending, e.g. when a hidden widget was resized. \i Qt kernel

    \row \i WA_UnderMouse \i Indicates that the widget is under the
    mouse cursor. The value is not updated correctly during drag and
    drop operations. There is also a getter function
    QWidget::underMouse(). \i Qt kernel

    \row \i WA_Disabled \i Indicates that the widget is disabled, i.e.
    it does not receive any mouse or keyboard events. There is also a
    getter functions QWidget::isEnabled().  \i Qt kernel

    \row \i WA_ContentsPropagated \i Allows the contents painted in a
    QWidget::paintEvent() to be used as the background for children
    that inherit their background. \i widget author or style

    \row \i WA_ForceDisabled \i Indicates that the widget is
    explicitely disabled, i.e. it will remain disabled even when all
    its ancestors are set to the enabled state. This implies
    WA_Disabled. \i Function QWidget::setEnabled() and
    QWidget::setDisabled()

    \row \i WA_SetPalette \i Indicates that the widgets has a palette
    of its own.  \i Functions QWidget::setPalette() and
    QWidget::unsetPalette()

    \row \i WA_SetFont \i Indicates that the widgets has a font of its
    own. \i Functions QWidget::setFont() and QWidget::unsetFont()

    \row \i WA_SetCursor \i Indicates that the widgets has a cursor of its
    own. \i Functions QWidget::setCursor() and QWidget::unsetCursorz()

    \row \i WA_SetForegroundRole \i Indicates that the widgets has an
    explicit foreground role \i Function QWidget::setForegroundRole()

    \row \i WA_SetBackgroundRole \i Indicates that the widgets has an
    explicit background role\i Function QWidget::setBackgroundRole()

    \endtable
*/

/*! Sets the attribute \a attribute on this widget if \b is true;
  otherwise clears the attribute.

  \sa hasAttribute()
 */
void QWidget::setAttribute(WidgetAttribute attribute, bool b)
{
    if (attribute <= int(8*sizeof(uint))) {
	if (b)
	    widget_attributes |= (1<<attribute);
	else
	    widget_attributes &= ~(1<<attribute);
    } else {
	int x = attribute - 8*sizeof(uint);
	if (b)
	    d->high_attributes[x / (8*sizeof(uint))] |= (1<<x);
	else
	    d->high_attributes[x / (8*sizeof(uint))] &= ~(1<<x);
    }
}

/*! \fn bool QWidget::testAttribute(WidgetAttribute attribute) const

  Returns true if attribute \a attribute is set on this widget;
  otherwise returns false.

  \sa setAttribute()
 */
bool QWidget::testAttribute_helper(WidgetAttribute attribute) const
{
    int x = attribute - 8*sizeof(uint);
    return (d->high_attributes[x / (8*sizeof(uint))] & (1<<x));
}
