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

#ifndef FORMEDITOR_GLOBAL_H
#define FORMEDITOR_GLOBAL_H

#include <qglobal.h>

#ifdef Q_OS_WIN
#ifdef QT_FORMEDITOR_LIBRARY
# define QT_FORMEDITOR_EXPORT
#else
# define QT_FORMEDITOR_EXPORT
#endif
#else
#define QT_FORMEDITOR_EXPORT
#endif

#endif // FORMEDITOR_GLOBAL_H
