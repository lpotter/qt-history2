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

#include "groupbox_taskmenu.h"
#include "inplace_editor.h"

#include <QtDesigner/abstractformeditor.h>
#include <QtDesigner/abstractformwindow.h>
#include <QtDesigner/abstractformwindowcursor.h>
#include <QtDesigner/abstractformwindowmanager.h>

#include <QtGui/QAction>
#include <QtGui/QStyle>
#include <QtGui/QStyleOption>

#include <QtCore/QEvent>
#include <QtCore/QVariant>
#include <QtCore/qdebug.h>

using namespace qdesigner_internal;

GroupBoxTaskMenu::GroupBoxTaskMenu(QGroupBox *groupbox, QObject *parent)
    : QDesignerTaskMenu(groupbox, parent),
      m_groupbox(groupbox)
{
    m_editTitleAction = new QAction(tr("Edit title"), this);
    connect(m_editTitleAction, SIGNAL(triggered()), this, SLOT(editTitle()));
}

GroupBoxTaskMenu::~GroupBoxTaskMenu()
{
}

QList<QAction*> GroupBoxTaskMenu::taskActions() const
{
#if 0 // ### implement me
    action = new QAction(that);
    action->setText(tr("Edit groupbox icon"));
    connect(action, SIGNAL(triggered()), this, SLOT(editIcon()));
    m_taskActions.append(action);
#endif

    QList<QAction*> action_list = QDesignerTaskMenu::taskActions();
    action_list.insert(1, m_editTitleAction);

    return action_list;
}

void GroupBoxTaskMenu::editTitle()
{
    QDesignerFormWindowInterface *fw = formWindow();

    if (fw != 0) {
        connect(fw, SIGNAL(selectionChanged()), this, SLOT(updateSelection()));
        Q_ASSERT(m_groupbox->parentWidget() != 0);

        m_editor = new InPlaceEditor(m_groupbox, fw);
        m_editor->setFrame(false);
        m_editor->setText(m_groupbox->title());
        m_editor->selectAll();
        m_editor->setBackgroundRole(m_groupbox->backgroundRole());
        m_editor->setObjectName(QLatin1String("__qt__passive_m_editor"));
        connect(m_editor, SIGNAL(returnPressed()), m_editor, SLOT(deleteLater()));
        connect(m_editor, SIGNAL(textChanged(QString)), this, SLOT(updateText(QString)));
        m_editor->installEventFilter(this); // ### we need this??
        QStyleOption opt; // ## QStyleOptionGroupBox
        opt.init(m_groupbox);
        QRect r = QRect(QPoint(), m_groupbox->size());
        // ### m_groupbox->style()->subRect(QStyle::SR_GroupBoxTitle, &opt, m_groupbox);
        r.setHeight(20);

        m_editor->setGeometry(QRect(m_groupbox->mapTo(m_groupbox->window(), r.topLeft()), r.size()));

        m_editor->setFocus();
        m_editor->show();
    }
}

void GroupBoxTaskMenu::editIcon()
{
    qDebug() << "edit icon";
}

GroupBoxTaskMenuFactory::GroupBoxTaskMenuFactory(QExtensionManager *extensionManager)
    : QExtensionFactory(extensionManager)
{
}

QObject *GroupBoxTaskMenuFactory::createExtension(QObject *object, const QString &iid, QObject *parent) const
{
    if (QGroupBox *groupbox = qobject_cast<QGroupBox*>(object)) {
        if (iid == Q_TYPEID(QDesignerTaskMenuExtension)) {
            return new GroupBoxTaskMenu(groupbox, parent);
        }
    }

    return 0;
}

void GroupBoxTaskMenu::updateText(const QString &text)
{
    formWindow()->cursor()->setWidgetProperty(m_groupbox,
                                QLatin1String("title"), QVariant(text));
}

void GroupBoxTaskMenu::updateSelection()
{
    if (m_editor)
        m_editor->deleteLater();
}

QAction *GroupBoxTaskMenu::preferredEditAction() const
{
    return m_editTitleAction;
}

