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

#ifndef QABSTRACTEVENTDISPATCHER_P_H
#define QABSTRACTEVENTDISPATCHER_P_H

#include "qabstracteventdispatcher.h"
#include "qobject_p.h"

class QAbstractEventDispatcherPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QAbstractEventDispatcher)
public:
    inline QAbstractEventDispatcherPrivate()
        : event_filter(0)
    { }
    QAbstractEventDispatcher::EventFilter event_filter;
};

#endif // QABSTRACTEVENTDISPATCHER_P_H
