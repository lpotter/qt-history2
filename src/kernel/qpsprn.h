/**********************************************************************
** $Id: //depot/qt/main/src/kernel/qpsprn.h#7 $
**
**		      ***   INTERNAL HEADER FILE   ***
**
**		This file is NOT a part of the Qt interface!
**
** Definition of internal QPSPrinter class.
** QPSPrinter implements PostScript (tm) output via QPrinter.
**
** Created : 940927
**
** Copyright (C) 1994-1996 by Troll Tech AS.  All rights reserved.
**
***********************************************************************/

#ifndef QPSPRN_H
#define QPSPRN_H

#include "qprinter.h"
#include "qtstream.h"


class QPSPrinter : public QPaintDevice
{
private:
    QPSPrinter( QPrinter * );
   ~QPSPrinter();

    bool	cmd ( int, QPainter *, QPDevCmdParam * );

    QPrinter   *printer;
    QIODevice  *device;
    QTextStream stream;
    int		pageCount;
    bool	dirtyMatrix;
    QString	fontsUsed;
    friend class QPrinter;

private:	// Disabled copy constructor and operator=
    QPSPrinter( const QPSPrinter & ):QPaintDevice(0) {}
    QPSPrinter &operator=( const QPSPrinter & ) { return *this; }
};


// Additional commands for QPSPrinter

#define PDC_PRT_NEWPAGE 100
#define PDC_PRT_ABORT	101


#endif // QPSPRN_H
