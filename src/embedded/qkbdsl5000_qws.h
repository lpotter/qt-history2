/****************************************************************************
** $Id$
**
** Definition of Qt/Embedded Sharp SL5000 keyboard driver
**
** Created : 20020219
**
** Copyright (C) 1992-2002 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses for Qt/Embedded may use this file in accordance with the
** Qt Embedded Commercial License Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef QKBDSL5000_QWS_H
#define QKBDSL5000_QWS_H

#include "qkbdtty_qws.h"

#ifdef QT_QWS_SHARP

class QWSSL5000KbPrivate;

class QWSSL5000KeyboardHandler : public QWSTtyKeyboardHandler
{
public:
    QWSSL5000KeyboardHandler( const QString& );
    virtual ~QWSSL5000KeyboardHandler();

    virtual void doKey(uchar scancode);
    virtual const QWSKeyMap *keyMap() const;

private:
    bool meta;
    bool fn;
    bool numLock;
    QWSSL5000KbPrivate *d;
};

#endif // QT_QWS_SHARP

#endif // QKBDTTY_QWS_H

