/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qfontmetrics.h#24 $
**
** Definition of QFontMetrics class
**
** Created : 940514
**
** Copyright (C) 1994-1996 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#ifndef QFONTMET_H
#define QFONTMET_H

#include "qfont.h"
#include "qrect.h"


class QFontMetrics
{
public:
    QFontMetrics( const QFontMetrics & );
   ~QFontMetrics();

    QFontMetrics &operator=( const QFontMetrics & );

    int		ascent()	const;
    int		descent()	const;
    int		height()	const;
    int		leading()	const;
    int		lineSpacing()	const;

    int		width( const char *, int len = -1 ) const;
    int		width( char )	const;
    QRect	boundingRect( const char *, int len = -1 ) const;
    QRect	boundingRect( char ) const;
    int		maxWidth()	const;

    int		underlinePos()	const;
    int		strikeOutPos()	const;
    int		lineWidth()	const;

    const QFont &font()		const;

private:
    QFontMetrics( const QWidget * );
    QFontMetrics( const QPainter * );
    static void reset( const QWidget * );
    static void reset( const QPainter * );

    QWidget    *w;
    QPainter   *p;

    friend class QWidget;
    friend class QPainter;
};


#endif // QFONTMET_H
