/****************************************************************************
**
** Implementation of win32 ActiveX server DLL routines.
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of the Active Qt integration.
** EDITIONS: PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <qapplication.h>
#include <qwidgetlist.h>

#include <qt_windows.h>
#include "../shared/types.h"

// in qaxservermain.cpp
extern char qAxModuleFilename[MAX_PATH];
extern bool qAxIsServer;
extern ITypeLib *qAxTypeLibrary;
extern unsigned long qAxLockCount();
extern void qAxInit();
extern void qAxCleanup();
extern void* qAxInstance;

extern HRESULT UpdateRegistry(int bRegister);
extern HRESULT GetClassObject( const GUID &clsid, const GUID &iid, void **ppUnk );

// Some local variables to handle module lifetime
static bool qAxActivity = FALSE;
static long qAxModuleRef = 0;
static CRITICAL_SECTION qAxModuleSection;

bool qax_ownQApp = FALSE;

// in qeventloop_win.cpp
extern Q_EXPORT bool qt_win_use_simple_timers;

STDAPI DllRegisterServer()
{
    return UpdateRegistry(TRUE);
}

STDAPI DllUnregisterServer()
{
    return UpdateRegistry(FALSE);
}

STDAPI DllGetClassObject(const GUID &clsid, const GUID &iid, void** ppv)
{
    GetClassObject( clsid, iid, ppv );
    if ( !*ppv )
	return CLASS_E_CLASSNOTAVAILABLE;
    return S_OK;
}

HHOOK hhook = 0;


STDAPI DllCanUnloadNow()
{
    if ( qAxLockCount() )
	return S_FALSE;
    if ( !qax_ownQApp )
	return S_OK;

    // check if qApp still runs widgets (in other DLLs)
    QWidgetList *widgets = qApp->allWidgets();
    int count = 0;
    if ( widgets ) {
	count = widgets->count();

	// remove all Qt generated widgets
	QWidgetListIt it( *widgets );
	while ( it.current() ) {
	    QWidget *w = it.current();
	    if ( w->testWFlags( Qt::WType_Desktop ) )
		count--;
	    ++it;
	}
	delete widgets;
    }
    if ( count )
	return S_FALSE;

    // no widgets left - destroy qApp
    if ( hhook )
	UnhookWindowsHookEx( hhook );

    delete qApp;
    qApp = 0;
    qax_ownQApp = FALSE;

    // never allow unloading - safety net for Internet Explorer
    return S_FALSE;
}


BOOL WINAPI DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID lpvReserved )
{
    GetModuleFileNameA( hInstance, qAxModuleFilename, MAX_PATH-1 );
    qAxInstance = hInstance;
    qAxIsServer = TRUE;

    if ( dwReason == DLL_PROCESS_ATTACH ) {
	qt_win_use_simple_timers = TRUE;
	DisableThreadLibraryCalls(hInstance);
	qAxInit();
    } else if ( dwReason == DLL_PROCESS_DETACH ) {
	qAxCleanup();
    }

    return TRUE;
}
