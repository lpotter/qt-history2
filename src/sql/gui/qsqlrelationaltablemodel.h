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

#ifndef QSQLRELATIONALTABLEMODEL_H
#define QSQLRELATIONALTABLEMODEL_H

#include <qsqltablemodel.h>

class Q_SQL_EXPORT QSqlRelation
{
public:
    QSqlRelation() {}
    QSqlRelation(const QString &tableName, const QString &indexColumn,
               const QString &displayColumn)
        : tName(tableName), iColumn(indexColumn), dColumn(displayColumn) {}
    inline QString tableName() const
    { return tName; }
    inline QString indexColumn() const
    { return iColumn; }
    inline QString displayColumn() const
    { return dColumn; }
    inline bool isValid() const
    { return !(tName.isEmpty() || iColumn.isEmpty() || dColumn.isEmpty()); }
private:
    QString tName, iColumn, dColumn;
};

class QSqlRelationalTableModelPrivate;

class Q_SQL_EXPORT QSqlRelationalTableModel: public QSqlTableModel
{
    Q_DECLARE_PRIVATE(QSqlRelationalTableModel)

public:
    QSqlRelationalTableModel(QObject *parent = 0, QSqlDatabase db = QSqlDatabase());
    virtual ~QSqlRelationalTableModel();

    QVariant data(const QModelIndex &item, int role = QAbstractItemModel::DisplayRole) const;
    bool setData(const QModelIndex &item, int role, const QVariant &value);
#ifdef Q_NO_USING_KEYWORD
    inline bool setData(const QModelIndex &index, const QVariant &value)
    { return QAbstractItemModel::setData(index, value); }
#else
    using QAbstractItemModel::setData;
#endif

    void clear();
    bool select();

    virtual void setRelation(int column, const QSqlRelation &relation);
    QSqlRelation relation(int column) const;
    virtual QSqlTableModel *relationModel(int column) const;

public slots:
    void revertRow(int row);

protected:
    QString selectStatement() const;
};

#endif
