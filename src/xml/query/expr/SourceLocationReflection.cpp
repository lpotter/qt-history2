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

#include "qsourcelocation.h"

#include "SourceLocationReflection.h"

using namespace Patternist;

QSourceLocation SourceLocationReflection::sourceLocation() const
{
    return QSourceLocation();
}

const SourceLocationReflection *DelegatingSourceLocationReflection::actualReflection() const
{
    return m_r->actualReflection();
}

QString DelegatingSourceLocationReflection::description() const
{
    return m_r->description();
}