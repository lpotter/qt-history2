/****************************************************************************
**
** Implementation of QWidget and QWindow classes for Win32.
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
** EDITIONS: PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qapplication.h"
#include "qapplication_p.h"
#include "qpainter.h"
#include "qbitmap.h"
#include "qimage.h"
#include "qlayout.h"
#include "qt_windows.h"
#include "qpaintdevicemetrics.h"
#include "qcursor.h"
#include <private/qapplication_p.h>
#include <private/qinputcontext_p.h>
#include "qevent.h"
#include "qstack.h"
#include "qwidget.h"
#include "qwidget_p.h"
#include "qlibrary.h"
#include "qdesktopwidget.h"
#include "qpaintengine_win.h"
#include "qcleanuphandler.h"

#if defined(QT_TABLET_SUPPORT)
#define PACKETDATA  (PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE | \
                      PK_ORIENTATION | PK_CURSOR)
#define PACKETMODE  0
#include <wintab.h>
#include <pktdef.h>

typedef HCTX        (API *PtrWTOpen)(HWND, LPLOGCONTEXT, BOOL);
typedef BOOL        (API *PtrWTClose)(HCTX);
typedef UINT        (API *PtrWTInfo)(UINT, UINT, LPVOID);
typedef BOOL        (API *PtrWTEnable)(HCTX, BOOL);
typedef BOOL        (API *PtrWTOverlap)(HCTX, BOOL);
typedef int        (API *PtrWTPacketsGet)(HCTX, int, LPVOID);
typedef BOOL        (API *PtrWTGet)(HCTX, LPLOGCONTEXT);
typedef int     (API *PtrWTQueueSizeGet)(HCTX);
typedef BOOL    (API *PtrWTQueueSizeSet)(HCTX, int);

static PtrWTOpen ptrWTOpen = 0;
static PtrWTClose ptrWTClose = 0;
static PtrWTInfo ptrWTInfo = 0;
static PtrWTQueueSizeGet ptrWTQueueSizeGet = 0;
static PtrWTQueueSizeSet ptrWTQueueSizeSet = 0;
static void init_wintab_functions();
static void qt_tablet_init();
static void qt_tablet_cleanup();
extern HCTX qt_tablet_context;
extern bool qt_tablet_tilt_support;

static QWidget *qt_tablet_widget = 0;
#endif

typedef BOOL    (WINAPI *PtrSetLayeredWindowAttributes)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
static PtrSetLayeredWindowAttributes ptrSetLayeredWindowAttributes = 0;
#define Q_WS_EX_LAYERED           0x00080000 // copied from WS_EX_LAYERED in winuser.h
#define Q_LWA_ALPHA               0x00000002 // copied from LWA_ALPHA in winuser.h

#ifdef Q_OS_TEMP
#include "sip.h"
#endif

#if defined(QT_NON_COMMERCIAL)
#include "qnc_win.h"
#endif

#include "qwidget_p.h"

#if !defined(WS_EX_TOOLWINDOW)
#define WS_EX_TOOLWINDOW 0x00000080
#endif

#if !defined(GWLP_WNDPROC)
#define GWLP_WNDPROC GWL_WNDPROC
#endif

const QString qt_reg_winclass(Qt::WFlags flags);                // defined in qapplication_win.cpp
void            qt_olednd_unregister(QWidget* widget, QOleDropTarget *dst); // dnd_win
QOleDropTarget* qt_olednd_register(QWidget* widget);


extern bool qt_nograb();
extern HRGN qt_win_bitmapToRegion(const QBitmap& bitmap);

static QWidget *mouseGrb    = 0;
static QCursor *mouseGrbCur = 0;
static QWidget *keyboardGrb = 0;
static HHOOK   journalRec  = 0;

extern "C" LRESULT CALLBACK QtWndProc(HWND, UINT, WPARAM, LPARAM);

extern void qt_set_paintevent_clipping(QPaintDevice* dev, const QRegion& region);
extern void qt_clear_paintevent_clipping();

#define d d_func()
#define q q_func()

#define XCOORD_MAX 32767
#define WRECT_MAX 16383

/*****************************************************************************
  QWidget member functions
 *****************************************************************************/


void QWidget::create(WId window, bool initializeWindow, bool destroyOldWindow)
{
    if (testWState(WState_Created) && window == 0)
        return;
    setWState(WState_Created);                        // set created flag

    if (!parentWidget() || parentWidget()->isDesktop())
        setWFlags(WType_TopLevel);                // top-level widget

    static int sw = -1, sh = -1;

    bool topLevel = testWFlags(WType_TopLevel);
    bool popup = testWFlags(WType_Popup);
    bool dialog = testWFlags(WType_Dialog);
    bool desktop  = testWFlags(WType_Desktop);
    HINSTANCE appinst  = qWinAppInst();
    HWND   parentw, destroyw = 0;
    WId           id;

    QString windowClassName = qt_reg_winclass(getWFlags());

    if (!window)                                // always initialize
        initializeWindow = true;

    if (popup)
        setWFlags(WStyle_StaysOnTop); // a popup stays on top

    if (sw < 0) {                                // get the (primary) screen size
        sw = GetSystemMetrics(SM_CXSCREEN);
        sh = GetSystemMetrics(SM_CYSCREEN);
    }

    if (dialog || popup || desktop) {                // these are top-level, too
        topLevel = true;
        setWFlags(WType_TopLevel);
    }

    if (desktop) {                                // desktop widget
        popup = false;                                // force this flags off
#ifndef Q_OS_TEMP
        if (QSysInfo::WindowsVersion != QSysInfo::WV_NT && QSysInfo::WindowsVersion != QSysInfo::WV_95)
            data->crect.setRect(GetSystemMetrics(76 /* SM_XVIRTUALSCREEN  */), GetSystemMetrics(77 /* SM_YVIRTUALSCREEN  */),
                           GetSystemMetrics(78 /* SM_CXVIRTUALSCREEN */), GetSystemMetrics(79 /* SM_CYVIRTUALSCREEN */));
        else
#endif
            data->crect.setRect(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    }

    parentw = parentWidget() ? parentWidget()->winId() : 0;

#ifdef UNICODE
    QString title;
    const TCHAR *ttitle = 0;
#endif
    const char *title95 = 0;
    int         style = WS_CHILD;
    int         exsty = WS_EX_NOPARENTNOTIFY;

    if (window) {
        style = GetWindowLongA(window, GWL_STYLE);
#ifndef QT_NO_DEBUG
        if (!style)
            qSystemWarning("QWidget: GetWindowLong failed");
#endif
        topLevel = false; // #### needed for some IE plugins??
    } else if (popup) {
        style = WS_POPUP;
    } else if (!topLevel) {
        if (!testWFlags(WStyle_Customize))
            setWFlags(WStyle_NormalBorder | WStyle_Title | WStyle_MinMax | WStyle_SysMenu );
    } else if (!desktop) {
        if (testWFlags(WStyle_Customize)) {
            if (testWFlags(WStyle_NormalBorder|WStyle_DialogBorder) == 0) {
                style = WS_POPUP;                // no border
            } else {
                style = 0;
            }
        } else {
            style = WS_OVERLAPPED;
            if (testWFlags(WType_Dialog))
#ifndef Q_OS_TEMP
                setWFlags(WStyle_NormalBorder | WStyle_Title | WStyle_SysMenu | WStyle_ContextHelp);
            else
                setWFlags(WStyle_NormalBorder | WStyle_Title | WStyle_MinMax | WStyle_SysMenu );
        }
#else
                setWFlags(WStyle_NormalBorder | WStyle_Title | WStyle_SysMenu);
            else
                setWFlags(WStyle_NormalBorder | WStyle_Title);
        }
#endif
        // workaround for some versions of Windows
        if (testWFlags(WStyle_MinMax))
            clearWFlags(WStyle_ContextHelp);
    }
    if (!desktop) {
        // if (!testAttribute(WA_PaintUnclipped))
        // ### Commented out for now as it causes some problems, but
        // this should be correct anyway, so dig some more into this
            style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN ;
        if (topLevel) {
#ifndef Q_OS_TEMP
            if (testWFlags(WStyle_NormalBorder)) {
                style |= WS_THICKFRAME;
                if(!testWFlags(WStyle_Title | WStyle_SysMenu | WStyle_Minimize | WStyle_Maximize | WStyle_ContextHelp))
                    style |= WS_POPUP;
            } else if (testWFlags(WStyle_DialogBorder))
                style |= WS_POPUP | WS_DLGFRAME;
#else
            if (testWFlags(WStyle_DialogBorder))
                style |= WS_POPUP;
#endif
            if (testWFlags(WStyle_Title))
                style |= WS_CAPTION;
            if (testWFlags(WStyle_SysMenu))
                style |= WS_SYSMENU;
            if (testWFlags(WStyle_Minimize))
                style |= WS_MINIMIZEBOX;
            if (testWFlags(WStyle_Maximize))
                style |= WS_MAXIMIZEBOX;
            if (testWFlags(WStyle_Tool) | testWFlags(WType_Popup))
                exsty |= WS_EX_TOOLWINDOW;
            if (testWFlags(WStyle_ContextHelp))
                exsty |= WS_EX_CONTEXTHELP;
        }
    }
    if (testWFlags(WStyle_Title)) {
        QT_WA({
            title = isTopLevel() ? QString::fromLocal8Bit(qAppName()) : objectName();
            ttitle = (TCHAR*)title.utf16();
        } , {
            title95 = isTopLevel() ? qAppName() : objectName().latin1();
        });
    }

        // The WState_Created flag is checked by translateConfigEvent() in
        // qapplication_win.cpp. We switch it off temporarily to avoid move
        // and resize events during creation
    clearWState(WState_Created);

    if (window) {                                // override the old window
        if (destroyOldWindow)
            destroyw = data->winid;
        id = window;
        setWinId(window);
        LONG res = SetWindowLongA(window, GWL_STYLE, style);
#ifndef QT_NO_DEBUG
        if (!res)
            qSystemWarning("QWidget: Failed to set window style");
#endif
#ifdef _WIN64
        res = SetWindowLongPtrA( window, GWLP_WNDPROC, (LONG_PTR)QtWndProc );
#else
        res = SetWindowLongA( window, GWL_WNDPROC, (LONG)QtWndProc );
#endif
#ifndef QT_NO_DEBUG
        if (!res)
            qSystemWarning("QWidget: Failed to set window procedure");
#endif
    } else if (desktop) {                        // desktop widget
#ifndef Q_OS_TEMP
        id = GetDesktopWindow();
        QWidget *otherDesktop = find(id);        // is there another desktop?
        if (otherDesktop && otherDesktop->testWFlags(WPaintDesktop)) {
            otherDesktop->setWinId(0);        // remove id from widget mapper
            setWinId(id);                        // make sure otherDesktop is
            otherDesktop->setWinId(id);        //   found first
        } else {
            setWinId(id);
        }
#endif
    } else if (topLevel) {                        // create top-level widget
        if (popup)
            parentw = 0;

#ifdef Q_OS_TEMP

        const TCHAR *cname = windowClassName.utf16();

        id = CreateWindowEx(exsty, cname, ttitle, style, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parentw, 0, appinst, 0);
#else

        QT_WA({
            const TCHAR *cname = (TCHAR*)windowClassName.utf16();
            id = CreateWindowEx(exsty, cname, ttitle, style,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                parentw, 0, appinst, 0);
        } , {
            id = CreateWindowExA(exsty, windowClassName.latin1(), title95, style,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                parentw, 0, appinst, 0);
        });

#endif

#ifndef QT_NO_DEBUG
        if (id == NULL)
            qSystemWarning("QWidget: Failed to create window");
#endif
        setWinId(id);
        if (testWFlags(WStyle_StaysOnTop))
            SetWindowPos(id, HWND_TOPMOST, 0, 0, 100, 100, SWP_NOACTIVATE);
    } else {                                        // create child widget
        QT_WA({
            const TCHAR *cname = (TCHAR*)windowClassName.utf16();
            id = CreateWindowEx(exsty, cname, ttitle, style, 0, 0, 100, 30,
                            parentw, NULL, appinst, NULL);
        } , {
            id = CreateWindowExA(exsty, windowClassName.latin1(), title95, style, 0, 0, 100, 30,
                            parentw, NULL, appinst, NULL);
        });
#ifndef QT_NO_DEBUG
        if (id == NULL)
            qSystemWarning("QWidget: Failed to create window");
#endif
        SetWindowPos(id, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        setWinId(id);
    }

    if (desktop) {
        setWState(WState_Visible);
    } else {
        RECT  fr, cr;
        GetWindowRect(id, &fr);                // update rects
        GetClientRect(id, &cr);
        if (cr.top == cr.bottom && cr.left == cr.right) {
            if (initializeWindow) {
                int x, y, w, h;
                if (topLevel) {
                    x = sw/4;
                    y = 3*sh/10;
                    w = sw/2;
                    h = 4*sh/10;
                } else {
                    x = y = 0;
                    w = 100;
                    h = 30;
                }

                MoveWindow(winId(), x, y, w, h, true);
            }
            GetWindowRect(id, &fr);                // update rects
            GetClientRect(id, &cr);
        }
        if (topLevel){
            // one cannot trust cr.left and cr.top, use a correction POINT instead
            POINT pt;
            pt.x = 0;
            pt.y = 0;
            ClientToScreen(id, &pt);
            data->crect = QRect(QPoint(pt.x, pt.y),
                           QPoint(pt.x+cr.right, pt.y+cr.bottom));

            QTLWExtra *top = d->topData();
            top->ftop = data->crect.top() - fr.top;
            top->fleft = data->crect.left() - fr.left;
            top->fbottom = fr.bottom - data->crect.bottom();
            top->fright = fr.right - data->crect.right();
            data->fstrut_dirty = false;

            d->createTLExtra();
        } else {
            data->crect.setCoords(cr.left, cr.top, cr.right, cr.bottom);
            // in case extra data already exists (eg. reparent()).  Set it.
        }
    }

    setWState(WState_Created);                // accept move/resize events
    hdc = 0;                                        // no display context

    if (window) {                                // got window from outside
        if (IsWindowVisible(window))
            setWState(WState_Visible);
        else
            clearWState(WState_Visible);
    }

#if defined(QT_NON_COMMERCIAL)
    QT_NC_WIDGET_CREATE
#endif

    if (destroyw) {
        DestroyWindow(destroyw);
    }

    d->setFont_sys();
    QInputContext::enable(this, data->im_enabled & isEnabled());
}


void QWidget::destroy(bool destroyWindow, bool destroySubWindows)
{
    deactivateWidgetCleanup();
    if (testWState(WState_Created)) {
        clearWState(WState_Created);
        for(int i = 0; i < d->children.size(); ++i) { // destroy all widget children
            register QObject *obj = d->children.at(i);
            if (obj->isWidgetType())
                ((QWidget*)obj)->destroy(destroySubWindows,
                                         destroySubWindows);
        }
        if (mouseGrb == this)
            releaseMouse();
        if (keyboardGrb == this)
            releaseKeyboard();
        if (testWFlags(WShowModal))                // just be sure we leave modal
            qt_leave_modal(this);
        else if (testWFlags(WType_Popup))
            qApp->closePopup(this);
        if (destroyWindow && !testWFlags(WType_Desktop)) {
            DestroyWindow(winId());
        }
        setWinId(0);
    }
}


void QWidget::reparent_sys(QWidget *parent, WFlags f, const QPoint &p, bool showIt)
{
    QWidget* oldtlw = topLevelWidget();
    WId old_winid = data->winid;
    // hide and reparent our own window away. Otherwise we might get
    // destroyed when emitting the child remove event below. See QWorkspace.
    if (isVisible()) {
        ShowWindow(data->winid, SW_HIDE);
        SetParent(data->winid, 0);
    }

    bool accept_drops = acceptDrops();
    if (accept_drops)
        setAcceptDrops(false); // ole dnd unregister (we will register again below)
    if (testWFlags(WType_Desktop))
        old_winid = 0;
    setWinId(0);

    QObject::setParent_helper(parent);
    bool     enable = isEnabled();                // remember status
    FocusPolicy fp = focusPolicy();
    QSize    s            = size();
    QString capt = windowTitle();
    data->widget_flags = f;
    clearWState(WState_Created | WState_Visible | WState_Hidden | WState_ExplicitShowHide);
    create();
    if (isTopLevel() || (!parent || parent->isVisible()))
        setWState(WState_Hidden);
    QObjectList chlist = children();
    for(int i = 0; i < chlist.size(); ++i) { // reparent children
        QObject *obj = chlist.at(i);
        if (obj->isWidgetType()) {
            QWidget *w = (QWidget *)obj;
            if (w->isPopup()) {
                ;
            } else if (w->isTopLevel()) {
                bool showIt = w->isShown();
                QPoint old_pos = w->pos();
                w->setParent(this, w->getWFlags());
                w->move(old_pos);
                if (showIt)
                    w->show();
            } else {
                SetParent(w->winId(), winId());
            }
        }
    }

    setGeometry(p.x(), p.y(), s.width(), s.height());
    setEnabled(enable);
    setFocusPolicy(fp);
    if (!capt.isNull()) {
        d->extra->topextra->caption = QString::null;
        setWindowTitle(capt);
    }
    if (showIt)
        show();
    if (old_winid)
        DestroyWindow(old_winid);

    reparentFocusWidgets(oldtlw);                // fix focus chains

    if (accept_drops)
        setAcceptDrops(true);

#ifdef Q_OS_TEMP
    // Show borderless toplevel windows in tasklist & NavBar
    if (!parent) {
        QString txt = caption().isEmpty()?qAppName():caption();
        SetWindowText(winId(), (TCHAR*)txt.utf16());
    }
#endif
}


QPoint QWidget::mapToGlobal(const QPoint &pos) const
{
    if (!isVisible() || isMinimized())
        return mapTo(topLevelWidget(), pos) + topLevelWidget()->pos() +
        (topLevelWidget()->geometry().topLeft() - topLevelWidget()->frameGeometry().topLeft());
    POINT p;
    QPoint tmp = d->mapToWS(pos);
    p.x = tmp.x();
    p.y = tmp.y();
    ClientToScreen(winId(), &p);
    return QPoint(p.x, p.y);
}

QPoint QWidget::mapFromGlobal(const QPoint &pos) const
{
    if (!isVisible() || isMinimized())
        return mapFrom(topLevelWidget(), pos - topLevelWidget()->pos());
    POINT p;
    p.x = pos.x();
    p.y = pos.y();
    ScreenToClient(winId(), &p);
    return d->mapFromWS(QPoint(p.x, p.y));
}


void QWidgetPrivate::setFont_sys(QFont *f)
{
    QInputContext::setFont(q, (f ? *f : q->font()));
}

void QWidget::setMicroFocusHint(int x, int y, int width, int height, bool text, QFont *f)
{
    CreateCaret(winId(), 0, width, height);
    HideCaret(winId());
    SetCaretPos(x, y);

    if (text)
        QInputContext::setFocusHint(x, y, width, height, this);
    d->setFont_sys(f);

    if (QRect(x, y, width, height) != microFocusHint()) {
        if (d && d->extraData())
            d->extraData()->micro_focus_hint.setRect(x, y, width, height);
    }
}

void QWidget::resetInputContext()
{
    QInputContext::accept();
}

void QWidgetPrivate::updateSystemBackground() {}

extern void qt_set_cursor(QWidget *, const QCursor &); // qapplication_win.cpp

void QWidget::setCursor(const QCursor &cursor)
{
    if (cursor.shape() != Qt::ArrowCursor
        || (d->extra && d->extra->curs)) {
        d->createExtra();
        delete d->extra->curs;
        d->extra->curs = new QCursor(cursor);
    }
    setAttribute(WA_SetCursor);
    qt_set_cursor(this, QWidget::cursor());
}

void QWidget::unsetCursor()
{
    if (d->extra) {
        delete d->extra->curs;
        d->extra->curs = 0;
    }
    if (!isTopLevel())
        setAttribute(WA_SetCursor, false);
    qt_set_cursor(this, cursor());
}

void QWidget::setWindowModified(bool mod)
{
    setAttribute(WA_WindowModified, mod);
    {
        QString caption = QWidget::windowTitle();
#if defined(QT_NON_COMMERCIAL)
        QT_NC_CAPTION
#else
            QString cap = caption;
#endif
        if(mod)
            cap += " *";
        QT_WA({
            SetWindowText(winId(), (TCHAR*)cap.utf16());
        } , {
            SetWindowTextA(winId(), cap.local8Bit());
        });
    }
    QEvent e(QEvent::ModifiedChange);
    QApplication::sendEvent(this, &e);
}

bool QWidget::isWindowModified() const
{
    return testAttribute(WA_WindowModified);
}

void QWidget::setWindowTitle(const QString &caption)
{
    if (QWidget::windowTitle() == caption)
        return; // for less flicker
    d->topData()->caption = caption;

#if defined(QT_NON_COMMERCIAL)
    QT_NC_CAPTION
#else
    QString cap = caption;
#endif
    if(isWindowModified())
        cap += " *";
    QT_WA({
        SetWindowText(winId(), (TCHAR*)cap.utf16());
    } , {
        SetWindowTextA(winId(), cap.local8Bit());
    });

    QEvent e(QEvent::WindowTitleChange);
    QApplication::sendEvent(this, &e);
}

/*
  Create an icon mask the way Windows wants it using CreateBitmap.
*/

HBITMAP qt_createIconMask(const QBitmap &bitmap)
{
    QImage bm = bitmap.convertToImage();
    int w = bm.width();
    int h = bm.height();
    int bpl = ((w+15)/16)*2;                        // bpl, 16 bit alignment
    uchar *bits = new uchar[bpl*h];
    bm.invertPixels();
    for (int y=0; y<h; y++)
        memcpy(bits+y*bpl, bm.scanLine(y), bpl);
    HBITMAP hbm = CreateBitmap(w, h, 1, 1, bits);
    delete [] bits;
    return hbm;
}


void QWidget::setWindowIcon(const QPixmap &pixmap)
{
    QTLWExtra* x = d->topData();
    delete x->icon;
    x->icon = 0;
    if (x->winIcon) {
        DestroyIcon(x->winIcon);
        x->winIcon = 0;
    }
    if (!pixmap.isNull()) {                        // valid icon
        QPixmap pm(pixmap.size(), pixmap.depth(), QPixmap::NormalOptim);
        QBitmap mask(pixmap.size(), false, QPixmap::NormalOptim);
        if (pixmap.mask()) {
            pm.fill(black);                        // make masked area black
            bitBlt(&mask, 0, 0, pixmap.mask());
        } else {
            mask.fill(color1);
        }
        bitBlt(&pm, 0, 0, &pixmap);
        HBITMAP im = qt_createIconMask(mask);
        ICONINFO ii;
        ii.fIcon    = true;
        ii.hbmMask  = im;
        ii.hbmColor = pm.hbm();
        ii.xHotspot = 0;
        ii.yHotspot = 0;
        x->icon = new QPixmap(pixmap);
        x->winIcon = CreateIconIndirect(&ii);
        DeleteObject(im);
    }
    SendMessageA(winId(), WM_SETICON, 0, /* ICON_SMALL */
                  (long)x->winIcon);
    SendMessageA(winId(), WM_SETICON, 1, /* ICON_BIG */
                  (long)x->winIcon);

    QEvent e(QEvent::WindowIconChange);
    QApplication::sendEvent(this, &e);
}


void QWidget::setWindowIconText(const QString &iconText)
{
    d->topData()->iconText = iconText;
}


QCursor *qt_grab_cursor()
{
    return mouseGrbCur;
}

// The procedure does nothing, but is required for mousegrabbing to work
LRESULT CALLBACK qJournalRecordProc(int nCode, WPARAM wParam, LPARAM lParam)
{
#ifndef Q_OS_TEMP
    return CallNextHookEx(journalRec, nCode, wParam, lParam);
#else
    return 0;
#endif
}

void QWidget::grabMouse()
{
    if (!qt_nograb()) {
        if (mouseGrb)
            mouseGrb->releaseMouse();
#ifndef Q_OS_TEMP
        journalRec = SetWindowsHookExA(WH_JOURNALRECORD, (HOOKPROC)qJournalRecordProc, GetModuleHandleA(0), 0);
#endif
        SetCapture(winId());
        mouseGrb = this;
    }
}

void QWidget::grabMouse(const QCursor &cursor)
{
    if (!qt_nograb()) {
        if (mouseGrb)
            mouseGrb->releaseMouse();
#ifndef Q_OS_TEMP
        journalRec = SetWindowsHookExA(WH_JOURNALRECORD, (HOOKPROC)qJournalRecordProc, GetModuleHandleA(0), 0);
#endif
        SetCapture(winId());
        mouseGrbCur = new QCursor(cursor);
        SetCursor(mouseGrbCur->handle());
        mouseGrb = this;
    }
}

void QWidget::releaseMouse()
{
    if (!qt_nograb() && mouseGrb == this) {
        ReleaseCapture();
        if (journalRec) {
#ifndef Q_OS_TEMP
            UnhookWindowsHookEx(journalRec);
#endif
            journalRec = 0;
        }
        if (mouseGrbCur) {
            delete mouseGrbCur;
            mouseGrbCur = 0;
        }
        mouseGrb = 0;
    }
}

void QWidget::grabKeyboard()
{
    if (!qt_nograb()) {
        if (keyboardGrb)
            keyboardGrb->releaseKeyboard();
        keyboardGrb = this;
    }
}

void QWidget::releaseKeyboard()
{
    if (!qt_nograb() && keyboardGrb == this)
        keyboardGrb = 0;
}


QWidget *QWidget::mouseGrabber()
{
    return mouseGrb;
}

QWidget *QWidget::keyboardGrabber()
{
    return keyboardGrb;
}

void QWidget::setActiveWindow()
{
    SetForegroundWindow(topLevelWidget()->winId());
}


void QWidget::update()
{
    if ((data->widget_state & (WState_Visible|WState_BlockUpdates)) == WState_Visible) {
        InvalidateRect(winId(), 0, false);
        setAttribute(WA_PendingUpdate);
    }
}

void QWidget::update(const QRegion &rgn)
{
    if ((data->widget_state & (WState_Visible|WState_BlockUpdates)) == WState_Visible)
        if (!rgn.isEmpty()) {
            InvalidateRgn(winId(), rgn.handle(), false);
            setAttribute(WA_PendingUpdate);
        }
}

void QWidget::update(int x, int y, int w, int h)
{
    if (w && h &&
         (data->widget_state & (WState_Visible|WState_BlockUpdates)) == WState_Visible) {
        RECT r;
        r.left = x;
        r.top  = y;
        if (w < 0)
            r.right = data->crect.width();
        else
            r.right = x + w;
        if (h < 0)
            r.bottom = data->crect.height();
        else
            r.bottom = y + h;
        InvalidateRect(winId(), &r, false);
        setAttribute(WA_PendingUpdate);
    }
}

struct QWinDoubleBuffer
{
    enum {
        MaxWidth = SHRT_MAX,
        MaxHeight = SHRT_MAX
    };

    HDC hdc;
    HBITMAP hbm;
    int width;
    int height;
};

static QWinDoubleBuffer *global_double_buffer = 0;

void qt_discard_double_buffer()
{
    if (!global_double_buffer)
        return;

    DeleteObject(global_double_buffer->hbm);
    DeleteDC(global_double_buffer->hdc);

    delete global_double_buffer;
    global_double_buffer = 0;
}

static void qt_win_get_double_buffer(HDC *hdc, int width, int height)
{
    // the db should consist of 128x128 chunks
    width  = qMin(((width / 128) + 1) * 128, (int)QWinDoubleBuffer::MaxWidth);
    height = qMin(((height / 128) + 1) * 128, (int)QWinDoubleBuffer::MaxHeight);

    if (global_double_buffer) {
        if (global_double_buffer->width >= width
            && global_double_buffer->height >= height) {
            *hdc = global_double_buffer->hdc;
            SelectClipRgn(*hdc, 0);
            return;
        }

        width  = qMax(global_double_buffer->width,  width);
        height = qMax(global_double_buffer->height, height);

        qt_discard_double_buffer();
    }

    global_double_buffer = new QWinDoubleBuffer;
    global_double_buffer->hdc = CreateCompatibleDC(qt_display_dc());
    global_double_buffer->hbm = CreateCompatibleBitmap(qt_display_dc(), width, height);
    global_double_buffer->width = width;
    global_double_buffer->height = height;
    Q_ASSERT(global_double_buffer->hdc);
    Q_ASSERT(global_double_buffer->hbm);
    bool success = SelectObject(global_double_buffer->hdc, global_double_buffer->hbm);
    Q_ASSERT(success);
    *hdc = global_double_buffer->hdc;
    SelectClipRgn(*hdc, 0);
};

extern void qt_erase_background(HDC, int, int, int, int, const QBrush &, int, int, QWidget *);

void QWidget::repaint(const QRegion& rgn)
{
    setAttribute(WA_PendingUpdate, false);
    if (testWState(WState_InPaintEvent))
        qWarning("QWidget::repaint: recursive repaint detected.");

    if ((data->widget_state & (WState_Visible|WState_BlockUpdates)) != WState_Visible
        || !testAttribute(WA_Mapped)
        || rgn.isEmpty())
        return;

    ValidateRgn(winId(),rgn.handle());

    setWState(WState_InPaintEvent);

    QRect br = rgn.boundingRect();
    QRect brWS = d->mapToWS(br);
    bool do_clipping = (br != QRect(0, 0, data->crect.width(), data->crect.height()));
    bool double_buffer = (!testAttribute(WA_PaintOnScreen)
                          && br.width()  <= QWinDoubleBuffer::MaxWidth
                          && br.height() <= QWinDoubleBuffer::MaxHeight);
    bool tmphdc = !hdc;
    if (tmphdc)
        hdc = GetDC(winId());
    HDC old_dc = hdc;

    QPoint redirectionOffset;

    if (double_buffer) {
        qt_win_get_double_buffer(&hdc, br.width(), br.height());
        redirectionOffset = br.topLeft();
    } else {
        redirectionOffset = data->wrect.topLeft();
    }

    if (!redirectionOffset.isNull())
        QPainter::setRedirected(this, this, redirectionOffset);

    if (do_clipping) {
        if (redirectionOffset.isNull()) {
            qt_set_paintevent_clipping(this, rgn);
        } else {
            QRegion redirectedRegion(rgn);
            redirectedRegion.translate(-redirectionOffset);
            qt_set_paintevent_clipping(this, redirectedRegion);
        }
    }

    if (testAttribute(WA_NoSystemBackground)) {
        if (double_buffer && !testAttribute(WA_NoBackground)) {
            BitBlt(hdc, 0, 0, brWS.width(), brWS.height(),
                   old_dc, brWS.x(), brWS.y(), SRCCOPY);
        }
    } else if (!testAttribute(WA_NoBackground)) {
        QPoint offset;
        QStack<QWidget*> parents;
        QWidget *w = q;
        while (w->d->isBackgroundInherited()) {
            offset += w->pos();
            w = w->parentWidget();
            parents += w;
        }

        if (double_buffer) {
            qt_erase_background(q->hdc, br.x()-redirectionOffset.x(), br.y()-redirectionOffset.y(),
                                br.width(), br.height(),
                                palette().brush(w->d->bg_role),
                                br.x() + offset.x(), br.y() + offset.y(),
                                this);
        } else {
            QRegion mappedRegion(rgn);
            mappedRegion.translate(-data->wrect.topLeft());
            SelectClipRgn(hdc, mappedRegion.handle());
            QSize bgsize = data->wrect.isValid() ? data->wrect.size() : data->crect.size();
            // ### This triggers bitblt on entire area. Potentially a lot. Clip here too!
            qt_erase_background(hdc, 0, 0, bgsize.width(), bgsize.height(),
                                palette().brush(w->d->bg_role), offset.x(), offset.y(),
                                this);
        }

        if (parents.size()) {
            w = parents.pop();
            for (;;) {
                if (w->testAttribute(Qt::WA_ContentsPropagated)) {
                    QPainter::setRedirected(w, q, offset);
                    QRect rr = d->clipRect();
                    rr.moveBy(offset);
                    QPaintEvent e(rr);
                    bool was_in_paint_event = w->testWState(WState_InPaintEvent);
                    w->setWState(WState_InPaintEvent);
                    QApplication::sendEvent(w, &e);
                    if(!was_in_paint_event) {
                        w->clearWState(WState_InPaintEvent);
                        if(w->paintingActive())
                            qWarning("It is dangerous to leave painters active on a widget outside of the PaintEvent");
                    }
                    QPainter::restoreRedirected(w);
                }
                if (parents.size() == 0)
                    break;
                w = parents.pop();
                offset -= w->pos();
            }
        }
        SelectClipRgn(hdc, 0);
    }

    QPaintEvent e(rgn);
    QApplication::sendSpontaneousEvent(this, &e);

    if (do_clipping)
        qt_clear_paintevent_clipping();

    if (!redirectionOffset.isNull())
        QPainter::restoreRedirected(this);



    if (double_buffer) {
        QVector<QRect> rects = rgn.rects();
        for (int i=0; i<rects.size(); ++i) {
            QRect &rr = d->mapToWS(rects.at(i));
            BitBlt(old_dc,
                   rr.x(), rr.y(),
                   rr.width(), rr.height(),
                   hdc,
                   rr.x()-brWS.x(), rr.y()-brWS.y(),
                   SRCCOPY);
        }

        hdc = old_dc;

        if (!qApp->active_window) {
            extern int qt_double_buffer_timer;
            if (qt_double_buffer_timer)
                qApp->killTimer(qt_double_buffer_timer);
            qt_double_buffer_timer = qApp->startTimer(500);
        }
    }

    if (tmphdc) {
        ReleaseDC(winId(), hdc);
        hdc = 0;
    }

    clearWState(WState_InPaintEvent);
    if(paintingActive())
        qWarning("It is dangerous to leave painters active on a widget outside of the PaintEvent");

    if (testAttribute(WA_ContentsPropagated))
        d->updatePropagatedBackground(&rgn);
}


void QWidget::setWindowState(uint newstate)
{
    uint oldstate = windowState();

    int max = SW_MAXIMIZE;
    int min = SW_MINIMIZE;
    int normal = SW_SHOWNOACTIVATE;
    if (newstate & WindowActive) {
        max = SW_SHOWMAXIMIZED;
        min = SW_SHOWMINIMIZED;
        normal = SW_SHOWNORMAL;
    }

    if (isTopLevel()) {
        if ((oldstate & WindowMaximized) != (newstate & WindowMaximized)) {
            if (isVisible() && !(newstate & WindowMinimized)) {
                ShowWindow(winId(), (newstate & WindowMaximized) ? max : normal);
                QRect r = d->topData()->normalGeometry;
                if (!(newstate & WindowMaximized) && r.width() >= 0) {
                    if (pos() != r.topLeft() || size() !=r.size()) {
                        d->topData()->normalGeometry = QRect(0,0,-1,-1);
                        r.addCoords(d->topData()->fleft, d->topData()->ftop, d->topData()->fleft, d->topData()->ftop);
                        setGeometry(r);
                    }
                }
            }
        }

        if ((oldstate & WindowFullScreen) != (newstate & WindowFullScreen)) {
            if (newstate & WindowFullScreen) {
                if (d->topData()->normalGeometry.width() < 0 && !(oldstate & WindowMaximized))
                    d->topData()->normalGeometry = QRect(pos(), size());
                d->topData()->savedFlags = GetWindowLongA(winId(), GWL_STYLE);
                UINT style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_POPUP;
                if (isVisible())
                    style |= WS_VISIBLE;
                SetWindowLongA(winId(), GWL_STYLE, style);
                QRect r = qApp->desktop()->screenGeometry(this);
                UINT swpf = SWP_FRAMECHANGED;
                if (newstate & WindowActive)
                    swpf |= SWP_NOACTIVATE;

                SetWindowPos(winId(), HWND_TOP, r.left(), r.top(), r.width(), r.height(), swpf);
                updateFrameStrut();
            } else {
                UINT style = d->topData()->savedFlags;
                if (isVisible())
                    style |= WS_VISIBLE;
                SetWindowLongA(winId(), GWL_STYLE, style);
                UINT swpf = SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE;
                if (newstate & WindowActive)
                    swpf |= SWP_NOACTIVATE;
                SetWindowPos(winId(), 0, 0, 0, 0, 0, swpf);
                updateFrameStrut();

                // the widget is still maximized
                if (newstate & WindowMaximized) {
                    if (isVisible())
                        ShowWindow(winId(), max);
                } else {
                    QRect r = d->topData()->normalGeometry;
                    d->topData()->normalGeometry = QRect(0,0,-1,-1);
                    r.addCoords(d->topData()->fleft, d->topData()->ftop, d->topData()->fleft, d->topData()->ftop);
                    setGeometry(r);
                }
            }
        }

        if ((oldstate & WindowMinimized) != (newstate & WindowMinimized)) {
            if (isVisible())
                ShowWindow(winId(), (newstate & WindowMinimized) ? min :
                                    (newstate & WindowMaximized) ? max : normal);
        }
    }

    data->widget_state &= ~(WState_Minimized | WState_Maximized | WState_FullScreen);
    if (newstate & WindowMinimized)
        data->widget_state |= WState_Minimized;
    if (newstate & WindowMaximized)
        data->widget_state |= WState_Maximized;
    if (newstate & WindowFullScreen)
        data->widget_state |= WState_FullScreen;

    QEvent e(QEvent::WindowStateChange);
    QApplication::sendEvent(this, &e);
}


/*
  \internal
  Platform-specific part of QWidget::hide().
*/

void QWidget::hide_sys()
{
    deactivateWidgetCleanup();
    ShowWindow(winId(), SW_HIDE);
}


#ifndef Q_OS_TEMP // ------------------------------------------------

/*
  \internal
  Platform-specific part of QWidget::show().
*/
void QWidget::show_sys()
{
#if defined(QT_NON_COMMERCIAL)
    QT_NC_SHOW_WINDOW
#endif
    if (testAttribute(WA_OutsideWSRange))
        return;
    setAttribute(WA_Mapped);

    int sm = SW_SHOWNORMAL;
    if (isTopLevel()) {
        if (testWState(WState_Minimized))
            sm = SW_SHOWMINIMIZED;
        else if (testWState(WState_Maximized))
            sm = SW_SHOWMAXIMIZED;
    }
    if (testWFlags(WStyle_Tool) || isPopup())
        sm = SW_SHOWNOACTIVATE;

    ShowWindow( winId(), sm );
    if (IsIconic(winId()))
        setWState(WState_Minimized);
    if (IsZoomed(winId()))
        setWState(WState_Maximized);

    UpdateWindow(winId());
}

#else // Q_OS_TEMP --------------------------------------------------
# if defined(WIN32_PLATFORM_PSPC) && (WIN32_PLATFORM_PSPC < 310)
#  define SHFS_SHOWTASKBAR            0x0001
#  define SHFS_SHOWSIPBUTTON          0x0004
   extern "C" BOOL __stdcall SHFullScreen(HWND hwndRequester, DWORD dwState);
# else
#  include <aygshell.h>
# endif

void QWidget::show_sys()
{
    if (testAttribute(WA_OutsideWSRange))
        return;
    setAttribute(WA_Mapped);
    uint sm = SW_SHOW;
    if (isTopLevel()) {
        switch (d->topData()->showMode) {
        case 1:
            sm = SW_HIDE;
            break;
        case 2:
            {
                int scrnum = qApp->desktop()->screenNumber(this);
                setGeometry(qApp->desktop()->availableGeometry(scrnum));
            }
            // Fall-through
        default:
            sm = SW_SHOW;
            break;
        }
        d->topData()->showMode = 0; // reset
    }

    if (testWFlags(WStyle_Tool) || isPopup())
        sm = SW_SHOWNOACTIVATE;

    ShowWindow(winId(), sm);
    if (isTopLevel() && sm == SW_SHOW)
        SetForegroundWindow(winId());
    UpdateWindow(winId());
}


#if 0
void QWidget::showMinimized()
{
    if (isTopLevel()) {
        if (d->topData()->fullscreen) {
            reparent(0, d->topData()->savedFlags, d->topData()->normalGeometry.topLeft());
            d->topData()->fullscreen = 0;
            SHFullScreen(winId(), SHFS_SHOWTASKBAR | SHFS_SHOWSIPBUTTON);
        }
        d->topData()->showMode = 1;
    }

    if (isVisible())
        show_sys();
    else
        show();

    QEvent e(QEvent::ShowMinimized);
    QApplication::sendEvent(this, &e);
    clearWState(WState_Maximized);
    setWState(WState_Minimized);
}


void QWidget::showMaximized()
{
    if (isTopLevel()) {
        if (d->topData()->normalGeometry.width() < 0) {
            d->topData()->savedFlags = getWFlags();
            d->topData()->normalGeometry = geometry();
            reparent(0,
                      WType_TopLevel | WStyle_Customize | WStyle_NoBorder |
                      (getWFlags() & 0xffff0000), // preserve some widget flags
                      d->topData()->normalGeometry.topLeft());
            d->topData()->fullscreen = 0;
            SHFullScreen(winId(), SHFS_SHOWTASKBAR | SHFS_SHOWSIPBUTTON);
        }
        d->topData()->showMode = 2;
    }

    if (isVisible())
        show_sys();
    else
        show();

    QEvent e(QEvent::ShowMaximized);
    QApplication::sendEvent(this, &e);
    clearWState(WState_Minimized);
    setWState(WState_Maximized);
}


void QWidget::showNormal()
{
    d->topData()->showMode = 0;
    if (isTopLevel()) {
        if (d->topData()->normalGeometry.width() > 0) {
            int val = d->topData()->savedFlags;
            int style = WS_OVERLAPPED,
                exsty = 0;

            style |= (val & WStyle_DialogBorder ? WS_POPUP : 0);
            style |= (val & WStyle_Title        ? WS_CAPTION : 0);
            style |= (val & WStyle_SysMenu        ? WS_SYSMENU : 0);
            style |= (val & WStyle_Minimize        ? WS_MINIMIZEBOX : 0);
            style |= (val & WStyle_Maximize        ? WS_MAXIMIZEBOX : 0);
            exsty |= (val & WStyle_Tool                ? WS_EX_TOOLWINDOW : 0);
            exsty |= (val & WType_Popup                ? WS_EX_TOOLWINDOW : 0);
            exsty |= (val & WStyle_ContextHelp        ? WS_EX_CONTEXTHELP : 0);

            SetWindowLong(winId(), GWL_STYLE, style);
            if (exsty)
                SetWindowLong(winId(), GWL_EXSTYLE, exsty);
            // flush Window style cache
            SHFullScreen(winId(), SHFS_SHOWTASKBAR | SHFS_SHOWSIPBUTTON);
            SetWindowPos(winId(), HWND_TOP, 0, 0, 200, 200, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

            d->topData()->fullscreen = 0;
            QRect r = d->topData()->normalGeometry;
            if (r.width() >= 0) {
                // the widget has been maximized
                d->topData()->normalGeometry = QRect(0,0,-1,-1);
                resize(r.size());
                move(r.topLeft());
            }
        }
    }
    show();
    ShowWindow(winId(), SW_SHOWNORMAL);

    if (d->extra && d->extra->topextra)
        d->extra->topextra->fullscreen = 0;
    QEvent e(QEvent::ShowNormal);
    QApplication::sendEvent(this, &e);
    clearWState(WState_Maximized | WState_Minimized);
}
#endif // 0

#endif // Q_OS_TEMP -------------------------------------------------

void QWidget::raise()
{
    QWidget *p = parentWidget();
    int from;
    if (p && (from = p->d->children.indexOf(this)) >= 0)
        p->d->children.move(from, p->d->children.size() - 1);
    uint f = (isPopup() || testWFlags(WStyle_Tool)) ? SWP_NOACTIVATE : 0;
    SetWindowPos(winId(), HWND_TOP, 0, 0, 0, 0, f | SWP_NOMOVE | SWP_NOSIZE);
}

void QWidget::lower()
{
    QWidget *p = parentWidget();
    int from;
    if (p && (from = p->d->children.indexOf(this)) >= 0)
        p->d->children.move(from, 0);
    uint f = (isPopup() || testWFlags(WStyle_Tool)) ? SWP_NOACTIVATE : 0;
    SetWindowPos(winId(), HWND_BOTTOM, 0, 0, 0, 0, f | SWP_NOMOVE |
                  SWP_NOSIZE);
}

void QWidget::stackUnder(QWidget* w)
{
    QWidget *p = parentWidget();
    if (!w || isTopLevel() || p != w->parentWidget() || this == w)
        return;

    int from;
    int to;
    if (p && (to = p->d->children.indexOf(w)) >= 0 && (from = p->d->children.indexOf(this)) >= 0) {
        if (from < to)
            --to;
        p->d->children.move(from, to);
    }
    SetWindowPos(winId(), w->winId() , 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}


/*
  Helper function for non-toplevel widgets. Helps to map Qt's 32bit
  coordinate system to Windpws's 16bit coordinate system.

  This code is duplicated from the X11 code, so any changes there
  should also (most likely) be reflected here.

  (In all comments below: s/X/Windows/g)
 */

void QWidgetPrivate::setWSGeometry()
{

    /*
      There are up to four different coordinate systems here:
      Qt coordinate system for this widget.
      X coordinate system for this widget (relative to wrect).
      Qt coordinate system for parent
      X coordinate system for parent (relative to parent's wrect).
     */
//     Display *dpy = xinfo->display();
    QRect validRange(-XCOORD_MAX,-XCOORD_MAX, 2*XCOORD_MAX, 2*XCOORD_MAX);
    QRect wrectRange(-WRECT_MAX,-WRECT_MAX, 2*WRECT_MAX, 2*WRECT_MAX);
    QRect wrect;
    //xrect is the X geometry of my X widget. (starts out in  parent's Qt coord sys, and ends up in parent's X coord sys)
    QRect xrect = data.crect;

    QRect parentWRect = q->parentWidget()->data->wrect;

    if (parentWRect.isValid()) {
        // parent is clipped, and we have to clip to the same limit as parent
        if (!parentWRect.contains(xrect)) {
            xrect &= parentWRect;
            wrect = xrect;
            //translate from parent's to my Qt coord sys
            wrect.moveBy(-data.crect.topLeft());
        }
        //translate from parent's Qt coords to parent's X coords
        xrect.moveBy(-parentWRect.topLeft());

    } else {
        // parent is not clipped, we may or may not have to clip

        if (data.wrect.isValid()) {
            // This is where the main optimization is: we are already
            // clipped, and if our clip is still valid, we can just
            // move our window, and do not need to move or clip
            // children

            QRect vrect = xrect & q->parentWidget()->rect();
            vrect.moveBy(-data.crect.topLeft()); //the part of me that's visible through parent, in my Qt coords
            if (data.wrect.contains(vrect)) {
                xrect = data.wrect;
                xrect.moveBy(data.crect.topLeft());
                MoveWindow(q->winId(), xrect.x(), xrect.y(), xrect.width(), xrect.height(), true);
                return;
            }
        }

        if (!validRange.contains(xrect)) {
            // we are too big, and must clip
            xrect &=wrectRange;
            wrect = xrect;
            wrect.moveBy(-data.crect.topLeft());
            //parent's X coord system is equal to parent's Qt coord
            //sys, so we don't need to map xrect.
        }

    }


    // unmap if we are outside the valid window system coord system
    bool outsideRange = !xrect.isValid();
    bool mapWindow = false;
    if (q->testAttribute(Qt::WA_OutsideWSRange) != outsideRange) {
        q->setAttribute(Qt::WA_OutsideWSRange, outsideRange);
        if (outsideRange) {
            ShowWindow(q->winId(), SW_HIDE);
            q->setAttribute(Qt::WA_Mapped, false);
        } else if (q->isShown()) {
            mapWindow = true;
        }
    }

    if (outsideRange)
        return;


    // and now recursively for all children...
    data.wrect = wrect;
    for (int i = 0; i < children.size(); ++i) {
        QObject *object = children.at(i);
        if (object->isWidgetType()) {
            QWidget *w = static_cast<QWidget *>(object);
            if (!w->isTopLevel())
                w->d->setWSGeometry();
        }
    }

    // move ourselves to the new position and map (if necessary) after
    // the movement. Rationale: moving unmapped windows is much faster
    // than moving mapped windows
    MoveWindow(q->winId(), xrect.x(), xrect.y(), xrect.width(), xrect.height(), true);
    if (mapWindow) {
            q->setAttribute(Qt::WA_Mapped);
            ShowWindow(q->winId(), SW_SHOWNOACTIVATE);
    }

}

//
// The internal qWinRequestConfig, defined in qapplication_win.cpp, stores move,
// resize and setGeometry requests for a widget that is already
// processing a config event. The purpose is to avoid recursion.
//
void qWinRequestConfig(WId, int, int, int, int, int);

void QWidget::setGeometry_sys(int x, int y, int w, int h, bool isMove)
{
    if (d->extra) {                                // any size restrictions?
        w = qMin(w,d->extra->maxw);
        h = qMin(h,d->extra->maxh);
        w = qMax(w,d->extra->minw);
        h = qMax(h,d->extra->minh);
    }
    if (isTopLevel()) {
        d->topData()->normalGeometry = QRect(0, 0, -1, -1);
        w = qMax(1, w);
        h = qMax(1, h);
    }

    QSize  oldSize(size());
    QPoint oldPos(pos());

    if (!isTopLevel())
        isMove = (data->crect.topLeft() != QPoint(x, y));
    bool isResize = w != oldSize.width() || h != oldSize.height();

    if (!isMove && !isResize)
        return;

    if (isResize) {
        if (!testAttribute(WA_StaticContents))
            ValidateRgn(winId(), 0);
        else if (!testWState(WState_InPaintEvent)) {
            QRegion region(0, 0, 1, 1);
            int result = GetUpdateRgn(winId(), region.handle(), false);
            if (result != NULLREGION && result != ERROR)
                repaint(region);
        }
    }

    if (isResize)
        clearWState(WState_Maximized);
    clearWState(WState_FullScreen);
    if (testWState(WState_ConfigPending)) {        // processing config event
        qWinRequestConfig(winId(), isMove ? 2 : 1, x, y, w, h);
    } else {
        setWState(WState_ConfigPending);
        if (isTopLevel()) {
            QRect fr(frameGeometry());
            if (d->extra) {
                fr.setLeft(fr.left() + x - data->crect.left());
                fr.setTop(fr.top() + y - data->crect.top());
                fr.setRight(fr.right() + (x + w - 1) - data->crect.right());
                fr.setBottom(fr.bottom() + (y + h - 1) - data->crect.bottom());
            }
            MoveWindow(winId(), fr.x(), fr.y(), fr.width(), fr.height(), true);
            data->crect.setRect(x, y, w, h);
        } else {
            data->crect.setRect(x, y, w, h);
            d->setWSGeometry();
        }
        clearWState(WState_ConfigPending);
    }

    // Process events immediately rather than in translateConfigEvent to
    // avoid windows message process delay.
    if (isVisible()) {
        if (isMove && pos() != oldPos) {
            QMoveEvent e(pos(), oldPos);
            QApplication::sendEvent(this, &e);
        }
        if (isResize) {
            QResizeEvent e(size(), oldSize);
            QApplication::sendEvent(this, &e);
            if (!testAttribute(WA_StaticContents))
                testWState(WState_InPaintEvent)?update():repaint();
        }
    } else {
        if (isMove && pos() != oldPos)
            setAttribute(WA_PendingMoveEvent, true);
        if (isResize)
            setAttribute(WA_PendingResizeEvent, true);
    }
}


void QWidget::setMinimumSize(int minw, int minh)
{
    if (minw < 0 || minh < 0)
        qWarning("QWidget::setMinimumSize: The smallest allowed size is (0,0)");
    d->createExtra();
    if (d->extra->minw == minw && d->extra->minh == minh)
        return;
    d->extra->minw = minw;
    d->extra->minh = minh;
    if (minw > width() || minh > height()) {
        bool resized = testAttribute(WA_Resized);
        resize(qMax(minw,width()), qMax(minh,height()));
        setAttribute(WA_Resized, resized); //not a user resize
    }
    updateGeometry();
}

void QWidget::setMaximumSize(int maxw, int maxh)
{
    if (maxw > QWIDGETSIZE_MAX || maxh > QWIDGETSIZE_MAX) {
        qWarning("QWidget::setMaximumSize: The largest allowed size is (%d,%d)",
                 QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        maxw = qMin(maxw, QWIDGETSIZE_MAX);
        maxh = qMin(maxh, QWIDGETSIZE_MAX);
    }
    if (maxw < 0 || maxh < 0) {
        qWarning("QWidget::setMaximumSize: (%s/%s) Negative sizes (%d,%d) "
                "are not possible",
                 objectName().isEmpty() ? QLatin1String("unnamed") : objectName(),
                 metaObject()->className(), maxw, maxh);
        maxw = qMax(maxw, 0);
        maxh = qMax(maxh, 0);
    }
    d->createExtra();
    if (d->extra->maxw == maxw && d->extra->maxh == maxh)
        return;
    d->extra->maxw = maxw;
    d->extra->maxh = maxh;
    if (maxw < width() || maxh < height()) {
        bool resized = testAttribute(WA_Resized);
        resize(qMin(maxw,width()), qMin(maxh,height()));
        setAttribute(WA_Resized, resized); //not a user resize
    }
    updateGeometry();
}

void QWidget::setSizeIncrement(int w, int h)
{
    d->createTLExtra();
    d->extra->topextra->incw = w;
    d->extra->topextra->inch = h;
}

void QWidget::setBaseSize(int w, int h)
{
    d->createTLExtra();
    d->extra->topextra->basew = w;
    d->extra->topextra->baseh = h;
}

void QWidget::scroll(int dx, int dy)
{
    if (testWState(WState_BlockUpdates) && children().size() == 0)
        return;
    UINT flags = SW_INVALIDATE | SW_SCROLLCHILDREN;
    if (!testAttribute(WA_NoBackground))
        flags |= SW_ERASE;

    ScrollWindowEx(winId(), dx, dy, 0, 0, 0, 0, flags);
    UpdateWindow(winId());
}

void QWidget::scroll(int dx, int dy, const QRect& r)
{
    if (testWState(WState_BlockUpdates))
        return;
    UINT flags = SW_INVALIDATE;
    if (!testAttribute(WA_NoBackground))
        flags |= SW_ERASE;

    RECT wr;
    wr.top = r.top();
    wr.left = r.left();
    wr.bottom = r.bottom()+1;
    wr.right = r.right()+1;
    ScrollWindowEx(winId(), dx, dy, &wr, &wr, 0, 0, flags);
    UpdateWindow(winId());
}


int QWidget::metric(int m) const
{
    int val;
    if (m == QPaintDeviceMetrics::PdmWidth) {
        val = data->crect.width();
    } else if (m == QPaintDeviceMetrics::PdmHeight) {
        val = data->crect.height();
    } else {
        HDC gdc = GetDC(0);
        switch (m) {
        case QPaintDeviceMetrics::PdmDpiX:
        case QPaintDeviceMetrics::PdmPhysicalDpiX:
            val = GetDeviceCaps(gdc, LOGPIXELSX);
            break;
        case QPaintDeviceMetrics::PdmDpiY:
        case QPaintDeviceMetrics::PdmPhysicalDpiY:
            val = GetDeviceCaps(gdc, LOGPIXELSY);
            break;
        case QPaintDeviceMetrics::PdmWidthMM:
            val = data->crect.width()
                    * GetDeviceCaps(gdc, HORZSIZE)
                    / GetDeviceCaps(gdc, HORZRES);
            break;
        case QPaintDeviceMetrics::PdmHeightMM:
            val = data->crect.height()
                    * GetDeviceCaps(gdc, VERTSIZE)
                    / GetDeviceCaps(gdc, VERTRES);
            break;
        case QPaintDeviceMetrics::PdmNumColors:
            if (GetDeviceCaps(gdc, RASTERCAPS) & RC_PALETTE)
                val = GetDeviceCaps(gdc, SIZEPALETTE);
            else {
                int bpp = GetDeviceCaps(hdc, BITSPIXEL);
                if(bpp==32)
                    val = INT_MAX;
                else if(bpp<=8)
                    val = GetDeviceCaps(hdc, NUMCOLORS);
                else
                    val = 1 << (bpp * GetDeviceCaps(hdc, PLANES));
            }
            break;
        case QPaintDeviceMetrics::PdmDepth:
            val = GetDeviceCaps(gdc, BITSPIXEL);
            break;
        default:
            val = 0;
            qWarning("QWidget::metric: Invalid metric command");
        }
        ReleaseDC(0, gdc);
    }
    return val;
}

void QWidgetPrivate::createSysExtra()
{
    extra->dropTarget = 0;
}

void QWidgetPrivate::deleteSysExtra()
{
    q->setAcceptDrops(false);
}

void QWidgetPrivate::createTLSysExtra()
{
    extra->topextra->winIcon = 0;
}

void QWidgetPrivate::deleteTLSysExtra()
{
    if (extra->topextra->winIcon)
        DestroyIcon(extra->topextra->winIcon);
}


bool QWidget::acceptDrops() const
{
    return (d->extra && d->extra->dropTarget);
}

void QWidget::setAcceptDrops(bool on)
{
    // Enablement is defined by d->extra->dropTarget != 0.

    if (on) {
        // Turn on.
        d->createExtra();
        QWExtra *extra = d->extraData();
        if (!extra->dropTarget)
            extra->dropTarget = qt_olednd_register(this);
    } else {
        // Turn off.
        QWExtra *extra = d->extraData();
        if (extra && extra->dropTarget) {
            qt_olednd_unregister(this, extra->dropTarget);
            extra->dropTarget = 0;
        }
    }
}

void QWidget::setMask(const QRegion &region)
{
    d->createExtra();
    if(QWExtra *extra = d->extraData())
        extra->mask = region;

    // Since SetWindowRegion takes ownership, and we need to translate,
    // we take a copy.
    HRGN wr = CreateRectRgn(0,0,1,1);
    CombineRgn(wr, region.handle(), 0, RGN_COPY);

    int fleft = 0, ftop = 0;
    if (isTopLevel()) {
        ftop = d->topData()->ftop;
        fleft = d->topData()->fleft;
    }
    OffsetRgn(wr, fleft, ftop);
    SetWindowRgn(winId(), wr, true);
}

void QWidget::setMask(const QBitmap &bitmap)
{
    d->createExtra();
    if(QWExtra *extra = d->extraData())
        extra->mask = QRegion(bitmap);

    HRGN wr = qt_win_bitmapToRegion(bitmap);

    int fleft = 0, ftop = 0;
    if (isTopLevel()) {
        ftop = d->topData()->ftop;
        fleft = d->topData()->fleft;
    }
    OffsetRgn(wr, fleft, ftop);
    SetWindowRgn(winId(), wr, true);
}

void QWidget::clearMask()
{
    d->createExtra();
    if(QWExtra *extra = d->extraData())
        extra->mask = QRegion();
    SetWindowRgn(winId(), 0, true);
}

void QWidget::updateFrameStrut() const
{
    QWidget *that = (QWidget *) this;

    if (!isVisible() || isDesktop()) {
        that->data->fstrut_dirty = isVisible();
        return;
    }

    RECT  fr, cr;
    GetWindowRect(winId(), &fr);
    GetClientRect(winId(), &cr);

    POINT pt;
    pt.x = 0;
    pt.y = 0;

    ClientToScreen(winId(), &pt);
    that->data->crect = QRect(QPoint(pt.x, pt.y),
                         QPoint(pt.x + cr.right, pt.y + cr.bottom));

    QTLWExtra *top = that->d->topData();
    top->ftop = data->crect.top() - fr.top;
    top->fleft = data->crect.left() - fr.left;
    top->fbottom = fr.bottom - data->crect.bottom();
    top->fright = fr.right - data->crect.right();

    that->data->fstrut_dirty = false;
}

#if defined(QT_TABLET_SUPPORT)
extern bool qt_is_gui_used;
static void init_wintab_functions()
{
    if (!qt_is_gui_used)
        return;
    QLibrary library("wintab32");
    library.setAutoUnload(false);
    QT_WA({
        ptrWTOpen = (PtrWTOpen)library.resolve("WTOpenW");
        ptrWTInfo = (PtrWTInfo)library.resolve("WTInfoW");
    } , {
        ptrWTOpen = (PtrWTOpen)library.resolve("WTOpenA");
        ptrWTInfo = (PtrWTInfo)library.resolve("WTInfoA");
    });

    ptrWTClose = (PtrWTClose)library.resolve("WTClose");
    ptrWTQueueSizeGet = (PtrWTQueueSizeGet)library.resolve("WTQueueSizeGet");
    ptrWTQueueSizeSet = (PtrWTQueueSizeSet)library.resolve("WTQueueSizeSet");
}

static void qt_tablet_init()
{
    if (qt_tablet_widget)
        return;
    qt_tablet_widget = new QWidget(0, "Qt internal tablet widget");
    LOGCONTEXT lcMine;
    qAddPostRoutine(qt_tablet_cleanup);
    struct tagAXIS tpOri[3];
    init_wintab_functions();
    if (ptrWTInfo && ptrWTOpen && ptrWTQueueSizeGet && ptrWTQueueSizeSet) {
        // make sure we have WinTab
        if (!ptrWTInfo(0, 0, NULL)) {
            qWarning("Wintab services not available");
            return;
        }

        // some tablets don't support tilt, check if it is possible,
        qt_tablet_tilt_support = ptrWTInfo(WTI_DEVICES, DVC_ORIENTATION, &tpOri);
        if (qt_tablet_tilt_support) {
            // check for azimuth and altitude
            qt_tablet_tilt_support = tpOri[0].axResolution && tpOri[1].axResolution;
        }
        // build our context from the default context
        ptrWTInfo(WTI_DEFSYSCTX, 0, &lcMine);
        lcMine.lcOptions |= CXO_MESSAGES | CXO_CSRMESSAGES;
        lcMine.lcPktData = PACKETDATA;
        lcMine.lcPktMode = PACKETMODE;
        lcMine.lcMoveMask = PACKETDATA;
        lcMine.lcOutOrgX = 0;
        lcMine.lcOutExtX = GetSystemMetrics(SM_CXSCREEN);
        lcMine.lcOutOrgY = 0;
        lcMine.lcOutExtY = -GetSystemMetrics(SM_CYSCREEN);
        qt_tablet_context = ptrWTOpen(qt_tablet_widget->winId(), &lcMine, true);
        if (qt_tablet_context == NULL) {
            qWarning("Failed to open the tablet");
            return;
        }
        // Set the size of the Packet Queue to the correct size...
        int currSize = ptrWTQueueSizeGet(qt_tablet_context);
        if (!ptrWTQueueSizeSet(qt_tablet_context, QT_TABLET_NPACKETQSIZE)) {
            // Ideally one might want to make use a smaller
            // multiple, but for now, since we managed to destroy
            // the existing Q with the previous call, set it back
            // to the other size, which should work.  If not,
            // we had best die.
            if (!ptrWTQueueSizeSet(qt_tablet_context, currSize))
                qWarning("There is no packet queue for the tablet.\n"
                "The tablet will not work");
        }

    }
}

static void qt_tablet_cleanup()
{
    if (ptrWTClose)
        ptrWTClose(qt_tablet_context);
    delete qt_tablet_widget;
    qt_tablet_widget = 0;
}
#endif

void QWidget::setWindowOpacity(double level)
{
    if(!isTopLevel())
        return;

    static bool function_resolved = false;
    if (!function_resolved) {
        ptrSetLayeredWindowAttributes =
            (PtrSetLayeredWindowAttributes) QLibrary::resolve("user32",
                                                              "SetLayeredWindowAttributes");
        function_resolved = true;
    }

    if (!ptrSetLayeredWindowAttributes)
        return;

    level = qMin(qMax(level, 0), 1.0);
    int wl = GetWindowLongA(winId(), GWL_EXSTYLE);

    if (level != 1.0) {
        if ((wl&Q_WS_EX_LAYERED) == 0)
            SetWindowLongA(winId(), GWL_EXSTYLE, wl|Q_WS_EX_LAYERED);
    } else if (wl&Q_WS_EX_LAYERED) {
        SetWindowLongA(winId(), GWL_EXSTYLE, wl & ~Q_WS_EX_LAYERED);
    }

    (*ptrSetLayeredWindowAttributes)(winId(), 0, (int)(level * 255), Q_LWA_ALPHA);
    d->topData()->opacity = (uchar)(level * 255);
}

double QWidget::windowOpacity() const
{
    return isTopLevel() ? (d->topData()->opacity / 255.0) : 0.0;
}

static QSingleCleanupHandler<QWin32PaintEngine> qt_paintengine_cleanup_handler;
static QWin32PaintEngine *qt_widget_paintengine = 0;
QPaintEngine *QWidget::paintEngine() const
{
    if (!qt_widget_paintengine) {
        qt_widget_paintengine = new QWin32PaintEngine(const_cast<QWidget*>(this));
        qt_paintengine_cleanup_handler.set(&qt_widget_paintengine);

    }
    return qt_widget_paintengine;
}
