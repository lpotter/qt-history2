/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qsizepolicy.h#9 $
**
** Definition of QSizePolicyclass
**
** Created : 980929
**
** Copyright (C) 1998-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Troll Tech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** Licensees with valid Qt Professional Edition licenses may distribute and
** use this file in accordance with the Qt Professional Edition License
** provided at sale or upon request.
**
** See http://www.troll.no/pricing.html or email sales@troll.no for
** information about the Professional Edition licensing, or see
** http://www.troll.no/qpl/ for QPL licensing information.
**
*****************************************************************************/

#ifndef QSIZEPOLICY_H
#define QSIZEPOLICY_H

#include "qglobal.h"

class Q_EXPORT QSizePolicy
{
private:
    enum { HSize = 6, HMask = 0x3f, VMask = HMask << HSize,
	   MayGrow = 1, ExpMask = 2, MayShrink = 4 };
public:
    enum SizeType { Fixed = 0,
		    Minimum = MayGrow,
		    Maximum = MayShrink,
		    Preferred = MayGrow|MayShrink ,
		    MinimumExpanding = Minimum|ExpMask,
		    Expanding = MinimumExpanding|MayShrink };

    enum ExpandData { NoDirection = 0,
		      Horizontal = 1,
		      Vertical = 2,
		      BothDirections = Horizontal | Vertical };

    QSizePolicy() { data = 0; }

    QSizePolicy( SizeType hor, SizeType ver ): data( hor | (ver<<HSize) ) {}

    SizeType horData() const { return (SizeType)( data & HMask ); }
    SizeType verData() const { return (SizeType)(( data & VMask ) >> HSize); }

    bool mayShrinkHorizontally() const { return horData() & MayShrink; }
    bool mayShrinkVertically() const { return verData() & MayShrink; }
    bool mayGrowHorizontally() const { return horData() & MayGrow; }
    bool mayGrowVertically() const { return verData() & MayGrow; }

    ExpandData expanding() const
    {
	return (ExpandData)( (int)(verData()|ExpMask ? Vertical : 0)+
			     (int)(horData()|ExpMask ? Horizontal : 0) );
    }

    void setHorData( SizeType d ) { data = (data & ~HMask) | d; }
    void setVerData( SizeType d ) { data = (data & ~HMask) | d; }

    bool hasWidthForHeight() { return data & ( 1 << 2*HSize ); }

private:
    QSizePolicy( int i ): data( i ) {}

    Q_UINT16 data;
};

#endif
