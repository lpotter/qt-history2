/****************************************************************************
**
** Copyright (C) 2004-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of an example program for Qt.
** EDITIONS: NOLIMITS
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef TABLEMODEL_H
#define TABLEMODEL_H

#include <QAbstractTableModel>
#include <qlist.h>
#include <QObject>
#include <QStringList>
#include <QVariant>

class TableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    TableModel(int rows = 1, int columns = 1, QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = DisplayRole) const;

    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = EditRole);

    bool insertRows(int position, int rows, const QModelIndex &parent = QModelIndex());
    bool insertColumns(int position, int columns, const QModelIndex &parent = QModelIndex());
    bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex());
    bool removeColumns(int position, int columns, const QModelIndex &parent = QModelIndex());

private:
    QList<QStringList> rowList;
    int tableColumns;
};

#endif
