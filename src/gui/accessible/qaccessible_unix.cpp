/****************************************************************************
**
** Implementation of QAccessible class for Unix.
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
** EDITIONS: PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qaccessible.h"
#include "qaccessiblebridge.h"

#ifndef QT_NO_ACCESSIBILITY

#include "qcoreapplication.h"
#include "qmutex.h"
#include "qvector.h"
#include "private/qfactoryloader_p.h"

#include <stdlib.h>

Q_GLOBAL_STATIC_LOCKED_WITH_ARGS(QFactoryLoader, loader,
   (QAccessibleBridgeFactoryInterface_iid, QCoreApplication::libraryPaths(), "/accessiblebridge"))

Q_GLOBAL_STATIC(QVector<QAccessibleBridge *>, bridges)
static bool isInit = false;

void QAccessible::initialize()
{
    if (isInit)
        return;
    isInit = true;

    if (qstrcmp(getenv("QT_ACCESSIBILITY"), "1") != 0)
        return;

    const QStringList l = loader()->keys();
    for (int i = 0; i < l.count(); ++i) {
        qDebug("GOT: %s, %p", l.at(i).ascii(), loader()->instance(l.at(i)));
        if (QAccessibleBridgeFactoryInterface *factory =
                qt_cast<QAccessibleBridgeFactoryInterface*>(loader()->instance(l.at(i)))) {
            qDebug("JO");
            QAccessibleBridge * bridge = factory->create(l.at(i));
            if (bridge)
                bridges()->append(bridge);
        }
    }
}

void QAccessible::cleanup()
{
    qDeleteAll(*bridges());
}

void QAccessible::updateAccessibility(QObject *o, int who, Event reason)
{
    Q_ASSERT(o);

    if (updateHandler) {
        updateHandler(o, who, reason);
        return;
    }

    initialize();
    if (bridges()->isEmpty())
        return;

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(o);
    if (!iface)
        return;

    for (int i = 0; i < bridges()->count(); ++i)
        bridges()->at(i)->notifyAccessibilityUpdate(reason, iface, who);
    delete iface;
}

void QAccessible::setRootObject(QObject *o)
{
    if (rootObjectHandler) {
        rootObjectHandler(o);
        return;
    }

    initialize();
    if (bridges()->isEmpty())
        return;

    if (!o)
        return;
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(o);
    for (int i = 0; i < bridges()->count(); ++i)
        bridges()->at(i)->setRootObject(iface);
    delete iface;
}

#endif
