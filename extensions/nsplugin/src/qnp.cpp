// Remaining _WS_X11_ considerations:
//   - What if !piApp upon NPP_NewStream?  Are we safe?
//      - Yes, but users need to know of this:  that no GUI can be
//         done until after setWindow is called.
//   - Use NPN_GetValue in Communicator4.0 to get the display earlier!
//   - For ClientMessage events, trap them, and if they are not for us,
//	untrap them and retransmit them and set a timer to retrap them
//	after N seconds.

// Remaining _WS_WIN_ considerations:
//   - we need to activateZeroTimers() at some time.
//   - we need to call winEventFilter on events
//   - timers:
//    if ( msg.message == WM_TIMER ) {            // timer message received
//        activateTimer( msg.wParam );
//        return TRUE;
//    }
//    if ( msg.message == WM_KEYDOWN || msg.message == WM_KEYUP ) {
//        if ( translateKeyCode(msg.wParam) == 0 ) {
//            TranslateMessage( &msg );           // translate to WM_CHAR
//            return TRUE;
//        }
//    }
//   - qWinProcessConfigRequests?

// Remaining general stuff:
//   - Provide the "reason" parameter to streamDestroyed

// Qt stuff
#include <qapp.h>
#include <qwidget.h>
#include <qobjcoll.h>
#include <qwidcoll.h>

#include "qnp.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#ifdef _WS_X11_
#define	 GC GC_QQQ
#endif

extern "C" {
//
// Netscape plugin API
//
#ifdef _WS_WIN_
#ifndef _WINDOWS
#define _WINDOWS
#endif
#endif
#ifdef _WS_X11_
#define XP_UNIX
#endif

#include "npapi.h"

#ifdef _WS_X11_
/* ### debugging in ns3 needs a REAL stderr */
#if 0
static
FILE* out()
{
    static FILE* f = 0;
    if (!f)
	return stderr;
	//f = fdopen(4,"w"); // 2>&4 needed on cmd line for this
    return f;
}
#define PLUGIN_TRACE
#endif

#undef XP_UNIX
#include "npunix.c"
#endif

//
// Stuff for the NPP_SetWindow function:
//
#ifdef _WS_X11_
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h> // for XtCreateWindow
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
//#include <dlfcn.h>
#endif
#ifdef _WS_WIN_
#include <windows.h>
#endif
}

#ifdef _WS_WIN_
#include "npwin.cpp"
#endif

struct _NPInstance
{
    NPWindow*        fWindow;
    uint16            fMode;

#ifdef _WS_WIN_
    HWND            window;
    WNDPROC            fDefaultWindowProc;
#endif

    NPP npp;

#ifdef _WS_X11_
    Window window;
    Display *display;
#endif

    uint32 x, y;
    uint32 width, height;

    QNPWidget* widget;
    QNPInstance* instance;

    int16 argc;
    QString *argn;
    QString *argv;
};



//struct AA { AA() { fprintf(stderr,"AAAA\n"); } } aa;

// The single global plugin
static QNPlugin *qNP=0;
static int instance_count=0;

// The single global application
static class PluginSDK_QApplication *piApp=0;

// Temporary parameter passed `around the side' of calls to user functions
static _NPInstance* next_pi=0;

// To avoid looping when browser OR plugin can delete streams
static int qnps_no_call_back = 0;

// The currently in-focus widget.  This focus tracking is an auxiliary
// service which we provide, since we know it anyway.
static QNPWidget* focussedWidget=0;

#ifdef _WS_WIN_
// defined in qapp_win.cpp
extern bool qt_win_use_simple_timers;
#endif

#ifdef _WS_X11_
static XtAppContext appcon;

typedef void (*SameAsXtTimerCallbackProc)(void*,void*);
typedef void (*IntervalSetter)(int);
typedef void (*ForeignEventProc)(XEvent*);

extern XtEventDispatchProc
 qt_np_cascade_event_handler[LASTEvent];      // defined in qnpsupport.cpp
void            qt_reset_color_avail();       // defined in qcol_x11.cpp
void            qt_activate_timers();         // defined in qapp_x11.cpp
timeval        *qt_wait_timer();              // defined in qapp_x11.cpp
void		qt_x11SendPostedEvents();     // defined in qapp_x11.cpp
Boolean  qt_event_handler( XEvent* event );   // defined in qnpsupport.cpp
extern int      qt_np_count;                  // defined in qnpsupport.cpp
void qt_np_timeout( void* p, void* id );      // defined in qnpsupport.cpp
void qt_np_add_timeoutcb(
	SameAsXtTimerCallbackProc cb );       // defined in qnpsupport.cpp
void qt_np_remove_timeoutcb(
	SameAsXtTimerCallbackProc cb );       // defined in qnpsupport.cpp
void qt_np_add_timer_setter(
	IntervalSetter is );                  // defined in qnpsupport.cpp
void qt_np_remove_timer_setter(
	IntervalSetter is );                  // defined in qnpsupport.cpp
extern XtIntervalId qt_np_timerid;            // defined in qnpsupport.cpp
extern bool qt_np_filters_installed[3];       // defined in qnpsupport.cpp
extern void (*qt_np_leave_cb)
              (XLeaveWindowEvent*);           // defined in qnpsupport.cpp
void qt_np_add_event_proc(
	    ForeignEventProc fep );           // defined in qnpsupport.cpp
void qt_np_remove_event_proc(
	    ForeignEventProc fep );           // defined in qnpsupport.cpp

enum FilterType { Safe, Dangerous, Blocked };

FilterType filterTypeFor(int event_type)
{
    switch (event_type) {
      case KeymapNotify:
      case Expose:
      case GraphicsExpose:
      case NoExpose:
      case VisibilityNotify:
      case PropertyNotify:
      case SelectionClear:
      case SelectionRequest:
      case SelectionNotify:
      case ColormapNotify:
      case ClientMessage: // Hmm... is this safe?  I want the wm_deletes
	return Safe;
      default:
	return Dangerous;
    }
}


static
void installXtEventFilters(FilterType t)
{
    if (qt_np_filters_installed[t]) return;
    // Get Xt out of our face - install filter on every event type
    for (int et=2; et < LASTEvent; et++) {
	if ( filterTypeFor(et) == t )
	    qt_np_cascade_event_handler[et] = XtSetEventDispatcher(
		qt_xdisplay(), et, qt_event_handler );
    }
    qt_np_filters_installed[t] = TRUE;
}

static
void removeXtEventFilters(FilterType t)
{
    if (!qt_np_filters_installed[t]) return;
    // We aren't needed any more... slink back into the shadows.
    for (int et=2; et < LASTEvent; et++) {
	if ( filterTypeFor(et) == t )
	    XtSetEventDispatcher(
		qt_xdisplay(), et, qt_np_cascade_event_handler[et] );
    }
    qt_np_filters_installed[t] = FALSE;
}

// When we are in an event loop of QApplication rather than the browser's
// event loop (eg. for a modal dialog), we still send repaint events to
// the browser.
static
void np_event_proc( XEvent* e )
{
    Widget xtw = XtWindowToWidget( e->xany.display, e->xany.window );
    if ( xtw && filterTypeFor( e->type ) == Safe ) {
	// Graciously allow the browser to process the event
	qt_np_cascade_event_handler[e->type]( e );
    }
}


#endif

#ifdef _WS_WIN_
class PluginSDK_QApplication : public QApplication {
#endif

#ifdef _WS_X11_
class PluginSDK_QApplication /* Not a QApplication */ {
public:
    PluginSDK_QApplication()
    {
	piApp = this;
    }

    ~PluginSDK_QApplication()
    {
	piApp = 0;
//if (npwidgets.count()) abort();
    }

#endif

#ifdef _WS_WIN_
private:
    static int argc;
    static char** argv;
    int mousecheck;

public:
    PluginSDK_QApplication() :
	QApplication(argc, argv),
	mousecheck(0)
    {
    }

    void timerEvent(QTimerEvent* event)
    {
	if (event->timerId() == mousecheck) {
	    killTimer( mousecheck );
	    checkFocussedWidget();
	} else {
	    QApplication::timerEvent(event);
	}
    }

    void checkFocussedWidget()
    {
	POINT curPos;
	if ( GetCursorPos( &curPos ) ) {
	    QPoint p(curPos.x, curPos.y);
	    
	    QNPWidget *newFocussedWidget = 0;
	    for ( QNPWidget* npw = npwidgets.first();
		npw; npw = npwidgets.next() )
	    {
		QRect r = npw->rect();
		r.moveTopLeft( npw->mapToGlobal(QPoint(0,0)) );
		if ( r.contains(p) ) {
		    newFocussedWidget = npw;
		    break;
		}
	    }
debug("foc = %p",newFocussedWidget);
	    if (newFocussedWidget != focussedWidget && focussedWidget)
		focussedWidget->leaveInstance();
	    
	    if (newFocussedWidget) {
		if (newFocussedWidget != focussedWidget)
		    newFocussedWidget->enterInstance();
	    } else {
		killTimer( mousecheck );
		mousecheck = 0;
	    }
	    
	    focussedWidget = newFocussedWidget;
	}
    }

    bool notify( QObject* obj, QEvent* event )
    {
	if (event->type() == Event_Enter) {
	    	debug("enter event");
	    if ( mousecheck ) {
		killTimer( mousecheck );
		mousecheck = 0;
	    }
	    checkFocussedWidget();
	} else if (event->type() == Event_Leave) {
	    debug("leave event");
	    if ( !mousecheck ) {
		mousecheck = startTimer(200);
	    }
	}

	return QApplication::notify( obj, event );
    }
#endif

    void addQNPWidget(QNPWidget* w)
    {
	npwidgets.append(w);
    }

    void removeQNPWidget(QNPWidget* w)
    {
	if (w == focussedWidget) focussedWidget = 0;
	npwidgets.remove(w);
    }

#ifdef _WS_X11_
    static void removeXtEventFiltersIfOutsideQNPWidget(XLeaveWindowEvent* e)
    {
	// If QApplication doesn't know about the widget at the
	// event point, we must should remove our filters.
	// ### is widgetAt efficient enough?
	QWidget* w = QApplication::widgetAt(e->x_root, e->y_root);

	if ( !w ) {
	    if ( focussedWidget ) {
		focussedWidget->leaveInstance();
		focussedWidget = 0;
	    }
	    removeXtEventFilters( Dangerous );
	} else if ( w->isTopLevel() ) {
	    for ( QNPWidget* npw = npwidgets.first();
		npw; npw = npwidgets.next())
	    {
		if ( npw == w ) {
		    if ( focussedWidget != npw ) {
			if ( focussedWidget ) {
			    focussedWidget->leaveInstance();
			}
			focussedWidget = npw;
			focussedWidget->enterInstance();
		    }

		    break;
		}
	    }
	}
    }
#endif

private:
    static QList<QNPWidget> npwidgets;
};
QList<QNPWidget> PluginSDK_QApplication::npwidgets;

#ifdef _WS_WIN_
int PluginSDK_QApplication::argc=0;
char **PluginSDK_QApplication::argv={ 0 };
#endif

#ifdef _WS_X11_
static void np_set_timer( int interval )
{
    // Ensure we only have one timeout in progress - QApplication is
    // computing the one amount of time we need to wait.
    if ( qt_np_timerid ) {
	XtRemoveTimeOut( qt_np_timerid );
    }
    qt_np_timerid = XtAppAddTimeOut(appcon, interval,
	(XtTimerCallbackProc)qt_np_timeout, 0);
}

static void np_do_timers( void*, void* )
{
    qt_np_timerid = 0; // It's us, and we just expired, that's why we are here.

    qt_activate_timers();

    timeval *tm = qt_wait_timer();

    if (tm) {
	int interval = QMIN(tm->tv_sec,INT_MAX/1000)*1000 + tm->tv_usec/1000;
	np_set_timer( interval );
    }
}
#endif









/*******************************************************************************
 * Plug-in Calls - these are called by Netscape
 ******************************************************************************/


// Instance state information about the plugin.

#ifdef _WS_X11_

extern "C" char*
NPP_GetMIMEDescription(void)
{
    if (!qNP) qNP = QNPlugin::create();
    return (char*)qNP->getMIMEDescription();
}



extern "C" NPError
NPP_GetValue(void * /*future*/, NPPVariable variable, void *value)
{
    if (!qNP) qNP = QNPlugin::create();
    NPError err = NPERR_NO_ERROR;
    if (variable == NPPVpluginNameString)
        *((const char **)value) = qNP->getPluginNameString();
    else if (variable == NPPVpluginDescriptionString)
        *((const char **)value) = qNP->getPluginDescriptionString();
    else
        err = NPERR_GENERIC_ERROR;

    return err;
}

#endif

/*
** NPP_Initialize is called when your DLL is being loaded to do any
** DLL-specific initialization.
*/
extern "C" NPError
NPP_Initialize(void)
{
#ifdef _WS_WIN_
    qt_win_use_simple_timers = TRUE;
    // Nothing more - we do it in DLLMain
#endif

    if (!qNP) qNP = QNPlugin::create();
    return NPERR_NO_ERROR;
}

static jref plugin_java_class = 0;

/*
** NPP_GetJavaClass is called during initialization to ask your plugin
** what its associated Java class is. If you don't have one, just return
** NULL. Otherwise, use the javah-generated "use_" function to both
** initialize your class and return it. If you can't find your class, an
** error will be signalled by "use_" and will cause the Navigator to
** complain to the user.
*/
extern "C" jref
NPP_GetJavaClass(void)
{
    if (!qNP) qNP = QNPlugin::create();
    plugin_java_class = qNP->getJavaClass();
    return plugin_java_class;
}

/*
** NPP_Shutdown is called when your DLL is being unloaded to do any
** DLL-specific shut-down. You should be a good citizen and declare that
** you're not using your java class any more. This allows java to unload
** it, freeing up memory.
*/
extern "C" void
NPP_Shutdown(void)
{
    if (qNP) {
	if (plugin_java_class)
	    qNP->unuseJavaClass();
	delete qNP;
	qNP = 0;
    }

    if (piApp) {
#ifdef _WS_X11_
	qt_np_remove_timeoutcb(np_do_timers);
	qt_np_remove_timer_setter(np_set_timer);
	qt_np_remove_event_proc(np_event_proc);
	qt_np_count--;

	if (qt_np_leave_cb == PluginSDK_QApplication::removeXtEventFiltersIfOutsideQNPWidget)
	    qt_np_leave_cb = 0;
	if ( qt_np_count == 0) {
	    // We are the last Qt-based plugin to leave
	    removeXtEventFilters(Safe);
	    removeXtEventFilters(Dangerous);
	    if (qt_np_timerid) XtRemoveTimeOut( qt_np_timerid );
	    qt_np_timerid = 0;
	    qt_np_leave_cb = 0;
	}
	delete piApp;
#endif
	piApp = 0;

	// delete qApp; ### Crashes under X11.  Waste memory until we can fix this.
#ifdef _WS_WIN_
	delete qApp;
#endif
    }
}


struct NS_Private {
    uchar* a;
    uchar* b;
};

/*
** NPP_New is called when your plugin is instantiated (i.e. when an EMBED
** tag appears on a page).
*/
extern "C" NPError 
NPP_New(NPMIMEType /*pluginType*/,
    NPP instance,
    uint16 mode,
    int16 argc,
    char* argn[],
    char* argv[],
    NPSavedData* /*saved*/)
{
/*
    {
	uchar* ndata;
	ndata = ((NS_Private*)(instance->ndata))->a;
	printf("New %p->ndata->a = %p:\n", instance, ndata);
	for (int i=0; i<64; i++) {
	    printf("%2x ",ndata[i]);
	    if (i%8==7) printf("\n");
	}
	ndata = (uchar*)((NS_Private*)instance->ndata)->b;
	printf("New %p->ndata->b = %p:\n", instance, ndata);
	for (int i=0; i<64; i++) {
	    printf("%2x ",ndata[i]);
	    if (i%8==7) printf("\n");
	}
    }
*/

    NPError result = NPERR_NO_ERROR;
    _NPInstance* This;

    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;
        
    instance->pdata = new _NPInstance;

    This = (_NPInstance*) instance->pdata;

    if (This == NULL)
        return NPERR_OUT_OF_MEMORY_ERROR;

    This->npp = instance;

    /* mode is NP_EMBED, NP_FULL, or NP_BACKGROUND (see npapi.h) */
    This->fWindow = NULL;
    This->fMode = mode;

    This->window = 0;

#ifdef _WS_WIN_
    This->fDefaultWindowProc = NULL;
#endif

    This->widget = 0;

    This->argc = argc;
    This->argn = new QString[argc+1];
    This->argv = new QString[argc+1];
    for (int i=0; i<This->argc; i++) {
	This->argn[i] = argn[i];
	This->argv[i] = argv[i];
    }

    // Everything is set up - we can let QNPInstance be created now.
    next_pi = This;
    qNP->newInstance();
    instance_count++;

#ifdef _WS_WIN_
    if (strstr(This->instance->userAgent(), "Mozilla/3.")) {
	// ### work-around for browser inconsistency
	const char* src = This->instance->arg("SRC");
	if (src)
	    This->instance->getURL(src);
    }
#endif

    return result;
}

extern "C" NPError 
NPP_Destroy(NPP instance, NPSavedData** /*save*/)
{
    _NPInstance* This;

    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;

    This = (_NPInstance*) instance->pdata;

    if (This != NULL) {
#ifdef _WS_WIN_
	SetWindowLong( This->window, GWL_WNDPROC,
		(LONG)This->fDefaultWindowProc );
#endif

	if (This->widget) {
	    This->widget->unsetWindow();
	    delete This->widget;
	}

	delete This->instance;
	delete [] This->argn;
	delete [] This->argv;

        delete This;
        instance->pdata = NULL;

	instance_count--;
    }

    return NPERR_NO_ERROR;
}


extern "C" NPError 
NPP_SetWindow(NPP instance, NPWindow* window)
{
    if (!qNP) qNP = QNPlugin::create();
    NPError result = NPERR_NO_ERROR;
    _NPInstance* This;

    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;

    This = (_NPInstance*) instance->pdata;

    /*
     * PLUGIN DEVELOPERS:
     *    Before setting window to point to the
     *    new window, you may wish to compare the new window
     *    info to the previous window (if any) to note window
     *    size changes, etc.
     */    
    if (!window) {

	if (This->widget) {
	    This->widget->unsetWindow();
	    This->window = 0;
	    delete This->widget;
	    This->widget = 0;
	}
#ifdef _WS_X11_
    } else if (This->window != (Window) window->window) {
	if (This->window)
	    NPP_SetWindow( instance, 0 ); // unset
	This->window = (Window) window->window;
#endif
#ifdef _WS_WIN_
    } else if (This->window != (HWND) window->window) {
	if (This->window)
	    SetWindowLong( This->window, GWL_WNDPROC,
		(LONG)This->fDefaultWindowProc );
	This->fDefaultWindowProc =
	    (WNDPROC)GetWindowLong( (HWND)window->window, GWL_WNDPROC);
	This->window = (HWND) window->window;
#endif
	This->x = window->x;
	This->y = window->y;
	This->width = window->width;
	This->height = window->height;

#ifdef _WS_X11_
	This->display =
	    ((NPSetWindowCallbackStruct *)window->ws_info)->display;
#endif

	if (!piApp) {
#ifdef _WS_X11_
	    if (!qApp) {
		// Thou Shalt Not Unload Qt
		// Increment the reference count...
		// dlopen("libqt.so.1", RTLD_LAZY);
		// ... and never close it.
		// Nice try.  Can't get that to work.

		// We are the first Qt-based plugin to arrive
		new QApplication(This->display);
		//XSynchronize(This->display,True);  // Helps debugging
		ASSERT(qt_np_count == 0);
	    }
	    installXtEventFilters(Safe);
	    qt_np_add_timeoutcb(np_do_timers);
	    qt_np_add_timer_setter(np_set_timer);
	    qt_np_add_event_proc(np_event_proc);
	    qt_np_count++;
	    appcon = XtDisplayToApplicationContext(This->display);
#endif
	    piApp = new PluginSDK_QApplication();
	}

	if (!This->widget) {
#ifdef _WS_WIN_
	    This->window = (HWND) window->window;

	    InvalidateRect( This->window, NULL, TRUE );
	    UpdateWindow( This->window );
#endif
	    // New widget on this new window.
	    next_pi = This;
	    /* This->widget = */ // (happens sooner - in QNPWidget constructor)
		This->instance->newWindow();
	} else {
	    // New window for existing widget, and all its children.
	    This->widget->setWindow(FALSE);
	}
    } else if (This->widget) {
	// ### Maybe need a geometry setter that bypasses some Qt code?
	// ### position is always (0,0), so we get by by ignoring it.
	//This->widget->setGeometry(window->x,window->y, window->width, window->height);
	if ( This->widget->width() != (int)window->width
	  || This->widget->height() != (int)window->height )
	{
#ifdef _WS_WIN_
	    This->widget->setGeometry(window->x, window->y, window->width, window->height);
#else
	    This->widget->resize(window->width, window->height);
#endif
	} else {
	    This->widget->update();
	}
    }

    This->fWindow = window;
    return result;
}


extern "C" NPError 
NPP_NewStream(NPP instance,
          NPMIMEType type,
          NPStream *stream, 
          NPBool seekable,
          uint16 *stype)
{
    _NPInstance* This;

    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;

    This = (_NPInstance*) instance->pdata;

    if ( This ) {
	QNPStream* qnps = new QNPStream(This->instance,type,stream,seekable);
	stream->pdata = qnps;
	QNPInstance::StreamMode sm = (QNPInstance::StreamMode)*stype;
	if (!This->instance->newStreamCreated(qnps, sm)) {
	    return NPERR_GENERIC_ERROR;
	}
	*stype = sm;
    }

    return NPERR_NO_ERROR;
}


/* PLUGIN DEVELOPERS:
 *    These next 2 functions are directly relevant in a plug-in which
 *    handles the data in a streaming manner. If you want zero bytes
 *    because no buffer space is YET available, return 0. As long as
 *    the stream has not been written to the plugin, Navigator will
 *    continue trying to send bytes.  If the plugin doesn't want them,
 *    just return some large number from NPP_WriteReady(), and
 *    ignore them in NPP_Write().  For a NP_ASFILE stream, they are
 *    still called but can safely be ignored using this strategy.
 */

int32 STREAMBUFSIZE = 0X0FFFFFFF; /* If we are reading from a file in NPAsFile
                                   * mode so we can take any size stream in our
                                   * write call (since we ignore it) */

extern "C" int32 
NPP_WriteReady(NPP instance, NPStream *stream)
{
    _NPInstance* This;
    if (instance != NULL) {
        This = (_NPInstance*) instance->pdata;
    } else {
	// Yikes, that's unusual!
	return 0;
    }

    if (This) {
	return This->instance->writeReady((QNPStream*)stream->pdata);
    }

    /* Number of bytes ready to accept in NPP_Write() */
    return STREAMBUFSIZE;
}


extern "C" int32 
NPP_Write(NPP instance, NPStream *stream, int32 offset, int32 len, void *buffer)
{
    if (instance != NULL)
    {
        _NPInstance* This = (_NPInstance*) instance->pdata;

	if (This) {
	    return This->instance->write((QNPStream*)stream->pdata,
		offset, len, buffer);
	}
    }

    return len;        /* The number of bytes accepted */
}


extern "C" NPError 
NPP_DestroyStream(NPP instance, NPStream *stream, NPError reason)
{
    _NPInstance* This;

    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;

    if (!qnps_no_call_back) {
	This = (_NPInstance*) instance->pdata;

	QNPStream* qnps = (QNPStream*)stream->pdata;

	if (This) {
	    // Give the instance a chance to do something
	    This->instance->streamDestroyed(qnps);
	}

	qnps_no_call_back++;
	delete qnps;
	qnps_no_call_back--;
    }

    return NPERR_NO_ERROR;
}


extern "C" void 
NPP_StreamAsFile(NPP instance, NPStream *stream, const char* fname)
{
    _NPInstance* This;

    if (instance == NULL) return;

    This = (_NPInstance*) instance->pdata;

    if ( This ) {
	QNPStream* qnps = (QNPStream*)stream->pdata;
	This->instance->streamAsFile(qnps, fname);
    }
}

extern "C" void 
NPP_Print(NPP instance, NPPrint* printInfo)
{
    if(printInfo == NULL)
        return;

    if (instance != NULL) {
        // _NPInstance* This = (_NPInstance*) instance->pdata;
    
        if (printInfo->mode == NP_FULL) {
            /*
             * PLUGIN DEVELOPERS:
             *    If your plugin would like to take over
             *    printing completely when it is in full-screen mode,
             *    set printInfo->pluginPrinted to TRUE and print your
             *    plugin as you see fit.  If your plugin wants Netscape
             *    to handle printing in this case, set
             *    printInfo->pluginPrinted to FALSE (the default) and
             *    do nothing.  If you do want to handle printing
             *    yourself, printOne is true if the print button
             *    (as opposed to the print menu) was clicked.
             *    On the Macintosh, platformPrint is a THPrint; on
             *    Windows, platformPrint is a structure
             *    (defined in npapi.h) containing the printer name, port,
             *    etc.
             */

            // void* platformPrint =
            //     printInfo->print.fullPrint.platformPrint;
            // NPBool printOne =
            //     printInfo->print.fullPrint.printOne;
            
            /* Do the default*/
            printInfo->print.fullPrint.pluginPrinted = FALSE;
        }
        else {    /* If not fullscreen, we must be embedded */
            /*
             * PLUGIN DEVELOPERS:
             *    If your plugin is embedded, or is full-screen
             *    but you returned false in pluginPrinted above, NPP_Print
             *    will be called with mode == NP_EMBED.  The NPWindow
             *    in the printInfo gives the location and dimensions of
             *    the embedded plugin on the printed page.  On the
             *    Macintosh, platformPrint is the printer port; on
             *    Windows, platformPrint is the handle to the printing
             *    device context.
             */

            // NPWindow* printWindow =
            //     &(printInfo->print.embedPrint.window);
            // void* platformPrint =
            //     printInfo->print.embedPrint.platformPrint;
        }
    }
}

extern "C" void
NPP_URLNotify(NPP /*instance*/,
    const char* /*url*/,
    NPReason /*reason*/,
    void* /*notifyData*/)
{
}



// Hackery for X11:  make Qt's toplevels widgets be Xt widgets too.

#ifdef _WS_X11_

// Called when a top-level widget (which has an Xt widget's window) is entered.
static
void enter_event_handler(Widget, XtPointer xtp, XEvent* event, Boolean* cont)
{
    _NPInstance* This = (_NPInstance*)xtp;

    if (piApp) {
	installXtEventFilters(Dangerous);
	if ( xtp ) {
	    if ( focussedWidget )
		focussedWidget->leaveInstance();

	    focussedWidget = This->widget;

	    if ( focussedWidget ) {
		focussedWidget->enterInstance();
		qt_np_leave_cb = PluginSDK_QApplication::removeXtEventFiltersIfOutsideQNPWidget;
	    }
	}
	// Post the event
	*cont = qt_event_handler(event);
    } else {
	*cont = FALSE;
    }
}

// Called when a top-level widget (which has an Xt widget's window) is left.
static
void leave_event_handler(Widget, XtPointer, XEvent*, Boolean* cont)
{
    if (piApp) {
	if ( !QApplication::activePopupWidget()
	  && !QApplication::activeModalWidget() )
	{
	    if ( focussedWidget ) {
		focussedWidget->leaveInstance();
		focussedWidget = 0;
	    }
	    removeXtEventFilters(Dangerous);
	}
    }
    *cont = FALSE;
}

// Relacement for Qt function - add Xt stuff for top-level widgets
Window qt_XCreateWindow( const QWidget* qw, Display *display, Window parent,
			 int x, int y, uint w, uint h,
			 int borderwidth, int depth,
			 uint windowclass, Visual *visual,
			 ulong valuemask, XSetWindowAttributes *attributes )
{
    // ### This isA will not work - we are still in QWidget's constructor.
    if ( qw->isTopLevel() && !qw->isA("QNPWidget") ) {
	// ### not sure it is good to use name() and className().
	Widget xtw = XtVaAppCreateShell( qw->name(), qw->className(),
	    applicationShellWidgetClass, display,
	    XtNx, x, XtNy, y, XtNwidth, w, XtNheight, h,
	    XtNborderWidth, borderwidth, XtNdepth, depth,
	    0, 0 );

	// Ensure it has a window, and get it.
	XtSetMappedWhenManaged( xtw, FALSE );
	XtRealizeWidget( xtw );
	Window xw = XtWindow( xtw );

	// Set the attributes (directly)
	XChangeWindowAttributes( display, xw, valuemask, attributes );

	// Inform us on enter/leave
	XtAddEventHandler( xtw, EnterWindowMask, TRUE, enter_event_handler, 0 );
	XtAddEventHandler( xtw, LeaveWindowMask, TRUE, leave_event_handler, 0 );

	// Return Xt's window for the widget
	return xw;
    } else {
	Window window = XCreateWindow( display, parent, x, y, w, h, borderwidth, depth,
			      windowclass, visual, valuemask, attributes );
	return window;
    }
}


// Relacement for Qt function - add Xt stuff for top-level widgets
Window qt_XCreateSimpleWindow( const QWidget* qw, Display *display, Window parent,
			       int x, int y, uint w, uint h, int borderwidth,
			       ulong border, ulong background )
{
    // ### This isA will not work - we are still in QWidget's constructor.
    Window window;
    if ( qw->isTopLevel() && !qw->isA("QNPWidget") ) {
	XSetWindowAttributes attributes;
	attributes.border_pixel = border;
	attributes.background_pixel = background;
	window = qt_XCreateWindow (
	    qw, display, parent, x, y, w, h, borderwidth,
	    CopyFromParent, CopyFromParent, CopyFromParent,
	    CWBackPixel | CWBorderPixel, &attributes );
    } else {
	window = XCreateSimpleWindow( display, parent, x, y, w, h, borderwidth,
				    border, background );
    }
    return window;
}


// Relacement for Qt function - add Xt stuff for top-level widgets
void qt_XDestroyWindow( const QWidget* qw, Display *display, Window window )
{

    if ( qw->isTopLevel() && !qw->isA("QNPWidget") ) {
	Widget xtw = XtWindowToWidget( display, window );
	if ( xtw ) {
	    XtRemoveEventHandler(xtw, LeaveWindowMask, TRUE, leave_event_handler, 0);
	    XtRemoveEventHandler(xtw, EnterWindowMask, TRUE, enter_event_handler, 0);
	    XtDestroyWidget( xtw );
	} else {
	    XDestroyWindow( display, window );
	}
    } else {
	XDestroyWindow( display, window );
    }
}

#endif





#ifdef _WS_WIN_

BOOL   WINAPI   DllMain (HANDLE hInst, 
                        ULONG ul_reason_for_call,
                        LPVOID lpReserved)
{
    switch ( ul_reason_for_call ) {
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	    WinMain( hInst, 0, "", SW_SHOW );
	    break;
	case DLL_PROCESS_DETACH:
	case DLL_THREAD_DETACH:
	    break;
    }
	    
    return TRUE;
}

int main(int argc, char** argv)
{
    return 0;
}

#endif











/*!
  \class QNPWidget qnp.h
  \brief A QWidget that is a Web-browser plugin window

  Derive from QNPWidget to create a widget that can be used as a 
  Browser plugin window, or create one and add child widgets.
  Instances of QNPWidget may only be created
  when QNPInstance::newWindow() is called by the browser.

  A common way to develop a plugin widget is to develop it as a stand-alone
  application window, then make it a \e child of a plugin widget to use
  it as a browser plugin.  The technique is:

\code
class MyPluginWindow : public QNPWidget {
    QWidget* child;
public:
    MyPluginWindow()
    {
        // Some widget that is normally used as a top-level widget
        child = new MyIndependentlyDevelopedWidget();

        // Use the background color of the web page
        child->setBackgroundColor( backgroundColor() );

        // Fill the plugin widget
        child->setGeometry( 0, 0, width(), height() );

        // Show now, since the QNPWidget is already shown
        child->show();
    }

    void resizeEvent(QResizeEvent*)
    {
        // Fill the plugin widget
        child->resize(size());
    }
};
\endcode

  The default implementation is an empty window.
*/

/*!
  Creates a QNPWidget.
*/
QNPWidget::QNPWidget() :
    pi(next_pi)
{
    if (!next_pi) {
	fatal("QNPWidget must only be created within call to newWindow");
    }
    next_pi->widget = this;
    next_pi = 0;

    setWindow(TRUE);

    piApp->addQNPWidget(this);

#ifdef _WS_WIN_
    // Communicator and explorer give us an unshown
    // widget.  Navigator gives us a shown one.
    show();
#endif
}

/*!
  Destroys the window.  This will be called by the plugin binding code
  when the window is no longer required.  The Web-browser will delete windows
  when they leave the page.  The bindings will change the QWidget::winId()
  of the window when the window is resized, but this should not affect
  normal widget behaviour.
*/
QNPWidget::~QNPWidget()
{
    piApp->removeQNPWidget(this);
}

/*!
  Called when the mouse enters the plugin window.  Default does nothing.
*/
void QNPWidget::enterInstance()
{
}

/*!
  Called when the mouse leaves the plugin window.  Default does nothing.
*/
void QNPWidget::leaveInstance()
{
}

/*!
  Returns the instance for which this widget is the window.
*/
QNPInstance* QNPWidget::instance()
{
    return pi->instance;
}

class QFixableWidget : public QWidget {
public:
    void fix()
    {
	QRect g = geometry();
	QColor bg = backgroundColor();
	bool mt = hasMouseTracking();
	bool hascurs = testWFlags( WCursorSet );
	QCursor curs = cursor();
	clearWFlags( WState_Created );
	clearWFlags( WState_Visible );
	create( 0, TRUE, FALSE );
	setGeometry(g);
	setBackgroundColor( bg );
	setMouseTracking( mt );
	if ( hascurs ) {
	    setCursor( curs );
	}
    }
};

static
void createNewWindowsForAllChildren(QWidget* parent, int indent=0)
{
    QObjectList* list = parent->queryList("QWidget", 0, FALSE, FALSE);

    if ( list ) {
	QObjectListIt it( *list );
	QFixableWidget* c;
	while ( (c = (QFixableWidget*)it.current()) ) {
	    bool vis = c->isVisible();
	    c->fix();
	    createNewWindowsForAllChildren(c,indent+1);
	    if ( vis ) c->show(); // Now that all children are valid.
	    ++it;
	}
	delete list;
    }
}

/*!
  For internal use only.
*/
void QNPWidget::setWindow(bool delold)
{
    saveWId = winId(); // ### Don't need this anymore

    create((WId)pi->window, FALSE, delold);

#ifdef _WS_X11_
    // It's open.  Believe me.
    setWFlags( WState_Visible );

    Widget w = XtWindowToWidget (qt_xdisplay(), pi->window);
    XtAddEventHandler(w, EnterWindowMask, FALSE, enter_event_handler, pi);
    XtAddEventHandler(w, LeaveWindowMask, FALSE, leave_event_handler, pi);
    Pixmap bgpm=0;
    XColor col;
    XtVaGetValues(w,
	XtNbackground, &col.pixel,
	XtNbackgroundPixmap, &bgpm,
	0, 0);
    XQueryColor(qt_xdisplay(), x11Colormap(), &col);
    setBackgroundColor(QColor(col.red >> 8, col.green >> 8, col.blue >> 8));
    if (bgpm) {
	// ### Need an under-the-hood function here, or we have to
	// ### rewrite lots of code from QPixmap::convertToImage().
	// ### Doesn't matter yet, because Netscape doesn't ever set
	// ### the background image of the window it gives us.
    }
#endif

    createNewWindowsForAllChildren(this);

#ifdef _WS_WIN_
    setGeometry( pi->x, pi->y, pi->width, pi->height );
#else
    resize( pi->width, pi->height );
#endif
}

/*!
  For internal use only.
*/
void QNPWidget::unsetWindow()
{
#ifdef _WS_X11_
    WId wi = winId();
    Widget w = XtWindowToWidget (qt_xdisplay(), wi);
    if ( w ) {
	XtRemoveEventHandler(w, LeaveWindowMask, FALSE, leave_event_handler, pi);
	XtRemoveEventHandler(w, EnterWindowMask, FALSE, enter_event_handler, pi);
    }
    destroy( FALSE, FALSE ); // Xt has already destroyed all the windows
#endif
#ifdef _WS_WIN_
    // Nothing special
    destroy( FALSE, TRUE ); // Browser will the window, but not the subwindows
#endif
}







/*!
  \class QNPInstance qnp.h
  \brief a QObject that is a Web-browser plugin

  Deriving from QNPInstance is creates an object that represents a single
  &lt;EMBED&gt; tag in an HTML document.

  The QNPInstance is responsible for creating an appropriate window if
  required (not all plugins have windows), and for interacting with the
  input/output facilities intrinsic to plugins.

  Note that there is <em>absolutely no garrantee</em> as to the order in
  which functions are called.  Sometimes the browser will call setWindow()
  first, at other times, newStreamCreated() will be called first (assuming the
  &lt;EMBED&gt; tag has a SRC parameter).

  <em>No GUI functionality</em> of Qt may be used until the first call
  to setWindow().  This includes any use of QPaintDevice (ie. QPixmap,
  QWidget, and all subclasses), QApplication, anything related to
  QPainter (QBrush, etc.), fonts, QMovie, QToolTip, etc.  Useful
  classes which specifically <em>can</em> be used are QImage, QFile,
  and QBuffer.

  By structuring your plugin so that
  the task of the QNPInstance is to gather data, while
  the task of the QNPWidget is to provide a graphical interface to that data,
  this restriction can be easily accommodated.
*/

/*!
  Creates a QNPInstance.
  Can only be called from within a derived class created
  within QNPlugin::newInstance().
*/
QNPInstance::QNPInstance() :
    pi(next_pi)
{
    if (!next_pi) {
	fatal("QNPInstance must only be created within call to newInstance");
    }
    next_pi->instance = this;
    next_pi = 0;
}

/*!
  Called when the plugin instance is about to disappear.
*/
QNPInstance::~QNPInstance()
{
}

/*!
  Called at most once, at some time after the QNPInstance is created.
  If the plugin requires a window, this function should return a derived
  class of QNPWidget that provides the required interface.
*/
QNPWidget* QNPInstance::newWindow()
{
    // No window by default
    next_pi = 0;
    return 0;
}

/*!
  Returns the plugin window created at newWindow(), if any.
*/
QNPWidget* QNPInstance::widget()
{
    return pi->widget;
}

/*!
  \fn bool QNPInstance::newStreamCreated(QNPStream*, StreamMode& smode)

  This function is called when a new stream has been created.
  The instance should return TRUE if it accepts the processing
  of the stream.  If the instance requires the stream as a file,
  it should set \a smode to AsFileOnly, in which case the data
  will be delivered some time later to the streamAsFile() function.
  Otherwise, the data will be delivered in chunks to the write()
  function which must consume at least as much data as was returned
  by the most recent call to writeReady().

  See also:
  <a href=http://developer.netscape.com/library/documentation/communicator/plugin/refpgst.htm#nppnewstream>
  Netscape: NPP_NewStream method</a>
*/
bool QNPInstance::newStreamCreated(QNPStream*, StreamMode&)
{
    return FALSE;
}

/*!
  Called when a stream is delivered as a single file rather than
  as chunks.  This may be simpler for a plugin to deal with, but
  precludes any incremental behaviour.
  \sa newStreamCreated(), newStream()

  See also:
  <a href=http://developer.netscape.com/library/documentation/communicator/plugin/refpgst.htm#nppstreamasfile>
  Netscape: NPP_StreamAsFile method</a>
*/
void QNPInstance::streamAsFile(QNPStream*, const char*)
{
}

/*!
  Called when a stream is destroyed.

  See also:
  <a href=http://developer.netscape.com/library/documentation/communicator/plugin/refpgst.htm#nppdestroystream>
  Netscape: NPP_DestroyStream method</a>
*/
void QNPInstance::streamDestroyed(QNPStream*)
{
}

/*!
  Called to inquire the minimum amount of data the instance is
  willing to receive from the given stream.

  The default returns a very large value.

  See also:
  <a href=http://developer.netscape.com/library/documentation/communicator/plugin/refpgst.htm#nppwriteready>
  Netscape: NPP_WriteReady method</a>
*/
int QNPInstance::writeReady(QNPStream*)
{
    // Yes, we can handle any amount of data at once.
    return 0X0FFFFFFF;
}

/*!
  \fn int QNPInstance::write(QNPStream*, int offset, int len, void* buffer)

  Called when incoming data is available for processing by the instance.
  The instance \e must consume at least the amount that it returned in
  the most recent call to writeReady(), but it may consume up to the
  amount given by \a len.  \a buffer is the data available for consumption.
  The \a offset argument is merely an informational
  value indicating the total amount of data that has been consumed
  in prior calls.

  This function should return the amount of data actually consumed.

  See also:
  <a href=http://developer.netscape.com/library/documentation/communicator/plugin/refpgst.htm#nppwrite>
  Netscape: NPP_NewStream method</a>
*/
int QNPInstance::write(QNPStream*, int, int len, void*)
{
    // Yes, we processed it all... into the bit bucket.
    return len;
}

/*!
  Requests that the given URL be retrieved and sent to the named
  window.  See Netscape's JavaScript documentation for an explanation
  of window names.

  See also:
  <a href=http://developer.netscape.com/library/documentation/communicator/plugin/refpgur.htm#npngeturl>
  Netscape: NPN_GetURL method</a>
*/
void QNPInstance::getURL(const char* url, const char* window)
{
    NPN_GetURL( pi->npp, url, window );
}

/*!
  This function is not tested.  It is an interface to the NPN_PostURL
  function of the Netscape Plugin API.

  See also:
  <a href=http://developer.netscape.com/library/documentation/communicator/plugin/refpgur.htm#npnposturl>
  Netscape: NPN_PostURL method</a>
*/
void QNPInstance::postURL(const char* url, const char* window,
	     uint len, const char* buf, bool file)
{
    NPN_PostURL( pi->npp, url, window, len, buf, file );
}

/*!
  Returns the number of arguments to the instance.  Note that you should
  not normally rely on the ordering of arguments, and also note that
  the SGML specification does not permit multiple arguments with the same
  name.

  \sa arg()
*/
int QNPInstance::argc() const
{
    return pi->argc;
}

/*!
  Returns the name of the <em>i</em>th argument.  See notes of argc().
*/
const char* QNPInstance::argn(int i) const
{
    return pi->argn[i];
}

/*!
  Returns the value of the <em>i</em>th argument.  See notes of argc().
*/
const char* QNPInstance::argv(int i) const
{
    return pi->argv[i];
}

/*!
  Returns the mode of the plugin.
*/
QNPInstance::InstanceMode QNPInstance::mode() const
{
    return (QNPInstance::InstanceMode)pi->fMode;
}

/*!
  Returns the value of the named arguments, or 0 if no argument
  with that name appears in the &lt;EMBED&gt; tag of this instance.
  If the argument appears, but has no value assigned, the empty
  string is returned.  In summary:

  <ul>
   <li> <b>&lt;EMBED ...&gt;</b> - arg("FOO") == 0
   <li> <b>&lt;EMBED FOO ...&gt;</b> - arg("FOO") == ""
   <li> <b>&lt;EMBED FOO=BAR ...&gt;</b> - arg("FOO") == "BAR"
  </ul>
*/
const char* QNPInstance::arg(const char* name) const
{
    for (int i=0; i<pi->argc; i++) {
	// SGML: names are case insensitive
	if ( stricmp( name, pi->argn[i] ) == 0 ) {
	    if (pi->argv[i].isEmpty())
		return "";
	    else
		return pi->argv[i];
	}
    }
    return 0;
}

/*!
  Returns the user agent (browser name) containing this instance.

  See also:
  <a href=http://developer.netscape.com/library/documentation/communicator/plugin/refpgve.htm#npnuseragent>
  Netscape: NPN_UserAgent method</a>
*/
const char* QNPInstance::userAgent() const
{
    return NPN_UserAgent(pi->npp);
}

/*!
  Requests the creation of a new data stream \e from the plug-in.
  This function is not tested.
  It is an interface to the NPN_NewStream
  function of the Netscape Plugin API.

  See also:
  <a href=http://developer.netscape.com/library/documentation/communicator/plugin/refpgst.htm#npnnewstream>
  Netscape: NPN_NewStream method</a>
*/
QNPStream* QNPInstance::newStream(const char* mimetype, const char* window,
    bool as_file)
{
    NPStream* s=0;
    NPN_NewStream(pi->npp, (char*)mimetype, window, &s);
    return s ? new QNPStream(this, mimetype, s, as_file) : 0;
}

/*!
  Sets the status message in the browser containing this instance.

  See also:
  <a href=http://developer.netscape.com/library/documentation/communicator/plugin/refpgve.htm#npnstatus>
  Netscape: NPN_Status method</a>
*/
void QNPInstance::status(const char* msg)
{
    NPN_Status(pi->npp, msg);
}


/*!
  Returns the Java object associated with the plug-in instance, an
  object of the 
  \link QNPlugin::getJavaClass() plug-in's Java class\endlink, 
  or 0 if the plug-in does not have a Java class, Java is disabled, or
  an error occurred .  

  The return value is actually a <tt>jref</tt> we use <tt>void*</tt> so
  as to avoid burdening plugins which do not require Java.

  \sa QNPlugin::getJavaClass(), QNPlugin::getJavaEnv(), getJavaPeer(),
      <a href=http://developer.netscape.com/library/documentation/communicator/plugin/refpgja.htm#npngetjavapeer>
      Netscape: NPN_GetJavaPeer</a>
*/
void* QNPInstance::getJavaPeer() const
{
    return NPN_GetJavaPeer(pi->npp);
}


/*!
  \class QNPStream qnp.h
  \brief A stream of data provided to a QNPInstance by the browser.

  \sa QNPInstance::write(), QNPInstance::newStreamCreated()
*/

/*!
  Creates a stream.  Plugins should not call this, but rather
  QNPInstance::newStream() if a stream is required.
*/
QNPStream::QNPStream(QNPInstance* in,const char* mt, _NPStream* st, bool se) :
    inst(in),
    stream(st),
    mtype(mt),
    seek(se)
{
}

/*!
  Destroys the stream.
*/
QNPStream::~QNPStream()
{
    if (!qnps_no_call_back) {
	qnps_no_call_back++;
	NPN_DestroyStream(inst->pi->npp, stream, 0);
	qnps_no_call_back--;
    }
}

/*!
  \fn QNPInstance* QNPStream::instance()

  Returns the QNPInstance for which this stream was created.
*/

/*!
  Returns the URL from which the stream was created.
*/
const char* QNPStream::url() const
{
    return stream->url;
}

/*!
  Returns the length of the stream (???).
*/
uint QNPStream::end() const
{
    return stream->end;
}

/*!
  Returns the time when the source of the stream was last modified.
*/
uint QNPStream::lastModified() const
{
    return stream->lastmodified;
}

/*!
  Returns the MIME type of the stream.
*/
const char* QNPStream::type() const
{
    return mtype;
}

/*!
  Returns TRUE if the stream is seekable.
*/
bool QNPStream::seekable() const
{
    return seek;
}

/*!
  Requests the given section of the stream be sent to the
  QNPInstance::write() function of the instance() of this stream.

  See also:
  <a href=http://developer.netscape.com/library/documentation/communicator/plugin/refpgst.htm#npnrequestread>
  Netscape: NPN_RequestRead method</a>
*/
void QNPStream::requestRead(int offset, uint length)
{
    NPByteRange range;
    range.offset = offset;
    range.length = length;
    range.next = 0; // ### Only one supported at this time
    NPN_RequestRead(stream, &range);
}

/*!
  Writes data \e to the stream.

  See also:
  <a href=http://developer.netscape.com/library/documentation/communicator/plugin/refpgst.htm#npnwrite>
  Netscape: NPN_Write method</a>
*/
int QNPStream::write( int len, void* buffer )
{
    return NPN_Write(inst->pi->npp, stream, len, buffer);
}





/*******************************************************************************
 * The plugin itself - only one ever exists, created by QNPlugin::create()
 ******************************************************************************/


/*!
  \class QNPlugin qnp.h
  \brief The plugin central factory.

  This class is the heart of the plugin.  One instance of this object is
  created when the plugin is \e first needed, by calling
  QNPlugin::create(), which must be implemented in your plugin code to
  return some derived class of QNPlugin.  The one QNPlugin object creates
  all instances for a single running Web-browser process.

  Additionally, if Qt is linked to the plugin as
  a dynamic library, only one instance of QApplication will exist
  <em>across all plugins that have been made with Qt.</em>  So,
  your plugin should tread lightly on global settings - do not for
  example, use QApplication::setFont() - that will change the font in
  every widget of every Qt-based plugin currently loaded!
*/

/*!
  \fn QNPlugin* QNPlugin::create()

  This must be implemented by your plugin code.  It should return a derived
  class of QNPlugin.
*/

/*!
  Returns the plugin most recently returns by QNPlugin::create().
*/
QNPlugin* QNPlugin::actual()
{
    return qNP;
}

/*!
  Creates a QNPlugin.  This may only be used by the constructor 
  derived class
  returned by plugin's implementation of the QNPlugin::create() function.
*/
QNPlugin::QNPlugin()
{
}

/*!
  Destroys the QNPlugin.  This is called by the plugin binding code
  just before the plugin is about to be unloaded from memory.  If newWindow()
  has been called, a QApplication will still exist at this time, but will
  be deleted shortly after before the plugin is deleted.
*/
QNPlugin::~QNPlugin()
{
}

/*!
  Returns the version information - the version of the plugin API, and
  the version of the browser.

  See also:
  <a href=http://developer.netscape.com/library/documentation/communicator/plugin/refpgve.htm#npnversion>
  Netscape: NPN_Version method</a>
*/
void QNPlugin::getVersionInfo(int& plugin_major, int& plugin_minor,
	     int& browser_major, int& browser_minor)
{
    NPN_Version(&plugin_major, &plugin_minor, &browser_major, &browser_minor);
}

/*!
    \fn QNPInstance* QNPlugin::newInstance()

  Override this to return an appropriate derived class of QNPInstance.
*/

/*!
    \fn const char* QNPlugin::getMIMEDescription() const

  Override this to return the MIME description of the data formats
  supported by your plugin.  The format of this string is described
  by the following example:

\code
    const char* getMIMEDescription() const
    {
        return "image/x-png:png:PNG Image;"
               "image/png:png:PNG Image;"
               "image/x-portable-bitmap:pbm:PBM Image;"
               "image/x-portable-graymap:pgm:PGM Image;"
               "image/x-portable-pixmap:ppm:PPM Image;"
               "image/bmp:bmp:BMP Image;"
               "image/x-ms-bmp:bmp:BMP Image;"
               "image/x-xpixmap:xpm:XPM Image;"
               "image/xpm:xpm:XPM Image";
    }
\endcode
*/

/*!
  \fn const char* QNPlugin::getPluginNameString() const

  Returns the plain-text name of the plugin.

  See also:
  <a href=http://developer.netscape.com/library/documentation/communicator/plugin/refpgdr.htm#nppgetvalue>
  Netscape: NPP_GetValue method</a>
*/

/*!
  \fn const char* QNPlugin::getPluginDescriptionString() const

  Returns a plain-text description of the plugin.
*/

/*!
  Override to return a reference to the Java class that represents
  the plugin.  The default returns 0, indicating no class.

  If you override this class, you must also override
  QNPlugin::unuseJavaClass().

  The return value is actually a <tt>jref</tt> we use <tt>void*</tt> so
  as to avoid burdening plugins which do not require Java.

  \sa getJavaEnv(), QNPInstance::getJavaPeer(),
      <a href=http://developer.netscape.com/library/documentation/communicator/plugin/refpgja.htm#nppgetjavaclass>
      Netscape: NPP_GetJavaClass</a>
*/
void* QNPlugin::getJavaClass()
{
    return NULL;
}

/*!
  This function is called when the plugin is shutting down,
  with \a jc set to the value returned earlier by getJavaClass().
  The function should \e unuse the Java class and return 0.

  \sa <a href=http://home.netscape.com/eng/mozilla/3.0/handbook/plugins/wr3.htm#Assoc>
    Netscape: Associating a Class with your Plug-in</a>
*/
void QNPlugin::unuseJavaClass()
{
    fatal("QNPlugin::unuseJavaClass() must overridden along with getJavaClass()");
}

/*!
  Returns a pointer to the Java execution environment, or 0 if
  Java is disabled or an error occurred.

  The return value is actually a <tt>JRIEnv*</tt> we use <tt>void*</tt> so
  as to avoid burdening plugins which do not require Java.

  \sa getJavaClass(), QNPInstance::getJavaPeer(),
      <a href=http://developer.netscape.com/library/documentation/communicator/plugin/refpgja.htm#npngetjavaenv>
      Netscape: NPN_GetJavaEnv</a>
*/
void* QNPlugin::getJavaEnv() const
{
    return NPN_GetJavaEnv();
}
