/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qsignalslotimp.h#1 $
**
** Definition of signal/slot collections etc.
**
** Created : 980821
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

#ifndef QSIGNALSLOTIMP_H
#define QSIGNALSLOTIMP_H

#ifndef QT_H
#include "qconnection.h"
#include "qlist.h"
#include "qdict.h"
#endif // QT_H


#if defined(QT_DLL)

template class Q_EXPORT QList<QConnection>;
template class Q_EXPORT QListIt<QConnection>;

class Q_EXPORT QConnectionList : public QList<QConnection>
{
public:
    QConnectionList() : QList<QConnection>() {}
    QConnectionList( const QConnectionList &list ) : QList<QConnection>(list) {}
   ~QConnectionList() { clear(); }
    QConnectionList &operator=(const QConnectionList &list)
	{ return (QConnectionList&)QList<QConnection>::operator=(list); }
};

class Q_EXPORT QConnectionListIt : public QListIterator<QConnection>
{
public:
    QConnectionListIt( const QConnectionList &list ) : QListIterator<QConnection>(list) {}
    QConnectionListIt &operator=(const QConnectionListIt &list)
	{ return (QConnectionListIt&)QListIterator<QConnection>::operator=(list); }
};


template class Q_EXPORT QDict<QConnectionList>;
template class Q_EXPORT QDictIt<QConnectionList>;

class Q_EXPORT QSignalDict : public QDict<QConnectionList>
{
public:
    QSignalDict(int size=17,bool cs=TRUE,bool ck=TRUE) :
	QDict<QConnectionList>(size,cs,ck) {}
    QSignalDict( const QSignalDict &dict ) : QDict<QConnectionList>(dict) {}
   ~QSignalDict() { clear(); }
    QSignalDict &operator=(const QSignalDict &dict)
	{ return (QSignalDict&)QDict<QConnectionList>::operator=(dict); }
};

class Q_EXPORT QSignalDictIt : public QListIterator<QConnectionList>
{
public:
    QSignalDictIt( const QSignalDict &list ) : QListIterator<QConnectionList>(list) {}
    QSignalDictIt &operator=(const QSignalDictIt &list)
	{ return (QSignalDictIt&)QListIterator<QConnectionList>::operator=(list); }
};

#endif


#endif // QSIGNALSLOTIMP_H
