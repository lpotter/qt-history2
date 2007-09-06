/****************************************************************************
 * **
 * ** Copyright (C) 2007-$THISYEAR$ $TROLLTECH$. All rights reserved.
 * **
 * ** This file is part of the Patternist project on Trolltech Labs.
 * **
 * ** $TROLLTECH_GPL_LICENSE$
 * **
 * ** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * ** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * **
 * ****************************************************************************/

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#ifndef Patternist_QXmlItemIteratorPrivate_p_h
#define Patternist_QXmlItemIteratorPrivate_p_h

#include "Exception.h"
#include "Expression.h"
#include "Item.h"
#include "Iterator.h"

QT_BEGIN_HEADER

class QXmlItemIteratorPrivate
{
public:
    inline QXmlItemIteratorPrivate(const Patternist::DynamicContext::Ptr aContext,
                                   const Patternist::Expression::Ptr aExpr) : context(aContext)
                                                                            , expr(aExpr)
    {
        /* We gets passed null pointers from QXmlItemIterator()'s
         * default constructor. */
        if(expr)
        {
            Q_ASSERT(context);
            try
            {
                it = expr->evaluateSequence(context);
                next = it->next();
            }
            catch(const Patternist::Exception &)
            {
                next = Patternist::Item();
            }
        }
    }

    friend class QXmlItemIterator;

private:
    Patternist::DynamicContext::Ptr context;
    Patternist::Expression::Ptr     expr;
    Patternist::Item::Iterator::Ptr it;
    Patternist::Item                current;
    Patternist::Item                next;
};

QT_END_HEADER

#endif
// vim: et:ts=4:sw=4:sts=4