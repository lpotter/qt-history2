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

#ifndef QSQLRECORDINFO_H
#define QSQLRECORDINFO_H

#include "qglobal.h"

#ifndef QT_NO_SQL

#include "qvaluelist.h"
#include "qsqlrecord.h"
#include "qsqlfieldinfo.h"

/* QSqlRecordInfo Class
   This class is obsolete, use QSqlRecord instead.
*/

typedef QValueList<QSqlFieldInfo> QSqlFieldInfoList;

class Q_COMPAT_EXPORT QSqlRecordInfo: public QSqlFieldInfoList
{
public:
    QSqlRecordInfo(): QSqlFieldInfoList() {}
    QSqlRecordInfo(const QSqlFieldInfoList& other): QSqlFieldInfoList(other) {}
    QSqlRecordInfo(const QSqlRecord& other)
    {
        for (int i = 0; i < other.count(); ++i)
            push_back(QSqlFieldInfo(other.field(i), other.isGenerated(i)));
    }

    size_type contains(const QString& fieldName) const;
    QSqlFieldInfo find(const QString& fieldName) const;
    QSqlRecord toRecord() const;
};

inline QSqlRecordInfo::size_type QSqlRecordInfo::contains(const QString& fieldName) const
{
    size_type i = 0;
    QString fName = fieldName.toUpper();

    for(const_iterator it = begin(); it != end(); ++it) {
        if ((*it).name().toUpper() == fName) {
            ++i;
        }
    }
    return i;
}

inline QSqlFieldInfo QSqlRecordInfo::find(const QString& fieldName) const
{
    QString fName = fieldName.toUpper();
    for(const_iterator it = begin(); it != end(); ++it) {
        if ((*it).name().toUpper() == fName) {
            return *it;
        }
    }
    return QSqlFieldInfo();
}

inline QSqlRecord QSqlRecordInfo::toRecord() const
{
    QSqlRecord buf;
    for(const_iterator it = begin(); it != end(); ++it) {
        buf.append((*it).toField());
    }
    return buf;
}

#endif        // QT_NO_SQL
#endif
