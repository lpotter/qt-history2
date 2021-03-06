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
#ifndef QDBUSSERVER_H
#define QDBUSSERVER_H

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtDBus/qdbusmacros.h>

QT_BEGIN_HEADER

class QDBusConnectionPrivate;
class QDBusError;
class QDBusConnection;

class QDBUS_EXPORT QDBusServer: public QObject
{
    Q_OBJECT
public:
    QDBusServer(const QString &address, QObject *parent = 0);

    bool isConnected() const;
    QDBusError lastError() const;
    QString address() const;

Q_SIGNALS:
    void newConnection(const QDBusConnection &connection);

private:
    Q_DISABLE_COPY(QDBusServer)
    QDBusConnectionPrivate *d;
};

QT_END_HEADER

#endif
