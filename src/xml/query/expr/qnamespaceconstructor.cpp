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

#include "qcommonsequencetypes_p.h"

#include "qnamespaceconstructor_p.h"

QT_BEGIN_NAMESPACE

using namespace Patternist;

NamespaceConstructor::NamespaceConstructor(const NamespaceBinding nb) : m_binding(nb)
{
    Q_ASSERT(!m_binding.isNull());
}

void NamespaceConstructor::evaluateToSequenceReceiver(const DynamicContext::Ptr &context) const
{
    context->outputReceiver()->namespaceBinding(m_binding);
}

SequenceType::Ptr NamespaceConstructor::staticType() const
{
    return CommonSequenceTypes::ExactlyOneAttribute;
}

SequenceType::List NamespaceConstructor::expectedOperandTypes() const
{
    SequenceType::List result;
    result.append(CommonSequenceTypes::ExactlyOneString);
    return result;
}

Expression::Properties NamespaceConstructor::properties() const
{
    return DisableElimination | IsNodeConstructor;
}

ExpressionVisitorResult::Ptr NamespaceConstructor::accept(const ExpressionVisitor::Ptr &visitor) const
{
    return visitor->visit(this);
}

// vim: et:ts=4:sw=4:sts=4

QT_END_NAMESPACE
