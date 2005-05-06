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

#ifndef WIDGETFACTORY_H
#define WIDGETFACTORY_H

#include "shared_global.h"
#include <QtDesigner/abstractwidgetfactory.h>

#include <pluginmanager.h>

#include <QtCore/QMap>
#include <QtCore/QVariant>
#include <QtCore/QPointer>

class QObject;
class QWidget;
class QLayout;
class QDesignerFormEditorInterface;
class QDesignerCustomWidgetInterface;

class QT_SHARED_EXPORT WidgetFactory: public QDesignerWidgetFactoryInterface
{
public:
    WidgetFactory(QDesignerFormEditorInterface *core, QObject *parent = 0);
    ~WidgetFactory();

    virtual QWidget* containerOfWidget(QWidget *widget) const;
    virtual QWidget* widgetOfContainer(QWidget *widget) const;

    virtual QWidget *createWidget(const QString &className, QWidget *parentWidget) const;
    virtual QLayout *createLayout(QWidget *widget, QLayout *layout, int type) const;

    virtual bool isPassiveInteractor(QWidget *widget);
    virtual void initialize(QObject *object) const;

    virtual QDesignerFormEditorInterface *core() const;

    static const char* classNameOf(QObject* o);

public slots:
    void loadPlugins();

private:
    QDesignerFormEditorInterface *m_core;
    QMap<QString, QDesignerCustomWidgetInterface*> m_customFactory;

    static QPointer<QWidget> *m_lastPassiveInteractor;
    static bool m_lastWasAPassiveInteractor;
};

#endif // WIDGETFACTORY_H
