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

#include "qdesigner_formbuilder_p.h"
#include "qdesigner_widget_p.h"

// sdk
#include <QtDesigner/extrainfo.h>
#include <QtDesigner/container.h>
#include <QtDesigner/customwidget.h>
#include <QtDesigner/propertysheet.h>
#include <QtDesigner/QExtensionManager>
#include <QtDesigner/abstractformeditor.h>
#include <QtDesigner/abstracticoncache.h>
#include <QtDesigner/ui4.h>

// shared
#include <resourcefile_p.h>
#include <widgetfactory_p.h>
#include <pluginmanager_p.h>

#include <QtGui/QWidget>
#include <QtGui/QMenu>
#include <QtGui/QToolBar>
#include <QtGui/QMenuBar>

#include <QtCore/QBuffer>
#include <QtCore/qdebug.h>

namespace qdesigner_internal {

QDesignerFormBuilder::QDesignerFormBuilder(QDesignerFormEditorInterface *core)
    : m_core(core)
{
    Q_ASSERT(m_core);
}

QWidget *QDesignerFormBuilder::createWidgetFromContents(const QString &contents, QWidget *parentWidget)
{
    QByteArray data = contents.toUtf8();
    QBuffer buffer(&data);
    return load(&buffer, parentWidget);
}


QWidget *QDesignerFormBuilder::createWidget(const QString &widgetName, QWidget *parentWidget, const QString &name)
{
    QWidget *widget = 0;

    widget = core()->widgetFactory()->createWidget(widgetName, parentWidget);

    if (widget)
        widget->setObjectName(name);

    return widget;
}

bool QDesignerFormBuilder::addItem(DomWidget *ui_widget, QWidget *widget, QWidget *parentWidget)
{
    if (QFormBuilder::addItem(ui_widget, widget, parentWidget)) {
        return true;
    } else if (QDesignerContainerExtension *container = qt_extension<QDesignerContainerExtension*>(m_core->extensionManager(), parentWidget)) {
        container->addWidget(widget);
        return true;
    }

    return false;
}

bool QDesignerFormBuilder::addItem(DomLayoutItem *ui_item, QLayoutItem *item, QLayout *layout)
{
    return QFormBuilder::addItem(ui_item, item, layout);
}

QIcon QDesignerFormBuilder::nameToIcon(const QString &filePath, const QString &qrcPath)
{
    return QIcon(core()->iconCache()->resolveQrcPath(filePath, qrcPath, workingDirectory().absolutePath()));
}

QPixmap QDesignerFormBuilder::nameToPixmap(const QString &filePath, const QString &qrcPath)
{
    return QPixmap(core()->iconCache()->resolveQrcPath(filePath, qrcPath, workingDirectory().absolutePath()));
}

void QDesignerFormBuilder::applyProperties(QObject *o, const QList<DomProperty*> &properties)
{
    QDesignerPropertySheetExtension *sheet = qt_extension<QDesignerPropertySheetExtension*>(core()->extensionManager(), o);
    Q_ASSERT(sheet != 0);

    foreach (DomProperty *p, properties) {
        int index = sheet->indexOf(p->attributeName());
        QVariant v = toVariant(o->metaObject(), p);

        if (!v.isNull()) {
            if (strcmp(o->metaObject()->className(), "QAxWidget") != 0)
                sheet->setProperty(index, v);

            if (o->metaObject()->indexOfProperty(p->attributeName().toUtf8()) != -1)
                o->setProperty(p->attributeName().toUtf8(), v);
        }
    }
}

DomWidget *QDesignerFormBuilder::createDom(QWidget *widget, DomWidget *ui_parentWidget, bool recursive)
{
    DomWidget *ui_widget = QFormBuilder::createDom(widget, ui_parentWidget, recursive);

    if (QDesignerExtraInfoExtension *extra = qt_extension<QDesignerExtraInfoExtension*>(m_core->extensionManager(), widget)) {
        extra->saveWidgetExtraInfo(ui_widget);
    }

    return ui_widget;
}

QWidget *QDesignerFormBuilder::create(DomWidget *ui_widget, QWidget *parentWidget)
{
    QWidget *widget = QFormBuilder::create(ui_widget, parentWidget);

    if (QDesignerExtraInfoExtension *extra = qt_extension<QDesignerExtraInfoExtension*>(m_core->extensionManager(), widget)) {
        extra->loadWidgetExtraInfo(ui_widget);
    }

    return widget;
}


} // namespace qdesigner_internal
