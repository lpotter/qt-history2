/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

/*!
    \class QMdiSubWindow
    \brief The QMdiSubWindow class provides a subwindow class for
    QMdiArea.
    \since 4.3
    \ingroup application
    \mainclass

    QMdiSubWindow represents a top-level window in a QMdiArea, and consists
    of a title bar with window decorations, an internal widget, and
    (depending on the current style) a window frame and a size
    grip. QMdiSubWindow has its own layout, which consists of the
    title bar and a center area for the internal widget.

    \image qmdisubwindowlayout.png

    The most common way to construct a QMdiSubWindow is to call
    QMdiArea::addSubWindow() with the internal widget as the argument.
    You can also create a subwindow yourself, and set an internal
    widget by calling setWidget().

    You use the same API when programming with subwindows as with
    regular top-level windows (e.g., you can call functions such as
    show(), hide(), showMaximized(), and setWindowTitle()).

    \section1 Subwindow Handling

    QMdiSubWindow also supports behavior specific to subwindows in
    an MDI area.

    By default, each QMdiSubWindow is visible inside the MDI area
    viewport when moved around, but it is also possible to specify
    transparent window movement and resizing behavior, where only
    the outline of a subwindow is updated during these operations.
    The setOption() function is used to enable this behavior.

    The isShaded() function detects whether the subwindow is
    currently shaded (i.e., the window is collapsed so that only the
    title bar is visible). To enter shaded mode, call showShaded().
    QMdiSubWindow emits the windowStateChanged() signal whenever the
    window state has changed (e.g., when the window becomes minimized,
    or is restored). It also emits aboutToActivate() before it is
    activated.

    In keyboard-interactive mode, the windows are moved and resized
    with the keyboard. You can enter this mode through the system menu
    of the window. The keyboardSingleStep and keyboardPageStep
    properties control the distance the widget is moved or resized for
    each keypress event. When shift is pressed down page step is used;
    otherwise single step is used.

    \sa QMdiArea
*/

/*!
    \enum QMdiSubWindow::SubWindowOption

    This enum describes options that customize the behavior
    of QMdiSubWindow.

    \omitvalue AllowOutsideAreaHorizontally
    \omitvalue AllowOutsideAreaVertically

    \value RubberBandResize If you enable this option, a rubber band
    control is used to represent the subwindow's outline, and the user
    resizes this instead of the subwindow itself.
    As a result, the subwindow maintains its original position and size
    until the resize operation has been completed, at which time it will
    receive a single QResizeEvent.
    By default, this option is disabled.

    \value RubberBandMove If you enable this option, a rubber band
    control is used to represent the subwindow's outline, and the user
    moves this instead of the subwindow itself.
    As a result, the subwindow remains in its original position until
    the move operation has completed, at which time a QMoveEvent is
    sent to the window. By default, this option is disabled.
*/

/*!
    \fn QMdiSubWindow::windowStateChanged(Qt::WindowStates oldState, Qt::WindowStates newState)

    QMdiSubWindow emits this signal after the window state changes. \a
    oldState is the window state before it changed, and \a newState is the
    new, current state.
*/

/*!
    \fn QMdiSubWindow::aboutToActivate()

    QMdiSubWindow emits this signal immediately before it is
    activated. After the subwindow has been activated, the QMdiArea that
    manages the subwindow will also emit the
    \l{QMdiArea::}{subWindowActivated()} signal.

    \sa QMdiArea::subWindowActivated()
*/

#include "qmdisubwindow_p.h"

#ifndef QT_NO_MDIAREA

#include <QApplication>
#include <QStylePainter>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QWhatsThis>
#include <QToolTip>
#include <QMainWindow>
#include <QStatusBar>
#include <QDebug>
#if defined(Q_WS_MAC) && !defined(QT_NO_STYLE_MAC)
#include <QMacStyle>
#endif

static const QStyle::SubControl SubControls[] =
{
    QStyle::SC_TitleBarLabel, // 1
    QStyle::SC_TitleBarSysMenu, // 2
    QStyle::SC_TitleBarMinButton, // 3
    QStyle::SC_TitleBarMaxButton, // 4
    QStyle::SC_TitleBarShadeButton, // 5
    QStyle::SC_TitleBarCloseButton, // 6
    QStyle::SC_TitleBarNormalButton, // 7
    QStyle::SC_TitleBarUnshadeButton, // 8
    QStyle::SC_TitleBarContextHelpButton // 9
};
static const int NumSubControls = sizeof(SubControls) / sizeof(SubControls[0]);

static const QStyle::StandardPixmap ButtonPixmaps[] =
{
    QStyle::SP_TitleBarMinButton,
    QStyle::SP_TitleBarNormalButton,
    QStyle::SP_TitleBarCloseButton
};
static const int NumButtonPixmaps = sizeof(ButtonPixmaps) / sizeof(ButtonPixmaps[0]);

static const Qt::WindowFlags CustomizeWindowFlags =
      Qt::FramelessWindowHint
    | Qt::CustomizeWindowHint
    | Qt::WindowTitleHint
    | Qt::WindowSystemMenuHint
    | Qt::WindowMinimizeButtonHint
    | Qt::WindowMaximizeButtonHint
    | Qt::WindowMinMaxButtonsHint;


static const int BoundaryMargin = 5;

static inline int getMoveDeltaComponent(uint cflags, uint moveFlag, uint resizeFlag,
                                        int delta, int maxDelta, int minDelta)
{
    if (cflags & moveFlag) {
        if (delta > 0)
            return (cflags & resizeFlag) ? qMin(delta, maxDelta) : delta;
        return (cflags & resizeFlag) ? qMax(delta, minDelta) : delta;
    }
    return 0;
}

static inline int getResizeDeltaComponent(uint cflags, uint resizeFlag,
                                          uint resizeReverseFlag, int delta)
{
    if (cflags & resizeFlag) {
        if (cflags & resizeReverseFlag)
            return -delta;
        return delta;
    }
    return 0;
}

static inline bool isChildOfQMdiSubWindow(const QWidget *child)
{
    Q_ASSERT(child);
    QWidget *parent = child->parentWidget();
    while (parent) {
        if (qobject_cast<QMdiSubWindow *>(parent))
            return true;
        parent = parent->parentWidget();
    }
    return false;
}

template<typename T>
static inline ControlElement<T> *ptr(QWidget *widget)
{
    if (widget && widget->qt_metacast("ControlElement")
            && strcmp(widget->metaObject()->className(), T::staticMetaObject.className()) == 0) {
        return static_cast<ControlElement<T> *>(widget);
    }
    return 0;
}

static inline QString originalWindowTitle(QMdiSubWindow *mdiChild)
{
    Q_ASSERT(mdiChild);
    QString originalTitle = mdiChild->window()->windowTitle();
    int index = originalTitle.indexOf(QString::fromLatin1(" - ["));
    if (index != -1)
        return originalTitle.left(index);
    if (originalTitle.isEmpty())
        return originalTitle;
    QList<QMdiSubWindow *> windows = qFindChildren<QMdiSubWindow *>(mdiChild->window());
    foreach (QMdiSubWindow *window, windows) {
        if (window->windowTitle() == originalTitle)
            return QString();
    }
    return originalTitle;
}

static inline void setNewWindowTitle(QMdiSubWindow *mdiChild)
{
    Q_ASSERT(mdiChild);
    if (!mdiChild)
        return;
    QString childTitle = mdiChild->windowTitle();
    if (childTitle.isEmpty())
        return;
    QString original = originalWindowTitle(mdiChild);
    if (!original.isEmpty()) {
        mdiChild->window()->setWindowTitle(QMdiSubWindow::tr("%1 - [%2]")
                                           .arg(original, childTitle));
    } else {
        mdiChild->window()->setWindowTitle(childTitle);
    }
}

static inline bool isHoverControl(QStyle::SubControl control)
{
    return control != QStyle::SC_None && control != QStyle::SC_TitleBarLabel;
}

#if defined(Q_WS_WIN)
static inline QRgb colorref2qrgb(COLORREF col)
{
    return qRgb(GetRValue(col),GetGValue(col),GetBValue(col));
}
#endif

/*
    \class ControlLabel
    \internal
*/
class ControlLabel : public QWidget
{
    Q_OBJECT
public:
    ControlLabel(QWidget *parent = 0);

    QSize sizeHint() const;

signals:
    void _q_clicked();
    void _q_doubleClicked();

protected:
    bool event(QEvent *event);
    void paintEvent(QPaintEvent *paintEvent);
    void mousePressEvent(QMouseEvent *mouseEvent);
    void mouseDoubleClickEvent(QMouseEvent *mouseEvent);
    void mouseReleaseEvent(QMouseEvent *mouseEvent);

private:
    QPixmap label;
    bool isPressed;
    void updateWindowIcon();
};

ControlLabel::ControlLabel(QWidget *parent)
    : QWidget(parent), isPressed(false)
{
    setFocusPolicy(Qt::NoFocus);
    updateWindowIcon();
    setFixedSize(label.size());
}

/*
    \internal
*/
QSize ControlLabel::sizeHint() const
{
    return label.size();
}

/*
    \internal
*/
bool ControlLabel::event(QEvent *event)
{
    if (event->type() == QEvent::WindowIconChange)
        updateWindowIcon();
    return QWidget::event(event);
}

/*
    \internal
*/
void ControlLabel::paintEvent(QPaintEvent * /*paintEvent*/)
{
    QPainter painter(this);
    painter.drawPixmap(0, 0, label);
}

/*
    \internal
*/
void ControlLabel::mousePressEvent(QMouseEvent *mouseEvent)
{
    if (mouseEvent->button() != Qt::LeftButton) {
        mouseEvent->ignore();
        return;
    }
    isPressed = true;
}

/*
    \internal
*/
void ControlLabel::mouseDoubleClickEvent(QMouseEvent *mouseEvent)
{
    if (mouseEvent->button() != Qt::LeftButton) {
        mouseEvent->ignore();
        return;
    }
    isPressed = false;
    emit _q_doubleClicked();
}

/*
    \internal
*/
void ControlLabel::mouseReleaseEvent(QMouseEvent *mouseEvent)
{
    if (mouseEvent->button() != Qt::LeftButton) {
        mouseEvent->ignore();
        return;
    }
    if (isPressed) {
        isPressed = false;
        emit _q_clicked();
    }
}

/*
    \internal
*/
void ControlLabel::updateWindowIcon()
{
    QIcon menuIcon = windowIcon();
    if (menuIcon.isNull())
        menuIcon = style()->standardIcon(QStyle::SP_TitleBarMenuButton);
    label = menuIcon.pixmap(16, 16);
    update();
}

/*
    \class ControllerWidget
    \internal
*/
class ControllerWidget : public QWidget
{
    Q_OBJECT
public:
    ControllerWidget(QWidget *parent = 0);
    QSize sizeHint() const;

signals:
    void _q_minimize();
    void _q_restore();
    void _q_close();

protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void leaveEvent(QEvent *event);
    bool event(QEvent *event);

private:
    QStyle::SubControl activeControl;
    QStyle::SubControl hoverControl;
    void initStyleOption(QStyleOptionComplex *option) const;
    inline QStyle::SubControl getSubControl(const QPoint &pos) const
    {
        QStyleOptionComplex opt;
        initStyleOption(&opt);
        return style()->hitTestComplexControl(QStyle::CC_MdiControls, &opt, pos, this);
    }
};

/*
    \internal
*/
ControllerWidget::ControllerWidget(QWidget *parent)
    : QWidget(parent),
      activeControl(QStyle::SC_None),
      hoverControl(QStyle::SC_None)
{
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setMouseTracking(true);
}

/*
    \internal
*/
QSize ControllerWidget::sizeHint() const
{
    ensurePolished();
    QStyleOptionComplex opt;
    initStyleOption(&opt);
    QSize size(48, 16);
    return style()->sizeFromContents(QStyle::CT_MdiControls, &opt, size, this);
}

/*
    \internal
*/
void ControllerWidget::paintEvent(QPaintEvent * /*paintEvent*/)
{
    QStyleOptionComplex opt;
    initStyleOption(&opt);
    if (activeControl == hoverControl) {
        opt.activeSubControls = activeControl;
        opt.state |= QStyle::State_Sunken;
    } else if (hoverControl != QStyle::SC_None && (activeControl == QStyle::SC_None)) {
        opt.activeSubControls = hoverControl;
        opt.state |= QStyle::State_MouseOver;
    }
    QPainter painter(this);
    style()->drawComplexControl(QStyle::CC_MdiControls, &opt, &painter, this);
}

/*
    \internal
*/
void ControllerWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }
    activeControl = getSubControl(event->pos());
    update();
}

/*
    \internal
*/
void ControllerWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }

    QStyle::SubControl under_mouse = getSubControl(event->pos());
    if (under_mouse == activeControl) {
        switch (activeControl) {
        case QStyle::SC_MdiCloseButton:
            emit _q_close();
            break;
        case QStyle::SC_MdiNormalButton:
            emit _q_restore();
            break;
        case QStyle::SC_MdiMinButton:
            emit _q_minimize();
            break;
        default:
            break;
        }
    }

    activeControl = QStyle::SC_None;
    update();
}

/*
    \internal
*/
void ControllerWidget::mouseMoveEvent(QMouseEvent *event)
{
    QStyle::SubControl under_mouse = getSubControl(event->pos());
    //test if hover state changes
    if (hoverControl != under_mouse) {
        hoverControl = under_mouse;
        update();
    }
}

/*
    \internal
*/
void ControllerWidget::leaveEvent(QEvent * /*event*/)
{
    hoverControl = QStyle::SC_None;
    update();
}

/*
    \internal
*/
bool ControllerWidget::event(QEvent *event)
{
#ifndef QT_NO_TOOLTIP
    if (event->type() != QEvent::ToolTip)
        return QWidget::event(event);

    QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
    QStyle::SubControl subControl = getSubControl(helpEvent->pos());
    switch (subControl) {
    case QStyle::SC_MdiCloseButton:
        QToolTip::showText(helpEvent->globalPos(), QMdiSubWindow::tr("Close"));
        break;
    case QStyle::SC_MdiMinButton:
        QToolTip::showText(helpEvent->globalPos(), QMdiSubWindow::tr("Minimize"));
        break;
    case QStyle::SC_MdiNormalButton:
        QToolTip::showText(helpEvent->globalPos(), QMdiSubWindow::tr("Restore Down"));
        break;
    default:
        QToolTip::hideText();
        break;
    }
#endif // QT_NO_TOOLTIP
    return QWidget::event(event);
}

/*
    \internal
*/
void ControllerWidget::initStyleOption(QStyleOptionComplex *option) const
{
    option->initFrom(this);
    option->subControls = QStyle::SC_All;
    option->activeSubControls = QStyle::SC_None;
}

/*
    \internal
*/
ControlContainer::ControlContainer(QMdiSubWindow *mdiChild)
    : QObject(mdiChild),
      previousLeft(0),
      previousRight(0),
#ifndef QT_NO_MENUBAR
      m_menuBar(0),
#endif
      mdiChild(mdiChild)
{
    Q_ASSERT(mdiChild);

    m_controllerWidget = new ControlElement<ControllerWidget>(mdiChild);
    connect(m_controllerWidget, SIGNAL(_q_close()), mdiChild, SLOT(close()));
    connect(m_controllerWidget, SIGNAL(_q_restore()), mdiChild, SLOT(showNormal()));
    connect(m_controllerWidget, SIGNAL(_q_minimize()), mdiChild, SLOT(showMinimized()));

    m_menuLabel = new ControlElement<ControlLabel>(mdiChild);
    m_menuLabel->setWindowIcon(mdiChild->windowIcon());
#ifndef QT_NO_MENU
    connect(m_menuLabel, SIGNAL(_q_clicked()), mdiChild, SLOT(showSystemMenu()));
#endif
    connect(m_menuLabel, SIGNAL(_q_doubleClicked()), mdiChild, SLOT(close()));
}

ControlContainer::~ControlContainer()
{
#ifndef QT_NO_MENUBAR
    removeButtonsFromMenuBar();
#endif
    delete m_menuLabel;
    m_menuLabel = 0;
    delete m_controllerWidget;
    m_controllerWidget = 0;
}

#ifndef QT_NO_MENUBAR
/*
    \internal
*/
void ControlContainer::showButtonsInMenuBar(QMenuBar *menuBar)
{
    if (!menuBar || !mdiChild)
        return;
    m_menuBar = menuBar;

    if (m_menuLabel) {
        QWidget *currentLeft = menuBar->cornerWidget(Qt::TopLeftCorner);
        if (currentLeft)
            currentLeft->hide();
        if (currentLeft != m_menuLabel) {
            menuBar->setCornerWidget(m_menuLabel, Qt::TopLeftCorner);
            previousLeft = currentLeft;
        }
        m_menuLabel->show();
    }
    if (m_controllerWidget) {
        QWidget *currentRight = menuBar->cornerWidget(Qt::TopRightCorner);
        if (currentRight)
            currentRight->hide();
        if (currentRight != m_controllerWidget) {
            menuBar->setCornerWidget(m_controllerWidget, Qt::TopRightCorner);
            previousRight = currentRight;
        }
        m_controllerWidget->show();
    }
    setNewWindowTitle(mdiChild);
}

/*
    \internal
*/
void ControlContainer::removeButtonsFromMenuBar(QMenuBar *menuBar)
{
    if (menuBar && menuBar != m_menuBar) {
        // m_menubar was deleted while sub-window was maximized
        previousRight = 0;
        previousLeft = 0;
        m_menuBar = menuBar;
    }

    if (!m_menuBar)
        return;

    QMdiSubWindow *child = 0;
    if (m_controllerWidget) {
        QWidget *currentRight = m_menuBar->cornerWidget(Qt::TopRightCorner);
        if (currentRight == m_controllerWidget) {
            if (ControlElement<ControllerWidget> *ce = ptr<ControllerWidget>(previousRight)) {
                if (!ce->mdiChild || !ce->mdiChild->isMaximized())
                    previousRight = 0;
                else
                    child = ce->mdiChild;
            }
            m_menuBar->setCornerWidget(previousRight, Qt::TopRightCorner);
            if (previousRight) {
                previousRight->show();
                previousRight = 0;
            }
        }
        m_controllerWidget->hide();
        m_controllerWidget->setParent(0);
    }
    if (m_menuLabel) {
        QWidget *currentLeft = m_menuBar->cornerWidget(Qt::TopLeftCorner);
        if (currentLeft == m_menuLabel) {
            if (ControlElement<ControlLabel> *ce = ptr<ControlLabel>(previousLeft)) {
                if (!ce->mdiChild || !ce->mdiChild->isMaximized())
                    previousLeft = 0;
                else if (!child)
                    child = mdiChild;
            }
            m_menuBar->setCornerWidget(previousLeft, Qt::TopLeftCorner);
            if (previousLeft) {
                previousLeft->show();
                previousLeft = 0;
            }
        }
        m_menuLabel->hide();
        m_menuLabel->setParent(0);
    }
    m_menuBar->update();
    if (child)
        setNewWindowTitle(child);
    else if (mdiChild)
        mdiChild->window()->setWindowTitle(originalWindowTitle(mdiChild));
}

#endif // QT_NO_MENUBAR

void ControlContainer::updateWindowIcon(const QIcon &windowIcon)
{
    if (m_menuLabel)
        m_menuLabel->setWindowIcon(windowIcon);
}

/*!
    \internal
*/
QMdiSubWindowPrivate::QMdiSubWindowPrivate()
    : baseWidget(0),
      restoreFocusWidget(0),
      controlContainer(0),
#ifndef QT_NO_SIZEGRIP
      sizeGrip(0),
#endif
#ifndef QT_NO_RUBBERBAND
      rubberBand(0),
#endif
      resizeEnabled(true),
      moveEnabled(true),
      isInInteractiveMode(false),
#ifndef QT_NO_RUBBERBAND
      isInRubberBandMode(false),
#endif
      isShadeMode(false),
      ignoreWindowTitleChange(false),
      ignoreNextActivationEvent(false),
      activationEnabled(true),
      isShadeRequestFromMinimizeMode(false),
      isMaximizeMode(false),
      isWidgetHiddenByUs(false),
      isActive(false),
      isExplicitlyDeactivated(false),
      keyboardSingleStep(5),
      keyboardPageStep(20),
      resizeTimerId(-1),
      currentOperation(None),
      hoveredSubControl(QStyle::SC_None),
      activeSubControl(QStyle::SC_None),
      focusInReason(Qt::ActiveWindowFocusReason)
{
    initOperationMap();
}

/*!
    \internal
*/
void QMdiSubWindowPrivate::_q_updateStaysOnTopHint()
{
#ifndef QT_NO_ACTION
    Q_Q(QMdiSubWindow);
    if (QAction *senderAction = qobject_cast<QAction *>(q->sender())) {
        if (senderAction->isChecked()) {
            q->setWindowFlags(q->windowFlags() | Qt::WindowStaysOnTopHint);
            q->raise();
        } else {
            q->setWindowFlags(q->windowFlags() & ~Qt::WindowStaysOnTopHint);
            q->lower();
        }
    }
#endif // QT_NO_ACTION
}

/*!
    \internal
*/
void QMdiSubWindowPrivate::_q_enterInteractiveMode()
{
#ifndef QT_NO_ACTION
    Q_Q(QMdiSubWindow);
    QAction *action = qobject_cast<QAction *>(q->sender());
    if (!action)
        return;

    QPoint pressPos;
    if (actions[MoveAction] && actions[MoveAction] == action) {
        currentOperation = Move;
        pressPos = QPoint(q->width() / 2, titleBarHeight() - 1);
    } else if (actions[ResizeAction] && actions[ResizeAction] == action) {
        currentOperation = q->isLeftToRight() ? BottomRightResize : BottomLeftResize;
        int offset = q->style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth) / 2;
        int x = q->isLeftToRight() ? q->width() - offset : offset;
        pressPos = QPoint(x, q->height() - offset);
    } else {
        return;
    }

    updateCursor();
#ifndef QT_NO_CURSOR
    q->cursor().setPos(q->mapToGlobal(pressPos));
#endif
    mousePressPosition = q->mapToParent(pressPos);
    oldGeometry = q->geometry();
    isInInteractiveMode = true;
    q->setFocus();
#ifndef QT_NO_RUBBERBAND
    if ((q->testOption(QMdiSubWindow::RubberBandResize)
            && (currentOperation == BottomRightResize || currentOperation == BottomLeftResize))
            || (q->testOption(QMdiSubWindow::RubberBandMove) && currentOperation == Move)) {
        enterRubberBandMode();
    } else
#endif // QT_NO_RUBBERBAND
    {
        q->grabMouse();
    }
#endif // QT_NO_ACTION
}

/*!
    \internal
*/
void QMdiSubWindowPrivate::_q_processFocusChanged(QWidget *old, QWidget *now)
{
    Q_UNUSED(old);
    Q_Q(QMdiSubWindow);
    if (now && (now == q || q->isAncestorOf(now))) {
        if (now == q && !isInInteractiveMode)
            setFocusWidget();
        setActive(true);
    }
}

/*!
    \internal
*/
void QMdiSubWindowPrivate::leaveInteractiveMode()
{
    Q_Q(QMdiSubWindow);
#ifndef QT_NO_RUBBERBAND
    if (isInRubberBandMode)
        leaveRubberBandMode();
    else
#endif
        q->releaseMouse();
    isInInteractiveMode = false;
    currentOperation = None;
    updateDirtyRegions();
    updateCursor();
    if (baseWidget && baseWidget->focusWidget())
        baseWidget->focusWidget()->setFocus();
}

/*!
    \internal
*/
void QMdiSubWindowPrivate::removeBaseWidget()
{
    if (!baseWidget)
        return;

    Q_Q(QMdiSubWindow);
    q->removeEventFilter(baseWidget);
    if (QLayout *layout = q->layout())
        layout->removeWidget(baseWidget);
    if (baseWidget->windowTitle() == q->windowTitle()) {
        ignoreWindowTitleChange = true;
        q->setWindowTitle(QString());
        ignoreWindowTitleChange = false;
        q->setWindowModified(false);
    }
    lastChildWindowTitle.clear();
    baseWidget->setParent(0);
    baseWidget = 0;
    isWidgetHiddenByUs = false;
}

/*!
    \internal
*/
void QMdiSubWindowPrivate::initOperationMap()
{
    operationMap.insert(Move, OperationInfo(HMove | VMove, Qt::ArrowCursor, false));
    operationMap.insert(TopResize, OperationInfo(VMove | VResize | VResizeReverse, Qt::SizeVerCursor));
    operationMap.insert(BottomResize, OperationInfo(VResize, Qt::SizeVerCursor));
    operationMap.insert(LeftResize, OperationInfo(HMove | HResize | HResizeReverse, Qt::SizeHorCursor));
    operationMap.insert(RightResize, OperationInfo(HResize, Qt::SizeHorCursor));
    operationMap.insert(TopLeftResize, OperationInfo(HMove | VMove | HResize | VResize | VResizeReverse
                                                     | HResizeReverse, Qt::SizeFDiagCursor));
    operationMap.insert(TopRightResize, OperationInfo(VMove | HResize | VResize
                                                      | VResizeReverse, Qt::SizeBDiagCursor));
    operationMap.insert(BottomLeftResize, OperationInfo(HMove | HResize | VResize | HResizeReverse,
                                                        Qt::SizeBDiagCursor));
    operationMap.insert(BottomRightResize, OperationInfo(HResize | VResize, Qt::SizeFDiagCursor));
}

#ifndef QT_NO_MENU

/*!
    \internal
*/
void QMdiSubWindowPrivate::createSystemMenu()
{
    Q_Q(QMdiSubWindow);
    Q_ASSERT_X(q, "QMdiSubWindowPrivate::createSystemMenu",
               "You can NOT call this function before QMdiSubWindow's ctor");
    systemMenu = new QMenu(q);
    const QStyle *style = q->style();
    addToSystemMenu(RestoreAction, QMdiSubWindow::tr("&Restore"), SLOT(showNormal()));
    actions[RestoreAction]->setIcon(style->standardIcon(QStyle::SP_TitleBarNormalButton));
    actions[RestoreAction]->setEnabled(false);
    addToSystemMenu(MoveAction, QMdiSubWindow::tr("&Move"), SLOT(_q_enterInteractiveMode()));
    addToSystemMenu(ResizeAction, QMdiSubWindow::tr("&Size"), SLOT(_q_enterInteractiveMode()));
    addToSystemMenu(MinimizeAction, QMdiSubWindow::tr("Mi&nimize"), SLOT(showMinimized()));
    actions[MinimizeAction]->setIcon(style->standardIcon(QStyle::SP_TitleBarMinButton));
    addToSystemMenu(MaximizeAction, QMdiSubWindow::tr("Ma&ximize"), SLOT(showMaximized()));
    actions[MaximizeAction]->setIcon(style->standardIcon(QStyle::SP_TitleBarMaxButton));
    addToSystemMenu(StayOnTopAction, QMdiSubWindow::tr("Stay on &Top"), SLOT(_q_updateStaysOnTopHint()));
    actions[StayOnTopAction]->setCheckable(true);
    systemMenu->addSeparator();
    addToSystemMenu(CloseAction, QMdiSubWindow::tr("&Close"), SLOT(close()));
    actions[CloseAction]->setIcon(style->standardIcon(QStyle::SP_TitleBarCloseButton));
#if !defined(QT_NO_SHORTCUT)
    actions[CloseAction]->setShortcut(QKeySequence::Close);
#endif
    updateActions();
}
#endif

/*!
    \internal
*/
void QMdiSubWindowPrivate::updateCursor()
{
#ifndef QT_NO_CURSOR
    Q_Q(QMdiSubWindow);
#if defined(Q_WS_MAC) && !defined(QT_NO_STYLE_MAC)
    if (qobject_cast<QMacStyle *>(q->style()))
        return;
#endif

    if (currentOperation == None) {
        q->unsetCursor();
        return;
    }

    if (currentOperation == Move || operationMap.find(currentOperation).value().hover) {
        q->setCursor(operationMap.find(currentOperation).value().cursorShape);
        return;
    }
#endif
}

/*!
    \internal
*/
void QMdiSubWindowPrivate::updateDirtyRegions()
{
    // No update necessary
    if (!q_func()->parent())
        return;

    foreach (Operation operation, operationMap.keys())
        operationMap.find(operation).value().region = getRegion(operation);
}

/*!
    \internal
*/
void QMdiSubWindowPrivate::updateGeometryConstraints()
{
    Q_Q(QMdiSubWindow);
    if (!q->parent())
        return;

    internalMinimumSize = q->minimumSizeHint();
    int margin, minWidth;
    sizeParameters(&margin, &minWidth);
    q->setContentsMargins(margin, titleBarHeight(), margin, margin);
    if (q->isMaximized() || (q->isMinimized() && !q->isShaded())) {
        moveEnabled = false;
        resizeEnabled = false;
    } else {
        moveEnabled = true;
        if ((q->windowFlags() & Qt::MSWindowsFixedSizeDialogHint) || q->isShaded())
            resizeEnabled = false;
        else
            resizeEnabled = true;
    }
    if (q->layout())
        q->layout()->invalidate();
    updateDirtyRegions();
}

/*!
    \internal
*/
void QMdiSubWindowPrivate::updateMask()
{
    Q_Q(QMdiSubWindow);
    if (!q->mask().isEmpty())
        q->clearMask();

    if (!q->parent())
        return;

    if (q->isMaximized() && !drawTitleBarWhenMaximized()
        || q->windowFlags() & Qt::FramelessWindowHint)
        return;

    if (resizeTimerId == -1)
        cachedStyleOptions = titleBarOptions();
    cachedStyleOptions.rect = q->rect();
    QStyleHintReturnMask frameMask;
    q->style()->styleHint(QStyle::SH_WindowFrame_Mask, &cachedStyleOptions, q, &frameMask);
    q->setMask(frameMask.region);
}

/*!
    \internal
*/
void QMdiSubWindowPrivate::setNewGeometry(const QPoint &pos)
{
    Q_Q(QMdiSubWindow);
    Q_ASSERT(currentOperation != None);
    Q_ASSERT(q->parent());

    uint cflags = operationMap.find(currentOperation).value().changeFlags;
    int posX = pos.x();
    int posY = pos.y();

    const bool restrictHorizontal = !q->testOption(QMdiSubWindow::AllowOutsideAreaHorizontally);
    const bool restrictVertical = !q->testOption(QMdiSubWindow::AllowOutsideAreaVertically);

    if (restrictHorizontal || restrictVertical) {
        QRect parentRect = q->parentWidget()->rect();
        if (restrictVertical && (cflags & VResizeReverse || currentOperation == Move)) {
            posY = qMin(qMax(mousePressPosition.y() - oldGeometry.y(), posY),
                        parentRect.height() - BoundaryMargin);
        }
        if (currentOperation == Move) {
            if (restrictHorizontal)
                posX = qMin(qMax(BoundaryMargin, posX), parentRect.width() - BoundaryMargin);
            if (restrictVertical)
                posY = qMin(posY, parentRect.height() - BoundaryMargin);
        } else {
            if (restrictHorizontal) {
                if (cflags & HResizeReverse)
                    posX = qMax(mousePressPosition.x() - oldGeometry.x(), posX);
                else
                    posX = qMin(parentRect.width() - (oldGeometry.x() + oldGeometry.width()
                                                      - mousePressPosition.x()), posX);
            }
            if (restrictVertical && !(cflags & VResizeReverse)) {
                posY = qMin(parentRect.height() - (oldGeometry.y() + oldGeometry.height()
                                                   - mousePressPosition.y()), posY);
            }
        }
    }

    QRect geometry;
    if (cflags & (HMove | VMove)) {
        int dx = getMoveDeltaComponent(cflags, HMove, HResize, posX - mousePressPosition.x(),
                                       oldGeometry.width() - internalMinimumSize.width(),
                                       oldGeometry.width() - q->maximumWidth());
        int dy = getMoveDeltaComponent(cflags, VMove, VResize, posY - mousePressPosition.y(),
                                       oldGeometry.height() - internalMinimumSize.height(),
                                       oldGeometry.height() - q->maximumHeight());
        geometry.setTopLeft(oldGeometry.topLeft() + QPoint(dx, dy));
    } else {
        geometry.setTopLeft(q->geometry().topLeft());
    }

    if (cflags & (HResize | VResize)) {
        int dx = getResizeDeltaComponent(cflags, HResize, HResizeReverse,
                                         posX - mousePressPosition.x());
        int dy = getResizeDeltaComponent(cflags, VResize, VResizeReverse,
                                         posY - mousePressPosition.y());
        geometry.setSize(oldGeometry.size() + QSize(dx, dy));
    } else {
        geometry.setSize(q->geometry().size());
    }

    setNewGeometry(&geometry);
}

/*!
    \internal
*/
void QMdiSubWindowPrivate::setMinimizeMode()
{
    Q_Q(QMdiSubWindow);
    Q_ASSERT(q->parent());

    ensureWindowState(Qt::WindowMinimized);
    isShadeRequestFromMinimizeMode = true;
    q->showShaded();
    isShadeRequestFromMinimizeMode = false;

    moveEnabled = false;
#ifndef QT_NO_ACTION
    setEnabled(MoveAction, moveEnabled);
#endif

    Q_ASSERT(q->windowState() & Qt::WindowMinimized);
    Q_ASSERT(!(q->windowState() & Qt::WindowMaximized));
    // This should be a valid assert, but people can actually re-implement
    // setVisible and do crazy stuff, so we're not guaranteed that
    // the widget is hidden after calling hide().
    // Q_ASSERT(baseWidget ? baseWidget->isHidden() : true);

    setActive(true);
}

/*!
    \internal
*/
void QMdiSubWindowPrivate::setNormalMode()
{
    Q_Q(QMdiSubWindow);
    Q_ASSERT(q->parent());

    isShadeMode = false;
    isMaximizeMode = false;

    ensureWindowState(Qt::WindowNoState);
#ifndef QT_NO_MENUBAR
    removeButtonsFromMenuBar();
#endif

    // Hide the window before we change the geometry to avoid multiple resize
    // events and wrong window state.
    const bool wasVisible = q->isVisible();
    if (wasVisible)
        q->setVisible(false);

    // Restore minimum size if set by user.
    if (userMinimumSize.isValid()) {
        q->setMinimumSize(userMinimumSize);
        userMinimumSize = QSize(-1, -1);
    }

    // Show the internal widget if it was hidden by us,
    if (baseWidget && isWidgetHiddenByUs) {
        baseWidget->show();
        isWidgetHiddenByUs = false;
    }

    updateGeometryConstraints();
    QRect newGeometry = oldGeometry;
    newGeometry.setSize(restoreSize.expandedTo(internalMinimumSize));
    q->setGeometry(newGeometry);

    if (wasVisible)
        q->setVisible(true);

    // Invalidate the restore size.
    restoreSize.setWidth(-1);
    restoreSize.setHeight(-1);

#ifndef QT_NO_SIZEGRIP
    setSizeGripVisible(true);
#endif

#ifndef QT_NO_ACTION
    setEnabled(MoveAction, true);
    setEnabled(MaximizeAction, true);
    setEnabled(MinimizeAction, true);
    setEnabled(RestoreAction, false);
    setEnabled(ResizeAction, resizeEnabled);
#endif // QT_NO_ACTION

    Q_ASSERT(!(q_func()->windowState() & (Qt::WindowMinimized | Qt::WindowMaximized)));
    Q_ASSERT(!isShadeMode);

    setActive(true);
    restoreFocus();
    updateMask();
}

/*!
    \internal
*/
void QMdiSubWindowPrivate::setMaximizeMode()
{
    Q_Q(QMdiSubWindow);
    Q_ASSERT(q->parent());

    ensureWindowState(Qt::WindowMaximized);
    isShadeMode = false;
    isMaximizeMode = true;

    if (!restoreFocusWidget && q->isAncestorOf(QApplication::focusWidget()))
        restoreFocusWidget = QApplication::focusWidget();

#ifndef QT_NO_SIZEGRIP
    setSizeGripVisible(false);
#endif

    // Store old geometry and set restore size if not already set.
    if (!restoreSize.isValid()) {
        oldGeometry = q->geometry();
        restoreSize.setWidth(oldGeometry.width());
        restoreSize.setHeight(oldGeometry.height());
    }

    // Hide the window before we change the geometry to avoid multiple resize
    // events and wrong window state.
    const bool wasVisible = q->isVisible();
    if (wasVisible)
        q->setVisible(false);

    // Show the internal widget if it was hidden by us.
    if (baseWidget && isWidgetHiddenByUs) {
        baseWidget->show();
        isWidgetHiddenByUs = false;
    }

    updateGeometryConstraints();

    if (!drawTitleBarWhenMaximized() && wasVisible) {
#ifndef QT_NO_MAINWINDOW
        if (QMainWindow *mainWindow = qobject_cast<QMainWindow *>(q->window())) {
#ifdef QT_NO_MENUBAR
            Q_UNUSED(mainWindow);
#else
            showButtonsInMenuBar(mainWindow->menuBar());
#endif
        } else
#endif // QT_NO_MAINWINDOW
        if (!controlContainer) {
            controlContainer = new ControlContainer(q);
        }
    }

    QRect availableRect = q->parentWidget()->contentsRect();
    setNewGeometry(&availableRect);
    // QWidget::setGeometry will reset Qt::WindowMaximized so we have to update it here.
    ensureWindowState(Qt::WindowMaximized);

    if (wasVisible)
        q->setVisible(true);

    resizeEnabled = false;
    moveEnabled = false;

#ifndef QT_NO_ACTION
    setEnabled(MoveAction, moveEnabled);
    setEnabled(MaximizeAction, false);
    setEnabled(MinimizeAction, true);
    setEnabled(RestoreAction, true);
    setEnabled(ResizeAction, resizeEnabled);
#endif // QT_NO_ACTION

    Q_ASSERT(q->windowState() & Qt::WindowMaximized);
    Q_ASSERT(!(q->windowState() & Qt::WindowMinimized));

    restoreFocus();
    updateMask();
}

/*!
    \internal
*/
void QMdiSubWindowPrivate::setActive(bool activate)
{
    Q_Q(QMdiSubWindow);
    if (!q->parent() || !activationEnabled)
        return;

    if (activate && !isActive && q->isEnabled()) {
        isActive = true;
        isExplicitlyDeactivated = false;
        Qt::WindowStates oldWindowState = q->windowState();
        ensureWindowState(Qt::WindowActive);
        emit q->aboutToActivate();
        if (q->isMaximized() && !drawTitleBarWhenMaximized()) {
#ifndef QT_NO_MAINWINDOW
            if (QMainWindow *mainWindow = qobject_cast<QMainWindow *>(q->window())) {
#ifdef QT_NO_MENUBAR
                Q_UNUSED(mainWindow);
#else
                showButtonsInMenuBar(mainWindow->menuBar());
#endif
            }
#endif // QT_NO_MAINWINDOW
        }
        Q_ASSERT(isActive);
        emit q->windowStateChanged(oldWindowState, q->windowState());
    } else if (!activate && isActive) {
        isActive = false;
        Qt::WindowStates oldWindowState = q->windowState();
        q->overrideWindowState(q->windowState() & ~Qt::WindowActive);
        QWidget *focusWidget = QApplication::focusWidget();
        if (focusWidget && (focusWidget == q || q->isAncestorOf(focusWidget)))
            focusWidget->clearFocus();
        if (baseWidget)
            baseWidget->overrideWindowState(baseWidget->windowState() & ~Qt::WindowActive);
        Q_ASSERT(!isActive);
        emit q->windowStateChanged(oldWindowState, q->windowState());
    }

    if (activate && isActive && q->isEnabled() && !q->hasFocus()
            && !q->isAncestorOf(QApplication::focusWidget())) {
        setFocusWidget();
        ensureWindowState(Qt::WindowActive);
    }

    int frameWidth = q->style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth);
    int titleBarHeight = this->titleBarHeight();
    QRegion windowDecoration = QRegion(0, 0, q->width(), q->height());
    windowDecoration -= QRegion(frameWidth, titleBarHeight, q->width() - 2 * frameWidth,
                                q->height() - titleBarHeight - frameWidth);

    // Make sure we don't use cached style options if we get
    // resize events right before activation/deactivation.
    if (resizeTimerId != -1) {
        q->killTimer(resizeTimerId);
        resizeTimerId = -1;
        updateDirtyRegions();
    }

    q->update(windowDecoration);
}

/*!
    \internal
*/
void QMdiSubWindowPrivate::processClickedSubControl()
{
    Q_Q(QMdiSubWindow);
    switch (activeSubControl) {
#ifndef QT_NO_MENU
    case QStyle::SC_TitleBarSysMenu:
        q->showSystemMenu();
        break;
#endif
    case QStyle::SC_TitleBarContextHelpButton:
#ifndef QT_NO_WHATSTHIS
        QWhatsThis::enterWhatsThisMode();
#endif
        break;
    case QStyle::SC_TitleBarShadeButton:
        q->showShaded();
        hoveredSubControl = QStyle::SC_TitleBarUnshadeButton;
        break;
    case QStyle::SC_TitleBarUnshadeButton:
        if (q->isShaded())
            hoveredSubControl = QStyle::SC_TitleBarShadeButton;
        q->showNormal();
        break;
    case QStyle::SC_TitleBarMinButton:
#if defined(Q_WS_MAC) && !defined(QT_NO_STYLE_MAC)
        if (qobject_cast<QMacStyle *>(q->style())) {
            if (q->isMinimized())
                q->showNormal();
            else
                q->showMinimized();
            break;
        }
#endif
        q->showMinimized();
        break;
    case QStyle::SC_TitleBarNormalButton:
        if (q->isShaded())
            hoveredSubControl = QStyle::SC_TitleBarMinButton;
        q->showNormal();
        break;
    case QStyle::SC_TitleBarMaxButton:
#if defined(Q_WS_MAC) && !defined(QT_NO_STYLE_MAC)
        if (qobject_cast<QMacStyle *>(q->style())) {
            if (q->isMaximized())
                q->showNormal();
            else
                q->showMaximized();
            break;
        }
#endif
        q->showMaximized();
        break;
    case QStyle::SC_TitleBarCloseButton:
        q->close();
        break;
    default:
        break;
    }
}

/*!
    \internal
*/
QRegion QMdiSubWindowPrivate::getRegion(Operation operation) const
{
    Q_Q(const QMdiSubWindow);
    int width = q->width();
    int height = q->height();
    int titleBarHeight = this->titleBarHeight();
    int frameWidth = q->style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth);
    int cornerConst = titleBarHeight - frameWidth;
    int titleBarConst = 2 * titleBarHeight;

    if (operation == Move) {
        QStyleOptionTitleBar titleBarOptions = this->titleBarOptions();
        QRegion move(frameWidth, frameWidth, width - 2 * frameWidth, cornerConst);
        // Depending on which window flags are set, activated sub controllers will
        // be subtracted from the 'move' region.
        for (int i = 0; i < NumSubControls; ++i) {
            if (SubControls[i] == QStyle::SC_TitleBarLabel)
                continue;
            move -= QRegion(q->style()->subControlRect(QStyle::CC_TitleBar, &titleBarOptions,
                            SubControls[i]));
        }
        return move;
    }

    QRegion region;
#if defined(Q_WS_MAC) && !defined(QT_NO_STYLE_MAC)
    if (qobject_cast<QMacStyle *>(q->style()))
        return region;
#endif

    switch (operation) {
    case TopResize:
        region = QRegion(titleBarHeight, 0, width - titleBarConst, frameWidth);
        break;
    case BottomResize:
        region = QRegion(titleBarHeight, height - frameWidth, width - titleBarConst, frameWidth);
        break;
    case LeftResize:
        region = QRegion(0, titleBarHeight, frameWidth, height - titleBarConst);
        break;
    case RightResize:
        region = QRegion(width - frameWidth, titleBarHeight, frameWidth, height - titleBarConst);
        break;
    case TopLeftResize:
        region = QRegion(0, 0, titleBarHeight, titleBarHeight)
                 - QRegion(frameWidth, frameWidth, cornerConst, cornerConst);
        break;
    case TopRightResize:
        region =  QRegion(width - titleBarHeight, 0, titleBarHeight, titleBarHeight)
                  - QRegion(width - titleBarHeight, frameWidth, cornerConst, cornerConst);
        break;
    case BottomLeftResize:
        region = QRegion(0, height - titleBarHeight, titleBarHeight, titleBarHeight)
                 - QRegion(frameWidth, height - titleBarHeight, cornerConst, cornerConst);
        break;
    case BottomRightResize:
        region = QRegion(width - titleBarHeight, height - titleBarHeight, titleBarHeight, titleBarHeight)
                 - QRegion(width - titleBarHeight, height - titleBarHeight, cornerConst, cornerConst);
        break;
    default:
        break;
    }

    return region;
}

/*!
    \internal
*/
QMdiSubWindowPrivate::Operation QMdiSubWindowPrivate::getOperation(const QPoint &pos) const
{
    OperationInfoMap::const_iterator it;
    for (it = operationMap.constBegin(); it != operationMap.constEnd(); ++it)
        if (it.value().region.contains(pos))
            return it.key();
    return None;
}

extern QString qt_setWindowTitle_helperHelper(const QString &, QWidget *);

/*!
    \internal
*/
QStyleOptionTitleBar QMdiSubWindowPrivate::titleBarOptions() const
{
    Q_Q(const QMdiSubWindow);
    QStyleOptionTitleBar titleBarOptions;
    titleBarOptions.initFrom(q);
    if (activeSubControl != QStyle::SC_None) {
        if (hoveredSubControl == activeSubControl) {
            titleBarOptions.state |= QStyle::State_Sunken;
            titleBarOptions.activeSubControls = activeSubControl;
        }
    } else if (autoRaise() && hoveredSubControl != QStyle::SC_None
               && hoveredSubControl != QStyle::SC_TitleBarLabel) {
        titleBarOptions.state |= QStyle::State_MouseOver;
        titleBarOptions.activeSubControls = hoveredSubControl;
    } else {
        titleBarOptions.state &= ~QStyle::State_MouseOver;
        titleBarOptions.activeSubControls = QStyle::SC_None;
    }

    titleBarOptions.subControls = QStyle::SC_All;
    titleBarOptions.titleBarFlags = q->windowFlags();
    titleBarOptions.titleBarState = q->windowState();
    titleBarOptions.palette = titleBarPalette;
    titleBarOptions.icon = menuIcon;

    if (isActive) {
        titleBarOptions.state |= QStyle::State_Active;
        titleBarOptions.titleBarState |= QStyle::State_Active;
        titleBarOptions.palette.setCurrentColorGroup(QPalette::Active);
    } else {
        titleBarOptions.state &= ~QStyle::State_Active;
        titleBarOptions.palette.setCurrentColorGroup(QPalette::Inactive);
    }

    int border = hasBorder(titleBarOptions) ? 4 : 0;
    int paintHeight = titleBarHeight(titleBarOptions);
    paintHeight -= q->isMinimized() ? 2 * border : border;
    titleBarOptions.rect = QRect(border, border, q->width() - 2 * border, paintHeight);

    if (!windowTitle.isEmpty()) {
        // Set the text here before asking for the width of the title bar label
        // in case people uses the actual text to calculate the width.
        titleBarOptions.text = windowTitle;
        titleBarOptions.fontMetrics = QFontMetrics(font);
        int width = q->style()->subControlRect(QStyle::CC_TitleBar, &titleBarOptions,
                                               QStyle::SC_TitleBarLabel, q).width();
        // Set elided text if we don't have enough space for the entire title.
        titleBarOptions.text = titleBarOptions.fontMetrics.elidedText(windowTitle, Qt::ElideRight, width);
    }
    return titleBarOptions;
}

/*!
    \internal
*/
void QMdiSubWindowPrivate::ensureWindowState(Qt::WindowState state)
{
    Q_Q(QMdiSubWindow);
    Qt::WindowStates windowStates = q->windowState() | state;
    switch (state) {
    case Qt::WindowMinimized:
        windowStates &= ~Qt::WindowMaximized;
        windowStates &= ~Qt::WindowNoState;
        break;
    case Qt::WindowMaximized:
        windowStates &= ~Qt::WindowMinimized;
        windowStates &= ~Qt::WindowNoState;
        break;
    case Qt::WindowNoState:
        windowStates &= ~Qt::WindowMinimized;
        windowStates &= ~Qt::WindowMaximized;
        break;
    default:
        break;
    }
    if (baseWidget) {
        if (!(baseWidget->windowState() & Qt::WindowActive) && windowStates & Qt::WindowActive)
            baseWidget->overrideWindowState(windowStates & ~Qt::WindowActive);
        else
            baseWidget->overrideWindowState(windowStates);
    }
    q->overrideWindowState(windowStates);
}

/*!
    \internal
*/
int QMdiSubWindowPrivate::titleBarHeight(const QStyleOptionTitleBar &options) const
{
    Q_Q(const QMdiSubWindow);
    if (!q->parent() || q->windowFlags() & Qt::FramelessWindowHint
        || (q->isMaximized() && !drawTitleBarWhenMaximized())) {
        return 0;
    }

    int height = q->style()->pixelMetric(QStyle::PM_TitleBarHeight, &options);
#if defined(Q_WS_MAC) && !defined(QT_NO_STYLE_MAC)
    // ### Fix mac style, the +4 pixels hack is not necessary anymore
    if (qobject_cast<QMacStyle *>(q->style()))
        height -= 4;
#endif
    if (hasBorder(options))
        height += q->isMinimized() ? 8 : 4;
    return height;
}

/*!
    \internal
*/
void QMdiSubWindowPrivate::sizeParameters(int *margin, int *minWidth) const
{
    Q_Q(const QMdiSubWindow);
    Qt::WindowFlags flags = q->windowFlags();
    if (!q->parent() || flags & Qt::FramelessWindowHint) {
        *margin = 0;
        *minWidth = 0;
        return;
    }

    if (q->isMaximized() && !drawTitleBarWhenMaximized())
        *margin = 0;
    else
        *margin = q->style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth);

    QStyleOptionTitleBar opt = this->titleBarOptions();
    int tempWidth = 0;
    for (int i = 0; i < NumSubControls; ++i) {
        if (SubControls[i] == QStyle::SC_TitleBarLabel) {
            tempWidth += 30;
            continue;
        }
        QRect rect = q->style()->subControlRect(QStyle::CC_TitleBar, &opt, SubControls[i], q);
        if (!rect.isValid())
            continue;
        tempWidth += rect.width();
    }
    *minWidth = tempWidth;
}

/*!
    \internal
*/
bool QMdiSubWindowPrivate::drawTitleBarWhenMaximized() const
{
    Q_Q(const QMdiSubWindow);
    if (q->window()->testAttribute(Qt::WA_CanHostQMdiSubWindowTitleBar))
        return false;
#if defined(Q_WS_MAC) && !defined(QT_NO_STYLE_MAC)
    return true;
#else
    if (q->style()->styleHint(QStyle::SH_Workspace_FillSpaceOnMaximize, 0, q))
        return true;
#if defined(QT_NO_MENUBAR) || defined(QT_NO_MAINWINDOW)
    return true;
#else
    QMainWindow *mainWindow = qobject_cast<QMainWindow *>(q->window());
    if (!mainWindow || !mainWindow->menuWidget() || mainWindow->menuWidget()->isHidden())
        return true;
    return isChildOfQMdiSubWindow(q);
#endif
#endif
}

#ifndef QT_NO_MENUBAR

/*!
    \internal
*/
void QMdiSubWindowPrivate::showButtonsInMenuBar(QMenuBar *menuBar)
{
    Q_Q(QMdiSubWindow);
    Q_ASSERT(q->isMaximized() && !drawTitleBarWhenMaximized());

    removeButtonsFromMenuBar();
    if (!controlContainer)
        controlContainer = new ControlContainer(q);

    ignoreWindowTitleChange = true;
    controlContainer->showButtonsInMenuBar(menuBar);
    ignoreWindowTitleChange = false;

    QWidget *topLevelWindow = q->window();
    topLevelWindow->setWindowModified(q->isWindowModified());
    topLevelWindow->installEventFilter(q);

    int buttonHeight = 0;
    if (controlContainer->controllerWidget())
        buttonHeight = controlContainer->controllerWidget()->height();
    else if (controlContainer->systemMenuLabel())
        buttonHeight = controlContainer->systemMenuLabel()->height();

    // This will rarely happen.
    if (menuBar && menuBar->height() < buttonHeight
            && topLevelWindow->layout()) {
        // Make sure topLevelWindow->contentsRect returns correct geometry.
        // topLevelWidget->updateGeoemtry will not do the trick here since it will post the event.
        QEvent event(QEvent::LayoutRequest);
        QApplication::sendEvent(topLevelWindow, &event);
    }
}

/*!
    \internal
*/
void QMdiSubWindowPrivate::removeButtonsFromMenuBar()
{
    if (!controlContainer)
        return;

    Q_Q(QMdiSubWindow);

    QMenuBar *currentMenuBar = 0;
#ifndef QT_NO_MAINWINDOW
    if (QMainWindow *mainWindow = qobject_cast<QMainWindow *>(q->window())) {
        // NB! We can't use menuBar() here because that one will actually create
        // a menubar for us if not set. That's not what we want :-)
        currentMenuBar = qobject_cast<QMenuBar *>(mainWindow->menuWidget());
    }
#endif

    ignoreWindowTitleChange = true;
    controlContainer->removeButtonsFromMenuBar(currentMenuBar);
    ignoreWindowTitleChange = false;

    QWidget *topLevelWindow = q->window();
    q->removeEventFilter(topLevelWindow);
    if (baseWidget && !drawTitleBarWhenMaximized())
        topLevelWindow->setWindowModified(false);
}

#endif // QT_NO_MENUBAR

void QMdiSubWindowPrivate::updateWindowTitle(bool isRequestFromChild)
{
    Q_Q(QMdiSubWindow);
    if (isRequestFromChild && !q->windowTitle().isEmpty() && !lastChildWindowTitle.isEmpty()
            && lastChildWindowTitle != q->windowTitle()) {
        return;
    }

    QWidget *titleWidget = 0;
    if (isRequestFromChild)
        titleWidget = baseWidget;
    else
        titleWidget = q;
    if (!titleWidget || titleWidget->windowTitle().isEmpty())
        return;

    ignoreWindowTitleChange = true;
    q->setWindowTitle(titleWidget->windowTitle());
    if (q->maximizedButtonsWidget())
        setNewWindowTitle(q);
    ignoreWindowTitleChange = false;
}

#ifndef QT_NO_RUBBERBAND
void QMdiSubWindowPrivate::enterRubberBandMode()
{
    Q_Q(QMdiSubWindow);
    if (q->isMaximized())
        return;
    Q_ASSERT(oldGeometry.isValid());
    Q_ASSERT(q->parent());
    if (!rubberBand)
        rubberBand = new QRubberBand(QRubberBand::Rectangle, q->parentWidget());
    QPoint rubberBandPos = q->mapToParent(QPoint(0, 0));
    rubberBand->setGeometry(rubberBandPos.x(), rubberBandPos.y(),
                            oldGeometry.width(), oldGeometry.height());
    rubberBand->show();
    isInRubberBandMode = true;
    q->grabMouse();
}

void QMdiSubWindowPrivate::leaveRubberBandMode()
{
    Q_Q(QMdiSubWindow);
    Q_ASSERT(rubberBand);
    Q_ASSERT(isInRubberBandMode);
    q->releaseMouse();
    isInRubberBandMode = false;
    q->setGeometry(rubberBand->geometry());
    rubberBand->hide();
    currentOperation = None;
}
#endif // QT_NO_RUBBERBAND

// Taken from the old QWorkspace (::readColors())
QPalette QMdiSubWindowPrivate::desktopPalette() const
{
    Q_Q(const QMdiSubWindow);
    QPalette newPalette = q->palette();

    bool colorsInitialized = false;
#ifdef Q_WS_WIN // ask system properties on windows
#ifndef SPI_GETGRADIENTCAPTIONS
#define SPI_GETGRADIENTCAPTIONS 0x1008
#endif
#ifndef COLOR_GRADIENTACTIVECAPTION
#define COLOR_GRADIENTACTIVECAPTION 27
#endif
#ifndef COLOR_GRADIENTINACTIVECAPTION
#define COLOR_GRADIENTINACTIVECAPTION 28
#endif
    if (QApplication::desktopSettingsAware()) {
        newPalette.setColor(QPalette::Active, QPalette::Highlight,
                            colorref2qrgb(GetSysColor(COLOR_ACTIVECAPTION)));
        newPalette.setColor(QPalette::Inactive, QPalette::Highlight,
                            colorref2qrgb(GetSysColor(COLOR_INACTIVECAPTION)));
        newPalette.setColor(QPalette::Active, QPalette::HighlightedText,
                            colorref2qrgb(GetSysColor(COLOR_CAPTIONTEXT)));
        newPalette.setColor(QPalette::Inactive, QPalette::HighlightedText,
                            colorref2qrgb(GetSysColor(COLOR_INACTIVECAPTIONTEXT)));
        if (QSysInfo::WindowsVersion != QSysInfo::WV_95
                && QSysInfo::WindowsVersion != QSysInfo::WV_NT) {
            colorsInitialized = true;
            BOOL hasGradient;
            QT_WA({
                SystemParametersInfo(SPI_GETGRADIENTCAPTIONS, 0, &hasGradient, 0);
            } , {
                SystemParametersInfoA(SPI_GETGRADIENTCAPTIONS, 0, &hasGradient, 0);
            });
            if (hasGradient) {
                newPalette.setColor(QPalette::Active, QPalette::Base,
                                    colorref2qrgb(GetSysColor(COLOR_GRADIENTACTIVECAPTION)));
                newPalette.setColor(QPalette::Inactive, QPalette::Base,
                                    colorref2qrgb(GetSysColor(COLOR_GRADIENTINACTIVECAPTION)));
            } else {
                newPalette.setColor(QPalette::Active, QPalette::Base,
                                    newPalette.color(QPalette::Active, QPalette::Highlight));
                newPalette.setColor(QPalette::Inactive, QPalette::Base,
                                    newPalette.color(QPalette::Inactive, QPalette::Highlight));
            }
        }
    }
#endif // Q_WS_WIN
    if (!colorsInitialized) {
        newPalette.setColor(QPalette::Active, QPalette::Highlight,
                            newPalette.color(QPalette::Active, QPalette::Highlight));
        newPalette.setColor(QPalette::Active, QPalette::Base,
                            newPalette.color(QPalette::Active, QPalette::Highlight));
        newPalette.setColor(QPalette::Inactive, QPalette::Highlight,
                            newPalette.color(QPalette::Inactive, QPalette::Dark));
        newPalette.setColor(QPalette::Inactive, QPalette::Base,
                            newPalette.color(QPalette::Inactive, QPalette::Dark));
        newPalette.setColor(QPalette::Inactive, QPalette::HighlightedText,
                            newPalette.color(QPalette::Inactive, QPalette::Window));
    }

    return newPalette;
}

#ifndef QT_NO_ACTION
void QMdiSubWindowPrivate::updateActions()
{
    Qt::WindowFlags windowFlags = q_func()->windowFlags();
    // Hide all
    for (int i = 0; i < NumWindowStateActions; ++i)
        setVisible(WindowStateAction(i), false);

    if (windowFlags & Qt::FramelessWindowHint)
        return;

    setVisible(StayOnTopAction, true);
    setVisible(MoveAction, moveEnabled);
    setVisible(ResizeAction, resizeEnabled);

    // CloseAction
    if (windowFlags & Qt::WindowSystemMenuHint)
        setVisible(CloseAction, true);

    // RestoreAction
    if (windowFlags & (Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint))
        setVisible(RestoreAction, true);

    // MinimizeAction
    if (windowFlags & Qt::WindowMinimizeButtonHint)
        setVisible(MinimizeAction, true);

    // MaximizeAction
    if (windowFlags & Qt::WindowMaximizeButtonHint)
        setVisible(MaximizeAction, true);
}
#endif // QT_NO_ACTION

void QMdiSubWindowPrivate::setFocusWidget()
{
    Q_Q(QMdiSubWindow);
    if (!baseWidget) {
        q->setFocus();
        return;
    }

    // This will give focus to the next child if possible, otherwise
    // do nothing, hence it's not possible to tab between windows with
    // just hitting tab (unless Qt::TabFocus is removed from the focus policy).
    if (focusInReason == Qt::TabFocusReason) {
        q->focusNextChild();
        return;
    }

    // Same as above, but gives focus to the previous child.
    if (focusInReason == Qt::BacktabFocusReason) {
        q->focusPreviousChild();
        return;
    }

    if (QWidget *focusWidget = baseWidget->focusWidget()) {
        if (!focusWidget->hasFocus() && q->isAncestorOf(focusWidget)
                && focusWidget->isVisible() && !q->isMinimized()
                && focusWidget->focusPolicy() != Qt::NoFocus) {
            focusWidget->setFocus();
        } else {
            q->setFocus();
        }
        return;
    }

    QWidget *focusWidget = q->nextInFocusChain();
    while (focusWidget && focusWidget != q && focusWidget->focusPolicy() == Qt::NoFocus)
        focusWidget = focusWidget->nextInFocusChain();
    if (focusWidget && q->isAncestorOf(focusWidget))
        focusWidget->setFocus();
    else if (baseWidget->focusPolicy() != Qt::NoFocus)
        baseWidget->setFocus();
    else if (!q->hasFocus())
        q->setFocus();
}

void QMdiSubWindowPrivate::restoreFocus()
{
    if (!restoreFocusWidget)
        return;
    if (!restoreFocusWidget->hasFocus() && q_func()->isAncestorOf(restoreFocusWidget)
            && restoreFocusWidget->isVisible()
            && restoreFocusWidget->focusPolicy() != Qt::NoFocus) {
        restoreFocusWidget->setFocus();
    }
    restoreFocusWidget = 0;
}

/*!
    \internal
    ### Please add QEvent::WindowFlagsChange event
*/
void QMdiSubWindowPrivate::setWindowFlags(Qt::WindowFlags windowFlags)
{
    Q_Q(QMdiSubWindow);
    if (!q->parent()) {
        q->setWindowFlags(windowFlags);
        return;
    }

    Qt::WindowFlags windowType = windowFlags & Qt::WindowType_Mask;
    if (windowType == Qt::Dialog || windowFlags & Qt::MSWindowsFixedSizeDialogHint)
        windowFlags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint;

    // Set standard flags if none of the customize flags are set
    if (!(windowFlags & CustomizeWindowFlags))
        windowFlags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint;
    else if (windowFlags & Qt::FramelessWindowHint && windowFlags & Qt::WindowStaysOnTopHint)
        windowFlags = Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint;
    else if (windowFlags & Qt::FramelessWindowHint)
        windowFlags = Qt::FramelessWindowHint;

    windowFlags &= ~windowType;
    windowFlags |= Qt::SubWindow;

#ifndef QT_NO_ACTION
    if (QAction *stayOnTopAction = actions[QMdiSubWindowPrivate::StayOnTopAction]) {
        if (windowFlags & Qt::WindowStaysOnTopHint)
            stayOnTopAction->setChecked(true);
        else
            stayOnTopAction->setChecked(false);
    }
#endif

#ifndef QT_NO_SIZEGRIP
    if ((windowFlags & Qt::FramelessWindowHint) && sizeGrip)
        delete sizeGrip;
#endif

    q->setWindowFlags(windowFlags);
    updateGeometryConstraints();
#ifndef QT_NO_ACTION
    updateActions();
#endif
    QSize currentSize = q->size();
    if (q->isVisible() && (currentSize.width() < internalMinimumSize.width()
            || currentSize.height() < internalMinimumSize.height())) {
        q->resize(currentSize.expandedTo(internalMinimumSize));
    }
}

#ifndef QT_NO_ACTION
void QMdiSubWindowPrivate::setEnabled(WindowStateAction action, bool enable)
{
    if (actions[action])
        actions[action]->setEnabled(enable);
}

void QMdiSubWindowPrivate::setVisible(WindowStateAction action, bool visible)
{
    if (actions[action])
        actions[action]->setVisible(visible);
}

#ifndef QT_NO_MENU
void QMdiSubWindowPrivate::addToSystemMenu(WindowStateAction action, const QString &text,
                                           const char *slot)
{
    if (!systemMenu)
        return;
    actions[action] = systemMenu->addAction(text, q_func(), slot);
}
#endif
#endif // QT_NO_ACTION

/*!
    \internal
*/
QSize QMdiSubWindowPrivate::iconSize() const
{
    Q_Q(const QMdiSubWindow);
    if (!q->parent() || q->windowFlags() & Qt::FramelessWindowHint)
        return QSize(-1, -1);
    return QSize(q->style()->pixelMetric(QStyle::PM_MdiSubWindowMinimizedWidth), titleBarHeight());
}

#ifndef QT_NO_SIZEGRIP

/*!
    \internal
*/
void QMdiSubWindowPrivate::setSizeGrip(QSizeGrip *newSizeGrip)
{
    Q_Q(QMdiSubWindow);
    if (!newSizeGrip || sizeGrip || q->windowFlags() & Qt::FramelessWindowHint)
        return;

    if (q->layout() && q->layout()->indexOf(newSizeGrip) != -1)
        return;
    newSizeGrip->setFixedSize(newSizeGrip->sizeHint());
    bool putSizeGripInLayout = q->layout() ? true : false;
#if defined(Q_WS_MAC) && !defined(QT_NO_STYLE_MAC)
    if (qobject_cast<QMacStyle *>(q->style()))
        putSizeGripInLayout = false;
#endif
    if (putSizeGripInLayout) {
        q->layout()->addWidget(newSizeGrip);
        q->layout()->setAlignment(newSizeGrip, Qt::AlignBottom | Qt::AlignRight);
    } else {
        newSizeGrip->setParent(q);
        newSizeGrip->move(q->isLeftToRight() ? q->width() - newSizeGrip->width() : 0,
                          q->height() - newSizeGrip->height());
        sizeGrip = newSizeGrip;
    }
    newSizeGrip->raise();
    updateGeometryConstraints();
    newSizeGrip->installEventFilter(q);
}

/*!
    \internal
*/
void QMdiSubWindowPrivate::setSizeGripVisible(bool visible) const
{
    // See if we can find any size grips
    QList<QSizeGrip *> sizeGrips = qFindChildren<QSizeGrip *>(q_func());
    foreach (QSizeGrip *grip, sizeGrips)
        grip->setVisible(visible);
}

#endif // QT_NO_SIZEGRIP

/*!
    \internal
*/
void QMdiSubWindowPrivate::updateInternalWindowTitle()
{
    Q_Q(QMdiSubWindow);
    if (q->isWindowModified()) {
        windowTitle = q->windowTitle();
        windowTitle.replace(QLatin1String("[*]"), QLatin1String("*"));
    } else {
        windowTitle = qt_setWindowTitle_helperHelper(q->windowTitle(), q);
    }
}

/*!
    Constructs a new QMdiSubWindow widget. The \a parent and \a
    flags arguments are passed to QWidget's constructor.

    Instead of using addSubWindow(), it is also simply possible to
    use setParent() when you add the subwindow to a QMdiArea.

    Note that only \l{QMdiSubWindow}s can be set as children of
    QMdiArea; you cannot, for instance, write:

    \badcode
        QMdiArea mdiArea;
        QTextEdit editor(&mdiArea); // invalid child widget
    \endcode

    \sa QMdiArea::addSubWindow()
*/
QMdiSubWindow::QMdiSubWindow(QWidget *parent, Qt::WindowFlags flags)
    : QWidget(*new QMdiSubWindowPrivate, parent, 0)
{
    Q_D(QMdiSubWindow);
#ifndef QT_NO_MENU
    d->createSystemMenu();
    addActions(d->systemMenu->actions());
#endif
    d->setWindowFlags(flags);
    setBackgroundRole(QPalette::Window);
    setAutoFillBackground(true);
    setMouseTracking(true);
    setLayout(new QVBoxLayout);
    setFocusPolicy(Qt::StrongFocus);
    layout()->setMargin(0);
    d->updateGeometryConstraints();
    setAttribute(Qt::WA_Resized, false);
    d->titleBarPalette = d->desktopPalette();
    d->font = QApplication::font("QWorkspaceTitleBar");
    // We don't want the menu icon by default on mac.
#ifndef Q_WS_MAC
    d->menuIcon = style()->standardIcon(QStyle::SP_TitleBarMenuButton);
#endif
    connect(qApp, SIGNAL(focusChanged(QWidget *, QWidget *)),
            this, SLOT(_q_processFocusChanged(QWidget *, QWidget *)));
}

/*!
    Destroys the subwindow.

    \sa QMdiArea::removeSubWindow()
*/
QMdiSubWindow::~QMdiSubWindow()
{
    Q_D(QMdiSubWindow);
#ifndef QT_NO_MENUBAR
    d->removeButtonsFromMenuBar();
#endif
    d->setActive(false);
}

/*!
    Sets \a widget as the internal widget of this subwindow. The
    internal widget is displayed in the center of the subwindow
    beneath the title bar.

    QMdiSubWindow takes temporary ownership of \a widget; you do
    not have to delete it. Any existing internal widget will be
    removed and reparented to the root window.

    \sa widget()
*/
void QMdiSubWindow::setWidget(QWidget *widget)
{
    Q_D(QMdiSubWindow);
    if (!widget) {
        d->removeBaseWidget();
        return;
    }

    if (widget == d->baseWidget) {
        qWarning("QMdiSubWindow::setWidget: widget is already set");
        return;
    }

    bool wasResized = testAttribute(Qt::WA_Resized);
    d->removeBaseWidget();

    if (QLayout *layout = this->layout())
        layout->addWidget(widget);
    else
        widget->setParent(this);

#ifndef QT_NO_SIZEGRIP
    QSizeGrip *sizeGrip = qFindChild<QSizeGrip *>(widget);
    if (sizeGrip)
        sizeGrip->installEventFilter(this);
    if (d->sizeGrip)
        d->sizeGrip->raise();
#endif

    d->baseWidget = widget;
    d->baseWidget->installEventFilter(this);

    d->ignoreWindowTitleChange = true;
    bool isWindowModified = this->isWindowModified();
    if (windowTitle().isEmpty()) {
        d->updateWindowTitle(true);
        isWindowModified = d->baseWidget->isWindowModified();
    }
    if (!this->isWindowModified() && isWindowModified
            && windowTitle().contains(QLatin1String("[*]"))) {
        setWindowModified(isWindowModified);
    }
    d->lastChildWindowTitle = d->baseWidget->windowTitle();
    d->ignoreWindowTitleChange = false;

    d->updateGeometryConstraints();
    if (!wasResized && testAttribute(Qt::WA_Resized))
        setAttribute(Qt::WA_Resized, false);
}

/*!
    Returns the current internal widget.

    \sa setWidget()
*/
QWidget *QMdiSubWindow::widget() const
{
    return d_func()->baseWidget;
}


/*!
    \internal
*/
QWidget *QMdiSubWindow::maximizedButtonsWidget() const
{
    Q_D(const QMdiSubWindow);
    if (isVisible() && d->controlContainer && isMaximized() && !d->drawTitleBarWhenMaximized())
        return d->controlContainer->controllerWidget();
    return 0;
}

/*!
    \internal
*/
QWidget *QMdiSubWindow::maximizedSystemMenuIconWidget() const
{
    Q_D(const QMdiSubWindow);
    if (isVisible() && d->controlContainer && isMaximized() && !d->drawTitleBarWhenMaximized())
        return d->controlContainer->systemMenuLabel();
    return 0;
}

/*!
    Returns true if this window is shaded; otherwise returns false.

    A window is shaded if it is collapsed so that only the title bar is
    visible.
*/
bool QMdiSubWindow::isShaded() const
{
    return d_func()->isShadeMode;
}

/*!
    If \a on is true, \a option is enabled on the subwindow; otherwise it is
    disabled. See SubWindowOption for the effect of each option.

    \sa SubWindowOption, testOption()
*/
void QMdiSubWindow::setOption(SubWindowOption option, bool on)
{
    Q_D(QMdiSubWindow);
    if (on && !(d->options & option))
        d->options |= option;
    else if (!on && (d->options & option))
        d->options &= ~option;

#ifndef QT_NO_RUBBERBAND
    if ((option & (RubberBandResize | RubberBandMove)) && !on && d->isInRubberBandMode)
        d->leaveRubberBandMode();
#endif
}

/*!
    Returns true if \a option is enabled; otherwise returns false.

    \sa SubWindowOption, setOption()
*/
bool QMdiSubWindow::testOption(SubWindowOption option) const
{
    return d_func()->options & option;
}

/*!
    \property QMdiSubWindow::keyboardSingleStep
    \brief sets how far a widget should move or resize when using the
    keyboard arrow keys.

    When in keyboard-interactive mode, you can use the arrow and page keys to
    either move or resize the window. This property controls the arrow keys.
    The common way to enter keyboard interactive mode is to enter the
    subwindow menu, and select either "resize" or "move".

    The default keyboard single step value is 5 pixels.

    \sa keyboardPageStep
*/
int QMdiSubWindow::keyboardSingleStep() const
{
    return d_func()->keyboardSingleStep;
}

void QMdiSubWindow::setKeyboardSingleStep(int step)
{
    // Haven't done any boundary check here since negative step only
    // means inverted behavior, which is OK if the user want it.
    // A step equal to zero means "do nothing".
    d_func()->keyboardSingleStep = step;
}

/*!
    \property QMdiSubWindow::keyboardPageStep
    \brief sets how far a widget should move or resize when using the
    keyboard page keys.

    When in keyboard-interactive mode, you can use the arrow and page keys to
    either move or resize the window. This property controls the page
    keys. The common way to enter keyboard interactive mode is to enter the
    subwindow menu, and select either "resize" or "move".

    The default keyboard page step value is 20 pixels.

    \sa keyboardSingleStep
*/
int QMdiSubWindow::keyboardPageStep() const
{
    return d_func()->keyboardPageStep;
}

void QMdiSubWindow::setKeyboardPageStep(int step)
{
    // Haven't done any boundary check here since negative step only
    // means inverted behavior, which is OK if the user want it.
    // A step equal to zero means "do nothing".
    d_func()->keyboardPageStep = step;
}

#ifndef QT_NO_MENU
/*!
    Sets \a systemMenu as the current system menu for this subwindow.

    By default, each QMdiSubWindow has a standard system menu.

    QActions for the system menu created by QMdiSubWindow will
    automatically be updated depending on the current window state;
    e.g., the minimize action will be disabled after the window is
    minimized.

    QActions added by the user are not updated by QMdiSubWindow.

    QMdiSubWindow takes ownership of \a systemMenu; you do not have to
    delete it. Any existing menus will be deleted.

    \sa systemMenu(), showSystemMenu()
*/
void QMdiSubWindow::setSystemMenu(QMenu *systemMenu)
{
    Q_D(QMdiSubWindow);
    if (systemMenu && systemMenu == d->systemMenu) {
        qWarning("QMdiSubWindow::setSystemMenu: system menu is already set");
        return;
    }

    if (d->systemMenu) {
        delete d->systemMenu;
        d->systemMenu = 0;
    }

    if (!systemMenu)
        return;

    if (systemMenu->parent() != this)
        systemMenu->setParent(this);
    d->systemMenu = systemMenu;
}

/*!
    Returns a pointer to the current system menu, or zero if no system
    menu is set. QMdiSubWindow provides a default system menu, but you can
    also set the menu with setSystemMenu().

    \sa setSystemMenu(), showSystemMenu()
*/
QMenu *QMdiSubWindow::systemMenu() const
{
    return d_func()->systemMenu;
}

/*!
    Shows the system menu below the system menu icon in the title bar.

    \sa setSystemMenu(), systemMenu()
*/
void QMdiSubWindow::showSystemMenu()
{
    Q_D(QMdiSubWindow);
    if (!d->systemMenu)
        return;

    QPoint globalPopupPos;
    if (QWidget *icon = maximizedSystemMenuIconWidget()) {
        if (isLeftToRight())
            globalPopupPos = icon->mapToGlobal(QPoint(0, icon->y() + icon->height()));
        else
            globalPopupPos = icon->mapToGlobal(QPoint(icon->width(), icon->y() + icon->height()));
    } else {
        if (isLeftToRight())
            globalPopupPos = mapToGlobal(contentsRect().topLeft());
        else // + QPoint(1, 0) because topRight() == QPoint(left() + width() -1, top())
            globalPopupPos = mapToGlobal(contentsRect().topRight()) + QPoint(1, 0);
    }

    // Adjust x() with -menuwidth in reverse mode.
    if (isRightToLeft())
        globalPopupPos -= QPoint(d->systemMenu->sizeHint().width(), 0);
    d->systemMenu->popup(globalPopupPos);
}
#endif // QT_NO_MENU

/*!
    Calling this function makes the subwindow enter the shaded mode.
    When the subwindow is shaded, only the title bar is visible.

    Although shading is not supported by all styles, this function will
    still show the subwindow as shaded, regardless of whether support
    for shading is available. However, when used with styles without
    shading support, the user will be unable to return from shaded mode
    through the user interface (e.g., through a shade button in the title
    bar).

    \sa isShaded()
*/
void QMdiSubWindow::showShaded()
{
    if (!parent())
        return;

    Q_D(QMdiSubWindow);
    // setMinimizeMode uses this function.
    if (!d->isShadeRequestFromMinimizeMode && isShaded())
        return;

    d->isMaximizeMode = false;

    QWidget *currentFocusWidget = QApplication::focusWidget();
    if (!d->restoreFocusWidget && isAncestorOf(currentFocusWidget))
        d->restoreFocusWidget = currentFocusWidget;

    if (!d->isShadeRequestFromMinimizeMode) {
        d->isShadeMode = true;
        d->ensureWindowState(Qt::WindowMinimized);
    }

#ifndef QT_NO_MENUBAR
    d->removeButtonsFromMenuBar();
#endif

    // showMinimized() will reset Qt::WindowActive, which makes sense
    // for top level widgets, but in MDI it makes sense to have an
    // active window which is minimized.
    if (hasFocus() || isAncestorOf(currentFocusWidget))
        d->ensureWindowState(Qt::WindowActive);

#ifndef QT_NO_SIZEGRIP
    d->setSizeGripVisible(false);
#endif

    if (!d->restoreSize.isValid() || d->isShadeMode) {
        d->oldGeometry = geometry();
        d->restoreSize.setWidth(d->oldGeometry.width());
        d->restoreSize.setHeight(d->oldGeometry.height());
    }

    // Hide the window before we change the geometry to avoid multiple resize
    // events and wrong window state.
    const bool wasVisible = isVisible();
    if (wasVisible)
        setVisible(false);

    d->updateGeometryConstraints();
    // Update minimum size to internalMinimumSize if set by user.
    if (minimumSize().isValid()) {
        d->userMinimumSize = minimumSize();
        setMinimumSize(d->internalMinimumSize);
    }
    resize(d->internalMinimumSize);

    // Hide the internal widget if not already hidden by the user.
    if (d->baseWidget && !d->baseWidget->isHidden()) {
        d->baseWidget->hide();
        d->isWidgetHiddenByUs = true;
    }

    if (wasVisible)
        setVisible(true);

    d->setFocusWidget();
    d->resizeEnabled = false;
    d->moveEnabled = true;
    d->updateDirtyRegions();
    d->updateMask();

#ifndef QT_NO_ACTION
    d->setEnabled(QMdiSubWindowPrivate::MinimizeAction, false);
    d->setEnabled(QMdiSubWindowPrivate::ResizeAction, d->resizeEnabled);
    d->setEnabled(QMdiSubWindowPrivate::MaximizeAction, true);
    d->setEnabled(QMdiSubWindowPrivate::RestoreAction, true);
    d->setEnabled(QMdiSubWindowPrivate::MoveAction, d->moveEnabled);
#endif
}

/*!
    \reimp
*/
bool QMdiSubWindow::eventFilter(QObject *object, QEvent *event)
{
    Q_D(QMdiSubWindow);
    if (!object)
        return QWidget::eventFilter(object, event);

#ifndef QT_NO_SIZEGRIP
    if (object != d->baseWidget && parent() && qobject_cast<QSizeGrip *>(object)) {
        if (event->type() != QEvent::MouseButtonPress || !testOption(QMdiSubWindow::RubberBandResize))
            return QWidget::eventFilter(object, event);
        const QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        d->mousePressPosition = parentWidget()->mapFromGlobal(mouseEvent->globalPos());
        d->oldGeometry = geometry();
        d->currentOperation = isLeftToRight() ? QMdiSubWindowPrivate::BottomRightResize
                                              : QMdiSubWindowPrivate::BottomLeftResize;
#ifndef QT_NO_RUBBERBAND
        d->enterRubberBandMode();
#endif
        return true;
    }
#endif

    if (object != d->baseWidget && event->type() != QEvent::WindowTitleChange)
        return QWidget::eventFilter(object, event);

    switch (event->type()) {
    case QEvent::Show:
        d->setActive(true);
        break;
    case QEvent::ShowToParent:
        if (!d->isWidgetHiddenByUs)
            show();
        break;
    case QEvent::WindowStateChange: {
        QWindowStateChangeEvent *changeEvent = static_cast<QWindowStateChangeEvent*>(event);
        if (changeEvent->isOverride())
            break;
        Qt::WindowStates oldState = changeEvent->oldState();
        Qt::WindowStates newState = d->baseWidget->windowState();
        if (!(oldState & Qt::WindowMinimized) && (newState & Qt::WindowMinimized))
            showMinimized();
        else if (!(oldState & Qt::WindowMaximized) && (newState & Qt::WindowMaximized))
            showMaximized();
        else if (!(newState & (Qt::WindowMaximized | Qt::WindowMinimized)))
            showNormal();
        break;
    }
    case QEvent::Enter:
        d->currentOperation = QMdiSubWindowPrivate::None;
        d->updateCursor();
        break;
    case QEvent::LayoutRequest:
        d->updateGeometryConstraints();
        break;
    case QEvent::WindowTitleChange:
        if (d->ignoreWindowTitleChange)
            break;
        if (object == d->baseWidget) {
            d->updateWindowTitle(true);
            d->lastChildWindowTitle = d->baseWidget->windowTitle();
#ifndef QT_NO_MENUBAR
        } else if (maximizedButtonsWidget() && d->controlContainer->menuBar() && d->controlContainer->menuBar()
                   ->cornerWidget(Qt::TopRightCorner) == maximizedButtonsWidget()) {
            if (d->baseWidget && d->baseWidget->windowTitle() == windowTitle())
                d->updateWindowTitle(true);
            else
                d->updateWindowTitle(false);
#endif
        }
        break;
    case QEvent::ModifiedChange: {
        if (object != d->baseWidget)
            break;
        bool windowModified = d->baseWidget->isWindowModified();
        if (!windowModified && d->baseWidget->windowTitle() != windowTitle())
            break;
        if (windowTitle().contains(QLatin1String("[*]")))
            setWindowModified(windowModified);
        break;
    }
    default:
        break;
    }
    return QWidget::eventFilter(object, event);
}

/*!
    \reimp
*/
bool QMdiSubWindow::event(QEvent *event)
{
    Q_D(QMdiSubWindow);
    switch (event->type()) {
    case QEvent::StyleChange: {
        bool wasShaded = isShaded();
        bool wasMinimized = isMinimized();
        bool wasMaximized = isMaximized();
        ensurePolished();
        setContentsMargins(0, 0, 0, 0);
        if (wasMinimized || wasMaximized || wasShaded)
            showNormal();
        d->updateGeometryConstraints();
        resize(d->internalMinimumSize.expandedTo(size()));
        d->updateMask();
        d->updateDirtyRegions();
        if (wasShaded)
            showShaded();
        else if (wasMinimized)
            showMinimized();
        else if (wasMaximized)
            showMaximized();
        break;
    }
    case QEvent::ParentAboutToChange:
        d->setActive(false);
        break;
    case QEvent::ParentChange: {
        bool wasResized = testAttribute(Qt::WA_Resized);
#ifndef QT_NO_MENUBAR
        d->removeButtonsFromMenuBar();
#endif
        d->currentOperation = QMdiSubWindowPrivate::None;
        d->activeSubControl = QStyle::SC_None;
        d->hoveredSubControl = QStyle::SC_None;
#ifndef QT_NO_RUBBERBAND
        if (d->isInRubberBandMode)
            d->leaveRubberBandMode();
#endif
        d->isShadeMode = false;
        d->isMaximizeMode = false;
        d->isWidgetHiddenByUs = false;
        if (!parent()) {
#if !defined(QT_NO_SIZEGRIP) && defined(Q_WS_MAC) && !defined(QT_NO_STYLE_MAC)
            if (qobject_cast<QMacStyle *>(style()))
                delete d->sizeGrip;
#endif
            setOption(RubberBandResize, false);
            setOption(RubberBandMove, false);
        } else {
            d->setWindowFlags(windowFlags());
        }
        setContentsMargins(0, 0, 0, 0);
        d->updateGeometryConstraints();
        d->updateCursor();
        d->updateMask();
        d->updateDirtyRegions();
#ifndef QT_NO_ACTION
        d->updateActions();
#endif
        if (!wasResized && testAttribute(Qt::WA_Resized))
            setAttribute(Qt::WA_Resized, false);
        break;
    }
    case QEvent::WindowActivate:
        if (d->ignoreNextActivationEvent) {
            d->ignoreNextActivationEvent = false;
            break;
        }
        d->isExplicitlyDeactivated = false;
        d->setActive(true);
        break;
    case QEvent::WindowDeactivate:
        if (d->ignoreNextActivationEvent) {
            d->ignoreNextActivationEvent = false;
            break;
        }
        d->isExplicitlyDeactivated = true;
        d->setActive(false);
        break;
    case QEvent::WindowTitleChange:
        if (!d->ignoreWindowTitleChange)
            d->updateWindowTitle(false);
        d->updateInternalWindowTitle();
        break;
    case QEvent::ModifiedChange:
        if (!windowTitle().contains(QLatin1String("[*]")))
            break;
#ifndef QT_NO_MENUBAR
        if (maximizedButtonsWidget() && d->controlContainer->menuBar() && d->controlContainer->menuBar()
                ->cornerWidget(Qt::TopRightCorner) == maximizedButtonsWidget()) {
            window()->setWindowModified(isWindowModified());
        }
#endif // QT_NO_MENUBAR
        d->updateInternalWindowTitle();
        update(0, 0, width(), d->titleBarHeight());
        break;
    case QEvent::LayoutDirectionChange:
        d->updateDirtyRegions();
        break;
    case QEvent::LayoutRequest:
        d->updateGeometryConstraints();
        break;
    case QEvent::WindowIconChange:
        d->menuIcon = windowIcon();
        if (d->menuIcon.isNull())
            d->menuIcon = style()->standardIcon(QStyle::SP_TitleBarMenuButton);
        if (d->controlContainer)
            d->controlContainer->updateWindowIcon(d->menuIcon);
        if (!maximizedSystemMenuIconWidget())
            update(0, 0, width(), d->titleBarHeight());
        break;
    case QEvent::PaletteChange:
        d->titleBarPalette = d->desktopPalette();
        break;
    default:
        break;
    }
    return QWidget::event(event);
}

/*!
    \reimp
*/
void QMdiSubWindow::showEvent(QShowEvent *showEvent)
{
    Q_D(QMdiSubWindow);
    if (!parent()) {
        QWidget::showEvent(showEvent);
        return;
    }

#if !defined(QT_NO_SIZEGRIP) && defined(Q_WS_MAC) && !defined(QT_NO_STYLE_MAC)
    if (qobject_cast<QMacStyle *>(style()) && !d->sizeGrip
            && !(windowFlags() & Qt::FramelessWindowHint)) {
        d->setSizeGrip(new QSizeGrip(0));
        Q_ASSERT(d->sizeGrip);
        if (isMinimized())
            d->setSizeGripVisible(false);
        else
            d->setSizeGripVisible(true);
        resize(size().expandedTo(d->internalMinimumSize));
    }
#endif

    d->updateDirtyRegions();
    // Show buttons in the menu bar if they're already not there.
    // We want to do this when QMdiSubWindow becomes visible after being hidden.
#if !defined(QT_NO_MENUBAR) && !defined(QT_NO_MAINWINDOW)
    if (isMaximized() && d->controlContainer && !d->drawTitleBarWhenMaximized()) {
        if (QMainWindow *mainWindow = qobject_cast<QMainWindow *>(window())) {
            QMenuBar *menuBar = mainWindow->menuBar();
            if (menuBar->cornerWidget(Qt::TopRightCorner) != maximizedButtonsWidget())
                d->showButtonsInMenuBar(menuBar);
        }
    }
#endif
    d->setActive(true);
}

/*!
    \reimp
*/
void QMdiSubWindow::hideEvent(QHideEvent * /*hideEvent*/)
{
#ifndef QT_NO_MENUBAR
    d_func()->removeButtonsFromMenuBar();
#endif
}

/*!
    \reimp
*/
void QMdiSubWindow::changeEvent(QEvent *changeEvent)
{
    if (!parent()) {
        QWidget::changeEvent(changeEvent);
        return;
    }

    if (changeEvent->type() != QEvent::WindowStateChange) {
        QWidget::changeEvent(changeEvent);
        return;
    }

    QWindowStateChangeEvent *event = static_cast<QWindowStateChangeEvent *>(changeEvent);
    if (event->isOverride()) {
        event->ignore();
        return;
    }

    Qt::WindowStates oldState = event->oldState();
    Qt::WindowStates newState = windowState();
    if (oldState == newState) {
        changeEvent->ignore();
        return;
    }

    // QWidget ensures that the widget is visible _after_ setWindowState(),
    // but we need to ensure that the widget is visible _before_
    // setWindowState() returns.
    Q_D(QMdiSubWindow);
    if (!isVisible()) {
        d->ensureWindowState(Qt::WindowNoState);
        setVisible(true);
    }

    if (!d->oldGeometry.isValid())
        d->oldGeometry = geometry();

    if ((oldState & Qt::WindowActive) && (newState & Qt::WindowActive))
        d->currentOperation = QMdiSubWindowPrivate::None;

    if (!(oldState & Qt::WindowMinimized) && (newState & Qt::WindowMinimized))
        d->setMinimizeMode();
    else if (!(oldState & Qt::WindowMaximized) && (newState & Qt::WindowMaximized))
        d->setMaximizeMode();
    else if (!(newState & (Qt::WindowMaximized | Qt::WindowMinimized)))
        d->setNormalMode();

    if (d->isActive)
        d->ensureWindowState(Qt::WindowActive);
    emit windowStateChanged(oldState, windowState());
}

/*!
    \reimp
*/
void QMdiSubWindow::closeEvent(QCloseEvent *closeEvent)
{
    Q_D(QMdiSubWindow);
    bool acceptClose = true;
    if (d->baseWidget)
        acceptClose = d->baseWidget->close();
    if (!acceptClose) {
        closeEvent->ignore();
        return;
    }
#ifndef QT_NO_MENUBAR
    d->removeButtonsFromMenuBar();
#endif
    d->setActive(false);
    if (parentWidget() && testAttribute(Qt::WA_DeleteOnClose)) {
        QChildEvent childRemoved(QEvent::ChildRemoved, this);
        QApplication::sendEvent(parentWidget(), &childRemoved);
    }
    closeEvent->accept();
}

/*!
    \reimp
*/
void QMdiSubWindow::leaveEvent(QEvent * /*leaveEvent*/)
{
    Q_D(QMdiSubWindow);
    if (d->hoveredSubControl != QStyle::SC_None) {
        d->hoveredSubControl = QStyle::SC_None;
        update(QRegion(0, 0, width(), d->titleBarHeight()));
    }
}

/*!
    \reimp
*/
void QMdiSubWindow::resizeEvent(QResizeEvent *resizeEvent)
{
    Q_D(QMdiSubWindow);
#ifndef QT_NO_SIZEGRIP
    if (d->sizeGrip) {
        d->sizeGrip->move(isLeftToRight() ? width() - d->sizeGrip->width() : 0,
                          height() - d->sizeGrip->height());
    }
#endif

    if (!parent()) {
        QWidget::resizeEvent(resizeEvent);
        return;
    }

    if (d->isMaximizeMode)
        d->ensureWindowState(Qt::WindowMaximized);

    d->updateMask();
    if (!isVisible())
        return;

    if (d->resizeTimerId <= 0)
        d->cachedStyleOptions = d->titleBarOptions();
    else
        killTimer(d->resizeTimerId);
    d->resizeTimerId = startTimer(200);
}

/*!
    \reimp
*/
void QMdiSubWindow::timerEvent(QTimerEvent *timerEvent)
{
    Q_D(QMdiSubWindow);
    if (timerEvent->timerId() == d->resizeTimerId) {
        killTimer(d->resizeTimerId);
        d->resizeTimerId = -1;
        d->updateDirtyRegions();
    }
}

/*!
    \reimp
*/
void QMdiSubWindow::moveEvent(QMoveEvent *moveEvent)
{
    if (!parent()) {
        QWidget::moveEvent(moveEvent);
        return;
    }

    Q_D(QMdiSubWindow);
    if (d->isMaximizeMode)
        d->ensureWindowState(Qt::WindowMaximized);
}

/*!
    \reimp
*/
void QMdiSubWindow::paintEvent(QPaintEvent *paintEvent)
{
    if (!parent() || (windowFlags() & Qt::FramelessWindowHint)) {
        QWidget::paintEvent(paintEvent);
        return;
    }

    Q_D(QMdiSubWindow);
    if (isMaximized() && !d->drawTitleBarWhenMaximized())
        return;

    if (d->resizeTimerId != -1) {
        // Only update the style option rect and the window title.
        int border = d->hasBorder(d->cachedStyleOptions) ? 4 : 0;
        int titleBarHeight = d->titleBarHeight(d->cachedStyleOptions);
        titleBarHeight -= isMinimized() ? 2 * border : border;
        d->cachedStyleOptions.rect = QRect(border, border, width() - 2 * border, titleBarHeight);
        if (!d->windowTitle.isEmpty()) {
            int width = style()->subControlRect(QStyle::CC_TitleBar, &d->cachedStyleOptions,
                                                QStyle::SC_TitleBarLabel, this).width();
            d->cachedStyleOptions.text = d->cachedStyleOptions.fontMetrics
                                         .elidedText(d->windowTitle, Qt::ElideRight, width);
        }
    } else {
        // Force full update.
        d->cachedStyleOptions = d->titleBarOptions();
    }

    QStylePainter painter(this);
    if (!d->windowTitle.isEmpty())
        painter.setFont(d->font);
    painter.drawComplexControl(QStyle::CC_TitleBar, d->cachedStyleOptions);

    if (isMinimized() && !d->hasBorder(d->cachedStyleOptions))
        return;

    QStyleOptionFrame frameOptions;
    frameOptions.initFrom(this);
    frameOptions.lineWidth = style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth, 0, this);
    frameOptions.midLineWidth = 1;
    if (d->isActive)
        frameOptions.state |= QStyle::State_Active;
    else
        frameOptions.state &= ~QStyle::State_Active;

    // ### Ensure that we do not require setting the cliprect for 4.4
    if (!isMinimized() && !d->hasBorder(d->cachedStyleOptions))
        painter.setClipRect(rect().adjusted(0, d->titleBarHeight(d->cachedStyleOptions), 0, 0));
    if (!isMinimized() || d->hasBorder(d->cachedStyleOptions))
        painter.drawPrimitive(QStyle::PE_FrameWindow, frameOptions);
}

/*!
    \reimp
*/
void QMdiSubWindow::mousePressEvent(QMouseEvent *mouseEvent)
{
    if (!parent()) {
        QWidget::mousePressEvent(mouseEvent);
        return;
    }

    Q_D(QMdiSubWindow);
    if (d->isInInteractiveMode)
        d->leaveInteractiveMode();
#ifndef QT_NO_RUBBERBAND
    if (d->isInRubberBandMode)
        d->leaveRubberBandMode();
#endif

    if (mouseEvent->button() != Qt::LeftButton) {
        mouseEvent->ignore();
        return;
    }

    if (d->currentOperation != QMdiSubWindowPrivate::None) {
        d->updateCursor();
        d->mousePressPosition = mapToParent(mouseEvent->pos());
        if (d->resizeEnabled || d->moveEnabled)
            d->oldGeometry = geometry();
#ifndef QT_NO_RUBBERBAND
        if (testOption(QMdiSubWindow::RubberBandResize) && d->isResizeOperation()
                || testOption(QMdiSubWindow::RubberBandMove) && d->isMoveOperation()) {
            d->enterRubberBandMode();
        }
#endif
        return;
    }
    d->activeSubControl = d->hoveredSubControl;
    update(QRegion(0, 0, width(), d->titleBarHeight()));
}

/*!
    \reimp
*/
void QMdiSubWindow::mouseDoubleClickEvent(QMouseEvent *mouseEvent)
{
    if (!parent()) {
        QWidget::mouseDoubleClickEvent(mouseEvent);
        return;
    }

    if (mouseEvent->button() != Qt::LeftButton) {
        mouseEvent->ignore();
        return;
    }

    if (!d_func()->isMoveOperation()) {
        mouseEvent->ignore();
        return;
    }

    Qt::WindowFlags flags = windowFlags();
    if (isMinimized()) {
        if (isShaded() && (flags & Qt::WindowShadeButtonHint)
                || (flags & Qt::WindowMinimizeButtonHint)) {
            showNormal();
        }
        return;
    }

    if (isMaximized()) {
       if (flags & Qt::WindowMaximizeButtonHint)
           showNormal();
       return;
    }

    if (flags & Qt::WindowShadeButtonHint)
        showShaded();
    else if (flags & Qt::WindowMaximizeButtonHint)
        showMaximized();
}

/*!
    \reimp
*/
void QMdiSubWindow::mouseReleaseEvent(QMouseEvent *mouseEvent)
{
    if (!parent()) {
        QWidget::mouseReleaseEvent(mouseEvent);
        return;
    }

    if (mouseEvent->button() != Qt::LeftButton) {
        mouseEvent->ignore();
        return;
    }

    Q_D(QMdiSubWindow);
    if (d->currentOperation != QMdiSubWindowPrivate::None) {
#ifndef QT_NO_RUBBERBAND
        if (d->isInRubberBandMode && !d->isInInteractiveMode)
            d->leaveRubberBandMode();
#endif
        if (d->resizeEnabled || d->moveEnabled)
            d->oldGeometry = geometry();
    }

    d->currentOperation = d->getOperation(mouseEvent->pos());
    d->updateCursor();

    d->hoveredSubControl = d->getSubControl(mouseEvent->pos());
    if (d->activeSubControl != QStyle::SC_None
            && d->activeSubControl == d->hoveredSubControl) {
        d->processClickedSubControl();
    }
    d->activeSubControl = QStyle::SC_None;
    update(QRegion(0, 0, width(), d->titleBarHeight()));
}

/*!
    \reimp
*/
void QMdiSubWindow::mouseMoveEvent(QMouseEvent *mouseEvent)
{
    if (!parent()) {
        QWidget::mouseMoveEvent(mouseEvent);
        return;
    }

    Q_D(QMdiSubWindow);
    // No update needed if we're in a move/resize operation.
    if (!d->isMoveOperation() && !d->isResizeOperation()) {
        // Find previous and current hover region.
        const QStyleOptionTitleBar options = d->titleBarOptions();
        QStyle::SubControl oldHover = d->hoveredSubControl;
        d->hoveredSubControl = d->getSubControl(mouseEvent->pos());
        QRegion hoverRegion;
        if (isHoverControl(oldHover) && oldHover != d->hoveredSubControl)
            hoverRegion += style()->subControlRect(QStyle::CC_TitleBar, &options, oldHover, this);
        if (isHoverControl(d->hoveredSubControl) && d->hoveredSubControl != oldHover) {
            hoverRegion += style()->subControlRect(QStyle::CC_TitleBar, &options,
                    d->hoveredSubControl, this);
        }
#if defined(Q_WS_MAC) && !defined(QT_NO_STYLE_MAC)
        if (qobject_cast<QMacStyle *>(style()) && !hoverRegion.isEmpty())
            hoverRegion += QRegion(0, 0, width(), d->titleBarHeight(options));
#endif
        if (!hoverRegion.isEmpty())
            update(hoverRegion);
    }

    if ((mouseEvent->buttons() & Qt::LeftButton) || d->isInInteractiveMode) {
        if (d->isResizeOperation() && d->resizeEnabled || d->isMoveOperation() && d->moveEnabled)
            d->setNewGeometry(mapToParent(mouseEvent->pos()));
        return;
    }

    // Do not resize/move if not allowed.
    d->currentOperation = d->getOperation(mouseEvent->pos());
    if (d->isResizeOperation() && !d->resizeEnabled || d->isMoveOperation() && !d->moveEnabled)
        d->currentOperation = QMdiSubWindowPrivate::None;
    d->updateCursor();
}

/*!
    \reimp
*/
void QMdiSubWindow::keyPressEvent(QKeyEvent *keyEvent)
{
    Q_D(QMdiSubWindow);
    if (!d->isInInteractiveMode || !parent()) {
        keyEvent->ignore();
        return;
    }

    QPoint delta;
    switch (keyEvent->key()) {
    case Qt::Key_Right:
        if (keyEvent->modifiers() & Qt::ShiftModifier)
            delta = QPoint(d->keyboardPageStep, 0);
        else
            delta = QPoint(d->keyboardSingleStep, 0);
        break;
    case Qt::Key_Up:
        if (keyEvent->modifiers() & Qt::ShiftModifier)
            delta = QPoint(0, -d->keyboardPageStep);
        else
            delta = QPoint(0, -d->keyboardSingleStep);
        break;
    case Qt::Key_Left:
        if (keyEvent->modifiers() & Qt::ShiftModifier)
            delta = QPoint(-d->keyboardPageStep, 0);
        else
            delta = QPoint(-d->keyboardSingleStep, 0);
        break;
    case Qt::Key_Down:
        if (keyEvent->modifiers() & Qt::ShiftModifier)
            delta = QPoint(0, d->keyboardPageStep);
        else
            delta = QPoint(0, d->keyboardSingleStep);
        break;
    case Qt::Key_Escape:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        d->leaveInteractiveMode();
        return;
    default:
        keyEvent->ignore();
        return;
    }

#ifndef QT_NO_CURSOR
    QPoint newPosition = parentWidget()->mapFromGlobal(cursor().pos() + delta);
    QRect oldGeometry =
#ifndef QT_NO_RUBBERBAND
        d->isInRubberBandMode ? d->rubberBand->geometry() :
#endif
        geometry();
    d->setNewGeometry(newPosition);
    QRect currentGeometry =
#ifndef QT_NO_RUBBERBAND
        d->isInRubberBandMode ? d->rubberBand->geometry() :
#endif
        geometry();
    if (currentGeometry == oldGeometry)
        return;

    // Update cursor position

    QPoint actualDelta;
    if (d->isMoveOperation()) {
        actualDelta = QPoint(currentGeometry.x() - oldGeometry.x(),
                             currentGeometry.y() - oldGeometry.y());
    } else {
        int dx = isLeftToRight() ? currentGeometry.width() - oldGeometry.width()
                                 : currentGeometry.x() - oldGeometry.x();
        actualDelta = QPoint(dx, currentGeometry.height() - oldGeometry.height());
    }

    // Adjust in case we weren't able to move as long as wanted.
    if (actualDelta != delta)
        newPosition += (actualDelta - delta);
    cursor().setPos(parentWidget()->mapToGlobal(newPosition));
#endif
}

#ifndef QT_NO_MENU
/*!
    \reimp
*/
void QMdiSubWindow::contextMenuEvent(QContextMenuEvent *contextMenuEvent)
{
    Q_D(QMdiSubWindow);
    if (!d->systemMenu) {
        contextMenuEvent->ignore();
        return;
    }

    if (d->hoveredSubControl == QStyle::SC_TitleBarSysMenu
            || d->getRegion(QMdiSubWindowPrivate::Move).contains(contextMenuEvent->pos())) {
        d->systemMenu->exec(contextMenuEvent->globalPos());
    } else {
        contextMenuEvent->ignore();
    }
}
#endif // QT_NO_MENU

/*!
    \reimp
*/
void QMdiSubWindow::focusInEvent(QFocusEvent *focusInEvent)
{
    d_func()->focusInReason = focusInEvent->reason();
}

/*!
    \reimp
*/
void QMdiSubWindow::focusOutEvent(QFocusEvent * /*focusOutEvent*/)
{
    // To avoid update() in QWidget::focusOutEvent.
}

/*!
    \reimp
*/
void QMdiSubWindow::childEvent(QChildEvent *childEvent)
{
    if (childEvent->type() != QEvent::ChildPolished)
        return;
#ifndef QT_NO_SIZEGRIP
    if (QSizeGrip *sizeGrip = qobject_cast<QSizeGrip *>(childEvent->child()))
        d_func()->setSizeGrip(sizeGrip);
#endif
}

/*!
    \reimp
*/
QSize QMdiSubWindow::sizeHint() const
{
    Q_D(const QMdiSubWindow);
    int margin, minWidth;
    d->sizeParameters(&margin, &minWidth);
    QSize size(2 * margin, d->titleBarHeight() + margin);
    if (d->baseWidget && d->baseWidget->sizeHint().isValid())
        size += d->baseWidget->sizeHint();
    return size.expandedTo(minimumSizeHint());
}

/*!
    \reimp
*/
QSize QMdiSubWindow::minimumSizeHint() const
{
    Q_D(const QMdiSubWindow);
    if (isVisible())
        ensurePolished();

    // Minimized window.
    if (parent() && isMinimized() && !isShaded())
        return d->iconSize();

    // Calculate window decoration.
    int margin, minWidth;
    d->sizeParameters(&margin, &minWidth);
    int decorationHeight = margin + d->titleBarHeight();
    int minHeight = decorationHeight;

    // Shaded window.
    if (parent() && isShaded())
        return QSize(qMax(minWidth, width()), d->titleBarHeight());

    // Content
    if (layout()) {
        QSize minLayoutSize = layout()->minimumSize();
        if (minLayoutSize.isValid()) {
            minWidth = qMax(minWidth, minLayoutSize.width() + 2 * margin);
            minHeight += minLayoutSize.height();
        }
    } else if (d->baseWidget && d->baseWidget->isVisible()) {
        QSize minBaseWidgetSize = d->baseWidget->minimumSizeHint();
        if (minBaseWidgetSize.isValid()) {
            minWidth = qMax(minWidth, minBaseWidgetSize.width() + 2 * margin);
            minHeight += minBaseWidgetSize.height();
        }
    }

#ifndef QT_NO_SIZEGRIP
    // SizeGrip
    int sizeGripHeight = 0;
    if (d->sizeGrip && d->sizeGrip->isVisibleTo(const_cast<QMdiSubWindow *>(this)))
        sizeGripHeight = d->sizeGrip->height();
#if defined(Q_WS_MAC) && !defined(QT_NO_STYLE_MAC)
    else if (parent() && qobject_cast<QMacStyle *>(style()) && !d->sizeGrip)
        sizeGripHeight = style()->pixelMetric(QStyle::PM_SizeGripSize, 0, this);
#endif
    minHeight = qMax(minHeight, decorationHeight + sizeGripHeight);
#endif

    return QSize(minWidth, minHeight).expandedTo(QApplication::globalStrut());
}

#include "moc_qmdisubwindow.cpp"
#include "qmdisubwindow.moc"

#endif //QT_NO_MDIAREA
