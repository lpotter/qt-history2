/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qhbuttongroup.h#1 $
**
** Definition of QHButtonGroup class
**
** Created : 990602
**
** Copyright (C) 1999-2000 Troll Tech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Troll Tech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** Licensees holding valid Qt Professional Edition licenses may use this
** file in accordance with the Qt Professional Edition License Agreement
** provided with the Qt Professional Edition.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
** information about the Professional Edition licensing, or see
** http://www.trolltech.com/qpl/ for QPL licensing information.
**
*****************************************************************************/

#ifndef QHBUTTONGROUP_H
#define QHBUTTONGROUP_H

#ifndef QT_H
#include "qbuttongroup.h"
#endif // QT_H

#if QT_FEATURE_WIDGETS

class Q_EXPORT QHButtonGroup : public QButtonGroup
{
    Q_OBJECT
public:
    QHButtonGroup( QWidget *parent=0, const char *name=0 );
    QHButtonGroup( const QString &title, QWidget *parent=0, const char* name=0 );
   ~QHButtonGroup();

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QHButtonGroup( const QHButtonGroup & );
    QHButtonGroup &operator=( const QHButtonGroup & );
#endif
};


#endif // QT_FEATURE_WIDGETS

#endif // QHBUTTONGROUP_H
