/****************************************************************************
**
** Definition of Windows functions, types and constants for the Windows.
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
** EDITIONS: PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QWINDOWDEFS_WIN_H
#ifndef QT_H
#endif // QT_H
#define QWINDOWDEFS_WIN_H

#ifndef QT_H
#endif // QT_H

#if defined(Q_CC_BOR) && !defined(__CONSOLE__)
#define NEEDS_QMAIN
#elif defined(Q_CC_WAT)
#define NEEDS_QMAIN
#endif

#if !defined(Q_NOWINSTRICT)
#define Q_WINSTRICT
#endif

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

#ifndef HINSTANCE
Q_DECLARE_HANDLE(HINSTANCE);
#endif
#ifndef HDC
Q_DECLARE_HANDLE(HDC);
#endif
#ifndef HWND
Q_DECLARE_HANDLE(HWND);
#endif
#ifndef HFONT
Q_DECLARE_HANDLE(HFONT);
#endif
#ifndef HPEN
Q_DECLARE_HANDLE(HPEN);
#endif
#ifndef HBRUSH
Q_DECLARE_HANDLE(HBRUSH);
#endif
#ifndef HBITMAP
Q_DECLARE_HANDLE(HBITMAP);
#endif
#ifndef HICON
Q_DECLARE_HANDLE(HICON);
#endif
#ifndef HCURSOR
typedef HICON HCURSOR;
#endif
#ifndef HPALETTE
Q_DECLARE_HANDLE(HPALETTE);
#endif
#ifndef HRGN
Q_DECLARE_HANDLE(HRGN);
#endif
#ifndef HMONITOR
Q_DECLARE_HANDLE(HMONITOR);
#endif
#ifndef HRESULT
typedef long HRESULT;
#endif

typedef struct tagMSG MSG;
typedef HWND WId;

Q_EXPORT HINSTANCE qWinAppInst();
Q_EXPORT HINSTANCE qWinAppPrevInst();
Q_EXPORT int	   qWinAppCmdShow();
Q_EXPORT HDC	   qt_display_dc();

#define QT_WIN_PAINTER_MEMBERS \
    HDC		hdc;		\
    HPEN	hpen;		\
    HFONT	hfont;		\
    HBRUSH	hbrush;		\
    HBITMAP	hbrushbm;	\
    HPALETTE	holdpal;	\
    uint	nocolBrush  : 1;\
    uint	pixmapBrush : 1;\
    bool	nativeXForm( bool );

#endif
