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

#include <qmousedriverplugin_qws.h>
#include <qmousetslib_qws.h>

QT_BEGIN_NAMESPACE

class TslibMouseDriver : public QMouseDriverPlugin
{
public:
    TslibMouseDriver();

    QStringList keys() const;
    QWSMouseHandler* create(const QString &driver, const QString &device);
};

TslibMouseDriver::TslibMouseDriver()
    : QMouseDriverPlugin()
{
}

QStringList TslibMouseDriver::keys() const
{
    return (QStringList() << "tslib");
}

QWSMouseHandler* TslibMouseDriver::create(const QString &driver,
                                          const QString &device)
{
    if (driver.toLower() != "tslib")
        return 0;
    return new QWSTslibMouseHandler(driver, device);
}

Q_EXPORT_STATIC_PLUGIN(TslibMouseDriver)
Q_EXPORT_PLUGIN2(qwstslibmousehandler, TslibMouseDriver)

QT_END_NAMESPACE
