/****************************************************************************
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

#ifndef QGPLUGIN_H
#define QGPLUGIN_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//
//

#ifndef QT_H
#include "qobject.h"
#endif // QT_H

#ifndef QT_NO_COMPONENT

#ifndef Q_EXTERN_C
#ifdef __cplusplus
#define Q_EXTERN_C    extern "C"
#else
#define Q_EXTERN_C    extern
#endif
#endif

#ifndef Q_EXPORT_PLUGIN
#if defined(QT_THREAD_SUPPORT)
#define QT_THREADED_BUILD 1
#define Q_PLUGIN_FLAGS_STRING "11"
#else
#define QT_THREADED_BUILD 0
#define Q_PLUGIN_FLAGS_STRING "01"
#endif

// this is duplicated at Q_UCM_VERIFICATION_DATA in qcom_p.h
// NOTE: if you change pattern, you MUST change the pattern in
// qcomlibrary.cpp as well.  changing the pattern will break all
// backwards compatibility as well (no old plugins will be loaded).
#ifndef Q_PLUGIN_VERIFICATION_DATA
#  define Q_PLUGIN_VERIFICATION_DATA \
	static const char *qt_ucm_verification_data =			\
            "pattern=""QT_UCM_VERIFICATION_DATA""\n"			\
            "version="QT_VERSION_STR"\n"				\
            "flags="Q_PLUGIN_FLAGS_STRING"\n"				\
	    "buildkey="QT_BUILD_KEY"\0";
#endif // Q_PLUGIN_VERIFICATION_DATA

#define Q_PLUGIN_INSTANTIATE( IMPLEMENTATION )	\
	{ \
	    IMPLEMENTATION *i = new IMPLEMENTATION;	\
	    return i->iface(); \
	}

#    ifdef Q_WS_WIN
#	ifdef Q_CC_BOR
#	    define Q_EXPORT_PLUGIN(PLUGIN) \
	        Q_PLUGIN_VERIFICATION_DATA \
		Q_EXTERN_C __declspec(dllexport) \
                const char * __stdcall qt_ucm_query_verification_data() \
                { return qt_ucm_verification_data; } \
		Q_EXTERN_C __declspec(dllexport) QUnknownInterface* \
                __stdcall ucm_instantiate() \
		Q_PLUGIN_INSTANTIATE( PLUGIN )
#	else
#	    define Q_EXPORT_PLUGIN(PLUGIN) \
	        Q_PLUGIN_VERIFICATION_DATA \
		Q_EXTERN_C __declspec(dllexport) \
                const char *qt_ucm_query_verification_data() \
                { return qt_ucm_verification_data; } \
		Q_EXTERN_C __declspec(dllexport) QUnknownInterface* ucm_instantiate() \
		Q_PLUGIN_INSTANTIATE( PLUGIN )
#	endif
#    else
#	define Q_EXPORT_PLUGIN(PLUGIN) \
	    Q_PLUGIN_VERIFICATION_DATA \
	    Q_EXTERN_C \
            const char *qt_ucm_query_verification_data() \
            { return qt_ucm_verification_data; } \
	    Q_EXTERN_C QUnknownInterface* ucm_instantiate() \
            Q_PLUGIN_INSTANTIATE( PLUGIN )
#    endif

#endif

struct QUnknownInterface;

class Q_KERNEL_EXPORT QGPlugin : public QObject
{
    Q_OBJECT
public:
    QGPlugin( QUnknownInterface *i );
    ~QGPlugin();

    QUnknownInterface* iface();
    void setIface( QUnknownInterface *iface );

private:
    QGPlugin();
    QUnknownInterface* _iface;
};

#endif // QT_NO_COMPONENT

#endif // QGPLUGIN_H
