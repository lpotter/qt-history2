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

#include "BuiltinTypes.h"
#include "Debug.h"

#include "AtomicString.h"

using namespace Patternist;

AtomicString::AtomicString(const QString &s) : m_value(s)
{
}

AtomicString::Ptr AtomicString::fromValue(const QString &value)
{
    return AtomicString::Ptr(new AtomicString(value));
}

bool AtomicString::evaluateEBV(const PlainSharedPtr<DynamicContext> &) const
{
    return m_value.length() > 0;
}

QString AtomicString::stringValue() const
{
    return m_value;
}

ItemType::Ptr AtomicString::type() const
{
    return BuiltinTypes::xsString;
}

// vim: et:ts=4:sw=4:sts=4