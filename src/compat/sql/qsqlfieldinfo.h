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

#ifndef QSQLFIELDINFO_H
#define QSQLFIELDINFO_H

#ifndef QT_NO_SQL

#include "qglobal.h"
#ifdef QT_COMPAT

#include "qsqlfield.h"

/* QSqlFieldInfo Class
   obsoleted, use QSqlField instead
*/

#if !defined(QT_MODULE_SQL) || defined(QT_LICENSE_PROFESSIONAL)
#define QM_COMPAT_EXPORT_SQL
#else
#define QM_COMPAT_EXPORT_SQL Q_COMPAT_EXPORT
#endif


class QM_COMPAT_EXPORT_SQL QSqlFieldInfo
{
    // class is obsoleted, won't change anyways,
    // so no d pointer
    int req, len, prec, tID;
    uint gen: 1;
    uint trim: 1;
    uint calc: 1;
    QString nm;
    QCoreVariant::Type typ;
    QCoreVariant defValue;

public:
    QSqlFieldInfo(const QString& name = QString(),
                   QCoreVariant::Type typ = QCoreVariant::Invalid,
                   int required = -1,
                   int len = -1,
                   int prec = -1,
                   const QCoreVariant& defValue = QCoreVariant(),
                   int sqlType = 0,
                   bool generated = true,
                   bool trim = false,
                   bool calculated = false) :
        req(required), len(len), prec(prec), tID(sqlType),
        gen(generated), trim(trim), calc(calculated),
        nm(name), typ(typ), defValue(defValue) {}

    virtual ~QSqlFieldInfo() {}

    QSqlFieldInfo(const QSqlField & other, bool generated = true)
    {
        nm = other.name();
        typ = other.type();
        req = other.isRequired();
        len = other.length();
        prec = other.precision();
        defValue = other.defaultValue();
        tID = other.typeID();
        calc = other.isAutoGenerated() > 0;
        gen = generated;
    }

    bool operator==(const QSqlFieldInfo& f) const
    {
        return (nm == f.nm &&
                typ == f.typ &&
                req == f.req &&
                len == f.len &&
                prec == f.prec &&
                defValue == f.defValue &&
                tID == f.tID &&
                gen == f.gen &&
                trim == f.trim &&
                calc == f.calc);
    }

    QSqlField toField() const
    { QSqlField f(nm, typ);
      f.setRequired(QSqlField::State(req));
      f.setLength(len);
      f.setPrecision(prec);
      f.setDefaultValue(defValue);
      f.setSqlType(tID);
      f.setAutoGenerated(QSqlField::State(calc));
      return f;
    }
    int isRequired() const
    { return req; }
    QCoreVariant::Type type() const
    { return typ; }
    int        length() const
    { return len; }
    int        precision() const
    { return prec; }
    QCoreVariant defaultValue() const
    { return defValue; }
    QString name() const
    { return nm; }
    int        typeID() const
    { return tID; }
    bool isGenerated() const
    { return gen; }
    bool isTrim() const
    { return trim; }
    bool isCalculated() const
    { return calc; }

    virtual void setTrim(bool trim)
    { this->trim = trim; }
    virtual void setGenerated(bool generated)
    { gen = generated; }
    virtual void setCalculated(bool calculated)
    { calc = calculated; }

};

#endif        // QT_NO_SQL
#endif  // QT_COMPAT
#endif
