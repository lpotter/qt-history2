/****************************************************************************
**
** Copyright (C) 2006-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "BuiltinTypes.h"
#include "Item.h"
#include "ValidationError.h"

#include "Integer.h"

using namespace Patternist;

Item Integer::fromValue(const xsInteger num)
{
    return toItem(Integer::Ptr(new Integer(num)));
}

AtomicValue::Ptr Integer::fromLexical(const QString &strNumeric)
{
    bool conversionOk = false;
    const xsInteger num = strNumeric.toLongLong(&conversionOk);

    if(conversionOk)
        return AtomicValue::Ptr(new Integer(num));
    else
        return ValidationError::createError();
}

Integer::Integer(const xsInteger num) : m_value(num)
{
}

bool Integer::evaluateEBV(const PlainSharedPtr<DynamicContext> &) const
{
    return m_value != 0;
}

QString Integer::stringValue() const
{
    return QString::number(m_value);
}

ItemType::Ptr Integer::type() const
{
    return BuiltinTypes::xsInteger;
}

xsDouble Integer::toDouble() const
{
    return static_cast<xsDouble>(m_value);
}

xsInteger Integer::toInteger() const
{
    return m_value;
}

xsFloat Integer::toFloat() const
{
    return static_cast<xsFloat>(m_value);
}

xsDecimal Integer::toDecimal() const
{
    return static_cast<xsDecimal>(m_value);
}

Numeric::Ptr Integer::round() const
{
    /* xs:integerS never has a mantissa. */
    return Numeric::Ptr(const_cast<Integer *>(this));
}

Numeric::Ptr Integer::roundHalfToEven(const xsInteger /*scale*/) const
{
    return Numeric::Ptr(const_cast<Integer *>(this));
}

Numeric::Ptr Integer::floor() const
{
    return Numeric::Ptr(const_cast<Integer *>(this));
}

Numeric::Ptr Integer::ceiling() const
{
    return Numeric::Ptr(const_cast<Integer *>(this));
}

Numeric::Ptr Integer::abs() const
{
    /* No reason to allocate an Integer if we're already absolute. */
    if(m_value < 0)
        return Numeric::Ptr(new Integer(qAbs(m_value)));
    else
        return Numeric::Ptr(const_cast<Integer *>(this));
}

bool Integer::isNaN() const
{
    return false;
}

bool Integer::isInf() const
{
    return false;
}

// vim: et:ts=4:sw=4:sts=4
