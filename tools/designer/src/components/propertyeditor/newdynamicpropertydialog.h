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

#ifndef NEWDYNAMICPROPERTYDIALOG_P_H
#define NEWDYNAMICPROPERTYDIALOG_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "propertyeditor_global.h"
#include <QtGui/QDialog>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE

class QAbstractButton;
class QDesignerDialogGuiInterface;

namespace qdesigner_internal {

namespace Ui
{
    class NewDynamicPropertyDialog;
}

class QT_PROPERTYEDITOR_EXPORT NewDynamicPropertyDialog: public QDialog
{
    Q_OBJECT
public:
    NewDynamicPropertyDialog(QDesignerDialogGuiInterface *dialogGui, QWidget *parent = 0);
    ~NewDynamicPropertyDialog();

    void setReservedNames(const QStringList &names);

    QString propertyName() const;
    QVariant propertyValue() const;

private slots:

    void on_m_buttonBox_clicked(QAbstractButton *btn);

private:
    void information(const QString &message);

    QDesignerDialogGuiInterface *m_dialogGui;
    Ui::NewDynamicPropertyDialog *m_ui;
    QStringList m_reservedNames;
};

}  // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // NEWDYNAMICPROPERTYDIALOG_P_H
