/****************************************************************************
**
** Declaration of QPrintDialogWin class.
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QPRINTDIALOGWIN_H
#define QPRINTDIALOGWIN_H

#ifdef Q_WS_WIN

#include "qabstractprintdialog.h"

class QPrintDialogWinPrivate;

class QPrintDialogWin : public QAbstractPrintDialog
{
    Q_DECLARE_PRIVATE(QPrintDialogWin)
public:
    QPrintDialogWin(QPrinter *printer, QWidget *parent = 0);

    int exec();
private:
#if defined(Q_DISABLE_COPY)
    QPrintDialogWin(const QPrintDialogWin &);
    QPrintDialogWin &operator=(const QPrintDialogWin &);
#endif
};

#endif // Q_WS_WIN

#endif QPRINTDIALOGWIN_H
