/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qurl.h#24 $
**
** Implementation of QUrl class
**
** Created : 950429
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
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
** See http://www.troll.no/pricing.html or email sales@troll.no for
** information about the Professional Edition licensing, or see
** http://www.troll.no/qpl/ for QPL licensing information.
**
*****************************************************************************/

#ifndef QURL_H
#define QURL_H

#ifndef QT_H
#include "qstring.h"
#include "qdir.h"
#endif // QT_H

struct QUrlPrivate;

class Q_EXPORT QUrl
{
public:
    QUrl();
    QUrl( const QString& url );
    QUrl( const QUrl& url );
    QUrl( const QUrl& url, const QString& relUrl, bool checkSlash = FALSE );
    virtual ~QUrl();

    QString protocol() const;
    virtual void setProtocol( const QString& protocol );

    QString user() const;
    virtual void setUser( const QString& user );
    bool hasUser() const;

    QString password() const;
    virtual void setPassword( const QString& pass );
    bool hasPassword() const;

    QString host() const;
    virtual void setHost( const QString& user );
    bool hasHost() const;

    int port() const;
    virtual void setPort( int port );

    QString path( bool correct = TRUE ) const;
    virtual void setPath( const QString& path );
    bool hasPath() const;

    virtual void setEncodedPathAndQuery( const QString& enc );
    QString encodedPathAndQuery();

    virtual void setQuery( const QString& txt );
    QString query() const;

    QString ref() const;
    virtual void setRef( const QString& txt );
    bool hasRef() const;

    bool isValid() const;
    bool isLocalFile() const;

    virtual void addPath( const QString& path );
    virtual void setFileName( const QString& txt );

    QString fileName() const;
    QString dirPath() const;

    QUrl& operator=( const QUrl& url );
    QUrl& operator=( const QString& url );

    bool operator==( const QUrl& url ) const;
    bool operator==( const QString& url ) const;

    static void decode( QString& url );
    static void encode( QString& url );

    operator QString() const;
    virtual QString toString( bool encodedPath = FALSE, bool forcePrependProtocol = FALSE ) const;

    virtual bool cdUp();

    static bool isRelativeUrl( const QString &url );

protected:
    virtual void reset();
    virtual bool parse( const QString& url );

private:
    QUrlPrivate *d;

};

#endif
