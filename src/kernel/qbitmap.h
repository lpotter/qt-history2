/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qbitmap.h#37 $
**
** Definition of QBitmap class
**
** Created : 941020
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

#ifndef QBITMAP_H
#define QBITMAP_H

#ifndef QT_H
#include "qpixmap.h"
#endif // QT_H


class Q_EXPORT QBitmap : public QPixmap
{
public:
    QBitmap();
    QBitmap( int w, int h,  bool clear = FALSE,
	     QPixmap::Optimization = QPixmap::DefaultOptim );
    QBitmap( const QSize &, bool clear = FALSE,
	     QPixmap::Optimization = QPixmap::DefaultOptim );
    QBitmap( int w, int h,  const uchar *bits, bool isXbitmap=FALSE );
    QBitmap( const QSize &, const uchar *bits, bool isXbitmap=FALSE );
    QBitmap( const QBitmap & );
#ifndef QT_NO_IMAGEIO
    QBitmap( const QString &fileName, const char *format=0 );
#endif
    QBitmap &operator=( const QBitmap & );
    QBitmap &operator=( const QPixmap & );
    QBitmap &operator=( const QImage  & );

#ifndef QT_NO_TRANSFORMATIONS
    QBitmap  xForm( const QWMatrix & ) const;
#endif
};


#endif // QBITMAP_H
