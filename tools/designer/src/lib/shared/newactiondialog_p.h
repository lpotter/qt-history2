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

#ifndef NEWACTIONDIALOG_P_H
#define NEWACTIONDIALOG_P_H

#include "ui_newactiondialog.h"

namespace qdesigner_internal {

class ActionEditor;

class NewActionDialog: public QDialog
{
    Q_OBJECT
public:
    NewActionDialog(ActionEditor *parent);
    virtual ~NewActionDialog();

    QString actionText() const;
    QString actionName() const;
    QIcon actionIcon() const;

    void setActionData(const QString &text, const QIcon &icon);

private slots:
    void accept();
    void on_editActionText_textChanged(const QString &text);
    void on_iconButton_clicked();
    void on_removeIconButton_clicked();

private:
    ActionEditor *m_actionEditor;
    Ui::NewActionDialog ui;
};

} // namespace qdesigner_internal

#endif // NEWACTIONDIALOG_P_H
