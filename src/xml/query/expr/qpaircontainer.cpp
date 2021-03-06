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

#include "qbuiltintypes_p.h"

#include "qpaircontainer_p.h"

QT_BEGIN_NAMESPACE

using namespace Patternist;

PairContainer::PairContainer(const Expression::Ptr &operand1,
                             const Expression::Ptr &operand2) : m_operand1(operand1),
                                                                m_operand2(operand2)
{
    Q_ASSERT(m_operand1);
    Q_ASSERT(m_operand2);
}

Expression::List PairContainer::operands() const
{
    Expression::List list;
    list.append(m_operand1);
    list.append(m_operand2);
    return list;
}

void PairContainer::setOperands(const Expression::List &ops)
{
    Q_ASSERT(ops.count() == 2);
    m_operand1 = ops.first();
    m_operand2 = ops.last();
    Q_ASSERT(m_operand1);
    Q_ASSERT(m_operand2);
}

bool PairContainer::compressOperands(const StaticContext::Ptr &context)
{
    Q_ASSERT(m_operand1);
    Q_ASSERT(m_operand2);
    rewrite(m_operand1, m_operand1->compress(context), context);
    rewrite(m_operand2, m_operand2->compress(context), context);

    return m_operand1->isEvaluated() && m_operand2->isEvaluated();
}

// vim: et:ts=4:sw=4:sts=4

QT_END_NAMESPACE
