/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qfontdta.h#7 $
**
** Definition of QFontData struct
**
** Author  : Eirik Eng
** Created : 941229
**
** Copyright (C) 1994,1995 by Troll Tech AS.  All rights reserved.
**
** --------------------------------------------------------------------------
** Internal header file containing private data common to QFont,
** QFontInfo and QFontMetrics.
**
** Uses definitions found in qfont.h, must always be included after qfont.h.
*****************************************************************************/

#ifndef QFONTDTA_H
#define QFONTDTA_H


struct QFontDef  {
	QString	  family;
	short	  pointSize;
	uint	  styleHint	: 8;
	uint	  charSet	: 8;
	uint	  weight	: 8;
	uint	  italic	: 1;
	uint	  underline	: 1;
	uint	  strikeOut	: 1;
	uint	  fixedPitch	: 1;
	uint	  hintSetByUser : 1;
	uint	  rawMode	: 1;
	uint	  dirty		: 1;
};

#if defined(_WS_X11_)
    struct QXFontData;
#endif

struct QFontData : QShared {
    QFontDef        req;                // requested
    QFontDef        act;                // actual
    uint	    exactMatch    : 1;
    short           lineW;              // width of underline and strikeOut

#if defined(_WS_WIN_)
	HANDLE	hfont;
#elif defined(_WS_PM_)
#elif defined(_WS_X11_)
	QXFontData *xfd;
#endif
};


#endif // QFONTDTA_H
