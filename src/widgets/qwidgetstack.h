/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qwidgetstack.h#1 $
**
** Definition of something or other
**
** Created : 979899
**
** Copyright (C) 1997 by Troll Tech AS.  All rights reserved.
**
****************************************************************************/

#ifndef QWIDGETSTACK_H
#define QWIDGETSTACK_H

#include "qframe.h"
#include "qintdict.h"


class QWidgetStackPrivate;

class QGridLayout;


class QWidgetStack: public QFrame
{
    Q_OBJECT
public:
    QWidgetStack( QWidget * parent = 0, const char * name = 0 );
    ~QWidgetStack();

    void addWidget( QWidget *, int );

    void removeWidget( QWidget * );

    void show();
    
    QWidget * widget( int ) const;
    int id( QWidget * ) const;

public slots:
    void raiseWidget( int );
    void raiseWidget( QWidget * );

protected:
    void frameChanged();

    void setChildGeometries();

private:
    bool isMyChild( QWidget * );
    QWidgetStackPrivate * d;
    QIntDict<QWidget> * dict;
    QGridLayout * l;
};


#endif
