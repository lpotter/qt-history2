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

#include "hexdelegate.h"
#include <qabstractitemmodel.h>
#include <qpainter.h>

HexDelegate::HexDelegate(QObject *parent) 
    : QItemDelegate(parent)
{
    sz = QSize(2, 2);
    textHex = "0x";
    textHex.reserve(8); // fine tuning
}

HexDelegate::~HexDelegate()
{
}

QSize HexDelegate::sizeHint(const QStyleOptionViewItem &option,
                            const QAbstractItemModel *,
                            const QModelIndex &) const
{
    static QString textSize("0xFFFFFFFF");
    return QFontMetrics(option.font).size(0, textSize) + sz;
}

void HexDelegate::paint(QPainter *painter,
                        const QStyleOptionViewItem &option,
                        const QAbstractItemModel *model,
                        const QModelIndex &index) const
{
    static QRect emptyRect;
    static QPoint pt;

    textHex.resize(2);
    textHex += QString::number(
                model->data(index, QAbstractItemModel::DisplayRole).toInt(), 16).toUpper();

    // Layout text
    QRect textRect(pt, painter->fontMetrics().size(0, textHex) + sz);
    doLayout(option, &emptyRect, &textRect, false);

    // draw the item
    drawDisplay(painter, option, textRect, textHex);
}
