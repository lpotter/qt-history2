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

#ifndef SHEET_DELEGATE_H
#define SHEET_DELEGATE_H

#include "shared_global.h"

#include <QItemDelegate>
#include <QTreeView>

class QTreeView;

class QT_SHARED_EXPORT SheetDelegate: public QItemDelegate
{
    Q_OBJECT
public:
    SheetDelegate(QTreeView *view, QWidget *parent);

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    QTreeView *m_view;
};

#endif // SHEET_DELEGATE_H
