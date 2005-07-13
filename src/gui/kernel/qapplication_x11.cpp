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

// ### 4.0: examine Q_EXPORT's below. The respective symbols had all
// been in use (e.g. in the KDE wm) before the introduction of a version
// map. One might want to turn some of them into proper public API and
// provide a proper alternative for others. See also the exports in
// qapplication_win.cpp, which suggest a unification.

#include "qplatformdefs.h"

#include "qcolormap.h"
#include "qdesktopwidget.h"
#include "qapplication.h"
#include "qapplication_p.h"
#include "qcursor.h"
#include "qwidget.h"
#include "qbitarray.h"
#include "qpainter.h"
#include "qfile.h"
#include "qpixmapcache.h"
#include "qdatetime.h"
#include "qtextcodec.h"
#include "qdatastream.h"
#include "qbuffer.h"
#include "qsocketnotifier.h"
#include "qsessionmanager.h"
#include "qclipboard.h"
#include "qwhatsthis.h"
#include "qsettings.h"
#include "qstylefactory.h"
#include "qfileinfo.h"
#include "qhash.h"
#include "qevent.h"
#include "qdebug.h"
#include <private/qunicodetables_p.h>
#include <private/qcrashhandler_p.h>
#include <private/qcolor_p.h>
#include <private/qcursor_p.h>

#include "qeventdispatcher_x11_p.h"
#include <private/qpaintengine_x11_p.h>

// Input method stuff - UNFINISHED
#ifndef QT_NO_IM
#include "qinputcontext.h"
#include "qinputcontextfactory.h"
#endif // QT_NO_IM

#include "qt_x11_p.h"
#include "qx11info_x11.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>

#include "qwidget_p.h"

//#define X_NOT_BROKEN
#ifdef X_NOT_BROKEN
// Some X libraries are built with setlocale #defined to _Xsetlocale,
// even though library users are then built WITHOUT such a definition.
// This creates a problem - Qt might setlocale() one value, but then
// X looks and doesn't see the value Qt set. The solution here is to
// implement _Xsetlocale just in case X calls it - redirecting it to
// the real libC version.
//
# ifndef setlocale
extern "C" char *_Xsetlocale(int category, const char *locale);
char *_Xsetlocale(int category, const char *locale)
{
    //qDebug("_Xsetlocale(%d,%s),category,locale");
    return setlocale(category,locale);
}
# endif // setlocale
#endif // X_NOT_BROKEN


// Fix old X libraries
#ifndef XK_KP_Home
#define XK_KP_Home              0xFF95
#endif
#ifndef XK_KP_Left
#define XK_KP_Left              0xFF96
#endif
#ifndef XK_KP_Up
#define XK_KP_Up                0xFF97
#endif
#ifndef XK_KP_Right
#define XK_KP_Right             0xFF98
#endif
#ifndef XK_KP_Down
#define XK_KP_Down              0xFF99
#endif
#ifndef XK_KP_Prior
#define XK_KP_Prior             0xFF9A
#endif
#ifndef XK_KP_Next
#define XK_KP_Next              0xFF9B
#endif
#ifndef XK_KP_End
#define XK_KP_End               0xFF9C
#endif
#ifndef XK_KP_Insert
#define XK_KP_Insert            0xFF9E
#endif
#ifndef XK_KP_Delete
#define XK_KP_Delete            0xFF9F
#endif

/* Warning: if you modify this string, modify the list of atoms in qt_x11_p.h as well! */
static const char * x11_atomnames = {
    // window-manager <-> client protocols
    "WM_PROTOCOLS\0"
    "WM_DELETE_WINDOW\0"
    "WM_TAKE_FOCUS\0"
    "_NET_WM_PING\0"
    "_NET_WM_CONTEXT_HELP\0"

    // ICCCM window state
    "WM_STATE\0"
    "WM_CHANGE_STATE\0"

    // Session management
    "WM_CLIENT_LEADER\0"
    "WM_WINDOW_ROLE\0"
    "SM_CLIENT_ID\0"

    // Clipboard
    "CLIPBOARD\0"
    "INCR\0"
    "TARGETS\0"
    "MULTIPLE\0"
    "TIMESTAMP\0"
    "CLIP_TEMPORARY\0"
    "_QT_SELECTION\0"
    "_QT_CLIPBOARD_SENTINEL\0"
    "_QT_SELECTION_SENTINEL\0"

    "RESOURCE_MANAGER\0"

    "_XSETROOT_ID\0"

    "_QT_SCROLL_DONE\0"
    "_QT_INPUT_ENCODING\0"
    "_QT_SIZEGRIP\0"

    "_MOTIF_WM_HINTS\0"

    "DTWM_IS_RUNNING\0"
    "KWIN_RUNNING\0"
    "KWM_RUNNING\0"
    "GNOME_BACKGROUND_PROPERTIES\0"
    "ENLIGHTENMENT_DESKTOP\0"
    "_SGI_DESKS_MANAGER\0"

    // EWMH (aka NETWM)
    "_NET_SUPPORTED\0"
    "_NET_VIRTUAL_ROOTS\0"
    "_NET_WORKAREA\0"

    "_NET_WM_NAME\0"
    "_NET_WM_ICON_NAME\0"

    "_NET_WM_PID\0"

    "_NET_WM_STATE\0"
    "_NET_WM_STATE_ABOVE\0"
    "_NET_WM_STATE_FULLSCREEN\0"
    "_NET_WM_STATE_MAXIMIZED_HORZ\0"
    "_NET_WM_STATE_MAXIMIZED_VERT\0"
    "_NET_WM_STATE_MODAL\0"
    "_NET_WM_STATE_STAYS_ON_TOP\0"

    "_NET_WM_USER_TIME\0"

    "_NET_WM_WINDOW_TYPE\0"
    "_NET_WM_WINDOW_TYPE_DIALOG\0"
    "_NET_WM_WINDOW_TYPE_MENU\0"
    "_NET_WM_WINDOW_TYPE_NORMAL\0"
    "_KDE_NET_WM_WINDOW_TYPE_OVERRIDE\0"
    "_NET_WM_WINDOW_TYPE_SPLASH\0"
    "_NET_WM_WINDOW_TYPE_TOOLBAR\0"
    "_NET_WM_WINDOW_TYPE_UTILITY\0"

    "_KDE_NET_WM_FRAME_STRUT\0"
    
    "_NET_STARTUP_INFO\0"
    "_NET_STARTUP_INFO_BEGIN\0"

    // Property formats
    "COMPOUND_TEXT\0"
    "TEXT\0"
    "UTF8_STRING\0"

    // xdnd
    "XdndEnter\0"
    "XdndPosition\0"
    "XdndStatus\0"
    "XdndLeave\0"
    "XdndDrop\0"
    "XdndFinished\0"
    "XdndTypeList\0"
    "XdndActionList\0"

    "XdndSelection\0"

    "XdndAware\0"
    "XdndProxy\0"

    "XdndActionCopy\0"
    "XdndActionLink\0"
    "XdndActionMove\0"
    "XdndActionPrivate\0"

    // Motif DND
    "_MOTIF_DRAG_AND_DROP_MESSAGE\0"
    "_MOTIF_DRAG_INITIATOR_INFO\0"
    "_MOTIF_DRAG_RECEIVER_INFO\0"
    "_MOTIF_DRAG_WINDOW\0"
    "_MOTIF_DRAG_TARGETS\0"

    "XmTRANSFER_SUCCESS\0"
    "XmTRANSFER_FAILURE\0"
};

Q_GUI_EXPORT QX11Data *qt_x11Data = 0;

/*****************************************************************************
  Internal variables and functions
 *****************************************************************************/
static const char *appName = 0;                        // application name
static const char *appClass = 0;                        // application class
static const char *appFont        = 0;                // application font
static const char *appBGCol        = 0;                // application bg color
static const char *appFGCol        = 0;                // application fg color
static const char *appBTNCol        = 0;                // application btn color
static const char *mwGeometry        = 0;                // main widget geometry
static const char *mwTitle        = 0;                // main widget title
char    *qt_ximServer        = 0;                // XIM Server will connect to
static bool        mwIconic        = false;        // main widget iconified
#if 0
static bool        noxim                = false;        // connect to xim or not
#endif
static bool        appSync                = false;        // X11 synchronization
#if defined(QT_DEBUG)
static bool        appNoGrab        = false;        // X11 grabbing enabled
static bool        appDoGrab        = false;        // X11 grabbing override (gdb)
#endif
static bool        app_save_rootinfo = false;        // save root info
static bool        app_do_modal        = false;        // modal mode
static Window        curWin = 0;                        // current window

// detect broken window managers
bool                qt_broken_wm                = false;
static void qt_detect_broken_window_manager();


// function to update the workarea of the screen - in qdesktopwidget_x11.cpp
extern void qt_desktopwidget_update_workarea();


// modifier masks for alt/meta - detected when the application starts
static long qt_alt_mask = 0;
static long qt_meta_mask = 0;
// modifier mask to remove mode switch from modifiers that have alt/meta set
// this problem manifests itself on HP/UX 10.20 at least, and without it
// modifiers do not work at all...
static long qt_mode_switch_remove_mask = 0;

// flags for extensions for special Languages, currently only for RTL languages
static bool         qt_use_rtl_extensions = false;

static Window        mouseActWindow             = 0;        // window where mouse is
static Qt::MouseButton  mouseButtonPressed   = Qt::NoButton; // last mouse button pressed
static Qt::MouseButtons mouseButtonState     = Qt::NoButton; // mouse button state
static Time        mouseButtonPressTime = 0;        // when was a button pressed
static short        mouseXPos, mouseYPos;                // mouse pres position in act window
static short        mouseGlobalXPos, mouseGlobalYPos; // global mouse press position

extern QWidgetList *qt_modal_stack;                // stack of modal widgets

// window where mouse buttons have been pressed
static Window pressed_window = XNone;

// popup control
static bool replayPopupMouseEvent = false;
static bool popupGrabOk;

static bool sm_blockUserInput = false;                // session management

bool qt_reuse_double_buffer = true;

Q_GUI_EXPORT int qt_xfocusout_grab_counter = 0;

#if !defined (QT_NO_TABLET_SUPPORT)
Q_GLOBAL_STATIC(TabletDeviceDataList, tablet_devices)
TabletDeviceDataList *qt_tablet_devices()
{
    return tablet_devices();
}

extern bool qt_tabletChokeMouse;
#endif

// last timestamp read from QSettings
static uint appliedstamp = 0;


static bool qt_x11EventFilter(XEvent* ev)
{
    long unused;
    if (qApp->filterEvent(ev, &unused))
        return true;
    return qApp->x11EventFilter(ev);
}

#if !defined(QT_NO_XIM)
XIMStyle        qt_xim_preferred_style = 0;
#endif
int qt_ximComposingKeycode=0;
QTextCodec * qt_input_mapper = 0;

extern bool     qt_check_clipboard_sentinel(); //def in qclipboard_x11.cpp
extern bool        qt_check_selection_sentinel(); //def in qclipboard_x11.cpp

static void        qt_save_rootinfo();
bool        qt_try_modal(QWidget *, XEvent *);

QWidget *qt_button_down = 0; // last widget to be pressed with the mouse
static QWidget *qt_popup_down = 0;  // popup that contains the pressed widget

extern bool qt_xdnd_dragging;

// gui or non-gui from qapplication.cpp
extern bool qt_is_gui_used;
extern bool qt_app_has_font;

class QETWidget : public QWidget                // event translator widget
{
public:
    bool translateMouseEvent(const XEvent *);
    bool translateKeyEventInternal(const XEvent *, int& count, QString& text,
                                   Qt::KeyboardModifiers& modifiers, int &code,
                                   QEvent::Type &type, bool /*willRepeat*/=false,
                                   bool statefulTranslation=true);
    bool translateKeyEvent(const XEvent *, bool grab);
    void translatePaintEvent(const XEvent *);
    bool translateConfigEvent(const XEvent *);
    bool translateCloseEvent(const XEvent *);
    bool translateScrollDoneEvent(const XEvent *);
    bool translateWheelEvent(int global_x, int global_y, int delta, Qt::MouseButtons buttons,
                             Qt::KeyboardModifiers modifiers, Qt::Orientation orient);
#if !defined (QT_NO_TABLET_SUPPORT)
    bool translateXinputEvent(const XEvent*, const TabletDeviceData *tablet);
#endif
    bool translatePropertyEvent(const XEvent *);
    void removePendingPaintEvents();
};


void QApplicationPrivate::createEventDispatcher()
{
    Q_Q(QApplication);
    eventDispatcher = (q->type() != QApplication::Tty
                       ? new QEventDispatcherX11(q)
                       : new QEventDispatcherUNIX(q));
}

/*****************************************************************************
  Default X error handlers
 *****************************************************************************/

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

static int (*original_x_errhandler)(Display *dpy, XErrorEvent *);
static int (*original_xio_errhandler)(Display *dpy);

static int qt_x_errhandler(Display *dpy, XErrorEvent *err)
{
    if (err->error_code == BadWindow) {
        X11->seen_badwindow = true;
        if (err->request_code == 25 /* X_SendEvent */ && X11->xdndHandleBadwindow())
            return 0;
        if (X11->ignore_badwindow)
            return 0;
    } else if (err->error_code == BadMatch && err->request_code == 42 /* X_SetInputFocus */) {
        return 0;
    }

    char errstr[256];
    XGetErrorText( dpy, err->error_code, errstr, 256 );
    char buffer[256];
    char request_str[256];
    sprintf(buffer, "%d", err->request_code);
    XGetErrorDatabaseText(dpy, "XRequest", buffer, "", request_str, 256);
    if (err->request_code < 128) {
        // X error for a normal protocol request
        qWarning( "X Error: %s %d\n"
                  "  Major opcode: %d (%s)\n"
                  "  Resource id:  0x%lx",
                  errstr, err->error_code,
                  err->request_code,
                  request_str,
                  err->resourceid );
    } else {
        // X error for an extension request
        const char *extensionName = 0;
        if (err->request_code == X11->xrender_major)
            extensionName = "RENDER";
        else if (err->request_code == X11->xrandr_major)
            extensionName = "RANDR";
        else if (err->request_code == X11->xinput_major)
            extensionName = "XInputExtension";

        char minor_str[256];
        if (extensionName) {
            sprintf(buffer, "%s.%d", extensionName, err->minor_code);
            XGetErrorDatabaseText(dpy, "XRequest", buffer, "", minor_str, 256);
        } else {
            extensionName = "Uknown extension";
            sprintf(minor_str, "Unknown request");
        }
        qWarning( "X Error: %s %d\n"
                  "  Extension:    %d (%s)\n"
                  "  Minor opcode: %d (%s)\n"
                  "  Resource id:  0x%lx",
                  errstr, err->error_code,
                  err->request_code,
                  extensionName,
                  err->minor_code,
                  minor_str,
                  err->resourceid );
    }

    // ### we really should distinguish between severe, non-severe and
    // ### application specific errors

    return 0;
}


static int qt_xio_errhandler(Display *)
{
    qWarning("%s: Fatal IO error: client killed", appName);
    QApplicationPrivate::reset_instance_pointer();
    exit(1);
    //### give the application a chance for a proper shutdown instead,
    //### exit(1) doesn't help.
    return 0;
}

#if defined(Q_C_CALLBACKS)
}
#endif


static void qt_x11_create_intern_atoms()
{
    const char *names[QX11Data::NAtoms];
    const char *ptr = x11_atomnames;

    int i = 0;
    while (*ptr) {
        names[i++] = ptr;
        while (*ptr)
            ++ptr;
        ++ptr;
    }

    Q_ASSERT(i == QX11Data::NPredefinedAtoms);

    QByteArray settings_atom_name("_QT_SETTINGS_TIMESTAMP_");
    settings_atom_name += XDisplayName(X11->displayName);
    names[i++] = settings_atom_name;

    Q_ASSERT(i == QX11Data::NAtoms);
#if defined(XlibSpecificationRelease) && (XlibSpecificationRelease >= 6)
    XInternAtoms(X11->display, (char **)names, i, False, X11->atoms);
#else
    for (i = 0; i < QX11Data::NAtoms; ++i)
        X11->atoms[i] = XInternAtom(X11->display, (char *)names[i], False);
#endif
}


/*! \internal
    apply the settings to the application
*/
bool QApplicationPrivate::x11_apply_settings()
{
    Atom type;
    int format;
    long offset = 0;
    unsigned long nitems, after = 1;
    unsigned char *data = 0;
    QDateTime timestamp, settingsstamp;
    bool update_timestamp = false;

    if (XGetWindowProperty(X11->display, QX11Info::appRootWindow(0),
                           ATOM(_QT_SETTINGS_TIMESTAMP), 0, 0,
                           False, AnyPropertyType, &type, &format, &nitems,
                           &after, &data) == Success && format == 8) {
        if (data)
            XFree(data);

        QBuffer ts;
        ts.open(QIODevice::WriteOnly);

        while (after > 0) {
            XGetWindowProperty(X11->display, QX11Info::appRootWindow(0),
                               ATOM(_QT_SETTINGS_TIMESTAMP),
                               offset, 1024, False, AnyPropertyType,
                               &type, &format, &nitems, &after, &data);
            if (format == 8) {
                ts.write(reinterpret_cast<char *>(data), nitems);
                offset += nitems / 4;
            }

            XFree(data);
        }

	QByteArray buf = ts.buffer();
        QDataStream ds(&buf, QIODevice::ReadOnly);
        ds >> timestamp;
    }

    QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
    settingsstamp = QFileInfo(settings.fileName()).lastModified();
    if (!settingsstamp.isValid())
        return false;

    if (appliedstamp && appliedstamp == settingsstamp.toTime_t())
        return true;

    settings.beginGroup(QLatin1String("Qt"));

    appliedstamp = settingsstamp.toTime_t();

    if (! timestamp.isValid() || settingsstamp > timestamp)
        update_timestamp = true;

    /*
      Qt settings. This is now they are written into the datastream.

      Palette / *                - QPalette
      font                       - QFont
      libraryPath                - QStringList
      style                      - QString
      doubleClickInterval        - int
      keyboardInputInterval  - int
      cursorFlashTime            - int
      wheelScrollLines           - int
      colorSpec                  - QString
      defaultCodec               - QString
      globalStrut/width          - int
      globalStrut/height         - int
      GUIEffects                 - QStringList
      Font Substitutions/ *      - QStringList
      Font Substitutions/...     - QStringList
    */

    QStringList strlist;
    int i;
    QPalette pal(Qt::black);
    int groupCount = 0;
    strlist = settings.value(QLatin1String("Palette/active")).toStringList();
    if (strlist.count() == QPalette::NColorRoles) {
        ++groupCount;
        for (i = 0; i < QPalette::NColorRoles; i++)
            pal.setColor(QPalette::Active, (QPalette::ColorRole) i,
                         QColor(strlist[i]));
    }
    strlist = settings.value(QLatin1String("Palette/inactive")).toStringList();
    if (strlist.count() == QPalette::NColorRoles) {
        ++groupCount;
        for (i = 0; i < QPalette::NColorRoles; i++)
            pal.setColor(QPalette::Inactive, (QPalette::ColorRole) i,
                         QColor(strlist[i]));
    }
    strlist = settings.value(QLatin1String("Palette/disabled")).toStringList();
    if (strlist.count() == QPalette::NColorRoles) {
        ++groupCount;
        for (i = 0; i < QPalette::NColorRoles; i++)
            pal.setColor(QPalette::Disabled, (QPalette::ColorRole) i,
                         QColor(strlist[i]));
    }


    if (groupCount == QPalette::NColorGroups)
        QApplicationPrivate::setSystemPalette(pal);

    if (!qt_app_has_font) {
        QFont font(QApplication::font());
        QString str = settings.value(QLatin1String("font")).toString();
        if (!str.isEmpty()) {
            font.fromString(str);
            if (font != QApplication::font())
                QApplication::setFont(font);
        }
    }

    // read library (ie. plugin) path list
    QString libpathkey =
        QString(QLatin1String("%1.%2/libraryPath"))
        .arg(QT_VERSION >> 16)
        .arg((QT_VERSION & 0xff00) >> 8);
    QStringList pathlist = settings.value(libpathkey).toString().split(QLatin1Char(':'));
    if (! pathlist.isEmpty()) {
        QStringList::ConstIterator it = pathlist.begin();
        while (it != pathlist.end())
            QApplication::addLibraryPath(*it++);
    }

    // read new QStyle
    QString stylename = settings.value(QLatin1String("style")).toString();
    if (QCoreApplication::startingUp()) {
        if (!stylename.isEmpty() && !QApplicationPrivate::styleOverride)
            QApplicationPrivate::styleOverride = new QString(stylename);
    } else {
        QApplication::setStyle(stylename);
    }

    int num =
        settings.value(QLatin1String("doubleClickInterval"),
                       QApplication::doubleClickInterval()).toInt();
    QApplication::setDoubleClickInterval(num);

    num =
        settings.value(QLatin1String("cursorFlashTime"),
                       QApplication::cursorFlashTime()).toInt();
    QApplication::setCursorFlashTime(num);

    num =
        settings.value(QLatin1String("wheelScrollLines"),
                       QApplication::wheelScrollLines()).toInt();
    QApplication::setWheelScrollLines(num);

    QString colorspec = settings.value(QLatin1String("colorSpec"),
                                       QVariant(QLatin1String("default"))).toString();
    if (colorspec == QLatin1String("normal"))
        QApplication::setColorSpec(QApplication::NormalColor);
    else if (colorspec == QLatin1String("custom"))
        QApplication::setColorSpec(QApplication::CustomColor);
    else if (colorspec == QLatin1String("many"))
        QApplication::setColorSpec(QApplication::ManyColor);
    else if (colorspec != QLatin1String("default"))
        colorspec = QLatin1String("default");

    QString defaultcodec = settings.value(QLatin1String("defaultCodec"),
                                          QVariant(QLatin1String("none"))).toString();
    if (defaultcodec != QLatin1String("none")) {
        QTextCodec *codec = QTextCodec::codecForName(defaultcodec.toLatin1());
        if (codec)
            QTextCodec::setCodecForTr(codec);
    }

    int w = settings.value(QLatin1String("globalStrut/width")).toInt();
    int h = settings.value(QLatin1String("globalStrut/height")).toInt();
    QSize strut(w, h);
    if (strut.isValid())
        QApplication::setGlobalStrut(strut);

    QStringList effects = settings.value(QLatin1String("GUIEffects")).toStringList();
    QApplication::setEffectEnabled(Qt::UI_General,
                                   effects.contains(QLatin1String("general")));
    QApplication::setEffectEnabled(Qt::UI_AnimateMenu,
                                   effects.contains(QLatin1String("animatemenu")));
    QApplication::setEffectEnabled(Qt::UI_FadeMenu,
                                   effects.contains(QLatin1String("fademenu")));
    QApplication::setEffectEnabled(Qt::UI_AnimateCombo,
                                   effects.contains(QLatin1String("animatecombo")));
    QApplication::setEffectEnabled(Qt::UI_AnimateTooltip,
                                   effects.contains(QLatin1String("animatetooltip")));
    QApplication::setEffectEnabled(Qt::UI_FadeTooltip,
                                   effects.contains(QLatin1String("fadetooltip")));
    QApplication::setEffectEnabled(Qt::UI_AnimateToolBox,
                                   effects.contains(QLatin1String("animatetoolbox")));

    settings.beginGroup(QLatin1String("Font Substitutions"));
    QStringList fontsubs = settings.childGroups();
    if (!fontsubs.isEmpty()) {
        QStringList::Iterator it = fontsubs.begin();
        for (; it != fontsubs.end(); ++it) {
            QString fam = *it;
            QStringList subs = settings.value(fam).toStringList();
            QFont::insertSubstitutions(fam, subs);
        }
    }
    settings.endGroup();

    qt_broken_wm =
        settings.value(QLatin1String("brokenWindowManager"), qt_broken_wm).toBool();

    qt_use_rtl_extensions =
        settings.value(QLatin1String("useRtlExtensions"), false).toBool();

    qt_reuse_double_buffer =
        settings.value(QLatin1String("reuseDoubleBuffer"), true).toBool();

#ifndef QT_NO_XIM
    if (qt_xim_preferred_style == 0) {
        QString ximInputStyle = settings.value(QLatin1String("XIMInputStyle"),
                                               QVariant(QLatin1String("on the spot"))).toString().toLower();
        if (ximInputStyle == QLatin1String("on the spot"))
            qt_xim_preferred_style = XIMPreeditCallbacks | XIMStatusNothing;
        else if (ximInputStyle == QLatin1String("over the spot"))
            qt_xim_preferred_style = XIMPreeditPosition | XIMStatusNothing;
        else if (ximInputStyle == QLatin1String("off the spot"))
            qt_xim_preferred_style = XIMPreeditArea | XIMStatusArea;
        else if (ximInputStyle == QLatin1String("root"))
            qt_xim_preferred_style = XIMPreeditNothing | XIMStatusNothing;
    }
#endif
    QStringList inputMethods = QInputContextFactory::keys();
    if (inputMethods.size() > 2 && inputMethods.contains(QLatin1String("imsw-multi"))) {
        X11->default_im = QLatin1String("imsw-multi");
    } else {
        X11->default_im = settings.value("DefaultInputMethod", QLatin1String("xim")).toString();
    }

    if (update_timestamp) {
        QByteArray stamp;
        QDataStream s(&stamp, QIODevice::WriteOnly);
        s << settingsstamp;

        XChangeProperty(X11->display, QX11Info::appRootWindow(0),
                        ATOM(_QT_SETTINGS_TIMESTAMP), ATOM(_QT_SETTINGS_TIMESTAMP), 8,
                        PropModeReplace, (unsigned char *)stamp.data(), stamp.size());
    }

    settings.endGroup(); // Qt

    return true;
}


/*! \internal
    Resets the QApplication::instance() pointer to zero
*/
void QApplicationPrivate::reset_instance_pointer()
{ QApplication::self = 0; }


// read the _QT_INPUT_ENCODING property and apply the settings to
// the application
static void qt_set_input_encoding()
{
    Atom type;
    int format;
    ulong  nitems, after = 1;
    const char *data;

    int e = XGetWindowProperty(X11->display, QX11Info::appRootWindow(),
                                ATOM(_QT_INPUT_ENCODING), 0, 1024,
                                False, XA_STRING, &type, &format, &nitems,
                                &after, (unsigned char**)&data);
    if (e != Success || !nitems || type == XNone) {
        // Always use the locale codec, since we have no examples of non-local
        // XIMs, and since we cannot get a sensible answer about the encoding
        // from the XIM.
        qt_input_mapper = QTextCodec::codecForLocale();

    } else {
        if (!qstricmp(data, "locale"))
            qt_input_mapper = QTextCodec::codecForLocale();
        else
            qt_input_mapper = QTextCodec::codecForName(data);
        // make sure we have an input codec
        if(!qt_input_mapper)
            qt_input_mapper = QTextCodec::codecForName("ISO 8859-1");
    }
    if (qt_input_mapper && qt_input_mapper->mibEnum() == 11) // 8859-8
        qt_input_mapper = QTextCodec::codecForName("ISO 8859-8-I");
    if(data)
        XFree((char *)data);
}

// set font, foreground and background from x11 resources. The
// arguments may override the resource settings.
static void qt_set_x11_resources(const char* font = 0, const char* fg = 0,
                                  const char* bg = 0, const char* button = 0)
{

    QString resFont, resFG, resBG, resEF, sysFont;

    QApplication::setEffectEnabled(Qt::UI_General, false);
    QApplication::setEffectEnabled(Qt::UI_AnimateMenu, false);
    QApplication::setEffectEnabled(Qt::UI_FadeMenu, false);
    QApplication::setEffectEnabled(Qt::UI_AnimateCombo, false);
    QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, false);
    QApplication::setEffectEnabled(Qt::UI_FadeTooltip, false);
    QApplication::setEffectEnabled(Qt::UI_AnimateToolBox, false);

    if (QApplication::desktopSettingsAware() && !QApplicationPrivate::x11_apply_settings()) {
        int format;
        ulong  nitems, after = 1;
        QString res;
        long offset = 0;
        Atom type = XNone;

        while (after > 0) {
            uchar *data;
            XGetWindowProperty(X11->display, QX11Info::appRootWindow(0),
                                ATOM(RESOURCE_MANAGER),
                                offset, 8192, False, AnyPropertyType,
                                &type, &format, &nitems, &after,
                                &data);
            res += (char*)data;
            offset += 2048; // offset is in 32bit quantities... 8192/4 == 2048
            if (data)
                XFree((char *)data);
        }

        QString key, value;
        int l = 0, r;
        QString apn = appName;
        QString apc = appClass;
        int apnl = apn.length();
        int apcl = apc.length();
        int resl = res.length();

        while (l < resl) {
            r = res.indexOf('\n', l);
            if (r < 0)
                r = resl;
            while (::isSpace(res[l]))
                l++;
            bool mine = false;
            if (res[l] == '*' &&
                 (res[l+1] == 'f' || res[l+1] == 'b' || res[l+1] == 'g' ||
                  res[l+1] == 'F' || res[l+1] == 'B' || res[l+1] == 'G' ||
                  res[l+1] == 's' || res[l+1] == 'S')) {
                // OPTIMIZED, since we only want "*[fbgs].."
                QString item = res.mid(l, r - l).simplified();
                int i = item.indexOf(':');
                key = item.left(i).trimmed().mid(1).toLower();
                value = item.right(item.length() - i - 1).trimmed();
                mine = true;
            } else if (res[l] == appName[0] || (appClass && res[l] == appClass[0])) {
                if (res.mid(l,apnl) == apn && (res[l+apnl] == '.' || res[l+apnl] == '*')) {
                    QString item = res.mid(l, r - l).simplified();
                    int i = item.indexOf(':');
                    key = item.left(i).trimmed().mid(apnl+1).toLower();
                    value = item.right(item.length() - i - 1).trimmed();
                    mine = true;
                } else if (res.mid(l,apcl) == apc && (res[l+apcl] == '.' || res[l+apcl] == '*')) {
                    QString item = res.mid(l, r - l).simplified();
                    int i = item.indexOf(':');
                    key = item.left(i).trimmed().mid(apcl+1).toLower();
                    value = item.right(item.length() - i - 1).trimmed();
                    mine = true;
                }
            }

            if (mine) {
                if (!font && key == "systemfont")
                    sysFont = value.left(value.lastIndexOf(':'));
                if (!font && key == "font")
                    resFont = value;
                else if  (!fg &&  key == "foreground")
                    resFG = value;
                else if (!bg && key == "background")
                    resBG = value;
                else if (key == "guieffects")
                    resEF = value;
                // NOTE: if you add more, change the [fbg] stuff above
            }

            l = r + 1;
        }
    }
    if (!sysFont.isEmpty())
        resFont = sysFont;
    if (resFont.isEmpty())
        resFont = font;
    if (resFG.isEmpty())
        resFG = fg;
    if (resBG.isEmpty())
        resBG = bg;
    if (!qt_app_has_font && !resFont.isEmpty()) { // set application font
        QFont fnt;
        fnt.setRawName(resFont);

        // the font we get may actually be an alias for another font,
        // so we reset the application font to the real font info.
        if (! fnt.exactMatch()) {
            QFontInfo fontinfo(fnt);
            fnt.setFamily(fontinfo.family());
            fnt.setRawMode(fontinfo.rawMode());

            if (! fnt.rawMode()) {
                fnt.setItalic(fontinfo.italic());
                fnt.setWeight(fontinfo.weight());
                fnt.setUnderline(fontinfo.underline());
                fnt.setStrikeOut(fontinfo.strikeOut());
                fnt.setStyleHint(fontinfo.styleHint());

                if (fnt.pointSize() <= 0 && fnt.pixelSize() <= 0)
                    // size is all wrong... fix it
                    fnt.setPointSize((int) ((fontinfo.pixelSize() * 72. /
                                                (float) QX11Info::appDpiY()) +
                                              0.5));
            }
        }

        if (fnt != QApplication::font()) {
            QApplication::setFont(fnt);
        }
    }

    if (button || !resBG.isEmpty() || !resFG.isEmpty()) {// set app colors
        (void) QApplication::style();  // trigger creation of application style and system palettes
        QColor btn;
        QColor bg;
        QColor fg;
        if (!resBG.isEmpty())
            bg = QColor(QString(resBG));
        else
            bg = QApplicationPrivate::sys_pal->color(QPalette::Active, QPalette::Background);
        if (!resFG.isEmpty())
            fg = QColor(QString(resFG));
        else
            fg = QApplicationPrivate::sys_pal->color(QPalette::Active, QPalette::Foreground);
        if (button)
            btn = QColor(button);
        else if (!resBG.isEmpty())
            btn = bg;
        else
            btn = QApplicationPrivate::sys_pal->color(QPalette::Active, QPalette::Button);

        int h,s,v;
        fg.getHsv(&h,&s,&v);
        QColor base = Qt::white;
        bool bright_mode = false;
        if (v >= 255-50) {
            base = btn.dark(150);
            bright_mode = true;
        }

        QPalette pal(fg, btn, btn.light(), btn.dark(), btn.dark(150), fg, Qt::white, base, bg);
        if (bright_mode) {
            pal.setColor(QPalette::HighlightedText, base);
            pal.setColor(QPalette::Highlight, Qt::white);
        } else {
            pal.setColor(QPalette::HighlightedText, Qt::white);
            pal.setColor(QPalette::Highlight, Qt::darkBlue);
        }
        QColor disabled((fg.red()   + btn.red())  / 2,
                        (fg.green() + btn.green())/ 2,
                        (fg.blue()  + btn.blue()) / 2);
        pal.setColorGroup(QPalette::Disabled, disabled, btn, btn.light(125),
                          btn.dark(), btn.dark(150), disabled, Qt::white, Qt::white, bg);
        if (bright_mode) {
            pal.setColor(QPalette::Disabled, QPalette::HighlightedText, base);
            pal.setColor(QPalette::Disabled, QPalette::Highlight, Qt::white);
        } else {
            pal.setColor(QPalette::Disabled, QPalette::HighlightedText, Qt::white);
            pal.setColor(QPalette::Disabled, QPalette::Highlight, Qt::darkBlue);
        }
        QApplicationPrivate::setSystemPalette(pal);
    }

    if (!resEF.isEmpty()) {
        QStringList effects = resEF.split(' ');
        QApplication::setEffectEnabled(Qt::UI_General, effects.contains("general"));
        QApplication::setEffectEnabled(Qt::UI_AnimateMenu, effects.contains("animatemenu"));
        QApplication::setEffectEnabled(Qt::UI_FadeMenu, effects.contains("fademenu"));
        QApplication::setEffectEnabled(Qt::UI_AnimateCombo, effects.contains("animatecombo"));
        QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, effects.contains("animatetooltip"));
        QApplication::setEffectEnabled(Qt::UI_FadeTooltip, effects.contains("fadetooltip"));
        QApplication::setEffectEnabled(Qt::UI_AnimateToolBox, effects.contains("animatetoolbox"));
    }
}


static void qt_detect_broken_window_manager()
{
    Atom type;
    int format;
    ulong nitems, after;
    uchar *data = 0;

    // look for SGI's 4Dwm
    int e = XGetWindowProperty(X11->display, QX11Info::appRootWindow(),
                               ATOM(_SGI_DESKS_MANAGER), 0, 1, False, XA_WINDOW,
                               &type, &format, &nitems, &after, &data);
    if (data)
        XFree(data);

    if (e == Success && type == XA_WINDOW && format == 32 && nitems == 1 && after == 0) {
        // detected SGI 4Dwm
        qt_broken_wm = true;
    }
}


// update the supported array
static void qt_get_net_supported()
{
    Atom type;
    int format;
    long offset = 0;
    unsigned long nitems, after;
    unsigned char *data = 0;

    int e = XGetWindowProperty(X11->display, QX11Info::appRootWindow(),
                               ATOM(_NET_SUPPORTED), 0, 0,
                               False, XA_ATOM, &type, &format, &nitems, &after, &data);
    if (data)
        XFree(data);

    if (X11->net_supported_list)
        delete [] X11->net_supported_list;
    X11->net_supported_list = 0;

    if (e == Success && type == XA_ATOM && format == 32) {
        QBuffer ts;
        ts.open(QIODevice::WriteOnly);

        while (after > 0) {
            XGetWindowProperty(X11->display, QX11Info::appRootWindow(),
                               ATOM(_NET_SUPPORTED), offset, 1024,
                               False, XA_ATOM, &type, &format, &nitems, &after, &data);

            if (type == XA_ATOM && format == 32) {
                ts.write(reinterpret_cast<char *>(data), nitems * sizeof(long));
                offset += nitems;
            } else
                after = 0;
            if (data)
                XFree(data);
        }

        // compute nitems
        QByteArray buffer(ts.buffer());
        nitems = buffer.size() / sizeof(Atom);
        X11->net_supported_list = new Atom[nitems + 1];
        Atom *a = (Atom *) buffer.data();
        uint i;
        for (i = 0; i < nitems; i++)
            X11->net_supported_list[i] = a[i];
        X11->net_supported_list[nitems] = 0;
    }
}


bool qt_net_supports(Atom atom)
{
    if (! X11->net_supported_list)
        return false;

    bool supported = false;
    int i = 0;
    while (X11->net_supported_list[i] != 0) {
        if (X11->net_supported_list[i++] == atom) {
            supported = true;
            break;
        }
    }

    return supported;
}


// update the virtual roots array
static void qt_get_net_virtual_roots()
{
    if (X11->net_virtual_root_list)
        delete [] X11->net_virtual_root_list;
    X11->net_virtual_root_list = 0;

    if (!qt_net_supports(ATOM(_NET_VIRTUAL_ROOTS)))
        return;

    Atom type;
    int format;
    long offset = 0;
    unsigned long nitems, after;
    unsigned char *data;

    int e = XGetWindowProperty(X11->display, QX11Info::appRootWindow(),
                               ATOM(_NET_VIRTUAL_ROOTS), 0, 0,
                               False, XA_ATOM, &type, &format, &nitems, &after, &data);
    if (data)
        XFree(data);

    if (e == Success && type == XA_ATOM && format == 32) {
        QBuffer ts;
        ts.open(QIODevice::WriteOnly);

        while (after > 0) {
            XGetWindowProperty(X11->display, QX11Info::appRootWindow(),
                               ATOM(_NET_VIRTUAL_ROOTS), offset, 1024,
                               False, XA_ATOM, &type, &format, &nitems, &after, &data);

            if (type == XA_ATOM && format == 32) {
                ts.write(reinterpret_cast<char *>(data), nitems * 4);
                offset += nitems;
            } else
                after = 0;
            if (data)
                XFree(data);
        }

        // compute nitems
        QByteArray buffer(ts.buffer());
        nitems = buffer.size() / sizeof(Window);
        X11->net_virtual_root_list = new Window[nitems + 1];
        Window *a = (Window *) buffer.data();
        uint i;
        for (i = 0; i < nitems; i++)
            X11->net_virtual_root_list[i] = a[i];
        X11->net_virtual_root_list[nitems] = 0;
    }
}

static void qt_net_update_user_time(QWidget *tlw)
{
    XChangeProperty(X11->display, tlw->winId(), ATOM(_NET_WM_USER_TIME),
                    XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &X11->userTime, 1);
}

static void qt_check_focus_model()
{
    Window fw = XNone;
    int unused;
    XGetInputFocus(X11->display, &fw, &unused);
    if (fw == PointerRoot)
        X11->focus_model = QX11Data::FM_PointerRoot;
    else
        X11->focus_model = QX11Data::FM_Other;
}

#ifndef QT_NO_TABLET_SUPPORT
static bool isXInputSupported(Display *dpy)
{
    Bool exists;
    XExtensionVersion *version;
    exists = XQueryExtension(dpy, "XInputExtension", &X11->xinput_major,
                             &X11->xinput_eventbase, &X11->xinput_errorbase);
    if (!exists)
        return false;
    version = XGetExtensionVersion(dpy, "XInputExtension");
    if (!version || version == reinterpret_cast<XExtensionVersion *>(NoSuchExtension))
        return false;

    XFree(version);
    return true;
}
#endif

/*****************************************************************************
  qt_init() - initializes Qt for X11
 *****************************************************************************/

#define XK_MISCELLANY
#define XK_LATIN1
#define XK_KOREAN
#define XK_XKB_KEYS
#include <X11/keysymdef.h>

#if !defined(QT_NO_FONTCONFIG)
static void getXDefault(const char *group, const char *key, int *val)
{
    char *str = XGetDefault(X11->display, group, key);
    if (str) {
        char *end = 0;
        int v = strtol(str, &end, 0);
        if (str != end)
            *val = v;
    }
}

static void getXDefault(const char *group, const char *key, double *val)
{
    char *str = XGetDefault(X11->display, group, key);
    if (str) {
        char *end = 0;
        double v = strtod(str, &end);
        if (str != end)
            *val = v;
    }
}

static void getXDefault(const char *group, const char *key, bool *val)
{
    char *str = XGetDefault(X11->display, group, key);
    if (str) {
        char c = str[0];
        if (isupper((int)c))
            c = tolower(c);
        if (c == 't' || c == 'y' || c == '1')
            *val = true;
        else if (c == 'f' || c == 'n' || c == '0')
            *val = false;
        if (c == 'o') {
            c = str[1];
            if (isupper((int)c))
                c = tolower(c);
            if (c == 'n')
                *val = true;
            if (c == 'f')
                *val = false;
        }
    }
}
#endif

// ### This should be static but it isn't because of the friend declaration
// ### in qpaintdevice.h which then should have a static too but can't have
// ### it because "storage class specifiers invalid in friend function
// ### declarations" :-) Ideas anyone?
void qt_init(QApplicationPrivate *priv, int,
	     Display *display, Qt::HANDLE visual, Qt::HANDLE colormap)
{
    X11 = new QX11Data;
    X11->display = display;
    X11->displayName = 0;
    X11->foreignDisplay = (display != 0);
    X11->focus_model = -1;

    // RANDR
    X11->use_xrandr = false;
    X11->xrandr_major = 0;
    X11->xrandr_eventbase = 0;
    X11->xrandr_errorbase = 0;

    // RENDER
    X11->use_xrender = false;
    X11->xrender_major = 0;
    X11->xrender_eventbase = 0;
    X11->xrender_errorbase = 0;

    // XInputExtension
    X11->use_xinput = false;
    X11->xinput_major = 0;
    X11->xinput_eventbase = 0;
    X11->xinput_errorbase = 0;

    X11->sip_serial = 0;
    X11->net_supported_list = 0;
    X11->net_virtual_root_list = 0;
    X11->wm_client_leader = 0;
    X11->screens = 0;
    X11->screenCount = 0;
    X11->time = CurrentTime;
    X11->userTime = CurrentTime;
    X11->ignore_badwindow = false;
    X11->seen_badwindow = false;

    X11->motifdnd_active = false;

    X11->default_im = "imsw-multi";
    priv->inputContext = 0;

    // colormap control
    X11->visual_class = -1;
    X11->visual_id = -1;
    X11->color_count = 0;
    X11->custom_cmap = false;

    // outside visual/colormap
    X11->visual = reinterpret_cast<Visual *>(visual);
    X11->colormap = colormap;

#ifndef QT_NO_XRENDER
    memset(X11->solid_fills, 0, sizeof(X11->solid_fills));
    for (int i = 0; i < X11->solid_fill_count; ++i)
        X11->solid_fills[i].screen = -1;
    memset(X11->pattern_fills, 0, sizeof(X11->pattern_fills));
    for (int i = 0; i < X11->pattern_fill_count; ++i)
        X11->pattern_fills[i].screen = -1;
#endif

    X11->startupId = 0;
    
    int argc = priv->argc;
    char **argv = priv->argv;

    if (X11->display) {
        // Qt part of other application

        // Set application name and class
        appName = qstrdup("Qt-subapplication");
        char *app_class = 0;
        if (argv) {
            const char* p = strrchr(argv[0], '/');
            app_class = qstrdup(p ? p + 1 : argv[0]);
            if (app_class[0])
                app_class[0] = toupper(app_class[0]);
        }
        appClass = app_class;
    } else {
        // Qt controls everything (default)

        // Set application name and class
        char *app_class = 0;
        if (argv && argv[0]) {
            const char *p = strrchr(argv[0], '/');
            appName = p ? p + 1 : argv[0];
            app_class = qstrdup(appName);
            if (app_class[0])
                app_class[0] = toupper(app_class[0]);
        }
        appClass = app_class;
    }

    // Install default error handlers
    original_x_errhandler = XSetErrorHandler(qt_x_errhandler);
    original_xio_errhandler = XSetIOErrorHandler(qt_xio_errhandler);

    // Get command line params
    int j = argc ? 1 : 0;
    for (int i=1; i<argc; i++) {
        if (argv[i] && *argv[i] != '-') {
            argv[j++] = argv[i];
            continue;
        }
        QByteArray arg(argv[i]);
        if (arg == "-display") {
            if (++i < argc && !X11->display)
                X11->displayName = argv[i];
        } else if (arg == "-fn" || arg == "-font") {
            if (++i < argc)
                appFont = argv[i];
        } else if (arg == "-bg" || arg == "-background") {
            if (++i < argc)
                appBGCol = argv[i];
        } else if (arg == "-btn" || arg == "-button") {
            if (++i < argc)
                appBTNCol = argv[i];
        } else if (arg == "-fg" || arg == "-foreground") {
            if (++i < argc)
                appFGCol = argv[i];
        } else if (arg == "-name") {
            if (++i < argc)
                appName = argv[i];
        } else if (arg == "-title") {
            if (++i < argc)
                mwTitle = argv[i];
        } else if (arg == "-geometry") {
            if (++i < argc)
                mwGeometry = argv[i];
        } else if (arg == "-im") {
            if (++i < argc)
                qt_ximServer = argv[i];
#if 0
        } else if (arg == "-noxim") {
            noxim=true;
#endif
        } else if (arg == "-iconic") {
            mwIconic = !mwIconic;
        } else if (arg == "-ncols") {   // xv and netscape use this name
            if (++i < argc)
                X11->color_count = qMax(0,atoi(argv[i]));
        } else if (arg == "-visual") {  // xv and netscape use this name
            if (++i < argc && !X11->visual) {
                QString s = QString(argv[i]).toLower();
                if (s == "staticgray")
                    X11->visual_class = StaticGray;
                else if (s == "grayscale")
                    X11->visual_class = XGrayScale;
                else if (s == "staticcolor")
                    X11->visual_class = StaticColor;
                else if (s == "pseudocolor")
                    X11->visual_class = PseudoColor;
                else if (s == "truecolor")
                    X11->visual_class = TrueColor;
                else if (s == "directcolor")
                    X11->visual_class = DirectColor;
                else
                    X11->visual_id = static_cast<int>(strtol(argv[i], 0, 0));
            }
#ifndef QT_NO_XIM
        } else if (arg == "-inputstyle/") {
            if (++i < argc) {
                QString s = QString(argv[i]).toLower();
                if (s == "onthespot")
                    qt_xim_preferred_style = XIMPreeditCallbacks |
                                             XIMStatusNothing;
                else if (s == "overthespot")
                    qt_xim_preferred_style = XIMPreeditPosition |
                                             XIMStatusNothing;
                else if (s == "offthespot")
                    qt_xim_preferred_style = XIMPreeditArea |
                                             XIMStatusArea;
                else if (s == "root")
                    qt_xim_preferred_style = XIMPreeditNothing |
                                             XIMStatusNothing;
            }
#endif
        } else if (arg == "-cmap") {    // xv uses this name
            if (!X11->colormap)
                X11->custom_cmap = true;
        }
#if defined(QT_DEBUG)
        else if (arg == "-sync")
            appSync = !appSync;
        else if (arg == "-nograb")
            appNoGrab = !appNoGrab;
        else if (arg == "-dograb")
            appDoGrab = !appDoGrab;
#endif
        else
            argv[j++] = argv[i];
    }

    priv->argc = j;

#if defined(QT_DEBUG) && defined(Q_OS_LINUX)
    if (!appNoGrab && !appDoGrab) {
        QString s;
        s.sprintf("/proc/%d/cmdline", getppid());
        QFile f(s);
        if (f.open(QIODevice::ReadOnly)) {
            s.clear();
            char c;
            while (f.getChar(&c) && c) {
                if (c == '/')
                    s.clear();
                else
                    s += c;
            }
            if (s == "gdb") {
                appNoGrab = true;
                qDebug("Qt: gdb: -nograb added to command-line options.\n"
                       "\t Use the -dograb option to enforce grabbing.");
            }
            f.close();
        }
    }
#endif

    // Connect to X server
    if (qt_is_gui_used && !X11->display) {
        if ((X11->display = XOpenDisplay(X11->displayName)) == 0) {
            qWarning("%s: cannot connect to X server %s", appName,
                     XDisplayName(X11->displayName));
            QApplicationPrivate::reset_instance_pointer();
            exit(1);
        }

        if (appSync)                                // if "-sync" argument
            XSynchronize(X11->display, true);
    }

    // Common code, regardless of whether display is foreign.

    // Get X parameters

    if (qt_is_gui_used) {
        X11->defaultScreen = DefaultScreen(X11->display);
        X11->screenCount = ScreenCount(X11->display);

        X11->screens = new QX11InfoData[X11->screenCount];

        for (int s = 0; s < X11->screenCount; s++) {
            QX11InfoData *screen = X11->screens + s;
            screen->ref = 1; // ensures it doesn't get deleted
            screen->screen = s;
            screen->dpiX = (DisplayWidth(X11->display, s) * 254 + DisplayWidthMM(X11->display, s)*5)
                           / (DisplayWidthMM(X11->display, s)*10);
            screen->dpiY = (DisplayHeight(X11->display, s) * 254 + DisplayHeightMM(X11->display, s)*5)
                           / (DisplayHeightMM(X11->display, s)*10);
        }

        QColormap::initialize();

        // Support protocols
        X11->xdndSetup();

        // Finally create all atoms
        qt_x11_create_intern_atoms();

        // look for broken window managers
        qt_detect_broken_window_manager();

        // initialize NET lists
        qt_get_net_supported();
        qt_get_net_virtual_roots();

#ifndef QT_NO_XRANDR
        // See if XRandR is supported on the connected display
        if (XQueryExtension(X11->display, "RANDR", &X11->xrandr_major,
                            &X11->xrandr_eventbase, &X11->xrandr_errorbase)
            && XRRQueryExtension(X11->display, &X11->xrandr_eventbase, &X11->xrandr_errorbase)) {
            // XRandR is supported
            X11->use_xrandr = true;
        }
#endif // QT_NO_XRANDR

#ifndef QT_NO_XRENDER
        // See if XRender is supported on the connected display
        if (XQueryExtension(X11->display, "RENDER", &X11->xrender_major,
                            &X11->xrender_eventbase, &X11->xrender_errorbase)
            && XRenderQueryExtension(X11->display, &X11->xrender_eventbase,
                                     &X11->xrender_errorbase)) {
            // XRender is supported, let's see if we have a PictFormat for the
            // default visual
            XRenderPictFormat *format =
                XRenderFindVisualFormat(X11->display,
                                        (Visual *) QX11Info::appVisual(X11->defaultScreen));
            // Check the version as well - we need v0.4 or higher
            int major = 0;
            int minor = 0;
            XRenderQueryVersion(X11->display, &major, &minor);
            if (qgetenv("QT_X11_NO_XRENDER").isNull())
                X11->use_xrender = (major >= 0 && minor >= 5) && (format != 0);
        }
#endif // QT_NO_XRENDER

#ifndef QT_NO_XKB
        // If XKB is detected, set the GrabsUseXKBState option so input method
        // compositions continue to work (ie. deadkeys)
        unsigned int state = XkbPCF_GrabsUseXKBStateMask;
        (void) XkbSetPerClientControls(X11->display, state, &state);
#endif

        X11->has_fontconfig = false;
#if !defined(QT_NO_FONTCONFIG)
        X11->has_fontconfig = FcInit();

        int dpi = 0;
        getXDefault("Xft", FC_DPI, &dpi);
        if (dpi) {
                    for (int s = 0; s < ScreenCount(X11->display); ++s) {
                        QX11Info::setAppDpiX(s, dpi);
                        QX11Info::setAppDpiY(s, dpi);
                    }
                }
        X11->fc_scale = 1.;
        getXDefault("Xft", FC_SCALE, &X11->fc_scale);
        for (int s = 0; s < ScreenCount(X11->display); ++s) {
            int subpixel = FC_RGBA_UNKNOWN;
#if RENDER_MAJOR > 0 || RENDER_MINOR >= 6
            if (X11->use_xrender) {
                int rsp = XRenderQuerySubpixelOrder(X11->display, s);
                switch (rsp) {
                default:
                case SubPixelUnknown:
                    subpixel = FC_RGBA_UNKNOWN;
                    break;
                case SubPixelHorizontalRGB:
                    subpixel = FC_RGBA_RGB;
                    break;
                case SubPixelHorizontalBGR:
                    subpixel = FC_RGBA_BGR;
                    break;
                case SubPixelVerticalRGB:
                    subpixel = FC_RGBA_VRGB;
                    break;
                case SubPixelVerticalBGR:
                    subpixel = FC_RGBA_VBGR;
                    break;
                case SubPixelNone:
                    subpixel = FC_RGBA_NONE;
                    break;
                }
            }
#endif
            getXDefault("Xft", FC_RGBA, &subpixel);
            X11->screens[s].subpixel = subpixel;
        }
        X11->fc_antialias = true;
        getXDefault("Xft", FC_ANTIALIAS, &X11->fc_antialias);
#if 0
        // ###### these are implemented by Xft, not sure we need them
        getXDefault("Xft", FC_AUTOHINT, &X11->fc_autohint);
#ifdef FC_HINT_STYLE
        getXDefault("Xft", FC_HINT_STYLE, &X11->fc_autohint);
#endif
        getXDefault("Xft", FC_HINTING, &X11->fc_autohint);
        getXDefault("Xft", FC_MINSPACE, &X11->fc_autohint);
#endif
#endif // QT_NO_XRENDER

        // look at the modifier mapping, and get the correct masks for alt/meta
        // find the alt/meta masks
        XModifierKeymap *map = XGetModifierMapping(X11->display);
        if (map) {
            int i, maskIndex = 0, mapIndex = 0;
            for (maskIndex = 0; maskIndex < 8; maskIndex++) {
                for (i = 0; i < map->max_keypermod; i++) {
                    if (map->modifiermap[mapIndex]) {
                        KeySym sym =
                            XKeycodeToKeysym(X11->display, map->modifiermap[mapIndex], 0);
                        if (qt_alt_mask == 0 &&
                            (sym == XK_Alt_L || sym == XK_Alt_R)) {
                            qt_alt_mask = 1 << maskIndex;
                        }
                        if (qt_meta_mask == 0 &&
                            (sym == XK_Meta_L || sym == XK_Meta_R)) {
                            qt_meta_mask = 1 << maskIndex;
                        }
                    }
                    mapIndex++;
                }
            }

            // not look for mode_switch in qt_alt_mask and qt_meta_mask - if it is
            // present in one or both, then we set qt_mode_switch_remove_mask.
            // see QETWidget::translateKeyEventInternal for an explanation
            // of why this is needed
            mapIndex = 0;
            for (maskIndex = 0; maskIndex < 8; maskIndex++) {
                if (qt_alt_mask  != (1 << maskIndex) &&
                    qt_meta_mask != (1 << maskIndex)) {
                    for (i = 0; i < map->max_keypermod; i++)
                        mapIndex++;
                    continue;
                }

                for (i = 0; i < map->max_keypermod; i++) {
                    if (map->modifiermap[mapIndex]) {
                        KeySym sym =
                            XKeycodeToKeysym(X11->display, map->modifiermap[mapIndex], 0);
                        if (sym == XK_Mode_switch) {
                            qt_mode_switch_remove_mask |= 1 << maskIndex;
                        }
                    }
                    mapIndex++;
                }
            }

            XFreeModifiermap(map);
        } else {
            // assume defaults
            qt_alt_mask = Mod1Mask;
            qt_meta_mask = Mod4Mask;
            qt_mode_switch_remove_mask = 0;
        }

        // Misc. initialization
#if 0 //disabled for now..
        QSegfaultHandler::initialize(priv->argv, priv->argc);
#endif
        QFont::initialize();
        QCursorData::initialize();
    }

    if(qt_is_gui_used) {
        qApp->setObjectName(appName);

        int screen;
        for (screen = 0; screen < X11->screenCount; ++screen) {
            XSelectInput(X11->display, QX11Info::appRootWindow(screen),
                         KeymapStateMask | EnterWindowMask | LeaveWindowMask | PropertyChangeMask);

#ifndef QT_NO_XRANDR
            if (X11->use_xrandr)
                XRRSelectInput(X11->display, QX11Info::appRootWindow(screen), True);
#endif // QT_NO_XRANDR
        }
    }

    setlocale(LC_ALL, "");                // use correct char set mapping
    setlocale(LC_NUMERIC, "C");        // make sprintf()/scanf() work

    if (qt_is_gui_used) {
        qt_set_input_encoding();

        qt_set_x11_resources(appFont, appFGCol, appBGCol, appBTNCol);

        // be smart about the size of the default font. most X servers have helvetica
        // 12 point available at 2 resolutions:
        //     75dpi (12 pixels) and 100dpi (17 pixels).
        // At 95 DPI, a 12 point font should be 16 pixels tall - in which case a 17
        // pixel font is a closer match than a 12 pixel font
        int ptsz = (X11->use_xrender
                    ? 9
                    : (int) (((QX11Info::appDpiY() >= 95 ? 17. : 12.) *
                              72. / (float) QX11Info::appDpiY()) + 0.5));

        if (!qt_app_has_font) {
            QFont f(X11->has_fontconfig ? "Sans Serif" : "Helvetica", ptsz);
            QApplication::setFont(f);
        }

#if !defined (QT_NO_TABLET_SUPPORT)
        if (isXInputSupported(X11->display)) {
            int ndev,
                i,
                j;
            bool gotStylus,
                gotEraser;
            XDeviceInfo *devices, *devs;
            XInputClassInfo *ip;
            XAnyClassPtr any;
            XValuatorInfoPtr v;
            XAxisInfoPtr a;
            XDevice *dev;

#if !defined(Q_OS_IRIX)
            // XFree86 divides a stylus and eraser into 2 devices, so we must do for both...
            const QString XFREENAMESTYLUS = QLatin1String("stylus");
            const QString XFREENAMEPEN = QLatin1String("pen");
            const QString XFREENAMEERASER = QLatin1String("eraser");
#endif

            devices = XListInputDevices(X11->display, &ndev);
            if (!devices) {
                qWarning("Failed to get list of devices");
                ndev = -1;
            }
            QTabletEvent::TabletDevice deviceType;
            dev = 0;
            for (devs = devices, i = 0; i < ndev; i++, devs++) {
                gotStylus = false;
                gotEraser = false;

                QString devName = devs->name;
                devName = devName.toLower();
#if defined(Q_OS_IRIX)
                if (devName == QLatin1String(WACOM_NAME)) {
                    deviceType = QTabletEvent::Stylus;
                    gotStylus = true;
                }
#else
                if (devName.startsWith(XFREENAMEPEN)
                    || devName.startsWith(XFREENAMESTYLUS)) {
                    deviceType = QTabletEvent::Stylus;
                    gotStylus = true;
                } else if (devName.startsWith(XFREENAMEERASER)) {
                    deviceType = QTabletEvent::XFreeEraser;
                    gotEraser = true;
                }
#endif

                if (gotStylus || gotEraser) {
                    dev = XOpenDevice(X11->display, devs->id);

                    if (!dev) {
                        qWarning("Failed to open device");
                        continue;
                    }

                    TabletDeviceData device_data;
                    device_data.deviceType = deviceType;
                    device_data.eventCount = 0;
                    device_data.device = dev;
                    device_data.xinput_motion = -1;
                    device_data.xinput_key_press = -1;
                    device_data.xinput_key_release = -1;
                    device_data.xinput_button_press = -1;
                    device_data.xinput_button_release = -1;

                    if (dev->num_classes > 0) {
                        for (ip = dev->classes, j = 0; j < devs->num_classes;
                             ip++, j++) {
                            switch (ip->input_class) {
                            case KeyClass:
                                DeviceKeyPress(dev, device_data.xinput_key_press,
                                               device_data.eventList[device_data.eventCount]);
                                ++device_data.eventCount;
                                DeviceKeyRelease(dev, device_data.xinput_key_release,
                                                 device_data.eventList[device_data.eventCount]);
                                ++device_data.eventCount;
                                break;
                            case ButtonClass:
                                DeviceButtonPress(dev, device_data.xinput_button_press,
                                                  device_data.eventList[device_data.eventCount]);
                                ++device_data.eventCount;
                                DeviceButtonRelease(dev, device_data.xinput_button_release,
                                                    device_data.eventList[device_data.eventCount]);
                                ++device_data.eventCount;
                                break;
                            case ValuatorClass:
                                // I'm only going to be interested in motion when the
                                // stylus is already down anyway!
                                DeviceMotionNotify(dev, device_data.xinput_motion,
                                                   device_data.eventList[device_data.eventCount]);
                                ++device_data.eventCount;
                                break;
                            default:
                                break;
                            }
                        }
                    }

                    // get the min/max value for pressure!
                    any = (XAnyClassPtr) (devs->inputclassinfo);
                    for (j = 0; j < devs->num_classes; j++) {
                        if (any->c_class == ValuatorClass) {
                            v = (XValuatorInfoPtr) any;
                            a = (XAxisInfoPtr) ((char *) v +
                                                sizeof (XValuatorInfo));
#if defined (Q_OS_IRIX)
                            // I'm not exaclty wild about this, but the
                            // dimensions of the tablet are more relevant here
                            // than the min and max values from the axis
                            // (actually it seems to be 2/3 or what is in the
                            // axis.  So we'll try to parse it from this
                            // string. --tws
                            char returnString[SGIDeviceRtrnLen];
                            int tmp;
                            if (XSGIMiscQueryExtension(X11->display, &tmp, &tmp)
                                && XSGIDeviceQuery(X11->display, devs->id,
                                                   "dimensions", returnString)) {
                                QString str = QLatin1String(returnString);
                                int comma = str.indexOf(',');
                                device_data.minX = 0;
                                device_data.minY = 0;
                                device_data.maxX = str.left(comma).toInt();
                                device_data.maxY = str.mid(comma + 1).toInt();
                            } else {
                                device_data.minX = a[WAC_XCOORD_I].min_value;
                                device_data.maxX = a[WAC_XCOORD_I].max_value;
                                device_data.minY = a[WAC_YCOORD_I].min_value;
                                device_data.maxY = a[WAC_YCOORD_I].max_value;
                            }
                            device_data.minPressure = a[WAC_PRESSURE_I].min_value;
                            device_data.maxPressure = a[WAC_PRESSURE_I].max_value;
                            device_data.minTanPressure = a[WAC_TAN_PRESSURE_I].min_value;
                            device_data.maxTanPressure = a[WAC_TAN_PRESSURE_I].max_value;
                            device_data.minZ = a[WAC_ZCOORD_I].min_value;
                            device_data.maxZ = a[WAC_ZCOORD_I].max_value;
#else
                            device_data.minX = a[0].min_value;
                            device_data.maxX = a[0].max_value;
                            device_data.minY = a[1].min_value;
                            device_data.maxY = a[1].max_value;
                            device_data.minPressure = a[2].min_value;
                            device_data.maxPressure = a[2].max_value;
                            device_data.minTanPressure = 0;
                            device_data.maxTanPressure = 0;
                            device_data.minZ = 0;
                            device_data.maxZ = 0;
#endif

                            // got the max pressure no need to go further...
                            break;
                        }
                        any = (XAnyClassPtr) ((char *) any + any->length);
                    } // end of for loop

                    tablet_devices()->append(device_data);
                } // if (gotStylus || gotEraser)
            }
            XFreeDeviceList(devices);
        }
#endif // QT_NO_TABLET_SUPPORT
       
        X11->startupId = getenv("DESKTOP_STARTUP_ID");
        putenv(strdup("DESKTOP_STARTUP_ID="));

    } else {
        // read some non-GUI settings when not using the X server...

        if (QApplication::desktopSettingsAware()) {
            QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
            settings.beginGroup(QLatin1String("Qt"));

            // read library (ie. plugin) path list
            QString libpathkey = QString(QLatin1String("%1.%2/libraryPath"))
                                 .arg(QT_VERSION >> 16)
                                 .arg((QT_VERSION & 0xff00) >> 8);
            QStringList pathlist =
                settings.value(libpathkey).toString().split(QLatin1Char(':'));
            if (! pathlist.isEmpty()) {
                QStringList::ConstIterator it = pathlist.begin();
                while (it != pathlist.end())
                    QApplication::addLibraryPath(*it++);
            }

            QString defaultcodec = settings.value(QLatin1String("defaultCodec"),
                                                  QVariant(QLatin1String("none"))).toString();
            if (defaultcodec != QLatin1String("none")) {
                QTextCodec *codec = QTextCodec::codecForName(defaultcodec.toLatin1());
                if (codec)
                    QTextCodec::setCodecForTr(codec);
            }

            settings.endGroup(); // Qt
        }
    }
}


    // run-time search for default style
/*!
    \internal
*/
void QApplicationPrivate::x11_initialize_style()
{
    Atom type;
    int format;
    unsigned long length, after;
    uchar *data;
    if (!QApplicationPrivate::app_style &&
         XGetWindowProperty(X11->display, QX11Info::appRootWindow(), ATOM(KWIN_RUNNING),
                             0, 1, False, AnyPropertyType, &type, &format, &length,
                             &after, &data) == Success && length) {
        if (data) XFree((char *)data);
        // kwin is there. check if KDE's styles are available,
        // otherwise use windows style
        QApplicationPrivate::app_style = QStyleFactory::create("plastique");
    }
    if (!QApplicationPrivate::app_style &&
         XGetWindowProperty(X11->display, QX11Info::appRootWindow(), ATOM(KWM_RUNNING),
                             0, 1, False, AnyPropertyType, &type, &format, &length,
                             &after, &data) == Success && length) {
        if (data) XFree((char *)data);
        QApplicationPrivate::app_style = QStyleFactory::create("plastique");
    }
    if (!QApplicationPrivate::app_style &&
         XGetWindowProperty(X11->display, QX11Info::appRootWindow(), ATOM(DTWM_IS_RUNNING),
                             0, 1, False, AnyPropertyType, &type, &format, &length,
                             &after, &data) == Success && length) {
        // DTWM is running, meaning most likely CDE is running...
        if (data) XFree((char *) data);
        QApplicationPrivate::app_style = QStyleFactory::create("cde");
    }
    // maybe another desktop?
    if (!QApplicationPrivate::app_style &&
         XGetWindowProperty(X11->display, QX11Info::appRootWindow(),
                             ATOM(GNOME_BACKGROUND_PROPERTIES), 0, 1, False, AnyPropertyType,
                             &type, &format, &length, &after, &data) == Success &&
         length) {
        if (data) XFree((char *)data);
        // default to MotifPlus with hovering
        QApplicationPrivate::app_style = QStyleFactory::create("plastique");
    }
}


/*****************************************************************************
  qt_cleanup() - cleans up when the application is finished
 *****************************************************************************/

void qt_cleanup()
{
    appliedstamp = 0;

    if (app_save_rootinfo)                        // root window must keep state
        qt_save_rootinfo();

    if (qt_is_gui_used) {
        QPixmapCache::clear();
        QCursorData::cleanup();
        QFont::cleanup();
        QColormap::cleanup();
    }

#ifndef QT_NO_XRENDER
    for (int i = 0; i < X11->solid_fill_count; ++i) {
        if (X11->solid_fills[i].picture)
            XRenderFreePicture(X11->display, X11->solid_fills[i].picture);
    }
    for (int i = 0; i < X11->pattern_fill_count; ++i) {
        if (X11->pattern_fills[i].picture)
            XRenderFreePicture(X11->display, X11->pattern_fills[i].picture);
    }
#endif
#if !defined (QT_NO_TABLET_SUPPORT)
    TabletDeviceDataList *devices = qt_tablet_devices();
    for (int i = 0; i < devices->size(); ++i)
        XCloseDevice(X11->display, (XDevice*)devices->at(i).device);
#endif

#if !defined(QT_NO_IM)
    delete QApplicationPrivate::inputContext;
    QApplicationPrivate::inputContext = 0;
#endif

    // Reset the error handlers
    XSetErrorHandler(original_x_errhandler);
    XSetIOErrorHandler(original_xio_errhandler);

    if (qt_is_gui_used && !X11->foreignDisplay)
        XCloseDisplay(X11->display);                // close X display
    X11->display = 0;

    delete [] X11->screens;

    if (X11->foreignDisplay) {
        delete [] (char *)appName;
        appName = 0;
    }

    delete [] (char *)appClass;
    appClass = 0;

    if (X11->net_supported_list)
        delete [] X11->net_supported_list;
    X11->net_supported_list = 0;

    if (X11->net_virtual_root_list)
        delete [] X11->net_virtual_root_list;
    X11->net_virtual_root_list = 0;

    delete X11;
    X11 = 0;
}


/*****************************************************************************
  Platform specific global and internal functions
 *****************************************************************************/

void qt_save_rootinfo()                                // save new root info
{
    Atom type;
    int format;
    unsigned long length, after;
    uchar *data;

    if (ATOM(_XSETROOT_ID)) {                        // kill old pixmap
        if (XGetWindowProperty(X11->display, QX11Info::appRootWindow(),
                                 ATOM(_XSETROOT_ID), 0, 1,
                                 True, AnyPropertyType, &type, &format,
                                 &length, &after, &data) == Success) {
            if (type == XA_PIXMAP && format == 32 && length == 1 &&
                 after == 0 && data) {
                XKillClient(X11->display, *((Pixmap*)data));
            }
            Pixmap dummy = XCreatePixmap(X11->display, QX11Info::appRootWindow(),
                                          1, 1, 1);
            XChangeProperty(X11->display, QX11Info::appRootWindow(),
                             ATOM(_XSETROOT_ID), XA_PIXMAP, 32,
                             PropModeReplace, (uchar *)&dummy, 1);
            XSetCloseDownMode(X11->display, RetainPermanent);
        }
    }
    if (data)
        XFree((char *)data);
}

void qt_updated_rootinfo()
{
    app_save_rootinfo = true;
}

bool qt_wstate_iconified(WId winid)
{
    Atom type;
    int format;
    unsigned long length, after;
    uchar *data;
    int r = XGetWindowProperty(X11->display, winid, ATOM(WM_STATE), 0, 2,
                                 False, AnyPropertyType, &type, &format,
                                 &length, &after, &data);
    bool iconic = false;
    if (r == Success && data && format == 32) {
        // quint32 *wstate = (quint32*)data;
        unsigned long *wstate = (unsigned long *) data;
        iconic = (*wstate == IconicState);
        XFree((char *)data);
    }
    return iconic;
}

QString qAppName()                                // get application name
{
    return QString::fromLatin1(appName);
}

const char *QX11Info::appClass()                                // get application class
{
    return ::appClass;
}

bool qt_nograb()                                // application no-grab option
{
#if defined(QT_DEBUG)
    return appNoGrab;
#else
    return false;
#endif
}


/*****************************************************************************
  Platform specific QApplication members
 *****************************************************************************/

/*!
    \fn QWidget *QApplication::mainWidget()

    Returns the main application widget, or 0 if there is no main
    widget.

    \sa setMainWidget()
*/

/*!
    Sets the application's main widget to \a mainWidget.

    In most respects the main widget is like any other widget, except
    that if it is closed, the application exits. Note that
    QApplication does \e not take ownership of the \a mainWidget, so
    if you create your main widget on the heap you must delete it
    yourself.

    You need not have a main widget; connecting lastWindowClosed() to
    quit() is an alternative.

    For X11, this function also resizes and moves the main widget
    according to the \e -geometry command-line option, so you should
    set the default geometry (using \l QWidget::setGeometry()) before
    calling setMainWidget().

    \sa mainWidget(), exec(), quit()
*/

#ifdef QT3_SUPPORT
void QApplication::setMainWidget(QWidget *mainWidget)
{
#ifndef QT_NO_DEBUG
    if (mainWidget && mainWidget->parentWidget() && mainWidget->isWindow())
        qWarning("QApplication::setMainWidget(): New main widget (%s/%s) "
                  "has a parent!",
                  mainWidget->metaObject()->className(), mainWidget->objectName().toLocal8Bit().constData());
#endif
    QApplicationPrivate::main_widget = mainWidget;
    if (QApplicationPrivate::main_widget) // give WM command line
        QApplicationPrivate::applyX11SpecificCommandLineArguments(QApplicationPrivate::main_widget);
}
#endif

void QApplicationPrivate::applyX11SpecificCommandLineArguments(QWidget *main_widget)
{
    static bool beenHereDoneThat = false;
    if (beenHereDoneThat)
        return;
    beenHereDoneThat = true;
    XSetWMProperties(X11->display, main_widget->winId(), 0, 0, qApp->d_func()->argv, qApp->d_func()->argc, 0, 0, 0);
    if (mwTitle) {
        XStoreName(X11->display, main_widget->winId(), (char*)mwTitle);
        QByteArray net_wm_name = QString::fromLocal8Bit(mwTitle).toUtf8();
        XChangeProperty(X11->display, main_widget->winId(), ATOM(_NET_WM_NAME), ATOM(UTF8_STRING), 8,
                        PropModeReplace, (unsigned char *)net_wm_name.data(), net_wm_name.size());
    }
    if (mwGeometry) { // parse geometry
        int x, y;
        int w, h;
        int m = XParseGeometry((char*)mwGeometry, &x, &y, (uint*)&w, (uint*)&h);
        QSize minSize = main_widget->minimumSize();
        QSize maxSize = main_widget->maximumSize();
        if ((m & XValue) == 0)
            x = main_widget->geometry().x();
        if ((m & YValue) == 0)
            y = main_widget->geometry().y();
        if ((m & WidthValue) == 0)
            w = main_widget->width();
        if ((m & HeightValue) == 0)
            h = main_widget->height();
        w = qMin(w,maxSize.width());
        h = qMin(h,maxSize.height());
        w = qMax(w,minSize.width());
        h = qMax(h,minSize.height());
        if ((m & XNegative)) {
            x = QApplication::desktop()->width()  + x - w;
        }
        if ((m & YNegative)) {
            y = QApplication::desktop()->height() + y - h;
        }
        main_widget->setGeometry(x, y, w, h);
    }
}

#ifndef QT_NO_CURSOR

/*****************************************************************************
  QApplication cursor stack
 *****************************************************************************/

extern void qt_x11_enforce_cursor(QWidget * w);

/*!
    Sets the application override cursor to \a cursor.

    Application override cursors are intended for showing the user
    that the application is in a special state, for example during an
    operation that might take some time.

    This cursor will be displayed in all the application's widgets
    until restoreOverrideCursor() or another setOverrideCursor() is
    called.

    Application cursors are stored on an internal stack.
    setOverrideCursor() pushes the cursor onto the stack, and
    restoreOverrideCursor() pops the active cursor off the
    stack. changeOverrideCursor() changes the curently active
    application override cursor. Every setOverrideCursor() must
    eventually be followed by a corresponding restoreOverrideCursor(),
    otherwise the stack will never be emptied.

    Example:
    \code
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        calculateHugeMandelbrot();              // lunch time...
        QApplication::restoreOverrideCursor();
    \endcode

    \sa overrideCursor() restoreOverrideCursor() changeOverrideCursor() QWidget::setCursor()
*/

void QApplication::setOverrideCursor(const QCursor &cursor)
{
    qApp->d_func()->cursor_list.prepend(cursor);

    for (QWidgetMapper::ConstIterator it = QWidgetPrivate::mapper->constBegin(); it != QWidgetPrivate::mapper->constEnd(); ++it) {
        register QWidget *w = *it;
        if (w->testAttribute(Qt::WA_SetCursor))
            qt_x11_enforce_cursor(w);
    }
    XFlush(X11->display);                                // make X execute it NOW
}

/*!
    Undoes the last setOverrideCursor().

    If setOverrideCursor() has been called twice, calling
    restoreOverrideCursor() will activate the first cursor set.
    Calling this function a second time restores the original widgets'
    cursors.

    \sa setOverrideCursor(), overrideCursor()
*/

void QApplication::restoreOverrideCursor()
{
    if (qApp->d_func()->cursor_list.isEmpty())
        return;
    qApp->d_func()->cursor_list.removeFirst();

    if (QWidgetPrivate::mapper != 0 && !closingDown()) {
        for (QWidgetMapper::ConstIterator it = QWidgetPrivate::mapper->constBegin();
             it != QWidgetPrivate::mapper->constEnd(); ++it) {
            register QWidget *w = *it;
            if (w->testAttribute(Qt::WA_SetCursor))
                qt_x11_enforce_cursor(w);
        }
        XFlush(X11->display);
    }
}

#endif


/*****************************************************************************
  Routines to find a Qt widget from a screen position
 *****************************************************************************/

Window QX11Data::findClientWindow(Window win, Atom property, bool leaf)
{
    Atom   type = XNone;
    int           format, i;
    ulong  nitems, after;
    uchar *data;
    Window root, parent, target=0, *children=0;
    uint   nchildren;
    if (XGetWindowProperty(X11->display, win, property, 0, 0, false, AnyPropertyType,
                             &type, &format, &nitems, &after, &data) == Success) {
        if (data)
            XFree((char *)data);
        if (type)
            return win;
    }
    if (!XQueryTree(X11->display,win,&root,&parent,&children,&nchildren)) {
        if (children)
            XFree((char *)children);
        return 0;
    }
    for (i=nchildren-1; !target && i >= 0; i--)
        target = X11->findClientWindow(children[i], property, leaf);
    if (children)
        XFree((char *)children);
    return target;
}

QWidget *QApplication::topLevelAt(const QPoint &p)
{
    QWidget *c = QApplicationPrivate::widgetAt_sys(p.x(), p.y());
    return c ? c->window() : 0;
}

QWidget *QApplicationPrivate::widgetAt_sys(int x, int y)
{
    int screen = QCursor::x11Screen();
    int unused;

    Window target;
    if (!XTranslateCoordinates(X11->display,
                               QX11Info::appRootWindow(screen),
                               QX11Info::appRootWindow(screen),
                               x, y, &unused, &unused, &target)) {
        return 0;
    }
    if (!target || target == QX11Info::appRootWindow(screen))
        return 0;
    QWidget *w, *c;
    w = QWidget::find((WId)target);

    if (!w) {
        X11->ignoreBadwindow();
        target = X11->findClientWindow(target, ATOM(WM_STATE), true);
        if (X11->badwindow())
            return 0;
        w = QWidget::find((WId)target);
        if (!w) {
            // Perhaps the widget at (x,y) is inside a foreign application?
            // Search all toplevel widgets to see if one is within target
            QWidgetList list = QApplication::topLevelWidgets();
            for (int i = 0; i < list.count(); ++i) {
                QWidget *widget = list.at(i);
                Window ctarget = target;
                if (widget->isVisible() && !(widget->windowType() == Qt::Desktop)) {
                    Window wid = widget->winId();
                    while (ctarget && !w) {
                        XTranslateCoordinates(X11->display,
                                              QX11Info::appRootWindow(screen),
                                              ctarget, x, y, &unused, &unused, &ctarget);
                        if (ctarget == wid) {
                            // Found!
                            w = widget;
                            break;
                        }
                    }
                }
                if (w)
                    break;
            }
        }
    }
    if (w && (c = w->childAt(w->mapFromGlobal(QPoint(x, y)))))
        return c;
    return w;
}

/*!
    Synchronizes with the X server in the X11 implementation. This
    normally takes some time. Does nothing on other platforms.
*/

void QApplication::syncX()
{
    if (X11->display)
        XSync(X11->display, False);                        // don't discard events
}


/*!
    Sounds the bell, using the default volume and sound.
*/

void QApplication::beep()
{
    if (X11->display)
        XBell(X11->display, 0);
}



/*****************************************************************************
  Special lookup functions for windows that have been reparented recently
 *****************************************************************************/

static QWidgetMapper *wPRmapper = 0;                // alternative widget mapper

void qPRCreate(const QWidget *widget, Window oldwin)
{                                                // QWidget::reparent mechanism
    if (!wPRmapper)
        wPRmapper = new QWidgetMapper;

    QETWidget *w = static_cast<QETWidget *>(const_cast<QWidget *>(widget));
    wPRmapper->insert((int)oldwin, w);        // add old window to mapper
    w->setAttribute(Qt::WA_WState_Reparented);        // set reparented flag
}

void qPRCleanup(QWidget *widget)
{
    QETWidget *etw = static_cast<QETWidget *>(const_cast<QWidget *>(widget));
    if (!(wPRmapper && widget->testAttribute(Qt::WA_WState_Reparented)))
        return;                                        // not a reparented widget
    for (QWidgetMapper::ConstIterator it = wPRmapper->constBegin(); it != wPRmapper->constEnd(); ++it) {
        QWidget *w = *it;
        int key = it.key();
        if (w == etw) {                       // found widget
            etw->setAttribute(Qt::WA_WState_Reparented, false); // clear flag
            wPRmapper->remove(key);// old window no longer needed
            if (wPRmapper->size() == 0) {        // became empty
                delete wPRmapper;                // then reset alt mapper
                wPRmapper = 0;
            }
            return;
        }
    }
}

static QETWidget *qPRFindWidget(Window oldwin)
{
    return wPRmapper ? (QETWidget*)wPRmapper->value((int)oldwin, 0) : 0;
}

/*!
    \internal
*/
int QApplication::x11ClientMessage(QWidget* w, XEvent* event, bool passive_only)
{
    QETWidget *widget = (QETWidget*)w;
    if (event->xclient.format == 32 && event->xclient.message_type) {
        if (event->xclient.message_type == ATOM(WM_PROTOCOLS)) {
            Atom a = event->xclient.data.l[0];
            if (a == ATOM(WM_DELETE_WINDOW)) {
                if (passive_only) return 0;
                widget->translateCloseEvent(event);
            }
            else if (a == ATOM(WM_TAKE_FOCUS)) {
                QWidget * amw = activeModalWidget();
                if ((ulong) event->xclient.data.l[1] > X11->time)
                    X11->time = event->xclient.data.l[1];
                if (amw && amw != widget) {
                    QWidget* groupLeader = widget;
                    while (groupLeader && !groupLeader->testAttribute(Qt::WA_GroupLeader)
                           && groupLeader != amw)
                        groupLeader = groupLeader->parentWidget();
                    if (!groupLeader) {
                        QWidget *p = amw->parentWidget();
                        while (p && p != widget)
                            p = p->parentWidget();
                        if (!p || !X11->net_supported_list)
                            amw->raise(); // help broken window managers
                        amw->activateWindow();
                    }
                }
#ifndef QT_NO_WHATSTHIS
            } else if (a == ATOM(_NET_WM_CONTEXT_HELP)) {
                QWhatsThis::enterWhatsThisMode();
#endif // QT_NO_WHATSTHIS
            } else if (a == ATOM(_NET_WM_PING)) {
                // avoid send/reply loops
                Window root = RootWindow(X11->display, w->x11Info().screen());
                if (event->xclient.window != root) {
                    event->xclient.window = root;
                    XSendEvent(event->xclient.display, event->xclient.window,
                                False, SubstructureNotifyMask|SubstructureRedirectMask, event);
                }
            }
        } else if (event->xclient.message_type == ATOM(_QT_SCROLL_DONE)) {
            widget->translateScrollDoneEvent(event);
        } else if (event->xclient.message_type == ATOM(XdndPosition)) {
            X11->xdndHandlePosition(widget, event, passive_only);
        } else if (event->xclient.message_type == ATOM(XdndEnter)) {
            X11->xdndHandleEnter(widget, event, passive_only);
        } else if (event->xclient.message_type == ATOM(XdndStatus)) {
            X11->xdndHandleStatus(widget, event, passive_only);
        } else if (event->xclient.message_type == ATOM(XdndLeave)) {
            X11->xdndHandleLeave(widget, event, passive_only);
        } else if (event->xclient.message_type == ATOM(XdndDrop)) {
            X11->xdndHandleDrop(widget, event, passive_only);
        } else if (event->xclient.message_type == ATOM(XdndFinished)) {
            X11->xdndHandleFinished(widget, event, passive_only);
        } else {
            if (passive_only) return 0;
            // All other are interactions
        }
    } else {
        X11->motifdndHandle(widget, event, passive_only);
    }

    return 0;
}

/*!
    This function does the core processing of individual X
    \a{event}s, normally by dispatching Qt events to the right
    destination.

    It returns 1 if the event was consumed by special handling, 0 if
    the \a event was consumed by normal handling, and -1 if the \a
    event was for an unrecognized widget.

    \sa x11EventFilter()
*/
int QApplication::x11ProcessEvent(XEvent* event)
{
    Q_D(QApplication);
    switch (event->type) {
    case ButtonPress:
        pressed_window = event->xbutton.window;
        X11->userTime = event->xbutton.time;
        // fallthrough intended
    case ButtonRelease:
        X11->time = event->xbutton.time;
        break;
    case MotionNotify:
        X11->time = event->xmotion.time;
        break;
    case XKeyPress:
        X11->userTime = event->xkey.time;
        // fallthrough intended
    case XKeyRelease:
        X11->time = event->xkey.time;
        break;
    case PropertyNotify:
        X11->time = event->xproperty.time;
        break;
    case EnterNotify:
    case LeaveNotify:
        X11->time = event->xcrossing.time;
        break;
    case SelectionClear:
        X11->time = event->xselectionclear.time;
        break;
    default:
        break;
    }

    QETWidget *widget = (QETWidget*)QWidget::find((WId)event->xany.window);

    if (wPRmapper) {                                // just did a widget reparent?
        if (widget == 0) {                        // not in std widget mapper
            switch (event->type) {                // only for mouse/key events
            case ButtonPress:
            case ButtonRelease:
            case MotionNotify:
            case XKeyPress:
            case XKeyRelease:
                widget = qPRFindWidget(event->xany.window);
                break;
            }
        }
        else if (widget->testAttribute(Qt::WA_WState_Reparented))
            qPRCleanup(widget);                // remove from alt mapper
    }

    QETWidget *keywidget=0;
    bool grabbed=false;
    if (event->type==XKeyPress || event->type==XKeyRelease) {
        keywidget = (QETWidget*)QWidget::keyboardGrabber();
        if (keywidget) {
            grabbed = true;
        } else if (!keywidget) {
            if (d->inPopupMode()) // no focus widget, see if we have a popup
                keywidget = (QETWidget*) (activePopupWidget()->focusWidget() ? activePopupWidget()->focusWidget() : activePopupWidget());
            else if (QApplicationPrivate::focus_widget)
                keywidget = (QETWidget*)QApplicationPrivate::focus_widget;
            else if (widget)
                keywidget = (QETWidget*)widget->window();
        }
    }

#ifndef QT_NO_IM
    // Filtering input events by the input context. It has to be taken
    // place before any other key event consumers such as eventfilters
    // and accelerators because some input methods require quite
    // various key combination and sequences. It often conflicts with
    // accelerators and so on, so we must give the input context the
    // filtering opportunity first to ensure all input methods work
    // properly regardless of application design.

    if(keywidget && keywidget->isEnabled() && keywidget->testAttribute(Qt::WA_InputMethodEnabled)) {
        // block user interaction during session management
	if((event->type==XKeyPress || event->type==XKeyRelease) && sm_blockUserInput)
	    return true;

        // for XIM handling
	QInputContext *qic = keywidget->inputContext();
	if(qic && qic->x11FilterEvent(keywidget, event))
	    return true;

	// filterEvent() accepts QEvent *event rather than preexpanded
	// key event attribute values. This is intended to pass other
	// QInputEvent in future. Other non IM-related events should
	// not be forwarded to input contexts to prevent weird event
	// handling.
	if ((event->type == XKeyPress || event->type == XKeyRelease)) {
	    int code = -1;
	    int count = 0;
	    Qt::KeyboardModifiers modifiers;
	    QEvent::Type type;
	    QString text;

	    keywidget->translateKeyEventInternal(event, count, text,
                                                 modifiers, code, type,
                                                 false, false);

	    // both key press/release is required for some complex
	    // input methods. don't eliminate anything.
	    QKeyEvent keyevent(type, code, modifiers, text, false, count);
	    if(qic && qic->filterEvent(&keyevent))
		return true;
	}
    } else
#endif // QT_NO_IM
    {
        if (XFilterEvent(event, XNone))
            return true;
    }

    if (qt_x11EventFilter(event))                // send through app filter
        return 1;

    if (!widget) {                                // don't know this windows
        QWidget* popup = QApplication::activePopupWidget();
        if (popup) {

            /*
              That is more than suboptimal. The real solution should
              do some keyevent and buttonevent translation, so that
              the popup still continues to work as the user expects.
              Unfortunately this translation is currently only
              possible with a known widget. I'll change that soon
              (Matthias).
            */

            // Danger - make sure we don't lock the server
            switch (event->type) {
            case ButtonPress:
            case ButtonRelease:
            case XKeyPress:
            case XKeyRelease:
                do {
                    popup->close();
                } while ((popup = qApp->activePopupWidget()));
                return 1;
            }
        }
        return -1;
    }

    if (event->type == XKeyPress || event->type == XKeyRelease)
        widget = keywidget; // send XKeyEvents through keywidget->x11Event()

    if (app_do_modal)                                // modal event handling
        if (!qt_try_modal(widget, event)) {
            if (event->type == ClientMessage)
                x11ClientMessage(widget, event, true);
            return 1;
        }


    if (widget->x11Event(event))                // send through widget filter
        return 1;
#if !defined (QT_NO_TABLET_SUPPORT)
    TabletDeviceDataList *tablets = qt_tablet_devices();
    for (int i = 0; i < tablets->size(); ++i) {
        const TabletDeviceData &tab = tablets->at(i);
        if (event->type == tab.xinput_motion ||
            event->type == tab.xinput_button_release ||
            event->type == tab.xinput_button_press) {
            widget->translateXinputEvent(event, &tab);
            return 0;
        }
    }
#endif

#ifndef QT_NO_XRANDR
    if (X11->use_xrandr && event->type == (X11->xrandr_eventbase + RRScreenChangeNotify)) {
        // update Xlib internals with the latest screen configuration
        XRRUpdateConfiguration(event);

        // update the size for desktop widget
        int scr = XRRRootToScreen(X11->display, event->xany.window);
        QWidget *w = desktop()->screen(scr);
        QSize oldSize(w->size());
        w->data->crect.setWidth(DisplayWidth(X11->display, scr));
        w->data->crect.setHeight(DisplayHeight(X11->display, scr));
        if (w->size() != oldSize) {
            QResizeEvent e(w->size(), oldSize);
            QApplication::sendEvent(w, &e);
            emit desktop()->resized(scr);
        }
    }
#endif // QT_NO_XRANDR

    switch (event->type) {

    case ButtonRelease:                        // mouse event
        if (!d->inPopupMode() && !QWidget::mouseGrabber() && pressed_window != widget->winId()
                && (widget = (QETWidget*) QWidget::find((WId)pressed_window)) == 0)
            break;
        // fall through intended
    case ButtonPress:
        if (event->xbutton.root != RootWindow(X11->display, widget->x11Info().screen())
            && ! qt_xdnd_dragging) {
            while (activePopupWidget())
                activePopupWidget()->close();
            return 1;
        }
        if (event->type == ButtonPress)
            qt_net_update_user_time(widget->window());
        // fall through intended
    case MotionNotify:
#if !defined(QT_NO_TABLET_SUPPORT)
        if (!qt_tabletChokeMouse) {
#endif
            widget->translateMouseEvent(event);
#if !defined(QT_NO_TABLET_SUPPORT)
        } else {
            qt_tabletChokeMouse = false;
        }
#endif
        break;

    case XKeyPress:                                // keyboard event
        qt_net_update_user_time(widget->window());
        // fallthrough intended
    case XKeyRelease:
        {
            if (keywidget && keywidget->isEnabled()) { // should always exist
                // qDebug("sending key event");
                keywidget->translateKeyEvent(event, grabbed);
            }
            break;
        }

    case GraphicsExpose:
    case Expose:                                // paint event
        widget->translatePaintEvent(event);
        break;

    case ConfigureNotify:                        // window move/resize event
        if (event->xconfigure.event == event->xconfigure.window)
            widget->translateConfigEvent(event);
        break;

    case XFocusIn: {                                // got focus
        if ((widget->windowType() == Qt::Desktop))
            break;
        if (d->inPopupMode()) // some delayed focus event to ignore
            break;
        if (!widget->isWindow())
            break;
        if (event->xfocus.detail != NotifyAncestor &&
            event->xfocus.detail != NotifyInferior &&
            event->xfocus.detail != NotifyNonlinear)
            break;
        setActiveWindow(widget);
        if (X11->focus_model == QX11Data::FM_PointerRoot) {
            // We got real input focus from somewhere, but we were in PointerRoot
            // mode, so we don't trust this event.  Check the focus model to make
            // sure we know what focus mode we are using...
            qt_check_focus_model();
        }
    }
        break;

    case XFocusOut:                                // lost focus
        if ((widget->windowType() == Qt::Desktop))
            break;
        if (!widget->isWindow())
            break;
        if (event->xfocus.mode == NotifyGrab)
            qt_xfocusout_grab_counter++;
        if (event->xfocus.mode != NotifyNormal)
            break;
        if (event->xfocus.detail != NotifyAncestor &&
            event->xfocus.detail != NotifyNonlinearVirtual &&
            event->xfocus.detail != NotifyNonlinear)
            break;
        if (!d->inPopupMode() && widget == QApplicationPrivate::active_window)
            setActiveWindow(0);
        break;

    case EnterNotify: {                        // enter window
        if (QWidget::mouseGrabber()  && widget != QWidget::mouseGrabber())
            break;
        if (d->inPopupMode() && widget->window() != activePopupWidget())
            break;
        if (event->xcrossing.mode != NotifyNormal ||
            event->xcrossing.detail == NotifyVirtual  ||
            event->xcrossing.detail == NotifyNonlinearVirtual)
            break;
        if (event->xcrossing.focus &&
            !(widget->windowType() == Qt::Desktop) && !widget->isActiveWindow()) {
            if (X11->focus_model == QX11Data::FM_Unknown) // check focus model
                qt_check_focus_model();
            if (X11->focus_model == QX11Data::FM_PointerRoot) // PointerRoot mode
                setActiveWindow(widget);
        }
        QApplicationPrivate::dispatchEnterLeave(widget, QWidget::find(curWin));
        curWin = widget->winId();
        widget->translateMouseEvent(event); //we don't get MotionNotify, emulate it
    }
        break;

    case LeaveNotify: {                        // leave window
        if (QWidget::mouseGrabber()  && widget != QWidget::mouseGrabber())
            break;
        if (curWin && widget->winId() != curWin)
            break;
        if (event->xcrossing.mode != NotifyNormal)
            break;
        if (!(widget->windowType() == Qt::Desktop))
            widget->translateMouseEvent(event); //we don't get MotionNotify, emulate it

        QWidget* enter = 0;
        XEvent ev;
        while (XCheckMaskEvent(X11->display, EnterWindowMask | LeaveWindowMask , &ev)
               && !qt_x11EventFilter(&ev)) {
            QWidget* event_widget = QWidget::find(ev.xcrossing.window);
            if(event_widget && event_widget->x11Event(&ev))
                break;
            if (ev.type == LeaveNotify && ev.xcrossing.mode == NotifyNormal){
                enter = event_widget;
                XPutBackEvent(X11->display, &ev);
                break;
            }
            if (ev.xcrossing.mode != NotifyNormal ||
                ev.xcrossing.detail == NotifyVirtual  ||
                ev.xcrossing.detail == NotifyNonlinearVirtual)
                continue;
            enter = event_widget;
            if (ev.xcrossing.focus &&
                enter && !(enter->windowType() == Qt::Desktop) && !enter->isActiveWindow()) {
                if (X11->focus_model == QX11Data::FM_Unknown) // check focus model
                    qt_check_focus_model();
                if (X11->focus_model == QX11Data::FM_PointerRoot) // PointerRoot mode
                    setActiveWindow(enter);
            }
            break;
        }

        if ((! enter || (enter->windowType() == Qt::Desktop)) &&
            event->xcrossing.focus && widget == QApplicationPrivate::active_window &&
            X11->focus_model == QX11Data::FM_PointerRoot // PointerRoot mode
            ) {
            setActiveWindow(0);
        }

        if (!curWin)
            QApplicationPrivate::dispatchEnterLeave(widget, 0);

        QApplicationPrivate::dispatchEnterLeave(enter, widget);
        curWin = enter ? enter->winId() : 0;
    }
        break;

    case UnmapNotify:                                // window hidden
        if (widget->isWindow() && !(widget->windowType() == Qt::Popup)) {
            widget->setAttribute(Qt::WA_Mapped, false);
            if (widget->isVisible()) {
                widget->d_func()->topData()->spont_unmapped = 1;
                QHideEvent e;
                QApplication::sendSpontaneousEvent(widget, &e);
                widget->d_func()->hideChildren(true);
            }
        }
        break;

    case MapNotify:                                // window shown
        if (widget->isWindow() && !(widget->windowType() == Qt::Popup)) {
            widget->setAttribute(Qt::WA_Mapped);
            if (widget->d_func()->topData()->spont_unmapped) {
                widget->d_func()->topData()->spont_unmapped = 0;
                widget->d_func()->showChildren(true);
                QShowEvent e;
                QApplication::sendSpontaneousEvent(widget, &e);
            }
        }
        break;

    case ClientMessage:                        // client message
        return x11ClientMessage(widget,event,False);

    case ReparentNotify:                        // window manager reparents
        while (XCheckTypedWindowEvent(X11->display,
                                      widget->winId(),
                                      ReparentNotify,
                                      event))
            ;        // skip old reparent events
        if (event->xreparent.parent == QX11Info::appRootWindow()) {
            if (widget->isWindow()) {
                widget->d_func()->topData()->parentWinId = event->xreparent.parent;
                int idx = X11->deferred_map.indexOf(widget);
                if (idx != -1) {
                    X11->deferred_map.removeAt(idx);
                    XMapWindow(X11->display, widget->winId());
                }
            }
        } else
            // store the parent. Useful for many things, embedding for instance.
            widget->d_func()->topData()->parentWinId = event->xreparent.parent;
        if (widget->isWindow()) {
            // the widget frame strut should also be invalidated
            widget->d_func()->topData()->fleft = widget->d_func()->topData()->fright =
             widget->d_func()->topData()->ftop = widget->d_func()->topData()->fbottom = 0;

            if (X11->focus_model != QX11Data::FM_Unknown) {
                // toplevel reparented...
                QWidget *newparent = QWidget::find(event->xreparent.parent);
                if (! newparent || (newparent->windowType() == Qt::Desktop)) {
                    // we dont' know about the new parent (or we've been
                    // reparented to root), perhaps a window manager
                    // has been (re)started?  reset the focus model to unknown
                    X11->focus_model = QX11Data::FM_Unknown;
                }
            }
        }
        break;

    case SelectionRequest: {
        XSelectionRequestEvent *req = &event->xselectionrequest;
        if (! req)
            break;

        if (ATOM(XdndSelection) && req->selection == ATOM(XdndSelection)) {
            X11->xdndHandleSelectionRequest(req);

        } else if (qt_clipboard) {
            QClipboardEvent e(reinterpret_cast<QEventPrivate*>(event));
            QApplication::sendSpontaneousEvent(qt_clipboard, &e);
        }
        break;
    }
    case SelectionClear: {
        XSelectionClearEvent *req = &event->xselectionclear;
        // don't deliver dnd events to the clipboard, it gets confused
        if (! req || ATOM(XdndSelection) && req->selection == ATOM(XdndSelection))
            break;

        if (qt_clipboard) {
            QClipboardEvent e(reinterpret_cast<QEventPrivate*>(event));
            QApplication::sendSpontaneousEvent(qt_clipboard, &e);
        }
        break;
    }

    case SelectionNotify: {
        XSelectionEvent *req = &event->xselection;
        // don't deliver dnd events to the clipboard, it gets confused
        if (! req || ATOM(XdndSelection) && req->selection == ATOM(XdndSelection))
            break;

        if (qt_clipboard) {
            QClipboardEvent e(reinterpret_cast<QEventPrivate*>(event));
            QApplication::sendSpontaneousEvent(qt_clipboard, &e);
        }
        break;
    }

    case MappingNotify:
        // keyboard mapping changed
        XRefreshKeyboardMapping(&event->xmapping);
        break;

    case PropertyNotify:
        // some properties changed
        if (event->xproperty.window == QX11Info::appRootWindow(0)) {
            // root properties for the first screen
            if (event->xproperty.atom == ATOM(_QT_CLIPBOARD_SENTINEL)) {
                if (qt_check_clipboard_sentinel())
                    emit clipboard()->dataChanged();
            } else if (event->xproperty.atom == ATOM(_QT_SELECTION_SENTINEL)) {
                if (qt_check_selection_sentinel())
                    emit clipboard()->selectionChanged();
            } else if (QApplicationPrivate::obey_desktop_settings) {
                if (event->xproperty.atom == ATOM(RESOURCE_MANAGER))
                    qt_set_x11_resources();
                else if (event->xproperty.atom == ATOM(_QT_SETTINGS_TIMESTAMP))
                    QApplicationPrivate::x11_apply_settings();
            }
        }
        if (event->xproperty.window == QX11Info::appRootWindow()) {
            // root properties for the default screen
            if (event->xproperty.atom == ATOM(_QT_INPUT_ENCODING)) {
                qt_set_input_encoding();
            } else if (event->xproperty.atom == ATOM(_NET_SUPPORTED)) {
                qt_get_net_supported();
            } else if (event->xproperty.atom == ATOM(_NET_VIRTUAL_ROOTS)) {
                qt_get_net_virtual_roots();
            } else if (event->xproperty.atom == ATOM(_NET_WORKAREA)) {
                qt_desktopwidget_update_workarea();
            }
        } else if (widget) {
            widget->translatePropertyEvent(event);
        }  else {
            return -1; // don't know this window
        }
        break;

    default:
        break;
    }

    return 0;
}

/*!
    \fn bool QApplication::x11EventFilter(XEvent *event)

    \warning This virtual function is only implemented under X11.

    If you create an application that inherits QApplication and
    reimplement this function, you get direct access to all X events
    that the are received from the X server. The events are passed in
    the \a event parameter.

    Return true if you want to stop the event from being processed.
    Return false for normal event dispatching. THe default
    implementation returns false.

    \sa x11ProcessEvent()
*/

bool QApplication::x11EventFilter(XEvent *)
{
    return false;
}



/*****************************************************************************
  Modal widgets; Since Xlib has little support for this we roll our own
  modal widget mechanism.
  A modal widget without a parent becomes application-modal.
  A modal widget with a parent becomes modal to its parent and grandparents..

  QApplicationPrivate::enterModal()
        Enters modal state
        Arguments:
            QWidget *widget        A modal widget

  QApplicationPrivate::leaveModal()
        Leaves modal state for a widget
        Arguments:
            QWidget *widget        A modal widget
 *****************************************************************************/

bool QApplicationPrivate::modalState()
{
    return app_do_modal;
}

void QApplicationPrivate::enterModal(QWidget *widget)
{
    if (!qt_modal_stack) {                        // create modal stack
        qt_modal_stack = new QWidgetList;
    }

    QApplicationPrivate::dispatchEnterLeave(0, QWidget::find((WId)curWin));
    qt_modal_stack->insert(0, widget);
    app_do_modal = true;
    curWin = 0;

    if (widget->parentWidget()) {
        QEvent e(QEvent::WindowBlocked);
        QApplication::sendEvent(widget->parentWidget(), &e);
    }
}


void QApplicationPrivate::leaveModal(QWidget *widget)
{
    if (qt_modal_stack && qt_modal_stack->removeAll(widget)) {
        if (qt_modal_stack->isEmpty()) {
            delete qt_modal_stack;
            qt_modal_stack = 0;
            QPoint p(QCursor::pos());
            QWidget* w = QApplication::widgetAt(p.x(), p.y());
            QApplicationPrivate::dispatchEnterLeave(w, QWidget::find(curWin)); // send synthetic enter event
            curWin = w? w->winId() : 0;
        }
    }
    app_do_modal = qt_modal_stack != 0;

    if (widget->parentWidget()) {
        QEvent e(QEvent::WindowUnblocked);
        QApplication::sendEvent(widget->parentWidget(), &e);
    }
}


bool qt_try_modal(QWidget *widget, XEvent *event)
{
    if (qt_xdnd_dragging) {
        // allow mouse events while DnD is active
        switch (event->type) {
        case ButtonPress:
        case ButtonRelease:
        case MotionNotify:
            return true;
        default:
            break;
        }
    }

    // allow mouse release events to be sent to widgets that have been pressed
    if (event->type == ButtonRelease && widget == qt_button_down)
        return true;

    if (QApplicationPrivate::tryModalHelper(widget))
        return true;

    // disallow mouse/key events
    switch (event->type) {
    case ButtonPress:
    case ButtonRelease:
    case MotionNotify:
    case XKeyPress:
    case XKeyRelease:
    case EnterNotify:
    case LeaveNotify:
    case ClientMessage:
        return false;
    default:
        break;
    }

    return true;
}


/*****************************************************************************
  Popup widget mechanism

  openPopup()
        Adds a widget to the list of popup widgets
        Arguments:
            QWidget *widget        The popup widget to be added

  closePopup()
        Removes a widget from the list of popup widgets
        Arguments:
            QWidget *widget        The popup widget to be removed
 *****************************************************************************/


static int openPopupCount = 0;
void QApplicationPrivate::openPopup(QWidget *popup)
{
    Q_Q(QApplication);
    openPopupCount++;
    if (!QApplicationPrivate::popupWidgets) {                        // create list
        QApplicationPrivate::popupWidgets = new QWidgetList;
    }
    QApplicationPrivate::popupWidgets->append(popup);                // add to end of list
    Display *dpy = X11->display;
    if (QApplicationPrivate::popupWidgets->count() == 1 && !qt_nograb()){ // grab mouse/keyboard
        int r = XGrabKeyboard(dpy, popup->winId(), false,
                              GrabModeAsync, GrabModeAsync, X11->time);
        if ((popupGrabOk = (r == GrabSuccess))) {
            r = XGrabPointer(dpy, popup->winId(), true,
                             (ButtonPressMask | ButtonReleaseMask | ButtonMotionMask
                              | EnterWindowMask | LeaveWindowMask | PointerMotionMask),
                             GrabModeAsync, GrabModeAsync, XNone, XNone, X11->time);
            if (!(popupGrabOk = (r == GrabSuccess)))
                XUngrabKeyboard(dpy, X11->time);
        }
    }

    // popups are not focus-handled by the window system (the first
    // popup grabbed the keyboard), so we have to do that manually: A
    // new popup gets the focus
    if (popup->focusWidget()) {
        popup->focusWidget()->setFocus(Qt::PopupFocusReason);
    } else if (QApplicationPrivate::popupWidgets->count() == 1) { // this was the first popup
        if (QWidget *fw = QApplication::focusWidget()) {
            QFocusEvent e(QEvent::FocusOut, Qt::PopupFocusReason);
            q->sendEvent(fw, &e);
        }
    }
}

void QApplicationPrivate::closePopup(QWidget *popup)
{
    Q_Q(QApplication);
    if (!QApplicationPrivate::popupWidgets)
        return;
    QApplicationPrivate::popupWidgets->removeAll(popup);
    if (popup == qt_popup_down) {
        qt_button_down = 0;
        qt_popup_down = 0;
    }
    if (QApplicationPrivate::popupWidgets->count() == 0) {                // this was the last popup
        delete QApplicationPrivate::popupWidgets;
        QApplicationPrivate::popupWidgets = 0;
        if (!qt_nograb() && popupGrabOk) {        // grabbing not disabled
            Display *dpy = X11->display;
            if (popup->geometry().contains(QPoint(mouseGlobalXPos, mouseGlobalYPos))
                || popup->testAttribute(Qt::WA_NoMouseReplay)) {
                // mouse release event or inside
                replayPopupMouseEvent = false;
            } else {                                // mouse press event
                mouseButtonPressTime -= 10000;        // avoid double click
                replayPopupMouseEvent = true;
            }
            XUngrabPointer(dpy, X11->time);
            XUngrabKeyboard(dpy, X11->time);
            XFlush(dpy);
        }
        if (QApplicationPrivate::active_window) {
            if (QWidget *fw = QApplicationPrivate::active_window->focusWidget()) {
                if (fw != QApplication::focusWidget()) {
                    fw->setFocus(Qt::PopupFocusReason);
                } else {
                    QFocusEvent e(QEvent::FocusIn, Qt::PopupFocusReason);
                    q->sendEvent(fw, &e);
                }
            }
        }
    } else {
        // popups are not focus-handled by the window system (the
        // first popup grabbed the keyboard), so we have to do that
        // manually: A popup was closed, so the previous popup gets
        // the focus.
        QWidget* aw = QApplicationPrivate::popupWidgets->last();
        if (QWidget *fw = aw->focusWidget())
            fw->setFocus(Qt::PopupFocusReason);

        // regrab the keyboard and mouse in case 'popup' lost the grab
        if (QApplicationPrivate::popupWidgets->count() == 1 && !qt_nograb()){ // grab mouse/keyboard
            Display *dpy = X11->display;
            int r = XGrabKeyboard(dpy, aw->winId(), false,
                                  GrabModeAsync, GrabModeAsync, X11->time);
            if ((popupGrabOk = (r == GrabSuccess))) {
                r = XGrabPointer(dpy, aw->winId(), true,
                                 (ButtonPressMask | ButtonReleaseMask | ButtonMotionMask
                                  | EnterWindowMask | LeaveWindowMask | PointerMotionMask),
                                 GrabModeAsync, GrabModeAsync, XNone, XNone, X11->time);
                if (!(popupGrabOk = (r == GrabSuccess)))
                    XUngrabKeyboard(dpy, X11->time);
            }
        }
    }
}

/*****************************************************************************
  Event translation; translates X11 events to Qt events
 *****************************************************************************/

//
// Mouse event translation
//
// Xlib doesn't give mouse double click events, so we generate them by
// comparing window, time and position between two mouse press events.
//

static Qt::MouseButtons translateMouseButtons(int s)
{
    Qt::MouseButtons ret = 0;
    if (s & Button1Mask)
        ret |= Qt::LeftButton;
    if (s & Button2Mask)
        ret |= Qt::MidButton;
    if (s & Button3Mask)
        ret |= Qt::RightButton;
    return ret;
}

static Qt::KeyboardModifiers translateModifiers(int s)
{
    Qt::KeyboardModifiers ret = 0;
    if (s & ShiftMask)
        ret |= Qt::ShiftModifier;
    if (s & ControlMask)
        ret |= Qt::ControlModifier;
    if (s & qt_alt_mask)
        ret |= Qt::AltModifier;
    if (s & qt_meta_mask)
        ret |= Qt::MetaModifier;
    return ret;
}

bool QETWidget::translateMouseEvent(const XEvent *event)
{
    Q_D(QWidget);
    static bool manualGrab = false;
    QEvent::Type type;                                // event parameters
    QPoint pos;
    QPoint globalPos;
    Qt::MouseButton button = Qt::NoButton;
    Qt::MouseButtons buttons;
    Qt::KeyboardModifiers modifiers;
    XEvent nextEvent;

    if (sm_blockUserInput) // block user interaction during session management
        return true;

    if (event->type == MotionNotify) { // mouse move
        if (event->xmotion.root != RootWindow(X11->display, x11Info().screen()) &&
            ! qt_xdnd_dragging)
            return false;

        XMotionEvent lastMotion = event->xmotion;
        while(XPending(X11->display))  { // compress mouse moves
            XNextEvent(X11->display, &nextEvent);
            if (nextEvent.type == ConfigureNotify
                || nextEvent.type == PropertyNotify
                || nextEvent.type == Expose
                || nextEvent.type == GraphicsExpose
                || nextEvent.type == NoExpose
                || (nextEvent.type == ClientMessage
                    && nextEvent.xclient.message_type == ATOM(_QT_SCROLL_DONE))) {
                qApp->x11ProcessEvent(&nextEvent);
                continue;
            } else if (nextEvent.type != MotionNotify ||
                       nextEvent.xmotion.window != event->xmotion.window ||
                       nextEvent.xmotion.state != event->xmotion.state) {
                XPutBackEvent(X11->display, &nextEvent);
                break;
            }
            if (!qt_x11EventFilter(&nextEvent)
                && !x11Event(&nextEvent)) // send event through filter
                lastMotion = nextEvent.xmotion;
            else
                break;
        }
        type = QEvent::MouseMove;
        pos.rx() = lastMotion.x;
        pos.ry() = lastMotion.y;
        pos = d->mapFromWS(pos);
        globalPos.rx() = lastMotion.x_root;
        globalPos.ry() = lastMotion.y_root;
        buttons = translateMouseButtons(lastMotion.state);
        modifiers = translateModifiers(lastMotion.state);
        if (qt_button_down && !buttons)
            qt_button_down = 0;
    } else if (event->type == EnterNotify || event->type == LeaveNotify) {
        XEvent *xevent = (XEvent *)event;
        //unsigned int xstate = event->xcrossing.state;
        type = QEvent::MouseMove;
        pos.rx() = xevent->xcrossing.x;
        pos.ry() = xevent->xcrossing.y;
        pos = d->mapFromWS(pos);
        globalPos.rx() = xevent->xcrossing.x_root;
        globalPos.ry() = xevent->xcrossing.y_root;
        buttons = translateMouseButtons(xevent->xcrossing.state);
        modifiers = translateModifiers(xevent->xcrossing.state);
        if (qt_button_down && !buttons)
            qt_button_down = 0;
    } else {                                        // button press or release
        pos.rx() = event->xbutton.x;
        pos.ry() = event->xbutton.y;
        pos = d->mapFromWS(pos);
        globalPos.rx() = event->xbutton.x_root;
        globalPos.ry() = event->xbutton.y_root;
        buttons = translateMouseButtons(event->xbutton.state);
        modifiers = translateModifiers(event->xbutton.state);
        switch (event->xbutton.button) {
        case Button1: button = Qt::LeftButton; break;
        case Button2: button = Qt::MidButton; break;
        case Button3: button = Qt::RightButton; break;
        case Button4:
        case Button5:
        case 6:
        case 7:
            // the fancy mouse wheel.

            // We are only interested in ButtonPress.
            if (event->type == ButtonPress){
                // compress wheel events (the X Server will simply
                // send a button press for each single notch,
                // regardless whether the application can catch up
                // or not)
                int delta = 1;
                XEvent xevent;
                while (XCheckTypedWindowEvent(X11->display, winId(), ButtonPress, &xevent)){
                    if (xevent.xbutton.button != event->xbutton.button){
                        XPutBackEvent(X11->display, &xevent);
                        break;
                    }
                    delta++;
                }

                // the delta is defined as multiples of
                // WHEEL_DELTA, which is set to 120. Future wheels
                // may offer a finer-resolution. A positive delta
                // indicates forward rotation, a negative one
                // backward rotation respectively.
                int btn = event->xbutton.button;
                delta *= 120 * ((btn == Button4 || btn == 6) ? 1 : -1);
                bool hor = ((btn == Button4 || btn == Button5) && (modifiers & Qt::AltModifier) ||
                            (btn == 6 || btn == 7));
                translateWheelEvent(globalPos.x(), globalPos.y(), delta, buttons,
                                    modifiers, (hor) ? Qt::Horizontal: Qt::Vertical);
            }
            return true;
        }
        if (event->type == ButtonPress) {        // mouse button pressed
            buttons |= button;
#if defined(Q_OS_IRIX) && !defined(QT_NO_TABLET_SUPPORT)
            TabletDeviceDataList *tablets = qt_tablet_devices();
            for (int i = 0; i < tablets->size(); ++i) {
                const TabletDeviceData &tab = tablets->at(i);
                XEvent myEv;
                if (XCheckTypedEvent(X11->display, tab.xinput_button_press, &myEv)) {
                        if (translateXinputEvent(&myEv, &tab)) {
                            //Spontaneous event sent.  Check if we need to continue.
                            if (qt_tabletChokeMouse) {
                                qt_tabletChokeMouse = false;
                                return false;
                            }
                        }
                }
            }
#endif
            qt_button_down = childAt(pos);        //magic for masked widgets
            if (!qt_button_down || !qt_button_down->testAttribute(Qt::WA_MouseNoMask))
                qt_button_down = this;
            if (mouseActWindow == event->xbutton.window &&
                mouseButtonPressed == button &&
                (long)event->xbutton.time -(long)mouseButtonPressTime
                < QApplication::doubleClickInterval() &&
                qAbs(event->xbutton.x - mouseXPos) < 5 &&
                qAbs(event->xbutton.y - mouseYPos) < 5) {
                type = QEvent::MouseButtonDblClick;
                mouseButtonPressTime -= 2000;        // no double-click next time
            } else {
                type = QEvent::MouseButtonPress;
                mouseButtonPressTime = event->xbutton.time;
            }
            mouseButtonPressed = button;        // save event params for
            mouseXPos = pos.x();                // future double click tests
            mouseYPos = pos.y();
            mouseGlobalXPos = globalPos.x();
            mouseGlobalYPos = globalPos.y();
        } else {                                // mouse button released
            buttons &= ~button;
#if defined(Q_OS_IRIX) && !defined(QT_NO_TABLET_SUPPORT)
            TabletDeviceDataList *tablets = qt_tablet_devices();
            for (int i = 0; i < tablets->size(); ++i) {
                const TabletDeviceData &tab = tablets->at(i);
                XEvent myEv;
                if (XCheckTypedEvent(X11->display, tab.xinput_button_press, &myEv)) {
                        if (translateXinputEvent(&myEv, &tab)) {
                            //Spontaneous event sent.  Check if we need to continue.
                            if (qt_tabletChokeMouse) {
                                qt_tabletChokeMouse = false;
                                return false;
                            }
                        }
                }
            }
#endif
            if (manualGrab) {                        // release manual grab
                manualGrab = false;
                XUngrabPointer(X11->display, X11->time);
                XFlush(X11->display);
            }

            type = QEvent::MouseButtonRelease;
        }
    }
    mouseActWindow = winId();                        // save some event params
    mouseButtonState = buttons;
    if (type == 0)                                // don't send event
        return false;

    if (qApp->d_func()->inPopupMode()) {                        // in popup mode
        QWidget *activePopupWidget = qApp->activePopupWidget();
        QWidget *popup = qApp->activePopupWidget();
        if (popup != this) {
            if (event->type == LeaveNotify)
                return false;
            if ((windowType() == Qt::Popup) && rect().contains(pos))
                popup = this;
            else                                // send to last popup
                pos = popup->mapFromGlobal(globalPos);
        }
        bool releaseAfter = false;
        QWidget *popupChild  = popup->childAt(pos);

        if (popup != qt_popup_down){
            qt_button_down = 0;
            qt_popup_down = 0;
        }

        switch (type) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
            qt_button_down = popupChild;
            qt_popup_down = popup;
            break;
        case QEvent::MouseButtonRelease:
            releaseAfter = true;
            break;
        default:
            break;                                // nothing for mouse move
        }

        int oldOpenPopupCount = openPopupCount;

        // deliver event
        replayPopupMouseEvent = false;
        if (qt_button_down) {
            QMouseEvent e(type, qt_button_down->mapFromGlobal(globalPos),
                          globalPos, button, buttons, modifiers);
            QApplication::sendSpontaneousEvent(qt_button_down, &e);
        } else if (popupChild) {
            QMouseEvent e(type, popupChild->mapFromGlobal(globalPos),
                          globalPos, button, buttons, modifiers);
            QApplication::sendSpontaneousEvent(popupChild, &e);
        } else {
            QMouseEvent e(type, pos, globalPos, button, buttons, modifiers);
            QApplication::sendSpontaneousEvent(popup, &e);
        }

        if (qApp->activePopupWidget() != activePopupWidget
            && replayPopupMouseEvent) {
            // the active popup was closed, replay the mouse event
            if (!(windowType() == Qt::Popup)) {
                QMouseEvent e(type, mapFromGlobal(globalPos), globalPos, button,
                              buttons, modifiers);
                QApplication::sendSpontaneousEvent(this, &e);
            }
            replayPopupMouseEvent = false;
        } else if (type == QEvent::MouseButtonPress
                   && button == Qt::RightButton
                   && (openPopupCount == oldOpenPopupCount)) {
            QWidget *popupEvent = popup;
            if (qt_button_down)
                popupEvent = qt_button_down;
            else if(popupChild)
                popupEvent = popupChild;
            QContextMenuEvent e(QContextMenuEvent::Mouse, pos, globalPos);
            QApplication::sendSpontaneousEvent(popupEvent, &e);
        }

        if (releaseAfter) {
            qt_button_down = 0;
            qt_popup_down = 0;
        }
        if (!qApp->d_func()->inPopupMode()) {
             if (type != QEvent::MouseButtonRelease && !buttons &&
                 QWidget::find((WId)mouseActWindow)) {
                 manualGrab = true;                // need to manually grab
                 XGrabPointer(X11->display, mouseActWindow, False,
                              (uint)(ButtonPressMask | ButtonReleaseMask |
                                     ButtonMotionMask |
                                     EnterWindowMask | LeaveWindowMask),
                              GrabModeAsync, GrabModeAsync,
                              XNone, XNone, CurrentTime);
             }
        }

    } else {
        QWidget *widget = this;
        QWidget *w = QWidget::mouseGrabber();
        if (!w)
            w = qt_button_down;
        if (w && w != this) {
            widget = w;
            pos = w->mapFromGlobal(globalPos);
        }

        if (type == QEvent::MouseButtonRelease && !buttons) {
            // no more buttons pressed on the widget
            qt_button_down = 0;
        }

        int oldOpenPopupCount = openPopupCount;

        QMouseEvent e(type, pos, globalPos, button, buttons, modifiers);
        QApplication::sendSpontaneousEvent(widget, &e);

        if (type == QEvent::MouseButtonPress
            && button == Qt::RightButton
            && (openPopupCount == oldOpenPopupCount)) {
            QContextMenuEvent e(QContextMenuEvent::Mouse, pos, globalPos);
            QApplication::sendSpontaneousEvent(widget, &e);
        }
    }
    return true;
}


//
// Wheel event translation
//
bool QETWidget::translateWheelEvent(int global_x, int global_y, int delta,
                                    Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers,
                                    Qt::Orientation orient)
{
    // send the event to the widget or its ancestors
    {
        QWidget* popup = qApp->activePopupWidget();
        if (popup && window() != popup)
            popup->close();
        QWheelEvent e(mapFromGlobal(QPoint(global_x, global_y)),
                       QPoint(global_x, global_y), delta, buttons, modifiers, orient);
        if (QApplication::sendSpontaneousEvent(this, &e))
            return true;
    }

    // send the event to the widget that has the focus or its ancestors, if different
    QWidget *w = this;
    if (w != qApp->focusWidget() && (w = qApp->focusWidget())) {
        QWidget* popup = qApp->activePopupWidget();
        if (popup && w != popup)
            popup->hide();
        QWheelEvent e(mapFromGlobal(QPoint(global_x, global_y)),
                       QPoint(global_x, global_y), delta, buttons, modifiers, orient);
        if (QApplication::sendSpontaneousEvent(w, &e))
            return true;
    }
    return false;
}


//
// XInput Translation Event
//
#if !defined (QT_NO_TABLET_SUPPORT)
bool QETWidget::translateXinputEvent(const XEvent *ev, const TabletDeviceData *tablet)
{
#if defined (Q_OS_IRIX)
    // Wacom has put defines in their wacom.h file so it would be quite wise
    // to use them, need to think of a decent way of not using
    // it when it doesn't exist...
    XDeviceState *s;
    XInputClass *iClass;
    XValuatorState *vs;
    int j;
#endif

    Q_ASSERT(tablet != 0);

    QWidget *w = this;
    QPoint global,
        curr;
    QPointF hiRes;
    qreal pressure = 0;
    int xTilt = 0,
        yTilt = 0,
        z = 0;
    qreal tangentialPressure = 0;
    qreal rotation = 0;
    int deviceType = QTabletEvent::NoDevice;
    int pointerType = QTabletEvent::UnknownPointer;
    XEvent xinputMotionEvent;
    XEvent mouseMotionEvent;
    const XDeviceMotionEvent *motion = 0;
    XDeviceButtonEvent *button = 0;
    QEvent::Type t;
    Qt::KeyboardModifiers modifiers = 0;

    if (ev->type == tablet->xinput_motion) {
        motion = reinterpret_cast<const XDeviceMotionEvent*>(ev);
        for (;;) {
            if (!XCheckTypedWindowEvent(X11->display, winId(), MotionNotify, &mouseMotionEvent))
                break;
            if (!XCheckTypedWindowEvent(X11->display, winId(), tablet->xinput_motion, &xinputMotionEvent)) {
                XPutBackEvent(X11->display, &mouseMotionEvent);
                break;
            }
            if (mouseMotionEvent.xmotion.time != motion->time) {
                XPutBackEvent(X11->display, &mouseMotionEvent);
                XPutBackEvent(X11->display, &xinputMotionEvent);

            }
            motion = (reinterpret_cast<const XDeviceMotionEvent*>(&xinputMotionEvent));
        }
        t = QEvent::TabletMove;
        global = QPoint(motion->x_root, motion->y_root);
        curr = QPoint(motion->x, motion->y);
    } else {
        if (ev->type == tablet->xinput_button_press) {
            t = QEvent::TabletPress;
        } else {
            t = QEvent::TabletRelease;
        }
        button = (XDeviceButtonEvent*)ev;

        global = QPoint(button->x_root, button->y_root);
        curr = QPoint(button->x, button->y);
    }

    qint64 uid;
    QRect screenArea = qApp->desktop()->screenGeometry(global);
#if defined (Q_OS_IRIX)
    s = XQueryDeviceState(X11->display, static_cast<XDevice *>(tablet->device));
    if (!s)
        return false;
    iClass = s->data;
    for (j = 0; j < s->num_classes; j++) {
        if (iClass->c_class == ValuatorClass) {
            vs = reinterpret_cast<XValuatorState *>(iClass);
            // figure out what device we have, based on bitmasking...
            if (vs->valuators[WAC_TRANSDUCER_I]
                 & WAC_TRANSDUCER_PROX_MSK) {
                switch (vs->valuators[WAC_TRANSDUCER_I]
                         & WAC_TRANSDUCER_MSK) {
                case WAC_PUCK_ID:
                    pointerType = QTabletEvent::Puck;
                    break;
                case WAC_STYLUS_ID:
                    pointerType = QTabletEvent::Pen;
                    break;
                case WAC_ERASER_ID:
                    pointerType = QTabletEvent::Eraser;
                    break;
                }
                // Get a Unique Id for the device, Wacom gives us this ability
                uid = vs->valuators[WAC_TRANSDUCER_I] & WAC_TRANSDUCER_ID_MSK;
                uid = (uid << 24) | vs->valuators[WAC_SERIAL_NUM_I];
                switch (WAC_TRANSDUCER_I & 0x0F0600) {
                case 0x080200:
                    deviceType = QTabletEvent::Stylus;
                    break;
                case 0x090200:
                    deviceType = QTabletEvent::Airbrush;
                    break;
                case 0x000400:
                    deviceType = QTabletEvent::FourDMouse;
                    break;
                case 0x000600:
                    deviceType = QTabletEvent::Puck;
                    break;
                }
            } else {
                pointerType = QTabletEvent::UnknownPointer;
                deviceType = QTabletEvent::NoDevice;
                uid = 0;
            }

            // apparently Wacom needs a cast for the +/- values to make sense
            xTilt = short(vs->valuators[WAC_XTILT_I]);
            yTilt = short(vs->valuators[WAC_YTILT_I]);
            pressure = vs->valuators[WAC_PRESSURE_I];
            if (deviceType == QTabletEvent::FourDMouse) {
                rotation = vs->valuators[WAC_ROTATION_I] / 64.0;
                z = vs->valuators[WAC_ZCOORD_I];
            } else if (deviceType == QTabletEvent::Airbrush) {
                tangentialPressure = vs->valuators[WAC_TAN_PRESSURE_I]
                                        / qreal(tablet->maxTanPressure - tablet->minTanPressure);
            }

            hiRes = tablet->scaleCoord(vs->valuators[WAC_XCOORD_I], vs->valuators[WAC_YCOORD_I],
                                       screenArea.x(), screenArea.width(),
                                       screenArea.y(), screenArea.height());
            break;
        }
        iClass = reinterpret_cast<XInputClass*>(reinterpret_cast<char*>(iClass) + iClass->length);
    }
    XFreeDeviceState(s);
#else
    TabletDeviceDataList *tablet_list = qt_tablet_devices();
    XID device_id = ev->type == tablet->xinput_motion ? motion->deviceid : button->deviceid;
    for (int i = 0; i < tablet_list->size(); ++i) {
        const TabletDeviceData &t = tablet_list->at(i);
        if (device_id == static_cast<XDevice *>(t.device)->device_id) {
            deviceType = t.deviceType;
            if (deviceType == QTabletEvent::XFreeEraser) {
                deviceType = QTabletEvent::Stylus;
                pointerType = QTabletEvent::Eraser;
            }
            // qDebug() << ((XDevice*)t.device)->device_id;
            break;
        }
    }

    if (motion) {
        xTilt = short(motion->axis_data[3]);
        yTilt = short(motion->axis_data[4]);
        pressure = motion->axis_data[2];
        modifiers = translateModifiers(motion->state);
        hiRes = tablet->scaleCoord(motion->axis_data[0], motion->axis_data[1],
                                    screenArea.x(), screenArea.width(),
                                    screenArea.y(), screenArea.height());
    } else {
        xTilt = short(button->axis_data[3]);
        yTilt = short(button->axis_data[4]);
        pressure = button->axis_data[2];
        hiRes = tablet->scaleCoord(button->axis_data[0], button->axis_data[1],
                                    screenArea.x(), screenArea.width(),
                                    screenArea.y(), screenArea.height());
        modifiers = translateModifiers(button->state);
    }
    // The only way to get these Ids is to scan the XFree86 log, which I'm not going to do.
    uid = -1;
#endif
    QTabletEvent e(t, curr, global, hiRes,
                   deviceType, pointerType,
                   qreal(pressure / qreal(tablet->maxPressure - tablet->minPressure)),
                   xTilt, yTilt, tangentialPressure, rotation, z, modifiers, uid);
    QApplication::sendSpontaneousEvent(w, &e);
    return true;
}
#endif

bool QETWidget::translatePropertyEvent(const XEvent *event)
{
    Q_D(QWidget);
    if (!isWindow()) return true;

    Atom ret;
    int format, e;
    unsigned char *data = 0;
    unsigned long nitems, after;

    if (event->xproperty.atom == ATOM(_KDE_NET_WM_FRAME_STRUT)) {
        d->topData()->fleft = d->topData()->fright = d->topData()->ftop = d->topData()->fbottom = 0;
        this->data->fstrut_dirty = 1;

        if (event->xproperty.state == PropertyNewValue) {
            e = XGetWindowProperty(X11->display, event->xproperty.window, ATOM(_KDE_NET_WM_FRAME_STRUT),
                                   0, 4, // struts are 4 longs
                                   False, XA_CARDINAL, &ret, &format, &nitems, &after, &data);

            if (e == Success && ret == XA_CARDINAL &&
                format == 32 && nitems == 4) {
                long *strut = (long *) data;
                d->topData()->fleft   = strut[0];
                d->topData()->fright  = strut[1];
                d->topData()->ftop    = strut[2];
                d->topData()->fbottom = strut[3];
                this->data->fstrut_dirty = 0;
            }
        }
    } else if (event->xproperty.atom == ATOM(_NET_WM_STATE)) {
        bool max = false;
        bool full = false;
        Qt::WindowStates oldState = Qt::WindowStates(this->data->window_state);

        if (event->xproperty.state == PropertyNewValue) {
            // using length of 1024 should be safe for all current and
            // possible NET states...
            e = XGetWindowProperty(X11->display, event->xproperty.window, ATOM(_NET_WM_STATE), 0, 1024,
                                   False, XA_ATOM, &ret, &format, &nitems, &after, &data);

            if (e == Success && ret == XA_ATOM && format == 32 && nitems > 0) {
                Atom *states = (Atom *) data;

                unsigned long i;
                for (i = 0; i < nitems; i++) {
                    if (states[i] == ATOM(_NET_WM_STATE_MAXIMIZED_VERT)
                        || states[i] == ATOM(_NET_WM_STATE_MAXIMIZED_HORZ))
                        max = true;
                    else if (states[i] == ATOM(_NET_WM_STATE_FULLSCREEN))
                        full = true;
                }
            }
        }

        bool send_event = false;

        if (qt_net_supports(ATOM(_NET_WM_STATE_MAXIMIZED_VERT))
            && qt_net_supports(ATOM(_NET_WM_STATE_MAXIMIZED_HORZ))) {
            if (max && !isMaximized()) {
                this->data->window_state = this->data->window_state | Qt::WindowMaximized;
                send_event = true;
            } else if (!max && isMaximized()) {
                this->data->window_state &= !Qt::WindowMaximized;
                send_event = true;
            }
        }

        if (qt_net_supports(ATOM(_NET_WM_STATE_FULLSCREEN))) {
            if (full && !isFullScreen()) {
                this->data->window_state = this->data->window_state | Qt::WindowFullScreen;
                send_event = true;
            } else if (!full && isFullScreen()) {
                this->data->window_state &= ~Qt::WindowFullScreen;
                send_event = true;
            }
        }

        if (send_event) {
            QWindowStateChangeEvent e(oldState);
            QApplication::sendSpontaneousEvent(this, &e);
        }
    } else if (event->xproperty.atom == ATOM(WM_STATE)) {
        // the widget frame strut should also be invalidated
        d->topData()->fleft = d->topData()->fright = d->topData()->ftop = d->topData()->fbottom = 0;
        this->data->fstrut_dirty = 1;

        if (event->xproperty.state == PropertyDelete) {
            // the window manager has removed the WM State property,
            // so it is now in the withdrawn state (ICCCM 4.1.3.1) and
            // we are free to reuse this window
            d->topData()->parentWinId = 0;
            // map the window if we were waiting for a transition to
            // withdrawn
            if (X11->deferred_map.removeAll(this)) {
                XMapWindow(X11->display, winId());
            } else if (isVisible()) {
                // so that show() will work again
                hide();
            }
        } else if (d->topData()->parentWinId != QX11Info::appRootWindow(x11Info().screen())) {
            // the window manager has changed the WM State property...
            // we are wanting to see if we are withdrawn so that we
            // can reuse this window... we only do this check *IF* we
            // haven't been reparented to root - (the parentWinId !=
            // QX11Info::x11AppRootWindow(x11Screen())) check
            // above

            e = XGetWindowProperty(X11->display, winId(), ATOM(WM_STATE), 0, 2, False, ATOM(WM_STATE),
                                   &ret, &format, &nitems, &after, &data);

            if (e == Success && ret == ATOM(WM_STATE) && format == 32 && nitems > 0) {
                long *state = (long *) data;
                switch (state[0]) {
                case WithdrawnState:
                    // if we are in the withdrawn state, we are free
                    // to reuse this window provided we remove the
                    // WM_STATE property (ICCCM 4.1.3.1)
                    XDeleteProperty(X11->display, winId(), ATOM(WM_STATE));

                    // set the parent id to zero, so that show() will
                    // work again
                    d->topData()->parentWinId = 0;
                    // map the window if we were waiting for a
                    // transition to withdrawn
                    if (X11->deferred_map.removeAll(this)) {
                        XMapWindow(X11->display, winId());
                    } else if (isVisible()) {
                        // so that show() will work again
                        hide();
                    }
                    break;

                case IconicState:
                    if (!isMinimized()) {
                        // window was minimized
                        this->data->window_state = this->data->window_state | Qt::WindowMinimized;
                        QWindowStateChangeEvent e(Qt::WindowStates(this->data->window_state & ~Qt::WindowMinimized));
                        QApplication::sendSpontaneousEvent(this, &e);
                    }
                    break;

                default:
                    if (isMinimized()) {
                        // window was un-minimized
                        this->data->window_state &= ~Qt::WindowMinimized;
                        QWindowStateChangeEvent e(Qt::WindowStates(this->data->window_state | Qt::WindowMinimized));
                        QApplication::sendSpontaneousEvent(this, &e);
                    }
                    break;
                }
            }
        }
    }

    if (data)
        XFree(data);

    return true;
}

//
// Keyboard event translation
//

#ifndef XK_ISO_Left_Tab
#define        XK_ISO_Left_Tab                                        0xFE20
#endif

#ifndef XK_dead_hook
#define XK_dead_hook 0xFE61
#endif

#ifndef XK_dead_horn
#define XK_dead_horn 0xFE62
#endif

#ifndef XK_Codeinput
#define XK_Codeinput 0xFF37
#endif

#ifndef XK_Kanji_Bangou
#define XK_Kanji_Bangou 0xFF37 /* same as codeinput */
#endif

// the next lines are taken from XFree > 4.0 (X11/XF86keysyms.h), defining some special
// multimedia keys. They are included here as not every system has them.
#define XF86XK_Standby                0x1008FF10
#define XF86XK_AudioLowerVolume        0x1008FF11
#define XF86XK_AudioMute        0x1008FF12
#define XF86XK_AudioRaiseVolume        0x1008FF13
#define XF86XK_AudioPlay        0x1008FF14
#define XF86XK_AudioStop        0x1008FF15
#define XF86XK_AudioPrev        0x1008FF16
#define XF86XK_AudioNext        0x1008FF17
#define XF86XK_HomePage                0x1008FF18
#define XF86XK_Calculator        0x1008FF1D
#define XF86XK_Mail                0x1008FF19
#define XF86XK_Start                0x1008FF1A
#define XF86XK_Search                0x1008FF1B
#define XF86XK_AudioRecord        0x1008FF1C
#define XF86XK_Back                0x1008FF26
#define XF86XK_Forward                0x1008FF27
#define XF86XK_Stop                0x1008FF28
#define XF86XK_Refresh                0x1008FF29
#define XF86XK_Favorites        0x1008FF30
#define XF86XK_AudioPause        0x1008FF31
#define XF86XK_AudioMedia        0x1008FF32
#define XF86XK_MyComputer        0x1008FF33
#define XF86XK_OpenURL                0x1008FF38
#define XF86XK_Launch0                0x1008FF40
#define XF86XK_Launch1                0x1008FF41
#define XF86XK_Launch2                0x1008FF42
#define XF86XK_Launch3                0x1008FF43
#define XF86XK_Launch4                0x1008FF44
#define XF86XK_Launch5                0x1008FF45
#define XF86XK_Launch6                0x1008FF46
#define XF86XK_Launch7                0x1008FF47
#define XF86XK_Launch8                0x1008FF48
#define XF86XK_Launch9                0x1008FF49
#define XF86XK_LaunchA                0x1008FF4A
#define XF86XK_LaunchB                0x1008FF4B
#define XF86XK_LaunchC                0x1008FF4C
#define XF86XK_LaunchD                0x1008FF4D
#define XF86XK_LaunchE                0x1008FF4E
#define XF86XK_LaunchF                0x1008FF4F
// end of XF86keysyms.h



static const unsigned int KeyTbl[] = {                // keyboard mapping table
    XK_Escape,                Qt::Key_Escape,                // misc keys
    XK_Tab,                Qt::Key_Tab,
    XK_ISO_Left_Tab,    Qt::Key_Backtab,
    XK_BackSpace,        Qt::Key_Backspace,
    XK_Return,                Qt::Key_Return,
    XK_Insert,                Qt::Key_Insert,
    XK_KP_Insert,        Qt::Key_Insert,
    XK_Delete,                Qt::Key_Delete,
    XK_KP_Delete,        Qt::Key_Delete,
    XK_Clear,                Qt::Key_Delete,
    XK_Pause,                Qt::Key_Pause,
    XK_Print,                Qt::Key_Print,
    XK_KP_Begin,        Qt::Key_Clear,
    0x1005FF60,                Qt::Key_SysReq,                // hardcoded Sun SysReq
    0x1007ff00,                Qt::Key_SysReq,                // hardcoded X386 SysReq
    XK_Home,                Qt::Key_Home,                // cursor movement
    XK_End,                Qt::Key_End,
    XK_Left,                Qt::Key_Left,
    XK_Up,                Qt::Key_Up,
    XK_Right,                Qt::Key_Right,
    XK_Down,                Qt::Key_Down,
    XK_Prior,                Qt::Key_PageUp,
    XK_Next,                Qt::Key_PageDown,
    XK_KP_Home,                Qt::Key_Home,
    XK_KP_End,                Qt::Key_End,
    XK_KP_Left,                Qt::Key_Left,
    XK_KP_Up,                Qt::Key_Up,
    XK_KP_Right,        Qt::Key_Right,
    XK_KP_Down,                Qt::Key_Down,
    XK_KP_Prior,        Qt::Key_PageUp,
    XK_KP_Next,                Qt::Key_PageDown,
    XK_Shift_L,                Qt::Key_Shift,                // modifiers
    XK_Shift_R,                Qt::Key_Shift,
    XK_Shift_Lock,        Qt::Key_Shift,
    XK_Control_L,        Qt::Key_Control,
    XK_Control_R,        Qt::Key_Control,
    XK_Meta_L,                Qt::Key_Meta,
    XK_Meta_R,                Qt::Key_Meta,
    XK_Alt_L,                Qt::Key_Alt,
    XK_Alt_R,                Qt::Key_Alt,
    XK_Caps_Lock,        Qt::Key_CapsLock,
    XK_Num_Lock,        Qt::Key_NumLock,
    XK_Scroll_Lock,        Qt::Key_ScrollLock,
    XK_KP_Space,        Qt::Key_Space,                // numeric keypad
    XK_KP_Tab,                Qt::Key_Tab,
    XK_KP_Enter,        Qt::Key_Enter,
    XK_KP_Equal,        Qt::Key_Equal,
    XK_KP_Multiply,        Qt::Key_Asterisk,
    XK_KP_Add,                Qt::Key_Plus,
    XK_KP_Separator,        Qt::Key_Comma,
    XK_KP_Subtract,        Qt::Key_Minus,
    XK_KP_Decimal,        Qt::Key_Period,
    XK_KP_Divide,        Qt::Key_Slash,
    XK_Super_L,                Qt::Key_Super_L,
    XK_Super_R,                Qt::Key_Super_R,
    XK_Menu,                Qt::Key_Menu,
    XK_Hyper_L,                Qt::Key_Hyper_L,
    XK_Hyper_R,                Qt::Key_Hyper_R,
    XK_Help,                Qt::Key_Help,
    0x1000FF74,         Qt::Key_Backtab,     // hardcoded HP backtab

    // International input method support keys

    // International & multi-key character composition
    XK_Multi_key,		Qt::Key_Multi_key,
    XK_Codeinput,		Qt::Key_Codeinput,
    XK_SingleCandidate,		Qt::Key_SingleCandidate,
    XK_MultipleCandidate,	Qt::Key_MultipleCandidate,
    XK_PreviousCandidate,	Qt::Key_PreviousCandidate,

    // Misc Functions
    XK_Mode_switch,		Qt::Key_Mode_switch,
    //XK_script_switch,		Qt::Key_script_switch,
    XK_script_switch,		Qt::Key_Mode_switch,

    // Japanese keyboard support
    XK_Kanji,			Qt::Key_Kanji,
    XK_Muhenkan,		Qt::Key_Muhenkan,
    //XK_Henkan_Mode,		Qt::Key_Henkan_Mode,
    XK_Henkan_Mode,		Qt::Key_Henkan,
    XK_Henkan,			Qt::Key_Henkan,
    XK_Romaji,			Qt::Key_Romaji,
    XK_Hiragana,		Qt::Key_Hiragana,
    XK_Katakana,		Qt::Key_Katakana,
    XK_Hiragana_Katakana,	Qt::Key_Hiragana_Katakana,
    XK_Zenkaku,			Qt::Key_Zenkaku,
    XK_Hankaku,			Qt::Key_Hankaku,
    XK_Zenkaku_Hankaku,		Qt::Key_Zenkaku_Hankaku,
    XK_Touroku,			Qt::Key_Touroku,
    XK_Massyo,			Qt::Key_Massyo,
    XK_Kana_Lock,		Qt::Key_Kana_Lock,
    XK_Kana_Shift,		Qt::Key_Kana_Shift,
    XK_Eisu_Shift,		Qt::Key_Eisu_Shift,
    XK_Eisu_toggle,		Qt::Key_Eisu_toggle,
    //XK_Kanji_Bangou,		Qt::Key_Kanji_Bangou,
    //XK_Zen_Koho,		Qt::Key_Zen_Koho,
    //XK_Mae_Koho,		Qt::Key_Mae_Koho,
    XK_Kanji_Bangou,		Qt::Key_Codeinput,
    XK_Zen_Koho,		Qt::Key_MultipleCandidate,
    XK_Mae_Koho,		Qt::Key_PreviousCandidate,

#ifdef XK_KOREAN
    // Korean keyboard support
    XK_Hangul,			Qt::Key_Hangul,
    XK_Hangul_Start,		Qt::Key_Hangul_Start,
    XK_Hangul_End,		Qt::Key_Hangul_End,
    XK_Hangul_Hanja,		Qt::Key_Hangul_Hanja,
    XK_Hangul_Jamo,		Qt::Key_Hangul_Jamo,
    XK_Hangul_Romaja,		Qt::Key_Hangul_Romaja,
    //XK_Hangul_Codeinput,	Qt::Key_Hangul_Codeinput,
    XK_Hangul_Codeinput,	Qt::Key_Codeinput,
    XK_Hangul_Jeonja,		Qt::Key_Hangul_Jeonja,
    XK_Hangul_Banja,		Qt::Key_Hangul_Banja,
    XK_Hangul_PreHanja,		Qt::Key_Hangul_PreHanja,
    XK_Hangul_PostHanja,	Qt::Key_Hangul_PostHanja,
    //XK_Hangul_SingleCandidate,	Qt::Key_Hangul_SingleCandidate,
    //XK_Hangul_MultipleCandidate, Qt::Key_Hangul_MultipleCandidate,
    //XK_Hangul_PreviousCandidate, Qt::Key_Hangul_PreviousCandidate,
    XK_Hangul_SingleCandidate,	Qt::Key_SingleCandidate,
    XK_Hangul_MultipleCandidate, Qt::Key_MultipleCandidate,
    XK_Hangul_PreviousCandidate, Qt::Key_PreviousCandidate,
    XK_Hangul_Special,		Qt::Key_Hangul_Special,
    //XK_Hangul_switch,		Qt::Key_Hangul_switch,
    XK_Hangul_switch,		Qt::Key_Mode_switch,
#endif  // XK_KOREAN

    // dead keys
    XK_dead_grave,              Qt::Key_Dead_Grave,
    XK_dead_acute,              Qt::Key_Dead_Acute,
    XK_dead_circumflex,         Qt::Key_Dead_Circumflex,
    XK_dead_tilde,              Qt::Key_Dead_Tilde,
    XK_dead_macron,             Qt::Key_Dead_Macron,
    XK_dead_breve,              Qt::Key_Dead_Breve,
    XK_dead_abovedot,           Qt::Key_Dead_Abovedot,
    XK_dead_diaeresis,          Qt::Key_Dead_Diaeresis,
    XK_dead_abovering,          Qt::Key_Dead_Abovering,
    XK_dead_doubleacute,        Qt::Key_Dead_Doubleacute,
    XK_dead_caron,              Qt::Key_Dead_Caron,
    XK_dead_cedilla,            Qt::Key_Dead_Cedilla,
    XK_dead_ogonek,             Qt::Key_Dead_Ogonek,
    XK_dead_iota,               Qt::Key_Dead_Iota,
    XK_dead_voiced_sound,       Qt::Key_Dead_Voiced_Sound,
    XK_dead_semivoiced_sound,   Qt::Key_Dead_Semivoiced_Sound,
    XK_dead_belowdot,           Qt::Key_Dead_Belowdot,
    XK_dead_hook,               Qt::Key_Dead_Hook,
    XK_dead_horn,               Qt::Key_Dead_Horn,

    // Special multimedia keys
    // currently only tested with MS internet keyboard

    // browsing keys
    XF86XK_Back,        Qt::Key_Back,
    XF86XK_Forward,        Qt::Key_Forward,
    XF86XK_Stop,        Qt::Key_Stop,
    XF86XK_Refresh,        Qt::Key_Refresh,
    XF86XK_Favorites,        Qt::Key_Favorites,
    XF86XK_AudioMedia,        Qt::Key_LaunchMedia,
    XF86XK_OpenURL,        Qt::Key_OpenUrl,
    XF86XK_HomePage,        Qt::Key_HomePage,
    XF86XK_Search,        Qt::Key_Search,

    // media keys
    XF86XK_AudioLowerVolume, Qt::Key_VolumeDown,
    XF86XK_AudioMute,        Qt::Key_VolumeMute,
    XF86XK_AudioRaiseVolume, Qt::Key_VolumeUp,
    XF86XK_AudioPlay,        Qt::Key_MediaPlay,
    XF86XK_AudioStop,        Qt::Key_MediaStop,
    XF86XK_AudioPrev,        Qt::Key_MediaPrevious,
    XF86XK_AudioNext,        Qt::Key_MediaNext,
    XF86XK_AudioRecord,        Qt::Key_MediaRecord,

    // launch keys
    XF86XK_Mail,        Qt::Key_LaunchMail,
    XF86XK_MyComputer,        Qt::Key_Launch0,
    XF86XK_Calculator,        Qt::Key_Launch1,
    XF86XK_Standby,         Qt::Key_Standby,

    XF86XK_Launch0,        Qt::Key_Launch2,
    XF86XK_Launch1,        Qt::Key_Launch3,
    XF86XK_Launch2,        Qt::Key_Launch4,
    XF86XK_Launch3,        Qt::Key_Launch5,
    XF86XK_Launch4,        Qt::Key_Launch6,
    XF86XK_Launch5,        Qt::Key_Launch7,
    XF86XK_Launch6,        Qt::Key_Launch8,
    XF86XK_Launch7,        Qt::Key_Launch9,
    XF86XK_Launch8,        Qt::Key_LaunchA,
    XF86XK_Launch9,        Qt::Key_LaunchB,
    XF86XK_LaunchA,        Qt::Key_LaunchC,
    XF86XK_LaunchB,        Qt::Key_LaunchD,
    XF86XK_LaunchC,        Qt::Key_LaunchE,
    XF86XK_LaunchD,        Qt::Key_LaunchF,

    0,                        0
};


#if !defined(QT_NO_XIM)
static const unsigned short katakanaKeysymsToUnicode[] = {
    0x0000, 0x3002, 0x300C, 0x300D, 0x3001, 0x30FB, 0x30F2, 0x30A1,
    0x30A3, 0x30A5, 0x30A7, 0x30A9, 0x30E3, 0x30E5, 0x30E7, 0x30C3,
    0x30FC, 0x30A2, 0x30A4, 0x30A6, 0x30A8, 0x30AA, 0x30AB, 0x30AD,
    0x30AF, 0x30B1, 0x30B3, 0x30B5, 0x30B7, 0x30B9, 0x30BB, 0x30BD,
    0x30BF, 0x30C1, 0x30C4, 0x30C6, 0x30C8, 0x30CA, 0x30CB, 0x30CC,
    0x30CD, 0x30CE, 0x30CF, 0x30D2, 0x30D5, 0x30D8, 0x30DB, 0x30DE,
    0x30DF, 0x30E0, 0x30E1, 0x30E2, 0x30E4, 0x30E6, 0x30E8, 0x30E9,
    0x30EA, 0x30EB, 0x30EC, 0x30ED, 0x30EF, 0x30F3, 0x309B, 0x309C
};

static const unsigned short cyrillicKeysymsToUnicode[] = {
    0x0000, 0x0452, 0x0453, 0x0451, 0x0454, 0x0455, 0x0456, 0x0457,
    0x0458, 0x0459, 0x045a, 0x045b, 0x045c, 0x0000, 0x045e, 0x045f,
    0x2116, 0x0402, 0x0403, 0x0401, 0x0404, 0x0405, 0x0406, 0x0407,
    0x0408, 0x0409, 0x040a, 0x040b, 0x040c, 0x0000, 0x040e, 0x040f,
    0x044e, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433,
    0x0445, 0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e,
    0x043f, 0x044f, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432,
    0x044c, 0x044b, 0x0437, 0x0448, 0x044d, 0x0449, 0x0447, 0x044a,
    0x042e, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413,
    0x0425, 0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e,
    0x041f, 0x042f, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412,
    0x042c, 0x042b, 0x0417, 0x0428, 0x042d, 0x0429, 0x0427, 0x042a
};

static const unsigned short greekKeysymsToUnicode[] = {
    0x0000, 0x0386, 0x0388, 0x0389, 0x038a, 0x03aa, 0x0000, 0x038c,
    0x038e, 0x03ab, 0x0000, 0x038f, 0x0000, 0x0000, 0x0385, 0x2015,
    0x0000, 0x03ac, 0x03ad, 0x03ae, 0x03af, 0x03ca, 0x0390, 0x03cc,
    0x03cd, 0x03cb, 0x03b0, 0x03ce, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397,
    0x0398, 0x0399, 0x039a, 0x039b, 0x039c, 0x039d, 0x039e, 0x039f,
    0x03a0, 0x03a1, 0x03a3, 0x0000, 0x03a4, 0x03a5, 0x03a6, 0x03a7,
    0x03a8, 0x03a9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7,
    0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf,
    0x03c0, 0x03c1, 0x03c3, 0x03c2, 0x03c4, 0x03c5, 0x03c6, 0x03c7,
    0x03c8, 0x03c9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};

static const unsigned short technicalKeysymsToUnicode[] = {
    0x0000, 0x23B7, 0x250C, 0x2500, 0x2320, 0x2321, 0x2502, 0x23A1,
    0x23A3, 0x23A4, 0x23A6, 0x239B, 0x239D, 0x239E, 0x23A0, 0x23A8,
    0x23AC, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x2264, 0x2260, 0x2265, 0x222B,
    0x2234, 0x221D, 0x221E, 0x0000, 0x0000, 0x2207, 0x0000, 0x0000,
    0x223C, 0x2243, 0x0000, 0x0000, 0x0000, 0x21D4, 0x21D2, 0x2261,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x221A, 0x0000,
    0x0000, 0x0000, 0x2282, 0x2283, 0x2229, 0x222A, 0x2227, 0x2228,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2202,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0192, 0x0000,
    0x0000, 0x0000, 0x0000, 0x2190, 0x2191, 0x2192, 0x2193, 0x0000
};

static const unsigned short specialKeysymsToUnicode[] = {
    0x25C6, 0x2592, 0x2409, 0x240C, 0x240D, 0x240A, 0x0000, 0x0000,
    0x2424, 0x240B, 0x2518, 0x2510, 0x250C, 0x2514, 0x253C, 0x23BA,
    0x23BB, 0x2500, 0x23BC, 0x23BD, 0x251C, 0x2524, 0x2534, 0x252C,
    0x2502, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};

static const unsigned short publishingKeysymsToUnicode[] = {
    0x0000, 0x2003, 0x2002, 0x2004, 0x2005, 0x2007, 0x2008, 0x2009,
    0x200a, 0x2014, 0x2013, 0x0000, 0x0000, 0x0000, 0x2026, 0x2025,
    0x2153, 0x2154, 0x2155, 0x2156, 0x2157, 0x2158, 0x2159, 0x215a,
    0x2105, 0x0000, 0x0000, 0x2012, 0x2329, 0x0000, 0x232a, 0x0000,
    0x0000, 0x0000, 0x0000, 0x215b, 0x215c, 0x215d, 0x215e, 0x0000,
    0x0000, 0x2122, 0x2613, 0x0000, 0x25c1, 0x25b7, 0x25cb, 0x25af,
    0x2018, 0x2019, 0x201c, 0x201d, 0x211e, 0x0000, 0x2032, 0x2033,
    0x0000, 0x271d, 0x0000, 0x25ac, 0x25c0, 0x25b6, 0x25cf, 0x25ae,
    0x25e6, 0x25ab, 0x25ad, 0x25b3, 0x25bd, 0x2606, 0x2022, 0x25aa,
    0x25b2, 0x25bc, 0x261c, 0x261e, 0x2663, 0x2666, 0x2665, 0x0000,
    0x2720, 0x2020, 0x2021, 0x2713, 0x2717, 0x266f, 0x266d, 0x2642,
    0x2640, 0x260e, 0x2315, 0x2117, 0x2038, 0x201a, 0x201e, 0x0000
};

static const unsigned short aplKeysymsToUnicode[] = {
    0x0000, 0x0000, 0x0000, 0x003c, 0x0000, 0x0000, 0x003e, 0x0000,
    0x2228, 0x2227, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x00af, 0x0000, 0x22a5, 0x2229, 0x230a, 0x0000, 0x005f, 0x0000,
    0x0000, 0x0000, 0x2218, 0x0000, 0x2395, 0x0000, 0x22a4, 0x25cb,
    0x0000, 0x0000, 0x0000, 0x2308, 0x0000, 0x0000, 0x222a, 0x0000,
    0x2283, 0x0000, 0x2282, 0x0000, 0x22a2, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x22a3, 0x0000, 0x0000, 0x0000
};

static const unsigned short koreanKeysymsToUnicode[] = {
    0x0000, 0x3131, 0x3132, 0x3133, 0x3134, 0x3135, 0x3136, 0x3137,
    0x3138, 0x3139, 0x313a, 0x313b, 0x313c, 0x313d, 0x313e, 0x313f,
    0x3140, 0x3141, 0x3142, 0x3143, 0x3144, 0x3145, 0x3146, 0x3147,
    0x3148, 0x3149, 0x314a, 0x314b, 0x314c, 0x314d, 0x314e, 0x314f,
    0x3150, 0x3151, 0x3152, 0x3153, 0x3154, 0x3155, 0x3156, 0x3157,
    0x3158, 0x3159, 0x315a, 0x315b, 0x315c, 0x315d, 0x315e, 0x315f,
    0x3160, 0x3161, 0x3162, 0x3163, 0x11a8, 0x11a9, 0x11aa, 0x11ab,
    0x11ac, 0x11ad, 0x11ae, 0x11af, 0x11b0, 0x11b1, 0x11b2, 0x11b3,
    0x11b4, 0x11b5, 0x11b6, 0x11b7, 0x11b8, 0x11b9, 0x11ba, 0x11bb,
    0x11bc, 0x11bd, 0x11be, 0x11bf, 0x11c0, 0x11c1, 0x11c2, 0x316d,
    0x3171, 0x3178, 0x317f, 0x3181, 0x3184, 0x3186, 0x318d, 0x318e,
    0x11eb, 0x11f0, 0x11f9, 0x0000, 0x0000, 0x0000, 0x0000, 0x20a9
};


static QChar keysymToUnicode(unsigned char byte3, unsigned char byte4)
{
    if (byte3 == 0x04) {
        // katakana
        if (byte4 > 0xa0 && byte4 < 0xe0)
           return QChar(katakanaKeysymsToUnicode[byte4 - 0xa0]);
        else if (byte4 == 0x7e)
            return QChar(0x203e); // Overline
    } else if (byte3 == 0x06) {
        // russian, use lookup table
        if (byte4 > 0xa0)
            return QChar(cyrillicKeysymsToUnicode[byte4 - 0xa0]);
    } else if (byte3 == 0x07) {
        // greek
        if (byte4 > 0xa0)
            return QChar(greekKeysymsToUnicode[byte4 - 0xa0]);
    } else if (byte3 == 0x08) {
       // technical
       if (byte4 > 0xa0)
           return QChar(technicalKeysymsToUnicode[byte4 - 0xa0]);
    } else if (byte3 == 0x09) {
       // special
       if (byte4 >= 0xe0)
           return QChar(specialKeysymsToUnicode[byte4 - 0xe0]);
    } else if (byte3 == 0x0a) {
       // publishing
       if (byte4 > 0xa0)
           return QChar(publishingKeysymsToUnicode[byte4 - 0xa0]);
    } else if (byte3 == 0x0b) {
       // APL
       if (byte4 > 0xa0)
           return QChar(aplKeysymsToUnicode[byte4 - 0xa0]);
    } else if (byte3 == 0x0e) {
       // Korean
       if (byte4 > 0xa0)
           return QChar(koreanKeysymsToUnicode[byte4 - 0xa0]);
    }
    return QChar(0x0);
}
#endif

bool QETWidget::translateKeyEventInternal(const XEvent *event, int& count, QString& text,
                                          Qt::KeyboardModifiers &modifiers, int& code, QEvent::Type &type,
                                          bool /*willRepeat*/, bool statefulTranslation)
{
    QTextCodec *mapper = qt_input_mapper;
    // some XmbLookupString implementations don't return buffer overflow correctly,
    // so we increase the input buffer to allow for long strings...
    // 256 chars * 2 bytes + 1 null-term == 513 bytes
    QByteArray chars;
    chars.resize(513);
    QChar converted;
    KeySym key = 0;

    // ###
    // QWidget* tlw = window();

    XKeyEvent xkeyevent = event->xkey;

    // save the modifier state, we will use the keystate uint later by passing
    // it to translateButtonState
    uint keystate = event->xkey.state;
    // remove the modifiers where mode_switch exists... HPUX machines seem
    // to have alt *AND* mode_switch both in Mod1Mask, which causes
    // XLookupString to return things like '� (aring) for Qt::ALT-A.  This
    // completely breaks modifiers.  If we remove the modifier for Mode_switch,
    // then things work correctly...
    xkeyevent.state &= ~qt_mode_switch_remove_mask;

    type = (event->type == XKeyPress)
           ? QEvent::KeyPress : QEvent::KeyRelease;
#if defined(QT_NO_XIM)

    count = XLookupString(&xkeyevent, chars.data(), chars.size(), &key, 0);

#else
    // Implementation for X11R5 and newer, using XIM

    int               keycode = event->xkey.keycode;
//    Status     status;

    bool mb=false;
    if (!mb) {
        count = XLookupString(&xkeyevent,
                              chars.data(), chars.size(), &key, 0);
    }
    if (count && !keycode) {
        keycode = qt_ximComposingKeycode;
        qt_ximComposingKeycode = 0;
    }
    // all keysyms smaller than that are actally keys that can be mapped
    // to unicode chars
    if (count == 0 && key < 0xff00) {
        unsigned char byte3 = (unsigned char)(key >> 8);
        int mib = -1;
        switch(byte3) {
        case 0: // Latin 1
        case 1: // Latin 2
        case 2: //latin 3
        case 3: // latin4
            mib = byte3 + 4; break;
        case 5: // arabic
            mib = 82; break;
        case 12: // Hebrew
            mib = 85; break;
        case 13: // Thai
            mib = 2259; break;
        case 4: // kana
        case 6: // cyrillic
        case 7: // greek
        case 8: // technical, no mapping here at the moment
        case 9: // Special
        case 10: // Publishing
        case 11: // APL
        case 14: // Korean, no mapping
            mib = -1; // manual conversion
            mapper = 0;
            converted = keysymToUnicode(byte3, key & 0xff);
        case 0x20:
            // currency symbols
            if (key >= 0x20a0 && key <= 0x20ac) {
                mib = -1; // manual conversion
                mapper = 0;
                converted = (uint)key;
            }
            break;
        default:
            break;
        }
        if (mib != -1) {
            mapper = QTextCodec::codecForMib(mib);
            chars[0] = (unsigned char) (key & 0xff); // get only the fourth bit for conversion later
            count++;
        }
    } else if (key >= 0x1000000 && key <= 0x100ffff) {
        converted = (ushort) (key - 0x1000000);
        mapper = 0;
    }
    if (count < (int)chars.size()-1)
        chars[count] = '\0';
    // tlw = 0;
#endif // !QT_NO_XIM

    // convert chars (8bit) to text (unicode).
    if (mapper)
        text = mapper->toUnicode(chars.data(), count, 0);
    else if (!mapper && converted.unicode() != 0x0)
        text = converted;
    else
        text = chars;

    modifiers = translateModifiers(keystate);

    static int directionKeyEvent = 0;
    static unsigned int lastWinId = 0;
    if (statefulTranslation && qt_use_rtl_extensions && type == QEvent::KeyRelease) {
        if (directionKeyEvent == Qt::Key_Direction_R || directionKeyEvent == Qt::Key_Direction_L) {
            type = QEvent::KeyPress;
            code = directionKeyEvent;
            text = QString();
            directionKeyEvent = 0;
	    lastWinId = 0;
            return true;
        } else {
            directionKeyEvent = 0;
	    lastWinId = 0;
        }
    }

    // Watch for keypresses and if its a key belonging to the Ctrl-Shift
    // direction-changing accel, remember it.
    // We keep track of those keys instead of using the event's state
    // (to figure out whether the Ctrl modifier is held while Shift is pressed,
    // or Shift is held while Ctrl is pressed) since the 'state' doesn't tell
    // us whether the modifier held is Left or Right.
    if (statefulTranslation && qt_use_rtl_extensions && type  == QEvent::KeyPress)
        if (key == XK_Control_L || key == XK_Control_R || key == XK_Shift_L || key == XK_Shift_R) {
	    if (!directionKeyEvent) {
		directionKeyEvent = key;
		// This code exists in order to check that
		// the event is occurred in the same widget.
		lastWinId = winId();
	    }
        } else {
            // this can no longer be a direction-changing accel.
            // if any other key was pressed.
            directionKeyEvent = Qt::Key_Space;
        }

    // Commentary in X11/keysymdef says that X codes match ASCII, so it
    // is safe to use the locale functions to process X codes in ISO8859-1.
    //
    // This is mainly for compatibility - applications should not use the
    // Qt keycodes between 128 and 255, but should rather use the
    // QKeyEvent::text().
    //
    if (key < 128 || (key < 256 && (!qt_input_mapper || qt_input_mapper->mibEnum()==4))) {
        code = isprint((int)key) ? toupper((int)key) : 0; // upper-case key, if known
    } else if (text.length() == 1 && text.unicode()->unicode() > 0x1f && text.unicode()->unicode() != 0x7f && !(key >= XK_dead_grave && key <= XK_dead_horn)) {
        code = text.unicode()->toUpper().unicode();
    } else if (key >= XK_F1 && key <= XK_F35) {
        code = Qt::Key_F1 + ((int)key - XK_F1);        // function keys
    } else if (key >= XK_KP_0 && key <= XK_KP_9) {
        code = Qt::Key_0 + ((int)key - XK_KP_0);        // numeric keypad keys
        modifiers |= Qt::KeypadModifier;
    } else {
        int i = 0;                                // any other keys
        while (KeyTbl[i]) {
            if (key == KeyTbl[i]) {
                code = (int)KeyTbl[i+1];
                break;
            }
            i += 2;
        }
        switch (key) {
        case XK_KP_Insert:
        case XK_KP_Delete:
        case XK_KP_Home:
        case XK_KP_End:
        case XK_KP_Left:
        case XK_KP_Up:
        case XK_KP_Right:
        case XK_KP_Down:
        case XK_KP_Prior:
        case XK_KP_Next:
        case XK_KP_Space:
        case XK_KP_Tab:
        case XK_KP_Enter:
        case XK_KP_Equal:
        case XK_KP_Multiply:
        case XK_KP_Add:
        case XK_KP_Separator:
        case XK_KP_Subtract:
        case XK_KP_Decimal:
        case XK_KP_Divide:
            modifiers |= Qt::KeypadModifier;
            break;
        default:
            break;
        }

        if (code == Qt::Key_Tab && (modifiers & Qt::ShiftModifier)) {
            // map shift+tab to shift+backtab, QShortcutMap knows about it
            // and will handle it.
            code = Qt::Key_Backtab;
            text = QString();
        }

        if (statefulTranslation && qt_use_rtl_extensions && type  == QEvent::KeyPress) {
            if (directionKeyEvent && lastWinId == winId()) {
                if (key == XK_Shift_L && directionKeyEvent == XK_Control_L ||
                    key == XK_Control_L && directionKeyEvent == XK_Shift_L) {
                    directionKeyEvent = Qt::Key_Direction_L;
                } else if (key == XK_Shift_R && directionKeyEvent == XK_Control_R ||
                           key == XK_Control_R && directionKeyEvent == XK_Shift_R) {
                    directionKeyEvent = Qt::Key_Direction_R;
                }
            }
            else if (directionKeyEvent == Qt::Key_Direction_L || directionKeyEvent == Qt::Key_Direction_R) {
                directionKeyEvent = Qt::Key_Space; // invalid
            }
        }
    }

    return true;
}


struct qt_auto_repeat_data
{
    // match the window and keycode with timestamp delta of 10ms
    Window window;
    KeyCode keycode;
    Time timestamp;

    // queue scanner state
    bool release;
    bool error;
};

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

static Bool qt_keypress_scanner(Display *, XEvent *event, XPointer arg)
{
    if (event->type != XKeyPress && event->type != XKeyRelease)
        return false;

    qt_auto_repeat_data *data = (qt_auto_repeat_data *) arg;
    if (data->error ||
        event->xkey.window  != data->window ||
        event->xkey.keycode != data->keycode)
        return false;

    if (event->type == XKeyPress) {
        data->error = (! data->release || event->xkey.time - data->timestamp > 10);
        return (! data->error);
    }

    // must be XKeyRelease event
    if (data->release) {
        // found a second release
        data->error = true;
        return false;
    }

    // found a single release
    data->release = true;
    data->timestamp = event->xkey.time;

    return false;
}

static Bool qt_keyrelease_scanner(Display *, XEvent *event, XPointer arg)
{
    const qt_auto_repeat_data *data = (const qt_auto_repeat_data *) arg;
    return (event->type == XKeyRelease &&
            event->xkey.window  == data->window &&
            event->xkey.keycode == data->keycode);
}

#if defined(Q_C_CALLBACKS)
}
#endif

bool QETWidget::translateKeyEvent(const XEvent *event, bool grab)
{
    int           code = -1;
    int           count = 0;
    Qt::KeyboardModifiers modifiers;

    if (sm_blockUserInput) // block user interaction during session management
        return true;

    Display *dpy = X11->display;

    if (!isEnabled())
        return true;

    QEvent::Type type;
    bool    autor = false;
    QString text;

    translateKeyEventInternal(event, count, text, modifiers, code, type,
                               qt_mode_switch_remove_mask != 0);

    // was this the last auto-repeater?
    qt_auto_repeat_data auto_repeat_data;
    auto_repeat_data.window = event->xkey.window;
    auto_repeat_data.keycode = event->xkey.keycode;
    auto_repeat_data.timestamp = event->xkey.time;

    static uint curr_autorep = 0;
    if (event->type == XKeyPress) {
        if (curr_autorep == event->xkey.keycode) {
            autor = true;
            curr_autorep = 0;
        }
    } else {
        // look ahead for auto-repeat
        XEvent nextpress;

        auto_repeat_data.release = true;
        auto_repeat_data.error = false;
        if (XCheckIfEvent(dpy, &nextpress, &qt_keypress_scanner,
                          (XPointer) &auto_repeat_data)) {
            autor = true;

            // Put it back... we COULD send the event now and not need
            // the static curr_autorep variable.
            XPutBackEvent(dpy,&nextpress);
        }
        curr_autorep = autor ? event->xkey.keycode : 0;
    }

#if defined QT3_SUPPORT && !defined(QT_NO_SHORTCUT)
    // process accelerators before doing key compression
    if (type == QEvent::KeyPress && !grab
        && static_cast<QApplicationPrivate*>(qApp->d_ptr)->use_compat()) {
        // send accel events if the keyboard is not grabbed
        QKeyEvent a(type, code, modifiers, text, autor, qMax(qMax(count,1), int(text.length())));
        if (static_cast<QApplicationPrivate*>(qApp->d_ptr)->qt_tryAccelEvent(this, &a))
            return true;
    }
#endif

    long save = 0;
    if (qt_mode_switch_remove_mask != 0) {
        save = qt_mode_switch_remove_mask;
        qt_mode_switch_remove_mask = 0;

        // translate the key event again, but this time apply any Mode_switch
        // modifiers
        translateKeyEventInternal(event, count, text, modifiers, code, type);
    }

#ifndef QT_NO_IM
    QInputContext *qic = inputContext();
#endif

    // compress keys
    if (!text.isEmpty() && testAttribute(Qt::WA_KeyCompression) &&
#ifndef QT_NO_IM
	 // Ordinary input methods require discrete key events to work
	 // properly, so key compression has to be disabled when input
	 // context exists.
	 //
	 // And further consideration, some complex input method
	 // require all key press/release events discretely even if
	 // the input method awares of key compression and compressed
	 // keys are ordinary alphabets. For example, the uim project
	 // is planning to implement "combinational shift" feature for
	 // a Japanese input method, uim-skk. It will work as follows.
	 //
	 // 1. press "r"
	 // 2. press "u"
	 // 3. release both "r" and "u" in arbitrary order
	 // 4. above key sequence generates "Ru"
	 //
	 // Of course further consideration about other participants
	 // such as key repeat mechanism is required to implement such
	 // feature.
	 !qic &&
#endif // QT_NO_IM
         // do not compress keys if the key event we just got above matches
         // one of the key ranges used to compute stopCompression
         !((code >= Qt::Key_Escape && code <= Qt::Key_SysReq)
            || (code >= Qt::Key_Home && code <= Qt::Key_PageDown)
            || (code >= Qt::Key_Super_L && code <= Qt::Key_Direction_R)
            || (code == 0)
            || (text.length() == 1 && text.unicode()->unicode() == '\n'))) {
        // the widget wants key compression so it gets it

        // sync the event queue, this makes key compress work better
        XSync(dpy, false);

        for (;;) {
            XEvent        evRelease;
            XEvent        evPress;
            if (!XCheckTypedWindowEvent(dpy,event->xkey.window,
                                         XKeyRelease,&evRelease))
                break;
            if (!XCheckTypedWindowEvent(dpy,event->xkey.window,
                                         XKeyPress,&evPress)) {
                XPutBackEvent(dpy, &evRelease);
                break;
            }
            QString textIntern;
            int codeIntern = -1;
            int countIntern = 0;
            Qt::KeyboardModifiers modifiersIntern;
            QEvent::Type t;
            translateKeyEventInternal(&evPress, countIntern, textIntern,
                                       modifiersIntern, codeIntern, t);
            // use stopCompression to stop key compression for the following
            // key event ranges:
            bool stopCompression =
                // 1) misc keys
                (codeIntern >= Qt::Key_Escape && codeIntern <= Qt::Key_SysReq)
                // 2) cursor movement
                || (codeIntern >= Qt::Key_Home && codeIntern <= Qt::Key_PageDown)
                // 3) extra keys
                || (codeIntern >= Qt::Key_Super_L && codeIntern <= Qt::Key_Direction_R)
                // 4) something that a) doesn't translate to text or b) translates
                //    to newline text
                || (codeIntern == 0)
                || (textIntern.length() == 1 && textIntern.unicode()->unicode() == '\n');
            if (modifiersIntern == modifiers && !textIntern.isEmpty() && !stopCompression) {
                text += textIntern;
                count += countIntern;
            } else {
                XPutBackEvent(dpy, &evPress);
                XPutBackEvent(dpy, &evRelease);
                break;
            }
        }
    }

    if (save != 0)
        qt_mode_switch_remove_mask = save;

    // autorepeat compression makes sense for all widgets (Windows
    // does it automatically ....)
    if (event->type == XKeyPress && text.length() <= 1
#ifndef QT_NO_IM
	 // input methods need discrete key events
	 && !qic
#endif// QT_NO_IM
	) {
        XEvent dummy;

        for (;;) {
            auto_repeat_data.release = false;
            auto_repeat_data.error = false;
            if (! XCheckIfEvent(dpy, &dummy, &qt_keypress_scanner,
                                (XPointer) &auto_repeat_data))
                break;
            if (! XCheckIfEvent(dpy, &dummy, &qt_keyrelease_scanner,
                                (XPointer) &auto_repeat_data))
                break;

            count++;
            if (!text.isEmpty())
                text += text[0];
        }
    }

    if (text.length() == 1 && text.unicode()->unicode() == '\n') {
        code = Qt::Key_Return;
        text = "\r";
    }

    // try the menukey first
    if (type == QEvent::KeyPress && code == Qt::Key_Menu) {
        QPoint pos = inputMethodQuery(Qt::ImMicroFocus).toRect().center();
        QContextMenuEvent e(QContextMenuEvent::Keyboard, pos, mapToGlobal(pos));
        QApplication::sendSpontaneousEvent(this, &e);
        if(e.isAccepted())
            return true;
    }

    QKeyEvent e(type, code, modifiers, text, autor, qMax(qMax(count,1), int(text.length())));
    return QApplication::sendSpontaneousEvent(this, &e);
}


//
// Paint event translation
//
// When receiving many expose events, we compress them (union of all expose
// rectangles) into one event which is sent to the widget.

struct PaintEventInfo {
    Window window;
};

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

static Bool isPaintOrScrollDoneEvent(Display *, XEvent *ev, XPointer a)
{
    PaintEventInfo *info = (PaintEventInfo *)a;
    if (ev->type == Expose || ev->type == GraphicsExpose
      ||    ev->type == ClientMessage
         && ev->xclient.message_type == ATOM(_QT_SCROLL_DONE))
    {
        if (ev->xexpose.window == info->window)
            return True;
    }
    return False;
}

#if defined(Q_C_CALLBACKS)
}
#endif



static
bool translateBySips(QWidget* that, QRect& paintRect)
{
    int dx=0, dy=0;
    int sips=0;
    for (int i = 0; i < X11->sip_list.size(); ++i) {
        const QX11Data::ScrollInProgress &sip = X11->sip_list.at(i);
        if (sip.scrolled_widget == that) {
            if (sips) {
                dx += sip.dx;
                dy += sip.dy;
            }
            sips++;
        }
    }
    if (sips > 1) {
        paintRect.translate(dx, dy);
        return true;
    }
    return false;
}

void QETWidget::removePendingPaintEvents()
{
    XEvent xevent;
    PaintEventInfo info;
    info.window = winId();
    while (XCheckIfEvent(X11->display,&xevent,isPaintOrScrollDoneEvent,
                         (XPointer)&info) &&
           !qt_x11EventFilter(&xevent)  &&
           !x11Event(&xevent)) // send event through filter
        ;
}



void QETWidget::translatePaintEvent(const XEvent *event)
{
    Q_D(QWidget);
    QRect  paintRect(event->xexpose.x, event->xexpose.y,
                     event->xexpose.width, event->xexpose.height);
    XEvent xevent;
    PaintEventInfo info;
    info.window = winId();
    translateBySips(this, paintRect);
    paintRect = d->mapFromWS(paintRect);

    QRegion paintRegion = d->invalidated_region;
    paintRegion |= paintRect;
    d->invalidated_region = QRegion();
    QRect clipRect = d->clipRect();
    paintRegion &= clipRect;

    // WARNING: this is O(number_of_events * number_of_matching_events)
    while (XCheckIfEvent(X11->display,&xevent,isPaintOrScrollDoneEvent,
                         (XPointer)&info) &&
           !qt_x11EventFilter(&xevent)  &&
           !x11Event(&xevent)) // send event through filter
    {
        if (xevent.type == Expose || xevent.type == GraphicsExpose) {
            QRect exposure(xevent.xexpose.x,
                           xevent.xexpose.y,
                           xevent.xexpose.width,
                           xevent.xexpose.height);
            translateBySips(this, exposure);
            exposure = d->mapFromWS(exposure);
            if (clipRect.contains(exposure)) {
                paintRegion = paintRegion.unite(exposure);
            } else if (!paintRegion.isEmpty()) {
                repaint(paintRegion);
                paintRegion = exposure;
            }
        } else {
            translateScrollDoneEvent(&xevent);
        }
    }

    if (!paintRegion.isEmpty())
        repaint(paintRegion);
}

//
// Scroll-done event translation.
//

bool QETWidget::translateScrollDoneEvent(const XEvent *event)
{
    long id = event->xclient.data.l[0];

    // Remove any scroll-in-progress record for the given id.
    for (int i = 0; i < X11->sip_list.size(); ++i) {
        const QX11Data::ScrollInProgress &sip = X11->sip_list.at(i);
        if (sip.id == id) {
            X11->sip_list.removeAt(i);
            return true;
        }
    }

    return false;
}

//
// ConfigureNotify (window move and resize) event translation

bool QETWidget::translateConfigEvent(const XEvent *event)
{
    Q_D(QWidget);
    bool wasResize = testAttribute(Qt::WA_WState_ConfigPending); // set in QWidget::setGeometry_sys()
    setAttribute(Qt::WA_WState_ConfigPending, false);

    if (isWindow()) {
        QPoint newCPos(geometry().topLeft());
        QSize  newSize(event->xconfigure.width, event->xconfigure.height);

        bool trust = (d->topData()->parentWinId == XNone ||
                      d->topData()->parentWinId == QX11Info::appRootWindow());

        if (event->xconfigure.send_event || trust) {
            // if a ConfigureNotify comes from a real sendevent request, we can
            // trust its values.
            newCPos.rx() = event->xconfigure.x + event->xconfigure.border_width;
            newCPos.ry() = event->xconfigure.y + event->xconfigure.border_width;
        }

        if (isVisible())
            QApplication::syncX();

        if (! d->extra || d->extra->compress_events) {
            // ConfigureNotify compression for faster opaque resizing
            XEvent otherEvent;
            while (XCheckTypedWindowEvent(X11->display, winId(), ConfigureNotify,
                                            &otherEvent)) {
                if (qt_x11EventFilter(&otherEvent))
                    continue;

                if (x11Event(&otherEvent))
                    continue;

                if (otherEvent.xconfigure.event != otherEvent.xconfigure.window)
                    continue;

                newSize.setWidth(otherEvent.xconfigure.width);
                newSize.setHeight(otherEvent.xconfigure.height);

                if (otherEvent.xconfigure.send_event || trust) {
                    newCPos.rx() = otherEvent.xconfigure.x +
                                   otherEvent.xconfigure.border_width;
                    newCPos.ry() = otherEvent.xconfigure.y +
                                   otherEvent.xconfigure.border_width;
                }
            }
        }

        QRect cr (geometry());
        if (newCPos != cr.topLeft()) { // compare with cpos (exluding frame)
            QPoint oldPos = geometry().topLeft();
            cr.moveTopLeft(newCPos);
            data->crect = cr;
            if (isVisible()) {
                QMoveEvent e(newCPos, oldPos); // pos (including frame), not cpos
                QApplication::sendSpontaneousEvent(this, &e);
            } else {
                setAttribute(Qt::WA_PendingMoveEvent, true);
            }
        }
        if (newSize != cr.size()) { // size changed
            QSize oldSize = size();
            cr.setSize(newSize);
            data->crect = cr;

            if (isVisible()) {
                QResizeEvent e(newSize, oldSize);
                QApplication::sendSpontaneousEvent(this, &e);
            } else {
                setAttribute(Qt::WA_PendingResizeEvent, true);
            }
            wasResize = true;
        }

    } else {
        XEvent xevent;
        while (XCheckTypedWindowEvent(X11->display,winId(), ConfigureNotify,&xevent) &&
                !qt_x11EventFilter(&xevent)  &&
                !x11Event(&xevent)) // send event through filter
            ;
    }

    if (wasResize && !testAttribute(Qt::WA_StaticContents)) {
        removePendingPaintEvents();
        testAttribute(Qt::WA_WState_InPaintEvent)?update():repaint();
    }

    return true;
}

//
// Close window event translation.
//
bool QETWidget::translateCloseEvent(const XEvent *)
{
    Q_D(QWidget);
    return d->close_helper(QWidgetPrivate::CloseWithSpontaneousEvent);
}


/*!
    \property QApplication::cursorFlashTime
    \brief the text cursor's flash (blink) time in milliseconds

    The flash time is the time required to display, invert and
    restore the caret display. Usually the text cursor is displayed
    for half the cursor flash time, then hidden for the same amount
    of time, but this may vary.

    The default value on X11 is 1000 milliseconds. On Windows, the
    control panel value is used. Widgets should not cache this value
    since it may be changed at any time by the user changing the
    global desktop settings.

    Note that on Microsoft Windows, setting this property sets the
    cursor flash time for all applications.
*/

void QApplication::setCursorFlashTime(int msecs)
{
    QApplicationPrivate::cursor_flash_time = msecs;
}

int QApplication::cursorFlashTime()
{
    return QApplicationPrivate::cursor_flash_time;
}

/*!
    \property QApplication::doubleClickInterval
    \brief the time limit in milliseconds that distinguishes a double click from two
    consecutive mouse clicks

    The default value on X11 is 400 milliseconds. On Windows and Mac
    OS X, the operating system's value is used.

    On Microsoft Windows, calling this function sets the
    double click interval for all applications.
*/

void QApplication::setDoubleClickInterval(int ms)
{
    QApplicationPrivate::mouse_double_click_time = ms;
}

int QApplication::doubleClickInterval()
{
    return QApplicationPrivate::mouse_double_click_time;
}

/*!
    \property QApplication::keyboardInputInterval
    \brief the time limit in milliseconds that distinguishes a key press
    from two consecutive key presses

    The default value on X11 is 400 milliseconds. On Windows and Mac OS X, the
    operating system's value is used.
*/

void QApplication::setKeyboardInputInterval(int ms)
{
    QApplicationPrivate::keyboard_input_time = ms;
}

int QApplication::keyboardInputInterval()
{
    return QApplicationPrivate::keyboard_input_time;
}


/*!
    \property QApplication::wheelScrollLines
    \brief the number of lines to scroll when the mouse wheel is rotated

    If this number exceeds the number of visible lines in a certain
    widget, the widget should interpret the scroll operation as a
    single "page up" or "page down" operation instead.
*/
void QApplication::setWheelScrollLines(int n)
{
    QApplicationPrivate::wheel_scroll_lines = n;
}

int QApplication::wheelScrollLines()
{
    return QApplicationPrivate::wheel_scroll_lines;
}

/*!
    Enables the UI effect \a effect if \a enable is true, otherwise
    the effect will not be used.

    Note: All effects are disabled on screens running at less than
    16-bit color depth.

    \sa isEffectEnabled(), Qt::UIEffect, setDesktopSettingsAware()
*/
void QApplication::setEffectEnabled(Qt::UIEffect effect, bool enable)
{
    switch (effect) {
    case Qt::UI_AnimateMenu:
        if (enable) QApplicationPrivate::fade_menu = false;
        QApplicationPrivate::animate_menu = enable;
        break;
    case Qt::UI_FadeMenu:
        if (enable)
            QApplicationPrivate::animate_menu = true;
        QApplicationPrivate::fade_menu = enable;
        break;
    case Qt::UI_AnimateCombo:
        QApplicationPrivate::animate_combo = enable;
        break;
    case Qt::UI_AnimateTooltip:
        if (enable) QApplicationPrivate::fade_tooltip = false;
        QApplicationPrivate::animate_tooltip = enable;
        break;
    case Qt::UI_FadeTooltip:
        if (enable)
            QApplicationPrivate::animate_tooltip = true;
        QApplicationPrivate::fade_tooltip = enable;
        break;
    case Qt::UI_AnimateToolBox:
        QApplicationPrivate::animate_toolbox = enable;
        break;
    default:
        QApplicationPrivate::animate_ui = enable;
        break;
    }
}

/*!
    Returns true if \a effect is enabled; otherwise returns false.

    By default, Qt will try to use the desktop settings. Call
    setDesktopSettingsAware(false) to prevent this.

    Note: All effects are disabled on screens running at less than
    16-bit color depth.

    \sa setEffectEnabled(), Qt::UIEffect
*/
bool QApplication::isEffectEnabled(Qt::UIEffect effect)
{
    if (QColormap::instance().depth() < 16 || !QApplicationPrivate::animate_ui)
        return false;

    switch(effect) {
    case Qt::UI_AnimateMenu:
        return QApplicationPrivate::animate_menu;
    case Qt::UI_FadeMenu:
        return QApplicationPrivate::fade_menu;
    case Qt::UI_AnimateCombo:
        return QApplicationPrivate::animate_combo;
    case Qt::UI_AnimateTooltip:
        return QApplicationPrivate::animate_tooltip;
    case Qt::UI_FadeTooltip:
        return QApplicationPrivate::fade_tooltip;
    case Qt::UI_AnimateToolBox:
        return QApplicationPrivate::animate_toolbox;
    default:
        return QApplicationPrivate::animate_ui;
    }
}

/*****************************************************************************
  Session management support
 *****************************************************************************/

#ifndef QT_NO_SM_SUPPORT

#include <X11/SM/SMlib.h>

class QSessionManagerPrivate : public QObjectPrivate
{
public:
    QSessionManagerPrivate(QSessionManager* mgr, QString& id, QString& key)
        : QObjectPrivate(), sm(mgr), sessionId(id), sessionKey(key), eventLoop(0) {}
    QSessionManager* sm;
    QStringList restartCommand;
    QStringList discardCommand;
    QString& sessionId;
    QString& sessionKey;
    QSessionManager::RestartHint restartHint;
    QEventLoop *eventLoop;
};

class QSmSocketReceiver : public QObject
{
    Q_OBJECT
public:
    QSmSocketReceiver(int socket)
        {
            QSocketNotifier* sn = new QSocketNotifier(socket, QSocketNotifier::Read, this);
            connect(sn, SIGNAL(activated(int)), this, SLOT(socketActivated(int)));
        }

public slots:
     void socketActivated(int);
};


static SmcConn smcConnection = 0;
static bool sm_interactionActive;
static bool sm_smActive;
static int sm_interactStyle;
static int sm_saveType;
static bool sm_cancel;
// static bool sm_waitingForPhase2;  ### never used?!?
static bool sm_waitingForInteraction;
static bool sm_isshutdown;
// static bool sm_shouldbefast;  ### never used?!?
static bool sm_phase2;
static bool sm_in_phase2;

static QSmSocketReceiver* sm_receiver = 0;

static void resetSmState();
static void sm_setProperty(const char* name, const char* type,
                            int num_vals, SmPropValue* vals);
static void sm_saveYourselfCallback(SmcConn smcConn, SmPointer clientData,
                                  int saveType, Bool shutdown , int interactStyle, Bool fast);
static void sm_saveYourselfPhase2Callback(SmcConn smcConn, SmPointer clientData) ;
static void sm_dieCallback(SmcConn smcConn, SmPointer clientData) ;
static void sm_shutdownCancelledCallback(SmcConn smcConn, SmPointer clientData);
static void sm_saveCompleteCallback(SmcConn smcConn, SmPointer clientData);
static void sm_interactCallback(SmcConn smcConn, SmPointer clientData);
static void sm_performSaveYourself(QSessionManagerPrivate*);

static void resetSmState()
{
//    sm_waitingForPhase2 = false; ### never used?!?
    sm_waitingForInteraction = false;
    sm_interactionActive = false;
    sm_interactStyle = SmInteractStyleNone;
    sm_smActive = false;
    sm_blockUserInput = false;
    sm_isshutdown = false;
//    sm_shouldbefast = false; ### never used?!?
    sm_phase2 = false;
    sm_in_phase2 = false;
}


// theoretically it's possible to set several properties at once. For
// simplicity, however, we do just one property at a time
static void sm_setProperty(const char* name, const char* type,
                            int num_vals, SmPropValue* vals)
{
    if (num_vals) {
      SmProp prop;
      prop.name = (char*)name;
      prop.type = (char*)type;
      prop.num_vals = num_vals;
      prop.vals = vals;

      SmProp* props[1];
      props[0] = &prop;
      SmcSetProperties(smcConnection, 1, props);
    }
    else {
      char* names[1];
      names[0] = (char*) name;
      SmcDeleteProperties(smcConnection, 1, names);
    }
}

static void sm_setProperty(const QString& name, const QString& value)
{
    QByteArray v = value.toUtf8();
    SmPropValue prop;
    prop.length = v.length();
    prop.value = (SmPointer) v.constData();
    sm_setProperty(name.toLatin1().data(), SmARRAY8, 1, &prop);
}

static void sm_setProperty(const QString& name, const QStringList& value)
{
    SmPropValue *prop = new SmPropValue[value.count()];
    int count = 0;
    QList<QByteArray> vl;
    for (QStringList::ConstIterator it = value.begin(); it != value.end(); ++it) {
      prop[count].length = (*it).length();
      vl.append((*it).toUtf8());
      prop[count].value = (char*)vl.last().data();
      ++count;
    }
    sm_setProperty(name.toLatin1().data(), SmLISTofARRAY8, count, prop);
    delete [] prop;
}


// workaround for broken libsm, see below
struct QT_smcConn {
    unsigned int save_yourself_in_progress : 1;
    unsigned int shutdown_in_progress : 1;
};

static void sm_saveYourselfCallback(SmcConn smcConn, SmPointer clientData,
                                  int saveType, Bool shutdown , int interactStyle, Bool /*fast*/)
{
    if (smcConn != smcConnection)
        return;
    sm_cancel = false;
    sm_smActive = true;
    sm_isshutdown = shutdown;
    sm_saveType = saveType;
    sm_interactStyle = interactStyle;
//    sm_shouldbefast = fast; ### never used?!?

    // ugly workaround for broken libSM. libSM should do that _before_
    // actually invoking the callback in sm_process.c
    ((QT_smcConn*)smcConn)->save_yourself_in_progress = true;
    if (sm_isshutdown)
        ((QT_smcConn*)smcConn)->shutdown_in_progress = true;

    sm_performSaveYourself((QSessionManagerPrivate*) clientData);
    if (!sm_isshutdown) // we cannot expect a confirmation message in that case
        resetSmState();
}

static void sm_performSaveYourself(QSessionManagerPrivate* smd)
{
    if (sm_isshutdown)
        sm_blockUserInput = true;

    QSessionManager* sm = smd->sm;

    // generate a new session key
    timeval tv;
    gettimeofday(&tv, 0);
    smd->sessionKey  = QString::number(qulonglong(tv.tv_sec)) + "_" + QString::number(qulonglong(tv.tv_usec));

    // tell the session manager about our program in best POSIX style
    sm_setProperty(SmProgram, QString(qApp->argv()[0]));
    // tell the session manager about our user as well.
    struct passwd* entry = getpwuid(geteuid());
    if (entry)
        sm_setProperty(SmUserID, QString::fromLatin1(entry->pw_name));

    // generate a restart and discard command that makes sense
    QStringList restart;
    restart  << qApp->argv()[0] << "-session" << smd->sessionId + "_" + smd->sessionKey;
    if (qstricmp(appName, QX11Info::appClass()) != 0)
        restart << "-name" << qAppName();
    sm->setRestartCommand(restart);
    QStringList discard;
    sm->setDiscardCommand(discard);

    switch (sm_saveType) {
    case SmSaveBoth:
        qApp->commitData(*sm);
        if (sm_isshutdown && sm_cancel)
            break; // we cancelled the shutdown, no need to save state
    // fall through
    case SmSaveLocal:
        qApp->saveState(*sm);
        break;
    case SmSaveGlobal:
        qApp->commitData(*sm);
        break;
    default:
        break;
    }

    if (sm_phase2 && !sm_in_phase2) {
        SmcRequestSaveYourselfPhase2(smcConnection, sm_saveYourselfPhase2Callback, (SmPointer*) smd);
        sm_blockUserInput = false;
    }
    else {
        // close eventual interaction monitors and cancel the
        // shutdown, if required. Note that we can only cancel when
        // performing a shutdown, it does not work for checkpoints
        if (sm_interactionActive) {
            SmcInteractDone(smcConnection, sm_isshutdown && sm_cancel);
            sm_interactionActive = false;
        }
        else if (sm_cancel && sm_isshutdown) {
            if (sm->allowsErrorInteraction()) {
                SmcInteractDone(smcConnection, True);
                sm_interactionActive = false;
            }
        }

        // set restart and discard command in session manager
        sm_setProperty(SmRestartCommand, sm->restartCommand());
        sm_setProperty(SmDiscardCommand, sm->discardCommand());

        // set the restart hint
        SmPropValue prop;
        prop.length = sizeof(int);
        int value = sm->restartHint();
        prop.value = (SmPointer) &value;
        sm_setProperty(SmRestartStyleHint, SmCARD8, 1, &prop);

        // we are done
        SmcSaveYourselfDone(smcConnection, !sm_cancel);
    }
}

static void sm_dieCallback(SmcConn smcConn, SmPointer /* clientData */)
{
    if (smcConn != smcConnection)
        return;
    resetSmState();
    QEvent quitEvent(QEvent::Quit);
    QApplication::sendEvent(qApp, &quitEvent);
}

static void sm_shutdownCancelledCallback(SmcConn smcConn, SmPointer clientData)
{
    if (smcConn != smcConnection)
        return;
    if (sm_waitingForInteraction)
        ((QSessionManagerPrivate *) clientData)->eventLoop->exit();
    resetSmState();
}

static void sm_saveCompleteCallback(SmcConn smcConn, SmPointer /*clientData */)
{
    if (smcConn != smcConnection)
        return;
    resetSmState();
}

static void sm_interactCallback(SmcConn smcConn, SmPointer clientData)
{
    if (smcConn != smcConnection)
        return;
    if (sm_waitingForInteraction)
        ((QSessionManagerPrivate *) clientData)->eventLoop->exit();
}

static void sm_saveYourselfPhase2Callback(SmcConn smcConn, SmPointer clientData)
{
    if (smcConn != smcConnection)
        return;
    sm_in_phase2 = true;
    sm_performSaveYourself((QSessionManagerPrivate*) clientData);
}


void QSmSocketReceiver::socketActivated(int)
{
    IceProcessMessages(SmcGetIceConnection(smcConnection), 0, 0);
}


#undef Bool
#include "qapplication_x11.moc"

QSessionManager::QSessionManager(QApplication * app, QString &id, QString& key)
    : QObject(*new QSessionManagerPrivate(this, id, key), app)
{
    Q_D(QSessionManager);
    d->restartHint = RestartIfRunning;

    resetSmState();
    char cerror[256];
    char* myId = 0;
    QByteArray b_id = id.toLatin1();
    char* prevId = b_id.data();

    SmcCallbacks cb;
    cb.save_yourself.callback = sm_saveYourselfCallback;
    cb.save_yourself.client_data = (SmPointer) d;
    cb.die.callback = sm_dieCallback;
    cb.die.client_data = (SmPointer) d;
    cb.save_complete.callback = sm_saveCompleteCallback;
    cb.save_complete.client_data = (SmPointer) d;
    cb.shutdown_cancelled.callback = sm_shutdownCancelledCallback;
    cb.shutdown_cancelled.client_data = (SmPointer) d;

    // avoid showing a warning message below
    if (qgetenv("SESSION_MANAGER").isEmpty())
        return;

    smcConnection = SmcOpenConnection(0, 0, 1, 0,
                                       SmcSaveYourselfProcMask |
                                       SmcDieProcMask |
                                       SmcSaveCompleteProcMask |
                                       SmcShutdownCancelledProcMask,
                                       &cb,
                                       prevId,
                                       &myId,
                                       256, cerror);

    id = QString::fromLatin1(myId);
    ::free(myId); // it was allocated by C

    QString error = cerror;
    if (!smcConnection) {
        qWarning("Session management error: %s", error.toLatin1().data());
    }
    else {
        sm_receiver = new QSmSocketReceiver(IceConnectionNumber(SmcGetIceConnection(smcConnection)));
    }
}

QSessionManager::~QSessionManager()
{
    if (smcConnection)
        SmcCloseConnection(smcConnection, 0, 0);
    smcConnection = 0;
    delete sm_receiver;
}

QString QSessionManager::sessionId() const
{
    Q_D(const QSessionManager);
    return d->sessionId;
}

QString QSessionManager::sessionKey() const
{
    Q_D(const QSessionManager);
    return d->sessionKey;
}


void* QSessionManager::handle() const
{
    return (void*) smcConnection;
}


bool QSessionManager::allowsInteraction()
{
    Q_D(QSessionManager);
    if (sm_interactionActive)
        return true;

    if (sm_waitingForInteraction)
        return false;

    if (sm_interactStyle == SmInteractStyleAny) {
        sm_waitingForInteraction =  SmcInteractRequest(smcConnection, SmDialogNormal,
                                                        sm_interactCallback, (SmPointer*) this);
    }
    if (sm_waitingForInteraction) {
        QEventLoop eventLoop;
        d->eventLoop = &eventLoop;
        (void) eventLoop.exec();
        d->eventLoop = 0;

        sm_waitingForInteraction = false;
        if (sm_smActive) { // not cancelled
            sm_interactionActive = true;
            sm_blockUserInput = false;
            return true;
        }
    }
    return false;
}

bool QSessionManager::allowsErrorInteraction()
{
    Q_D(QSessionManager);
    if (sm_interactionActive)
        return true;

    if (sm_waitingForInteraction)
        return false;

    if (sm_interactStyle == SmInteractStyleAny || sm_interactStyle == SmInteractStyleErrors) {
        sm_waitingForInteraction =  SmcInteractRequest(smcConnection, SmDialogError,
                                                        sm_interactCallback, (SmPointer*) this);
    }
    if (sm_waitingForInteraction) {
        QEventLoop eventLoop;
        d->eventLoop = &eventLoop;
        (void) eventLoop.exec();
        d->eventLoop = 0;

        sm_waitingForInteraction = false;
        if (sm_smActive) { // not cancelled
            sm_interactionActive = true;
            sm_blockUserInput = false;
            return true;
        }
    }
    return false;
}

void QSessionManager::release()
{
    if (sm_interactionActive) {
        SmcInteractDone(smcConnection, False);
        sm_interactionActive = false;
        if (sm_smActive && sm_isshutdown)
            sm_blockUserInput = true;
    }
}

void QSessionManager::cancel()
{
    sm_cancel = true;
}

void QSessionManager::setRestartHint(QSessionManager::RestartHint hint)
{
    Q_D(QSessionManager);
    d->restartHint = hint;
}

QSessionManager::RestartHint QSessionManager::restartHint() const
{
    Q_D(const QSessionManager);
    return d->restartHint;
}

void QSessionManager::setRestartCommand(const QStringList& command)
{
    Q_D(QSessionManager);
    d->restartCommand = command;
}

QStringList QSessionManager::restartCommand() const
{
    Q_D(const QSessionManager);
    return d->restartCommand;
}

void QSessionManager::setDiscardCommand(const QStringList& command)
{
    Q_D(QSessionManager);
    d->discardCommand = command;
}

QStringList QSessionManager::discardCommand() const
{
    Q_D(const QSessionManager);
    return d->discardCommand;
}

void QSessionManager::setManagerProperty(const QString& name, const QString& value)
{
    sm_setProperty(name, value);
}

void QSessionManager::setManagerProperty(const QString& name, const QStringList& value)
{
    sm_setProperty(name, value);
}

bool QSessionManager::isPhase2() const
{
    return sm_in_phase2;
}

void QSessionManager::requestPhase2()
{
    sm_phase2 = true;
}


#endif // QT_NO_SM_SUPPORT
