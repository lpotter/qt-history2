/****************************************************************************
**
** Definition of the QPaintEnginePrivate class.
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QPAINTENGINE_P_H
#define QPAINTENGINE_P_H

// Used to get Q_DECLARE_PUBLIC
#include "private/qobject_p.h"

class QPaintEngine;

class QPaintEnginePrivate
{
    Q_DECLARE_PUBLIC(QPaintEngine);
public:
    QPaintEnginePrivate() :
	q_ptr(0), pdev(0)
    {
    }

protected:
    QPaintEngine *q_ptr;
    QPaintDevice *pdev;
};

#endif // QPAINTENGINE_P_H
