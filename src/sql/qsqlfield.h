/****************************************************************************
**
** Definition of QSqlField class.
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the sql module of the Qt GUI Toolkit.
** EDITIONS: FREE, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QSQLFIELD_H
#define QSQLFIELD_H

#ifndef QT_H
#include "qcorevariant.h"
#include "qstring.h"
#endif // QT_H

#if !defined(QT_MODULE_SQL) || defined(QT_LICENSE_PROFESSIONAL)
#define QM_EXPORT_SQL
#else
#define QM_EXPORT_SQL Q_SQL_EXPORT
#endif

#ifndef QT_NO_SQL

class QSqlFieldPrivate;

class QM_EXPORT_SQL QSqlField
{
public:
    QSqlField(const QString& fieldName = QString(),
              QCoreVariant::Type type = QCoreVariant::Invalid,
              int required = -1,
              int fieldLength = -1,
              int prec = -1,
              const QCoreVariant& defValue = QCoreVariant(),
              int sqlType = 0,
              int autogen = -1);
    QSqlField(const QSqlField& other);
    QSqlField& operator=(const QSqlField& other);
    bool operator==(const QSqlField& other) const;
    ~QSqlField();

    void setValue(const QCoreVariant& value);
    inline QCoreVariant value() const
    { return val; }
    void setName(const QString& name);
    QString name() const;
    bool isNull() const;
    void setReadOnly(bool readOnly);
    bool isReadOnly() const;
    void clear();
    QCoreVariant::Type type() const;
    void setType(QCoreVariant::Type type);

    int isRequired() const;
    int length() const;
    int precision() const;
    QCoreVariant defaultValue() const;
    int typeID() const;
    int isAutoGenerated() const;
    bool isValid() const;

#ifdef QT_COMPAT
    inline QT_COMPAT void setNull() { clear(); }
#endif

private:
    void detach();
    QCoreVariant val;
    QSqlFieldPrivate* d;
};

#ifndef QT_NO_DEBUG
QM_EXPORT_SQL QDebug operator<<(QDebug, const QSqlField &);
#endif

#endif        // QT_NO_SQL
#endif
