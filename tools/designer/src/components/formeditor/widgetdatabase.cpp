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

#include "widgetdatabase.h"
#include "widgetfactory.h"
#include "spacer.h"

#include <pluginmanager.h>
#include <customwidget.h>

#include <qalgorithms.h>
#include <qdebug.h>

// ----------------------------------------------------------
WidgetDataBaseItem::WidgetDataBaseItem(const QString &name, const QString &group)
    : m_name(name),
      m_group(group),
      m_compat(0),
      m_container(0),
      m_form(0),
      m_custom(0)
{
}

QString WidgetDataBaseItem::name() const
{
    return m_name;
}

void WidgetDataBaseItem::setName(const QString &name)
{
    m_name = name;
}

QString WidgetDataBaseItem::group() const
{
    return m_group;
}

void WidgetDataBaseItem::setGroup(const QString &group)
{
    m_group = group;
}

QString WidgetDataBaseItem::toolTip() const
{
    return m_toolTip;
}

void WidgetDataBaseItem::setToolTip(const QString &toolTip)
{
    m_toolTip = toolTip;
}

QString WidgetDataBaseItem::whatsThis() const
{
    return m_whatsThis;
}

void WidgetDataBaseItem::setWhatsThis(const QString &whatsThis)
{
    m_whatsThis = whatsThis;
}

QString WidgetDataBaseItem::includeFile() const
{
    return m_includeFile;
}

void WidgetDataBaseItem::setIncludeFile(const QString &includeFile)
{
    m_includeFile = includeFile;
}

QIcon WidgetDataBaseItem::icon() const
{
    return m_icon;
}

void WidgetDataBaseItem::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

bool WidgetDataBaseItem::isCompat() const
{
    return m_compat;
}

void WidgetDataBaseItem::setCompat(bool b)
{
    m_compat = b;
}

bool WidgetDataBaseItem::isContainer() const
{
    return m_container;
}

void WidgetDataBaseItem::setContainer(bool b)
{
    m_container = b;
}

bool WidgetDataBaseItem::isForm() const
{
    return m_form;
}

void WidgetDataBaseItem::setForm(bool b)
{
    m_form = b;
}

bool WidgetDataBaseItem::isCustom() const
{
    return m_custom;
}

void WidgetDataBaseItem::setCustom(bool b)
{
    m_custom = b;
}

// ----------------------------------------------------------
WidgetDataBase::WidgetDataBase(AbstractFormEditor *core, QObject *parent)
    : AbstractWidgetDataBase(parent),
      m_core(core)
{
#define DECLARE_LAYOUT(L, C)
#define DECLARE_COMPAT_WIDGET(W, C) DECLARE_WIDGET(W, C)
#define DECLARE_WIDGET(W, C) append(new WidgetDataBaseItem(QString::fromUtf8(#W)));

#include "widgets.table"

#undef DECLARE_COMPAT_WIDGET
#undef DECLARE_LAYOUT
#undef DECLARE_WIDGET

    append(new WidgetDataBaseItem(QString::fromUtf8("Spacer")));
    append(new WidgetDataBaseItem(QString::fromUtf8("QSplitter")));
    append(new WidgetDataBaseItem(QString::fromUtf8("QLayoutWidget")));
    append(new WidgetDataBaseItem(QString::fromUtf8("QDesignerWidget")));
    append(new WidgetDataBaseItem(QString::fromUtf8("QDesignerDialog")));
    append(new WidgetDataBaseItem(QString::fromUtf8("QDesignerCompatWidget")));

// ### remove me
    // ### check the casts
    static_cast<WidgetDataBaseItem *>(item(indexOfClassName("QTabWidget")))->setContainer(true);
    static_cast<WidgetDataBaseItem *>(item(indexOfClassName("QGroupBox")))->setContainer(true);
    static_cast<WidgetDataBaseItem *>(item(indexOfClassName("QStackedWidget")))->setContainer(true);
    static_cast<WidgetDataBaseItem *>(item(indexOfClassName("QToolBox")))->setContainer(true);
    static_cast<WidgetDataBaseItem *>(item(indexOfClassName("QFrame")))->setContainer(true);
    static_cast<WidgetDataBaseItem *>(item(indexOfClassName("QLayoutWidget")))->setContainer(true);
    static_cast<WidgetDataBaseItem *>(item(indexOfClassName("QDesignerWidget")))->setContainer(true);
    static_cast<WidgetDataBaseItem *>(item(indexOfClassName("QDesignerDialog")))->setContainer(true);
    static_cast<WidgetDataBaseItem *>(item(indexOfClassName("QDesignerCompatWidget")))->setContainer(true);
    static_cast<WidgetDataBaseItem *>(item(indexOfClassName("QSplitter")))->setContainer(true);
    static_cast<WidgetDataBaseItem *>(item(indexOfClassName("QMainWindow")))->setContainer(true);

    static_cast<WidgetDataBaseItem *>(item(indexOfClassName("QWidget")))->setForm(true);
    static_cast<WidgetDataBaseItem *>(item(indexOfClassName("QDialog")))->setForm(true);
}

WidgetDataBase::~WidgetDataBase()
{
}

AbstractFormEditor *WidgetDataBase::core() const
{
    return m_core;
}

int WidgetDataBase::indexOfObject(QObject *object, bool resolveName) const
{
    if (resolveName)
        return AbstractWidgetDataBase::indexOfClassName(WidgetFactory::classNameOf(object));
    return AbstractWidgetDataBase::indexOfObject(object, resolveName);
}

AbstractWidgetDataBaseItem *WidgetDataBase::item(int index) const
{
    return AbstractWidgetDataBase::item(index);
}

void WidgetDataBase::loadPlugins()
{
    PluginManager pluginManager;
    
    QStringList plugins = pluginManager.registeredPlugins();
    
    QListMutableIterator<AbstractWidgetDataBaseItem *> it(m_items);
    while (it.hasNext()) {
        AbstractWidgetDataBaseItem *item = it.next();
        
        if (item->isCustom()) {
            it.remove();
            delete item;
        }
    }
    
    foreach (QString plugin, plugins) {
        QObject *o = pluginManager.instance(plugin);
        
        if (ICustomWidget *c = qt_cast<ICustomWidget*>(o)) {
            if (!c->isInitialized())
                c->initialize(core());
                
            WidgetDataBaseItem *item = new WidgetDataBaseItem();
            item->setContainer(c->isContainer());
            item->setCustom(true);
            item->setForm(c->isForm());
            item->setGroup(c->group());
            item->setIcon(c->icon());
            item->setIncludeFile(c->includeFile());
            item->setName(c->name());
            item->setToolTip(c->toolTip());
            item->setWhatsThis(c->whatsThis());
            
            append(item);            
        }
    }
}
