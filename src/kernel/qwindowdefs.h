/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qwindowdefs.h#151 $
**
** Definition of general window system dependent functions, types and
** constants
**
** Created : 931029
**
** Copyright (C) 1992-2000 Troll Tech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Troll Tech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** Licensees holding valid Qt Professional Edition licenses may use this
** file in accordance with the Qt Professional Edition License Agreement
** provided with the Qt Professional Edition.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
** information about the Professional Edition licensing, or see
** http://www.trolltech.com/qpl/ for QPL licensing information.
**
*****************************************************************************/

#ifndef QWINDOWDEFS_H
#define QWINDOWDEFS_H

#ifndef QT_H
#include "qobjectdefs.h"
#include "qstring.h"
#include "qnamespace.h"
#endif // QT_H


// Class forward definitions

class QPaintDevice;
class QPaintDeviceMetrics;
class QWidget;
class QWidgetMapper;
class QDialog;
class QColor;
class QColorGroup;
class QPalette;
class QCursor;
class QPoint;
class QSize;
class QRect;
class QPointArray;
class QPainter;
class QRegion;
class QFont;
class QFontMetrics;
class QFontInfo;
class QPen;
class QBrush;
class QWMatrix;
class QPixmap;
class QBitmap;
class QMovie;
class QImage;
class QImageIO;
class QPicture;
class QPrinter;
class QAccel;
class QTimer;
class QTime;
class QClipboard;


// Widget list (defined in qwidgetlist.h)

class QWidgetList;
class QWidgetListIt;


// Window system dependent definitions

#if defined(_WS_MAC_)

typedef void * HANDLE;
typedef int WId;
typedef void * MSG;

#endif


#if defined(_WS_WIN_)

#if defined(_CC_BOR_) || defined(_CC_WAT_)
#define NEEDS_QMAIN
#endif

#if !defined(Q_NOWINSTRICT)
#define Q_WINSTRICT
#endif

typedef void *HANDLE;

#if defined(Q_WINSTRICT)

#if !defined(STRICT)
#define STRICT
#endif
#undef NO_STRICT
#define Q_DECLARE_HANDLE(name) struct name##__; typedef struct name##__ *name

#else

#if !defined(NO_STRICT)
#define NO_STRICT
#endif
#undef  STRICT
#define Q_DECLARE_HANDLE(name) typedef HANDLE name

#endif

Q_DECLARE_HANDLE(HINSTANCE);
Q_DECLARE_HANDLE(HDC);
Q_DECLARE_HANDLE(HWND);
Q_DECLARE_HANDLE(HFONT);
Q_DECLARE_HANDLE(HPEN);
Q_DECLARE_HANDLE(HBRUSH);
Q_DECLARE_HANDLE(HBITMAP);
Q_DECLARE_HANDLE(HICON);
typedef HICON HCURSOR;
Q_DECLARE_HANDLE(HPALETTE);
Q_DECLARE_HANDLE(HRGN);

typedef struct tagMSG MSG;
typedef HWND  WId;


Q_EXPORT HINSTANCE qWinAppInst();
Q_EXPORT HINSTANCE qWinAppPrevInst();
Q_EXPORT int	   qWinAppCmdShow();
Q_EXPORT HDC	   qt_display_dc();

#endif // _WS_WIN_


#if defined(_WS_X11_)

typedef unsigned int  WId;
typedef unsigned int  HANDLE;
typedef struct _XDisplay Display;
typedef union  _XEvent XEvent;
typedef struct _XGC *GC;
typedef struct _XRegion *Region;

Q_EXPORT Display *qt_xdisplay();
Q_EXPORT int	 qt_xscreen();
Q_EXPORT WId	 qt_xrootwin();
Q_EXPORT GC	 qt_xget_readonly_gc( bool monochrome=FALSE );
Q_EXPORT GC	 qt_xget_temp_gc( bool monochrome=FALSE );

#endif // _WS_X11_

#if defined(_WS_QWS_)

#include "qlock_qws.h"

struct QWSEvent;
typedef unsigned int WId;
typedef void* HANDLE;

class QWSDisplayData;
class QGfx;
class QWSRegionManager;

class QWSDisplay
{
    friend class QApplication;
    QWSDisplayData *d;

public:
    QWSDisplay();

    bool eventPending() const;
    QWSEvent *getEvent();
    QGfx * screenGfx();
    QWSRegionManager *regionManager();

    uchar* frameBuffer() const;
    int width() const;
    int height() const;
    int depth() const;
    int greenDepth() const;

    void addProperty( int winId, int property );
    void setProperty( int winId, int property, int mode, const QByteArray &data );
    void removeProperty( int winId, int property );
    bool getProperty( int winId, int property, char *&data, int &len );

    void requestRegion( int winId, QRegion );
    void moveRegion( int winId, int dx, int dy );
    void destroyRegion( int winId );
    void requestFocus(int winId, bool get);
    void setAltitude( int winId, int altitude, bool fixed = FALSE );
    int takeId();
    void setSelectionOwner( int winId, const QTime &time );
    void convertSelection( int winId, int selectionProperty, const QString &mimeTypes );
    void defineCursor(int id, const QBitmap &curs, const QBitmap &mask,
			int hotX, int hotY);
    void selectCursor( WId winId, unsigned int id );
    void grabMouse( WId winId, bool grab );

    // Lock display for access only by this process
    static bool initLock( const QString &filename, bool create = FALSE );
    static bool grabbed() { return lock->locked(); }
    static void grab() { lock->lock( QLock::Write ); }
    static void ungrab() { lock->unlock(); }

private:
    int getPropertyLen;
    char *getPropertyData;
    static QLock *lock;
};

#endif // _WS_QWS_

class QApplication;

#if defined(NEEDS_QMAIN)
#define main qMain
#endif


// Global platform-independent types and functions


typedef Q_INT32 QCOORD;				// coordinate type
const QCOORD QCOORD_MAX =  2147483647;
const QCOORD QCOORD_MIN = -QCOORD_MAX - 1;

typedef unsigned int QRgb;			// RGB triplet

Q_EXPORT char *qAppName();			// get application name


// Misc functions

typedef void (*Q_CleanUpFunction)();
Q_EXPORT void qAddPostRoutine( Q_CleanUpFunction );


Q_EXPORT void *qt_find_obj_child( QObject *, const char *, const char * );
#define Q_CHILD(parent,type,name) \
	((type*)qt_find_obj_child(parent,#type,name))


#endif // QWINDOWDEFS_H
