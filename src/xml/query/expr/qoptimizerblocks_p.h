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

#ifndef Patternist_OptimizerBlocks_H
#define Patternist_OptimizerBlocks_H

#include "qatomiccomparator_p.h"
#include "qexpression_p.h"
#include "qoptimizerframework_p.h"

QT_BEGIN_HEADER 

QT_BEGIN_NAMESPACE

namespace Patternist
{
    /**
     * @short Identifies Expression instances by their Expression::id().
     *
     * @author Frans englich <fenglich@trolltech.com>
     * @ingroup Patternist_expressions
     */
    class ByIDIdentifier : public ExpressionIdentifier
    {
    public:
        ByIDIdentifier(const Expression::ID id);
        virtual bool matches(const Expression::Ptr &expr) const;
    private:
        const Expression::ID m_id;
    };

    /**
     * @short Identifies Expression instances based on their static type.
     *
     * BySequenceTypeIdentifier identifies Expression instances
     * if their Expression::staticType() matches the requested one,
     * regardless of whether the Expression is a particular one, such
     * as AndExpression.
     *
     * For example, constructing a BySequenceTypeIdentifier while
     * passing CommonSequenceTypes::EBV in its constructor will create
     * a ExpressionIdentifier that returns @c true for a static type with
     * item type <tt>xs:string</tt>, but returns @c false for a static type involving
     * <tt>xs:date</tt>.
     *
     * @author Frans englich <fenglich@trolltech.com>
     * @ingroup Patternist_expressions
     */
    class BySequenceTypeIdentifier : public ExpressionIdentifier
    {
    public:
        BySequenceTypeIdentifier(const SequenceType::Ptr &seqType);

        /**
         * @returns @c true, if the static type of @p expr is matches
         * the SequenceType passed in the BySequenceTypeIdentifier()
         * constructor, otherwise @c false.
         */
        virtual bool matches(const Expression::Ptr &expr) const;

    private:
        const SequenceType::Ptr m_seqType;
    };

    /**
     * @short Determines whether an Expression is a value or general comparison or both,
     * with a certain operator.
     *
     * @author Frans englich <fenglich@trolltech.com>
     * @ingroup Patternist_expressions
     */
    class ComparisonIdentifier : public ExpressionIdentifier
    {
    public:

        /**
         * @param comparatorHosts the possible parent that may have
         * the operator. This may be Expression::IDGeneralComparison or
         * Expression::IDValueComparison. The two values may also be OR'd,
         * meaning any of them will do.
         *
         * @param op the operator that the comparator host must have. For example,
         * if @p op is AtomicComparator::OperatorGreatorOrEqual this ComparisonIdentifier
         * will match operator >= in the case of IDGeneralComparison and 'ge' in the
         * case of IDValueComparison.
         */
        ComparisonIdentifier(const Expression::IDs comparatorHosts,
                             const AtomicComparator::Operator op);

        /**
         * @returns @c true, if @p expr is a ValueComparison with the operator
         * passed in ComparisonIdentifier().
         */
        virtual bool matches(const Expression::Ptr &expr) const;

    private:
        const Expression::IDs m_hosts;
        const AtomicComparator::Operator m_op;
    };

    /**
     * @short Matches numeric literals that are of type xs:integer and
     * has a specific value.
     *
     * For example IntegerIdentifier(4) would match the former but
     * not the latter operand in this expression: "4 + 5".
     *
     * @author Frans englich <fenglich@trolltech.com>
     * @ingroup Patternist_expressions
     */
    class IntegerIdentifier : public ExpressionIdentifier
    {
    public:
        IntegerIdentifier(const xsInteger num);
        virtual bool matches(const Expression::Ptr &expr) const;

    private:
        const xsInteger m_num;
    };

    /**
     * @short Matches boolean literals.
     *
     * For example BooleanIdentifier(true) would match the former but
     * not the latter operand in this expression: "true() + false()".
     *
     * @author Frans englich <fenglich@trolltech.com>
     * @ingroup Patternist_expressions
     */
    class BooleanIdentifier : public ExpressionIdentifier
    {
    public:
        BooleanIdentifier(const bool value);
        virtual bool matches(const Expression::Ptr &expr) const;

    private:
        const bool m_value;
    };

    /**
     * @short Creates a particular Expression instance identified by an Expression::ID.
     *
     * For example, if ByIDCreator() is passed Expression::IDCountFN, create()
     * will return CountFN instances.
     *
     * @author Frans englich <fenglich@trolltech.com>
     * @ingroup Patternist_expressions
     */
    class ByIDCreator : public ExpressionCreator
    {
    public:
        /**
         * Creates a ByIDCreator that creates expressions
         * of the type that @p id identifies.
         */
        ByIDCreator(const Expression::ID id);
        virtual Expression::Ptr create(const Expression::List &operands,
                                       const StaticContext::Ptr &context,
                                       const SourceLocationReflection *const r) const;

        /**
         * Creates an expression by id @p id with the arguments @p operands.
         */
        static Expression::Ptr create(const Expression::ID id,
                                      const Expression::List &operands,
                                      const StaticContext::Ptr &context,
                                      const SourceLocationReflection *const r);

    private:
        const Expression::ID m_id;
    };
}

QT_END_NAMESPACE

QT_END_HEADER 

#endif
// vim: et:ts=4:sw=4:sts=4
