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

#ifndef BUTTON_TASKMENU_H
#define BUTTON_TASKMENU_H

#include <QAbstractButton>
#include <QPointer>

#include <taskmenu.h>
#include <default_extensionfactory.h>

class QLineEdit;
class AbstractFormWindow;

class ButtonTaskMenu: public QObject, public ITaskMenu
{
    Q_OBJECT
    Q_INTERFACES(ITaskMenu)
public:
    ButtonTaskMenu(QAbstractButton *button, QObject *parent = 0);
    virtual ~ButtonTaskMenu();

    virtual QList<QAction*> taskActions() const;

    bool eventFilter(QObject *object, QEvent *event);

private slots:
    void editText();
    void editIcon();
    void updateText(const QString &text);
    void updateSelection();

private:
    QAbstractButton *m_button;
    QPointer<AbstractFormWindow> m_formWindow;
    QPointer<QLineEdit> m_editor;
    mutable QList<QAction*> m_taskActions;
};

class ButtonTaskMenuFactory: public DefaultExtensionFactory
{
    Q_OBJECT
public:
    ButtonTaskMenuFactory(QExtensionManager *extensionManager = 0);

protected:
    virtual QObject *createExtension(QObject *object, const QString &iid, QObject *parent) const;
};

#endif // BUTTON_TASKMENU_H
