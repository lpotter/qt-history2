/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qworkspace.h#1 $
**
** Definition of the QWorkspace class
**
** Created : 990210
**
** Copyright (C) 1999 Troll Tech AS.  All rights reserved.
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

#ifndef QWORKSPACE_H
#define QWORKSPACE_H

#include <qframe.h>
#include <qlist.h>

#include "qworkspacechild.h"

class QWorkspace : public QWidget
{
    Q_OBJECT
public:
    QWorkspace( QWidget *parent=0, const char *name=0 );
    ~QWorkspace();
    
    bool autoManage();
    void setAutoManage( bool );
    
    void activateClient( QWidget* w);
    QWidget* activeClient() const;

    
protected:
    void childEvent( QChildEvent * );
    
private:
    bool manage;
    QWorkspaceChild* active;
    QList<QWorkspaceChild> windows;
    void place( QWorkspaceChild* );

    int px;
    int py;
    
};
#endif
