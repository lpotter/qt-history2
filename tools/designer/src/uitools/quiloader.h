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

#ifndef QUILOADER_H
#define QUILOADER_H

#include <QtCore/QObject>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QWidget;
class QLayout;
class QAction;
class QActionGroup;
class QString;
class QIODevice;
class QDir;

class QUiLoaderPrivate;
class QUiLoader : public QObject
{
    Q_OBJECT
public:
    QUiLoader(QObject *parent = 0);
    virtual ~QUiLoader();

    QStringList pluginPaths() const;
    void clearPluginPaths();
    void addPluginPath(const QString &path);

    QWidget *load(QIODevice *device, QWidget *parentWidget = 0);
    QStringList availableWidgets() const;

    virtual QWidget *createWidget(const QString &className, QWidget *parent = 0, const QString &name = QString());
    virtual QLayout *createLayout(const QString &className, QObject *parent = 0, const QString &name = QString());
    virtual QActionGroup *createActionGroup(QObject *parent = 0, const QString &name = QString());
    virtual QAction *createAction(QObject *parent = 0, const QString &name = QString());

    void setWorkingDirectory(const QDir &dir);
    QDir workingDirectory() const;

    void setScriptingEnabled(bool enabled);
    bool isScriptingEnabled() const;

private:
    Q_DECLARE_PRIVATE(QUiLoader)
    Q_DISABLE_COPY(QUiLoader)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QUILOADER_H
