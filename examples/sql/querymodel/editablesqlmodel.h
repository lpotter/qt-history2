#ifndef EDITABLESQLMODEL_H
#define EDITABLESQLMODEL_H

#include <QSqlQueryModel>

class EditableSqlModel: public QSqlQueryModel
{
public:
    EditableSqlModel(QObject *parent = 0);

    ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, int role, const QVariant &value);

private:
    bool setLastName(int personId, const QString &lastName);
};

#endif
