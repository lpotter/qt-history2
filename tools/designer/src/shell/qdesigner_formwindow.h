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

#ifndef QDESIGNER_FORMWINDOW_H
#define QDESIGNER_FORMWINDOW_H

#include <QMainWindow>
#include <QPointer>

class QDesignerWorkbench;
class AbstractFormWindow;

class QDesignerFormWindow: public QMainWindow
{
    Q_OBJECT
public:
    QDesignerFormWindow(QDesignerWorkbench *workbench,
                        QWidget *parent = 0, Qt::WFlags flags = 0);

    QDesignerFormWindow(AbstractFormWindow *formWindow, QDesignerWorkbench *workbench,
                        QWidget *parent = 0, Qt::WFlags flags = 0);

    virtual ~QDesignerFormWindow();

    QAction *action() const;
    QDesignerWorkbench *workbench() const;
    AbstractFormWindow *editor() const;

    virtual QRect geometryHint() const;

signals:
    void activated(bool active);

private slots:
    void updateWindowTitle(const QString &fileName);

protected:
    virtual void changeEvent(QEvent *e);

private:
    AbstractFormWindow *m_editor;
    QPointer<QDesignerWorkbench> m_workbench;
    QAction *m_action;
};

#endif // QDESIGNER_FORMWINDOW_H
