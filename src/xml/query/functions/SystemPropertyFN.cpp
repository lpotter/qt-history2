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

#include "ExternalEnvironment.h"
#include "AtomicString.h"
#include "QNameConstructor.h"

#include "SystemPropertyFN.h"

using namespace Patternist;

Item SystemPropertyFN::evaluateSingleton(const DynamicContext::Ptr &context) const
{
    const QString lexQName(m_operands.first()->evaluateSingleton(context).stringValue());

    const QName name
        (QNameConstructor::expandQName<DynamicContext::Ptr,
                                       ReportContext::XTDE1390,
                                       ReportContext::XTDE1390>(lexQName,
                                                                context,
                                                                m_resolver, this));

    return AtomicString::fromValue(ExternalEnvironment::retrieveProperty(name));
}

Expression::Ptr SystemPropertyFN::typeCheck(const StaticContext::Ptr &context,
                                            const SequenceType::Ptr &reqType)
{
    m_resolver = NamespaceResolver::Ptr(context->namespaceBindings());
    Q_ASSERT(m_resolver);

    return FunctionCall::typeCheck(context, reqType);
}

// vim: et:ts=4:sw=4:sts=4