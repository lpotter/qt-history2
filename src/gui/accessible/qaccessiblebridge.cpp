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

#include "qaccessiblebridge.h"

#ifndef QT_NO_ACCESSIBILITY

/*!
    \class QAccessibleBridge
    \brief The QAccessibleBridge class is the base class for
    accessibility back-ends.

    \ingroup accessibility

    Qt supports Microsoft Active Accessibility (MSAA), Mac OS X
    Accessibility, and the Unix/X11 AT-SPI standard. By subclassing
    QAccessibleBridge, you can support other backends than the
    predefined ones.

    Currently, custom bridges are only supported on Unix. We might
    add support for them on other platforms as well if there is
    enough demand.

    \sa QAccessible, QAccessibleBridgePlugin
*/

/*!
    \fn QAccessibleBridge::~QAccessibleBridge()

    Destroys the accessibility bridge object.
*/

/*!
    \fn void QAccessibleBridge::setRootObject(QAccessibleInterface *object)

    This function is called by Qt at application startup to set the
    root accessible object of the application to \a object. All other
    accessible objects in the application can be reached by the
    client using object navigation.

    \sa QAccessible::setRootObject()
*/

/*!
    \fn void QAccessibleBridge::notifyAccessibilityUpdate(int reason, QAccessibleInterface *interface, int child)

    This function is called by Qt to notify the bridge about a change
    in the accessibility information for object wrapped by the given
    \a interface.

    \a reason specifies the cause of the change. It can take values
    of type QAccessible::Event.

    \a child is the (1-based) index of the child element that has
    changed. When \a child is 0, the object itself has changed.

    \sa QAccessible::updateAccessibility()
*/

/*!
    \class QAccessibleBridgePlugin
    \brief The QAccessibleBridgePlugin class provides an abstract
    base for accessibility bridge plugins.

    \ingroup plugins
    \ingroup accessibility

    Writing an accessibility bridge plugin is achieved by subclassing
    this base class, reimplementing the pure virtual functions keys()
    and create(), and exporting the class with the
    Q_EXPORT_PLUGIN2() macro.

    \sa QAccessibleBridge, QAccessiblePlugin, {How to Create Qt Plugins}
*/

/*!
    Constructs an accessibility bridge plugin with the given \a
    parent. This is invoked automatically by the Q_EXPORT_PLUGIN2()
    macro.
*/
QAccessibleBridgePlugin::QAccessibleBridgePlugin(QObject *parent)
    : QObject(parent)
{

}

/*!
    Destroys the accessibility bridge plugin.

    You never have to call this explicitly. Qt destroys a plugin
    automatically when it is no longer used.
*/
QAccessibleBridgePlugin::~QAccessibleBridgePlugin()
{

}

/*!
    \fn QStringList QAccessibleBridgePlugin::keys() const

    Returns the list of keys this plugins supports.

    These keys must be the names of the bridges that this
    plugin provides.

    \sa create()
*/

/*!
    \fn QAccessibleBridge *QAccessibleBridgePlugin::create(const QString &key)

    Creates and returns the QAccessibleBridge object corresponding to
    the given \a key.

    \sa keys()
*/

#endif // QT_NO_ACCESSIBILITY
