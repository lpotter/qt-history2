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

#ifndef QSQLQUERYMODEL_H
#define QSQLQUERYMODEL_H

#include <qabstractitemmodel.h>
#include <qsqldatabase.h>

class QSqlQueryModelPrivate;
class QSqlError;
class QSqlRecord;
class QSqlQuery;

class Q_SQL_EXPORT QSqlQueryModel: public QAbstractTableModel
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSqlQueryModel)

public:
    QSqlQueryModel(QObject *parent = 0);
    virtual ~QSqlQueryModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QSqlRecord record(int row = -1) const;

    QVariant data(const QModelIndex &item, int role = QAbstractItemModel::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = QAbstractItemModel::DisplayRole) const;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role);

    bool insertColumns(int column, int count, const QModelIndex &parent);
    bool removeColumns(int column, int count, const QModelIndex &parent);

    virtual void setQuery(const QSqlQuery &query);
    void setQuery(const QString &query, const QSqlDatabase &db = QSqlDatabase());
    QSqlQuery query() const;
    virtual void clear();

    QSqlError lastError() const;

    void fetchMore(const QModelIndex &parent = QModelIndex());

protected:
    QModelIndex dataIndex(const QModelIndex &item) const;
    void setLastError(const QSqlError &error);
    QSqlQueryModel(QSqlQueryModelPrivate &d, QObject *parent);
};

#endif
