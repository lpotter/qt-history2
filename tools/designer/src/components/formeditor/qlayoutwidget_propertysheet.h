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

#ifndef QLAYOUTWIDGET_PROPERTYSHEET_H
#define QLAYOUTWIDGET_PROPERTYSHEET_H

#include <qdesigner_propertysheet_p.h>
#include <extensionfactory_p.h>
#include <qlayout_widget_p.h>

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

class QLayoutWidgetPropertySheet: public QDesignerPropertySheet
{
    Q_OBJECT
    Q_INTERFACES(QDesignerPropertySheetExtension)
public:
    explicit QLayoutWidgetPropertySheet(QLayoutWidget *object, QObject *parent = 0);
    virtual ~QLayoutWidgetPropertySheet();

    virtual void setProperty(int index, const QVariant &value);
    virtual bool isVisible(int index) const;

    virtual bool dynamicPropertiesAllowed() const;
};

typedef QDesignerPropertySheetFactory<QLayoutWidget, QLayoutWidgetPropertySheet> QLayoutWidgetPropertySheetFactory;
}  // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // QLAYOUTWIDGET_PROPERTYSHEET_H
