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

#include <abstractformwindowtool.h>
#include "formwindow_widgetstack.h"

#include <QtGui/QWidget>
#include <QtGui/qevent.h>
#include <QtGui/QAction>

#include <QtCore/qdebug.h>


FormWindowWidgetStack::FormWindowWidgetStack(QWidget *parent)
    : QWidget(parent),
      m_current_index(-1)
{
}

FormWindowWidgetStack::~FormWindowWidgetStack()
{
}

int FormWindowWidgetStack::count() const
{
    return m_tools.count();
}

AbstractFormWindowTool *FormWindowWidgetStack::currentTool() const
{
    if (m_current_index == -1)
        return 0;
    return m_tools.at(m_current_index);
}

void FormWindowWidgetStack::setCurrentTool(int index)
{
    qDebug() << "FormWindowWidgetStack::setCurrentTool():" << index;
    if (index < 0 || index >= count()) {
        qWarning("FormWindowWidgetStack::setCurrentTool(): invalid index: %d", index);
        return;
    }

    if (index == m_current_index)
        return;

    if (m_current_index != -1)
        m_tools.at(m_current_index)->deactivated();

    m_current_index = index;

    AbstractFormWindowTool *tool = m_tools.at(m_current_index);
    tool->activated();
    QWidget *w = tool->editor();
    if (w != 0) {
        w->raise();
        qDebug() << "Raising" << w;
    }

    emit currentToolChanged(index);
}

void FormWindowWidgetStack::setSenderAsCurrentTool()
{
    AbstractFormWindowTool *tool = 0;
    QAction *action = qt_cast<QAction*>(sender());
    if (action == 0) {
        qWarning("FormWindowWidgetStack::setSenderAsCurrentTool(): sender is not a QAction");
        return;
    }

    foreach (AbstractFormWindowTool *t, m_tools) {
        if (action == t->action()) {
            tool = t;
            break;
        }
    }

    if (tool == 0) {
        qWarning("FormWindowWidgetStack::setSenderAsCurrentTool(): unknown tool");
        return;
    }

    setCurrentTool(tool);
}

int FormWindowWidgetStack::indexOf(AbstractFormWindowTool *tool) const
{
    for (int i = 0; i < m_tools.size(); ++i) {
        if (m_tools.at(i) == tool)
            return i;
    }

    return -1;
}

void FormWindowWidgetStack::setCurrentTool(AbstractFormWindowTool *tool)
{
    int index = indexOf(tool);
    if (index == -1) {
        qWarning("FormWindowWidgetStack::setCurrentTool(): unknown tool");
        return;
    }

    setCurrentTool(index);
}

void FormWindowWidgetStack::addTool(AbstractFormWindowTool *tool)
{
    QWidget *w = tool->editor();
    if (w != 0)
        w->setParent(this);

    m_tools.append(tool);

    connect(tool->action(), SIGNAL(triggered()), this, SLOT(setSenderAsCurrentTool()));
}

void FormWindowWidgetStack::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    foreach (AbstractFormWindowTool *tool, m_tools) {
        QWidget *w = tool->editor();
        if (w != 0)
            w->setGeometry(0, 0, event->size().width(), event->size().height());
    }
}

AbstractFormWindowTool *FormWindowWidgetStack::tool(int index) const
{
    if (index < 0 || index >= count())
        return 0;

    return m_tools.at(index);
}

int FormWindowWidgetStack::currentIndex() const
{
    return m_current_index;
}
