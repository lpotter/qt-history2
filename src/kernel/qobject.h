/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qobject.h#59 $
**
** Definition of QObject class
**
** Created : 930418
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

#ifndef QOBJECT_H
#define QOBJECT_H

#ifndef QT_H
#include "qobjectdefs.h"
#include "qstring.h"
#include "qevent.h"
#include "qnamespace.h"
#endif // QT_H


class Q_EXPORT QObject: public Qt
{
public:
    QObject( QObject *parent=0, const char *name=0 );
    virtual ~QObject();

    static QString tr(const char*);

    virtual bool event( QEvent * );
    virtual bool eventFilter( QObject *, QEvent * );

    virtual QMetaObject *metaObject() const { return metaObj; }
    virtual const char	*className()  const;

    bool	isA( const char * )	 const;
    bool	inherits( const char * ) const;

    const char *name( const char * defaultName = "unnamed" ) const;

    virtual void	setName( const char *name );
    bool	isWidgetType()	  const { return isWidget; }
    bool	highPriority()	  const { return FALSE; }

    bool	signalsBlocked()  const { return blockSig; }
    void	blockSignals( bool b );

    int		startTimer( int interval );
    void	killTimer( int id );
    void	killTimers();

    const QObjectList *children() const { return childObjects; }
    QObjectList	      *queryList( const char *inheritsClass = 0,
				  const char *objName = 0,
				  bool regexpMatch = TRUE,
				  bool recursiveSearch = TRUE );

    virtual void insertChild( QObject * );
    virtual void removeChild( QObject * );

    void	installEventFilter( const QObject * );
    void	removeEventFilter( const QObject * );

    static bool connect( const QObject *sender, const char *signal,
			 const QObject *receiver, const char *member );
    bool	connect( const QObject *sender, const char *signal,
			 const char *member ) const;
    static bool disconnect( const QObject *sender, const char *signal,
			    const QObject *receiver, const char *member );
    bool	disconnect( const char *signal=0,
			    const QObject *receiver=0, const char *member=0 );
    bool	disconnect( const QObject *receiver, const char *member=0 );

    void	dumpObjectTree();
    void	dumpObjectInfo();

signals:
    void	destroyed();

public:
    QObject	*parent() const { return parentObj; }

private slots:
    void	cleanupEventFilter();

protected:
    bool	activate_filters( QEvent * );
    QConnectionList *receivers( const char *signal ) const;
    void	activate_signal( const char *signal );
    void	activate_signal( const char *signal, short );
    void	activate_signal( const char *signal, int );
    void	activate_signal( const char *signal, long );
    void	activate_signal( const char *signal, const char * );
    const QObject *sender();

    virtual void initMetaObject();
    static void staticMetaObject();

    virtual void timerEvent( QTimerEvent * );
    virtual void childEvent( QChildEvent * );

    virtual void connectNotify( const char *signal );
    virtual void disconnectNotify( const char *signal );
    virtual bool checkConnectArgs( const char *signal, const QObject *receiver,
				   const char *member );

    static  void badSuperclassWarning( const char *className,
				       const char *superclassName );

    uint	isSignal   : 1;
    uint	isWidget   : 1;
    uint	pendTimer  : 1;
    uint	pendEvent  : 1;
    uint	blockSig   : 1;

private:
    uint	wasDeleted : 1;

    QMetaObject *queryMetaObject() const;
    static QMetaObject *metaObj;
    char	*objname;
    QObject	*parentObj;
    QObjectList *childObjects;
    QSignalDict *connections;
    QObjectList *senderObjects;
    QObjectList *eventFilters;
    QObject	*sigSender;

    friend class QApplication;
    friend class QWidget;
    friend class QSignal;
    friend class QSenderObject;

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QObject( const QObject & );
    QObject &operator=( const QObject & );
#endif
};


inline bool QObject::connect( const QObject *sender, const char *signal,
			      const char *member ) const
{
    return connect( sender, signal, this, member );
}

inline bool QObject::disconnect( const char *signal,
				 const QObject *receiver, const char *member )
{
    return disconnect( this, signal, receiver, member );
}

inline bool QObject::disconnect( const QObject *receiver, const char *member )
{
    return disconnect( this, 0, receiver, member );
}

inline const QObject *QObject::sender()
{
    return sigSender;
}


class Q_EXPORT QSenderObject : public QObject		// object for sending signals
{
public:
    void setSender( QObject *s ) { sigSender=s; }
};


#endif // QOBJECT_H
