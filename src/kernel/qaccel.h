/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qaccel.h#32 $
**
** Definition of QAccel class
**
** Created : 950419
**
** Copyright (C) 1992-1998 Troll Tech AS.  All rights reserved.
**
** This file is part of Troll Tech's internal development tree for Qt.
**
** This header text will be replaced by an appropriate text by the
** mkdist script which generates external distributions.
**
** If you are using the Qt Professional Edition or the Qt Free Edition,
** please notify Troll Tech at <info@troll.no> if you see this text.
**
** To Troll Tech developers: This header was generated by the script
** fixcopyright-int. It has the same number of text lines as the free
** and professional editions to avoid line number inconsistency.
**
*****************************************************************************/

#ifndef QACCEL_H
#define QACCEL_H

#ifndef QT_H
#include "qobject.h"
#include "qkeycode.h"
#endif // QT_H


class QAccelPrivate;				// internal class


class QAccel : public QObject			// accelerator class
{
    Q_OBJECT
public:
    QAccel( QWidget *parent, const char *name=0 );
   ~QAccel();

    bool	isEnabled() const;
    void	setEnabled( bool );

    uint	count() const;

    int		insertItem( int key, int id=-1 );
    void	removeItem( int id );
    void	clear();

    int		key( int id );
    int		findKey( int key ) const;

    bool	isItemEnabled( int id )	 const;
    void	setItemEnabled( int id, bool enable );

    bool	connectItem( int id,
			     const QObject *receiver, const char* member );
    bool	disconnectItem( int id,
				const QObject *receiver, const char* member );

    void	repairEventFilter();

signals:
    void	activated( int id );

protected:
    bool	eventFilter( QObject *, QEvent * );

private slots:
    void	tlwDestroyed();

private:
    QAccelPrivate * d;

private:	// Disabled copy constructor and operator=
    QAccel( const QAccel & );
    QAccel &operator=( const QAccel & );
};


#endif // QACCEL_H
