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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the public API.  This header file may
// change from version to version without notice, or even be
// removed.
//
// We mean it.
//
//

#ifndef QDBUSABSTRACTINTERFACEPRIVATE_H
#define QDBUSABSTRACTINTERFACEPRIVATE_H

#include <qdbusabstractinterface.h>
#include <qdbusconnection.h>
#include <qdbuserror.h>
#include "private/qobject_p.h"

#define ANNOTATION_NO_WAIT      "org.freedesktop.DBus.Method.NoReply"

class QDBusAbstractInterfacePrivate: public QObjectPrivate
{
public:
    Q_DECLARE_PUBLIC(QDBusAbstractInterface)
    
    QDBusConnection conn;
    QDBusConnectionPrivate *connp;
    QString service;
    QString path;
    QString interface;
    mutable QDBusError lastError;
    bool isValid;

    inline QDBusAbstractInterfacePrivate(const QDBusConnection& con, QDBusConnectionPrivate *conp,
                                         const QString &serv, const QString &p, const QString &iface)
        : conn(con), connp(conp), service(serv), path(p), interface(iface), isValid(true)
    { }
    virtual ~QDBusAbstractInterfacePrivate() { }

    // these functions do not check if the property is valid
    QVariant property(const QMetaProperty &mp) const;
    void setProperty(const QMetaProperty &mp, const QVariant &value);
};


#endif
