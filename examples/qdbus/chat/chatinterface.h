/*
 * This file was generated by dbusxml2cpp version 0.6
 * Command line was: dbusxml2cpp -p chatinterface -a chatadaptor -- com.trolltech.ChatInterface.xml
 *
 * dbusxml2cpp is Copyright (C) 2006 Trolltech AS. All rights reserved.
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef CHATINTERFACE_H_260331151587098
#define CHATINTERFACE_H_260331151587098

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

/*
 * Proxy class for interface com.trolltech.ChatInterface
 */
class ComTrolltechChatInterfaceInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "com.trolltech.ChatInterface"; }

public:
    ComTrolltechChatInterfaceInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~ComTrolltechChatInterfaceInterface();

public Q_SLOTS: // METHODS
Q_SIGNALS: // SIGNALS
    void action(const QString &nickname, const QString &text);
    void message(const QString &nickname, const QString &text);
};

namespace com {
  namespace trolltech {
    typedef ::ComTrolltechChatInterfaceInterface ChatInterface;
  }
}
#endif
