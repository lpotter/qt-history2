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

#include "buddyeditor_tool.h"
#include "buddyeditor.h"

#include <abstractformwindow.h>
#include <abstractformeditor.h>

#include <QtGui/QAction>

BuddyEditorTool::BuddyEditorTool(AbstractFormWindow *formWindow, QObject *parent)
    : AbstractFormWindowTool(parent),
      m_formWindow(formWindow)
{
    m_action = new QAction(tr("Edit Buddies"), this);
}

BuddyEditorTool::~BuddyEditorTool()
{
}

AbstractFormEditor *BuddyEditorTool::core() const
{
    return m_formWindow->core();
}

AbstractFormWindow *BuddyEditorTool::formWindow() const
{
    return m_formWindow;
}

bool BuddyEditorTool::handleEvent(QWidget *widget, QWidget *managedWidget, QEvent *event)
{
    Q_UNUSED(widget);
    Q_UNUSED(managedWidget);
    Q_UNUSED(event);

    return false;
}

QWidget *BuddyEditorTool::editor() const
{
    if (!m_editor) {
        Q_ASSERT(formWindow() != 0);
        m_editor = new BuddyEditor(formWindow(), 0);
        connect(formWindow(), SIGNAL(mainContainerChanged(QWidget*)), m_editor, SLOT(setBackground(QWidget*)));
    }

    return m_editor;
}

void BuddyEditorTool::activated()
{
    m_editor->updateBackground();
}

void BuddyEditorTool::deactivated()
{
}

QAction *BuddyEditorTool::action() const
{
    return m_action;
}

