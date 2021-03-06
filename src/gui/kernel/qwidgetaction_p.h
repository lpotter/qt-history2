/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QWIDGETACTION_P_H
#define QWIDGETACTION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "private/qaction_p.h"

QT_BEGIN_NAMESPACE

class QWidgetActionPrivate : public QActionPrivate
{
    Q_DECLARE_PUBLIC(QWidgetAction)
public:
    inline QWidgetActionPrivate() : defaultWidgetInUse(false), autoCreated(false) {}
    QPointer<QWidget> defaultWidget;
    QList<QWidget *> createdWidgets;
    uint defaultWidgetInUse : 1;
    uint autoCreated : 1; // created by QToolBar::addWidget and the like
    
    inline void _q_widgetDestroyed(QObject *o) {
        createdWidgets.removeAll(static_cast<QWidget *>(o));
    }
};

QT_END_NAMESPACE

#endif
