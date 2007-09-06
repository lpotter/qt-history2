/****************************************************************************
**
** Copyright (C) 2006-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the Patternist project on Trolltech Labs.
**
** $TROLLTECH_GPL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
***************************************************************************
*/

#include "AtomicType.h"
#include "BuiltinTypes.h"
#include "CommonValues.h"
#include "DynamicContext.h"
#include "PatternistLocale.h"
#include "ValidationError.h"

#include "Boolean.h"

using namespace Patternist;

bool Boolean::evaluateEBV(const Item::Iterator::Ptr &it,
                          const PlainSharedPtr<DynamicContext> &context)
{
    return evaluateEBV(it->next(), it, context);
}

bool Boolean::evaluateEBV(const Item &first,
                          const Item::Iterator::Ptr &it,
                          const PlainSharedPtr<DynamicContext> &context)
{
    Q_ASSERT(it);
    Q_ASSERT(context);

    if(!first)
        return false;
    else if(first.isNode())
        return true;

    const Item second(it->next());

    if(second)
    {
        Q_ASSERT(context);
        context->error(tr("Effective Boolean Value cannot be calculated for a sequence "
                                     "containing two or more atomic values."),
                          ReportContext::FORG0006,
                          QSourceLocation());
        return false;
    }
    else
        return first.as<AtomicValue>()->evaluateEBV(context);
}

bool Boolean::evaluateEBV(const Item &item,
                          const PlainSharedPtr<DynamicContext> &context)
{
    if(!item)
        return false;
    else if(item.isNode())
        return true;
    else
        return item.as<AtomicValue>()->evaluateEBV(context);
}

Boolean::Boolean(const bool value) : m_value(value)
{
}

QString Boolean::stringValue() const
{
    return m_value
            ? CommonValues::TrueString->stringValue()
            : CommonValues::FalseString->stringValue();
}

bool Boolean::evaluateEBV(const PlainSharedPtr<DynamicContext> &) const
{
    return m_value;
}

Boolean::Ptr Boolean::fromValue(const bool value)
{
    return value ? CommonValues::BooleanTrue : CommonValues::BooleanFalse;
}

AtomicValue::Ptr Boolean::fromLexical(const QString &lexical)
{
    const QString val(lexical.trimmed()); /* Apply the whitespace facet. */

    if(val == QLatin1String("true") || val == QChar(QLatin1Char('1')))
        return CommonValues::BooleanTrue;
    else if(val == QLatin1String("false") || val == QChar(QLatin1Char('0')))
        return CommonValues::BooleanFalse;
    else
        return ValidationError::createError();
}

ItemType::Ptr Boolean::type() const
{
    return BuiltinTypes::xsBoolean;
}

// vim: et:ts=4:sw=4:sts=4