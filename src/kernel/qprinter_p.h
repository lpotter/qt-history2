/**********************************************************************
** $Id: $
**
** Definition of QPrinter class
**
** Created : 940927
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef QPRINTER_P_H
#define QPRINTER_P_H
#ifndef QT_NO_PRINTER

#include <qshared.h>
#include <qstring.h>
#include <qsize.h>

class QPrinterPrivate
{
public:
    uint pageRangeEnabled;
    QPrinter::PageRange pageRange;
    QPrinterPageSize pageSize;
};


class QPrinterPageSizePrivate : public QShared
{
public:
    QString name;
    QSize dimension;
};

const char *qprinter_name_for_pagesize( QPrinter::PageSize ps );
QPrinter::PageSize qprinter_pagesize_for_name( const QString &name );

#endif
#endif
