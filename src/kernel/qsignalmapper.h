/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qsignalmapper.h#7 $
**
** Definition of QSignalMapper class
**
** Created : 980503
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

#ifndef QSIGNALMAPPER_H
#define QSIGNALMAPPER_H

#ifndef QT_H
#include "qobject.h"
#endif // QT_H


class  QSignalMapperData;
struct QSignalMapperRec;


class QSignalMapper : public QObject {
    Q_OBJECT
public:
    QSignalMapper( QObject* parent, const char* name=0 );
    ~QSignalMapper();

    virtual void setMapping( const QObject* sender, int identifier );
    virtual void setMapping( const QObject* sender, const QString &identifier );
    void removeMappings( const QObject* sender );

signals:
    void mapped(int);
    void mapped(const QString &);

public slots:
    void map();

private:
    QSignalMapperData* d;
    QSignalMapperRec* getRec( const QObject* );

private slots:
    void removeMapping();
};


#endif // QSIGNALMAPPER_H
