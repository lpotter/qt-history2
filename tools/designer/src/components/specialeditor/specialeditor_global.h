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

#ifndef SPECIALEDITORSUPPORT_GLOBAL_H
#define SPECIALEDITORSUPPORT_GLOBAL_H

#include <qglobal.h>

#ifdef Q_OS_WIN
#ifdef QT_SPECIALEDITORSUPPORT_LIBRARY
# define QT_SPECIALEDITORSUPPORT_EXPORT
#else
# define QT_SPECIALEDITORSUPPORT_EXPORT
#endif
#else
#define QT_SPECIALEDITORSUPPORT_EXPORT
#endif

#endif // SPECIALEDITORSUPPORT_GLOBAL_H
