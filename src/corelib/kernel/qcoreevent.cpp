/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qcoreevent.h"
#include "qcoreapplication.h"
#include "qcoreapplication_p.h"

/*!
    \class QEvent
    \brief The QEvent class is the base class of all
    event classes. Event objects contain event parameters.

    \ingroup events
    \ingroup environment

    Qt's main event loop (QCoreApplication::exec()) fetches native
    window system events from the event queue, translates them into
    QEvents, and sends the translated events to \l{QObject}s.

    In general, events come from the underlying window system
    (spontaneous() returns true), but it is also possible to manually
    send events using QCoreApplication::sendEvent() and
    QCoreApplication::postEvent() (spontaneous() returns false).

    QObjects receive events by having their QObject::event() function
    called. The function can be reimplemented in subclasses to
    customize event handling and add additional event types;
    QWidget::event() is a notable example. By default, events are
    dispatched to event handlers like QObject::timerEvent() and
    QWidget::mouseMoveEvent(). QObject::installEventFilter() allows an
    object to intercept events destined for another object.

    The basic QEvent contains only an event type parameter and an \l
    accept flag.  The accept flag set with accept(), and cleared with
    ignore(). It is set by default, but don't rely on this as
    subclasses may choose to clear it in their constructor.

    Subclasses of QEvent contain additional parameters that describe
    the particular event.

    \sa QObject::event(), QObject::installEventFilter(),
        QWidget::event(), QCoreApplication::sendEvent(),
        QCoreApplication::postEvent(), QCoreApplication::processEvents()
*/


/*!
    \enum QEvent::Type

    This enum type defines the valid event types in Qt. The event
    types and the specialized classes for each type are as follows:

    \value None                             Not an event.
    \value Accessibility                    Accessibility information is requested.
    \value AccessibilityHelp                Used to query for additional information about complex widgets (QAccessibleEvent).
    \value ActionAdded                      A new action has been added (QActionEvent).
    \value ActionChanged                    An action has been changed (QActionEvent).
    \value ActionRemoved                    An action has been removed (QActionEvent).
    \value ActivationChange                 A widget's top-level window activation state has changed.
    \value ApplicationActivated             The application has been made available to the user.
    \value ApplicationDeactivated           The application has been suspended, and is unavailable to the user.
    \value ApplicationFontChange            The default application font has changed.
    \value ApplicationLayoutDirectionChange The default application layout direction has changed.
    \value ApplicationPaletteChange         The default application palette has changed.
    \value ApplicationWindowIconChange      The application's icon has changed.
    \value ChildAdded                       An object gets a child (QChildEvent).
    \value ChildPolished                    A widget child gets polished (QChildEvent).
    \value ChildRemoved                     An object loses a child (QChildEvent).
    \value Clipboard                        The clipboard contents have changed (QClipboardEvent).
    \value Close                            Widget was closed (QCloseEvent).
    \value ContextMenu                      Context popup menu (QContextMenuEvent).
    \value DeferredDelete                   The object will be deleted after it has cleaned up.
    \value DragEnter                        The cursor enters a widget during a drag and drop action (QDragEnterEvent).
    \value DragLeave                        The cursor leaves a widget during a drag and drop action (QDragLeaveEvent).
    \value DragMove                         A drag and drop action is in progress (QDragMoveEvent).
    \value Drop                             A drag and drop action is completed (QDropEvent).
    \value EnabledChange                    Widget's enabled state has changed.
    \value Enter                            Mouse enters widget's boundaries.
    \value EnterWhatsThisMode               Send to toplevel widgets when the application enters "What's This?" mode.
    \value FileOpen                         File open request (QFileOpenEvent).
    \value FocusIn                          Widget gains keyboard focus (QFocusEvent).
    \value FocusOut                         Widget loses keyboard focus (QFocusEvent).
    \value FontChange                       Widget's font has changed.
    \value Hide                             Widget was hidden (QHideEvent).
    \value HideToParent                     A child widget has been hidden.
    \value HoverEnter                       The mouse cursor enters a hover widget (QHoverEvent).
    \value HoverLeave                       The mouse cursor leaves a hover widget (QHoverEvent).
    \value HoverMove                        The mouse cursor moves inside a hover widget (QHoverEvent).
    \value IconDrag                         The main icon of a window has been dragged away (QIconDragEvent).
    \value IconTextChange                   Widget's icon text has been changed.
    \value InputMethod                      An input method is being used (QInputMethodEvent).
    \value KeyPress                         Key press (QKeyEvent).
    \value KeyRelease                       Key release (QKeyEvent).
    \value LanguageChange                   The application translation changed.
    \value LayoutDirectionChange            The direction of layouts changed.
    \value LayoutRequest                    Widget layout needs to be redone.
    \value Leave                            Mouse leaves widget's boundaries.
    \value LeaveWhatsThisMode               Send to toplevel widgets when the application leaves "What's This?" mode.
    \value LocaleChange                     The system locale has changed.
    \value MetaCall                         An asynchronous method invocation via QMetaObject::invokeMethod().
    \value ModifiedChange                   Widgets modification state has been changed.
    \value MouseButtonDblClick              Mouse press again (QMouseEvent).
    \value MouseButtonPress                 Mouse press (QMouseEvent).
    \value MouseButtonRelease               Mouse release (QMouseEvent).
    \value MouseMove                        Mouse move (QMouseEvent).
    \value MouseTrackingChange              The mouse tracking state has changed.
    \value Move                             Widget's position changed (QMoveEvent).
    \value Paint                            Screen update necessary (QPaintEvent).
    \value PaletteChange                    Palette of the widget changed.
    \value Polish                           The widget is polished.
    \value PolishRequest                    The widget should be polished.
    \value QueryWhatsThis                   The widget should accept the event if it has "What's This?" help.
    \value Resize                           Widget's size changed (QResizeEvent).
    \value Shortcut                         Key press in child for shortcut key handling (QShortcutEvent).
    \value ShortcutOverride                 Key press in child, for overriding shortcut key handling (QKeyEvent).
    \value Show                             Widget was shown on screen (QShowEvent).
    \value ShowToParent                     A child widget has been shown.
    \value SockAct                          Socket activated, used to implement QSocketNotifier.
    \value StatusTip                        A status tip is requested (QStatusTipEvent).
    \value StyleChange                      Widget's style has been changed.
    \value TabletMove                       Wacom tablet move (QTabletEvent).
    \value TabletPress                      Wacom tablet press (QTabletEvent).
    \value TabletRelease                    Wacom tablet release (QTabletEvent).
    \value Timer                            Regular timer events (QTimerEvent).
    \value ToolBarChange                    The toolbar button is toggled on Mac OS X.
    \value ToolTip                          A tooltip was requested (QHelpEvent).
    \value UpdateRequest                    The widget should be repainted.
    \value WhatsThis                        The widget should reveal "What's This?" help (QHelpEvent).
    \value WhatsThisClicked                 A link in a widget's "What's This?" help was clicked.
    \value Wheel                            Mouse wheel rolled (QWheelEvent).
    \value WinEventAct                      A Windows-specific activation event has occurred.
    \value WindowActivate                   Window was activated.
    \value WindowBlocked                    The window is blocked by a modal dialog.
    \value WindowDeactivate                 Window was deactivated.
    \value WindowIconChange                 The window's icon has changed.
    \value WindowStateChange                The \l{QWidget::windowState()}{window's state} (minimized, maximized or full-screen) has changed (QWindowStateChangeEvent).
    \value WindowTitleChange                The window title has changed.
    \value WindowUnblocked                  The window is unblocked after a modal dialog exited.
    \value ZOrderChange                     The widget's z-order has changed. This event is never sent to top level windows.

    User events should have values between \c User and \c{MaxUser}:

    \value User                             User-defined event.
    \value MaxUser                          Last user event ID.

    \omitvalue Accel
    \omitvalue AccelAvailable
    \omitvalue AccelOverride
    \omitvalue ActivateControl
    \omitvalue CaptionChange
    \omitvalue ChildInserted
    \omitvalue Create
    \omitvalue DeactivateControl
    \omitvalue Destroy
    \omitvalue DragResponse
    \omitvalue EmbeddingControl
    \omitvalue HelpRequest
    \omitvalue IconChange
    \omitvalue LayoutHint
    \omitvalue OkRequest
    \omitvalue ParentChange
    \omitvalue QWSUpdate
    \omitvalue Quit
    \omitvalue Reparent
    \omitvalue ShowWindowRequest
    \omitvalue Speech
    \omitvalue Style
    \omitvalue ThreadChange
*/

/*!
    Contructs an event object of type \a type.
*/
QEvent::QEvent(Type type)
    : d(0), t(type), posted(false), spont(false), m_accept(true)
{}

/*!
    Destroys the event. If it was \link
    QCoreApplication::postEvent() posted \endlink,
    it will be removed from the list of events to be posted.
*/

QEvent::~QEvent()
{
    if (posted && QCoreApplication::instance())
        QCoreApplicationPrivate::removePostedEvent(this);
}


/*!
    \property  QEvent::accepted
    the accept flag of the event object

    Setting the accept parameter indicates that the event receiver
    wants the event. Unwanted events might be propagated to the parent
    widget. By default, isAccepted() is set to true, but don't rely on
    this as subclasses may choose to clear it in their constructor.

    For convenience, the accept flag can also be set with accept(),
    and cleared with ignore().
*/

/*!
    \fn void QEvent::accept()

    Sets the accept flag of the event object, the equivalent of
    callign setAccepted(true).

    Setting the accept parameter indicates that the event receiver
    wants the event. Unwanted events might be propagated to the parent
    widget.

    \sa ignore()
*/


/*!
    \fn void QEvent::ignore()

    Clears the accept flag parameter of the event object, the
    equivalent of calling setAccepted(false).

    Clearing the accept parameter indicates that the event receiver
    does not want the event. Unwanted events might be propgated to the
    parent widget.

    \sa accept()
*/


/*!
    \fn QEvent::Type QEvent::type() const

    Returns the event type.
*/

/*!
    \fn bool QEvent::spontaneous() const

    Returns true if the event originated outside the application
    (a system event); otherwise returns false.
*/


/*!
    \class QTimerEvent qcoreevent.h
    \brief The QTimerEvent class contains parameters that describe a
    timer event.

    \ingroup events

    Timer events are sent at regular intervals to objects that have
    started one or more timers. Each timer has a unique identifier. A
    timer is started with QObject::startTimer().

    The QTimer class provides a high-level programming interface that
    uses signals instead of events. It also provides single-shot timers.

    The event handler QObject::timerEvent() receives timer events.

    \sa QTimer, QObject::timerEvent(), QObject::startTimer(),
    QObject::killTimer()
*/

/*!
    Constructs a timer event object with the timer identifier set to
    \a timerId.
*/
QTimerEvent::QTimerEvent(int timerId)
    : QEvent(Timer), id(timerId)
{}

/*! \internal
*/
QTimerEvent::~QTimerEvent()
{
}

/*!
    \fn int QTimerEvent::timerId() const

    Returns the unique timer identifier, which is the same identifier
    as returned from QObject::startTimer().
*/

/*!
    \class QChildEvent
    \brief The QChildEvent class contains event parameters for child object
    events.

    \ingroup events

    Child events are sent immediately to objects when children are
    added or removed.

    In both cases you can only rely on the child being a QObject (or,
    if QObject::isWidgetType() returns true, a QWidget). This is
    because in the \l ChildAdded case the child is not yet fully
    constructed; in the \l ChildRemoved case it might have already
    been destructed.

    The handler for these events is QObject::childEvent().
*/

/*!
    Constructs a child event object of a particular \a type for the
    \a child.

    \a type can be QEvent::ChildAdded, QEvent::ChildRemoved,
    QEvent::ChildPolished, or QEvent::ChildRemoved.

    \sa child()
*/
QChildEvent::QChildEvent(Type type, QObject *child)
    : QEvent(type), c(child)
{}

/*! \internal
*/
QChildEvent::~QChildEvent()
{
}

/*!
    \fn QObject *QChildEvent::child() const

    Returns the child object that was added or removed.
*/

/*!
    \fn bool QChildEvent::added() const

    Returns true if type() is QEvent::ChildAdded; otherwise returns
    false.
*/

/*!
    \fn bool QChildEvent::removed() const

    Returns true if type() is QEvent::ChildRemoved; otherwise returns
    false.
*/

/*!
    \fn bool QChildEvent::polished() const

    Returns true if type() is QEvent::ChildPolished; otherwise returns
    false.
*/

/*!
    \class QCustomEvent
    \brief The QCustomEvent class provides support for custom events.

    \compat

    QCustomEvent has a \c{void *} that can be used to store custom
    data.

    In Qt 3, QObject::customEvent() took a QCustomEvent pointer. We
    found out that this approach was unsatisfactory, because
    there was often no safe way of deleting the data held in the
    \c{void *}.

    In Qt 4, QObject::customEvent() takes a plain QEvent pointer.
    You can add custom data by subclassing.

    \sa QObject::customEvent(), QCoreApplication::notify()
*/

/*!
    \fn QCustomEvent::QCustomEvent(int type, void *data)

    Constructs a custom event object with the event \a type and a
    pointer to \a data. The value of \a type must be at least as
    large as QEvent::User. By default, the data pointer is set to 0.
*/
#ifdef QT3_SUPPORT
QCustomEvent::QCustomEvent(int type, void *data)
    : QEvent(static_cast<Type>(type))
{
    d = reinterpret_cast<QEventPrivate *>(data);
}

/*! \internal
*/
QCustomEvent::~QCustomEvent()
{
}
#endif
/*!
    \fn void QCustomEvent::setData(void *data)

    \compat

    Sets the generic data pointer to \a data.

    \sa data()
*/

/*!
    \fn void *QCustomEvent::data() const

    \compat

    Returns a pointer to the generic event data.

    \sa setData()
*/

/*!
    \fn bool QChildEvent::inserted() const

    \compat

    A child has been inserted if the event's type() is ChildInserted.
*/


