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

#ifndef Patternist_EmptyContainer_H
#define Patternist_EmptyContainer_H

#include "qexpression_p.h"

QT_BEGIN_HEADER 

QT_BEGIN_NAMESPACE

namespace Patternist
{
    /**
     * @short Base class for expressions that has no operands.
     *
     * @author Frans Englich <fenglich@trolltech.com>
     * @ingroup Patternist_expressions
     */
    class Q_AUTOTEST_EXPORT EmptyContainer : public Expression
    {
    public:
        /**
         * @returns always an empty list.
         */
        virtual Expression::List operands() const;

        /**
         * Does nothing, since sub-classes has no operands. Calling
         * it makes hence no sense, and it also results in an assert crash.
         */
        virtual void setOperands(const Expression::List &);

    protected:
        /**
         * @returns always @c true
         */
        virtual bool compressOperands(const StaticContext::Ptr &context);

        /**
         * @returns always an empty list since it has no operands.
         */
        virtual SequenceType::List expectedOperandTypes() const;

    };
}

QT_END_NAMESPACE

QT_END_HEADER 

#endif
// vim: et:ts=4:sw=4:sts=4
