/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qurl.h#6 $
**
** Implementation of QFileDialog class
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

#include <qstring.h>
#include <qdir.h>
#include <qobject.h>
#include <qmap.h>

struct QUrlPrivate;
class QUrlInfo;
class QNetworkProtocol;

class QUrl : public QObject
{
    Q_OBJECT

public:
    enum Error {
	ErrDeleteFile = -1,
	ErrRenameFile = -2,
	ErrCopyFile = -3,
	ErrReadDir = -4,
	ErrCreateDir = -5,
	ErrUnknownProtocol = -6,
	ErrParse = -7
    };

    enum Action {
	ActListDirectory = 0,
	ActCopyFile,
	ActGet,
	ActPut
    };
    
    QUrl();
    QUrl( const QString& url );
    QUrl( const QUrl& url );
    QUrl( const QUrl& url, const QString& relUrl_ );
    virtual ~QUrl();

    QString protocol() const;
    virtual void setProtocol( const QString& protocol );

    QString user() const;
    virtual void setUser( const QString& user );
    bool hasUser() const;

    QString pass() const;
    virtual void setPass( const QString& pass );
    bool hasPass() const;

    QString host() const;
    virtual void setHost( const QString& user );
    bool hasHost() const;

    int port() const;
    virtual void setPort( int port );

    QString path() const;
    QString path( int trailing ) const;
    virtual void setPath( const QString& path );
    bool hasPath() const;

    virtual void setEncodedPathAndQuery( const QString& enc );
    QString encodedPathAndQuery( int trailing = 0, bool noEmptyPath = FALSE );

    virtual void setQuery( const QString& txt );
    QString query() const;

    QString ref() const;
    virtual void setRef( const QString& txt );
    bool hasRef() const;

    bool isMalformed() const;

    bool isLocalFile() const;

    virtual void addPath( const QString& path );
    virtual void setFileName( const QString& txt );

    QString filename( bool ignoreTrailingSlashInPath = TRUE );
    QString directory( bool stripTrailingSlashFromResult = TRUE,
		       bool ignoreTrailingSlashInPath = TRUE );

    QString url();
    QString url( int trailing, bool stripRef = FALSE );

    QUrl& operator=( const QUrl& url );
    QUrl& operator=( const QString& url );

    bool operator==( const QUrl& url ) const;
    bool operator==( const QString& url ) const;
    virtual bool cmp( QUrl &url, bool ignoreTrailing = FALSE );

    static void decode( QString& url );
    static void encode( QString& url );

    virtual void listEntries( int filterSpec = QDir::DefaultFilter,
			      int sortSpec   = QDir::DefaultSort );
    virtual void listEntries( const QString &nameFilter, int filterSpec = QDir::DefaultFilter,
			      int sortSpec   = QDir::DefaultSort );
    virtual void mkdir( const QString &dirname );
    virtual void remove( const QString &filename );
    virtual void rename( const QString &oldname, const QString &newname );
    virtual void copy( const QString &from, const QString &to );
    virtual void copy( const QStringList &files, const QString &dest, bool move );
    virtual bool isDir();
    virtual bool isFile();
    virtual void get( const QString &info );
    virtual void put( const QString &data );


    virtual void setNameFilter( const QString &nameFilter );
    QString nameFilter() const;

    virtual QUrlInfo info( const QString &entry ) const;
    operator QString() const;
    virtual QString toString() const;

    virtual bool cdUp();

    void emitEntry( const QUrlInfo & );
    void emitFinished( int action );
    void emitStart( int action );
    void emitCreatedDirectory( const QUrlInfo & );
    void emitRemoved( const QString & );
    void emitItemChanged( const QString &oldname, const QString &newname );
    void emitError( int ecode, const QString &msg );
    void emitData( const QString &d );
    void emitUrlIsDir();
    void emitUrlIsFile();
    void emitPutSuccessful( const QString &d );
    void emitCopyProgress( int step, int total );

signals:
    void entry( const QUrlInfo & );
    void finished( int );
    void start( int );
    void createdDirectory( const QUrlInfo & );
    void removed( const QString & );
    void itemChanged( const QString &oldname, const QString &newname );
    void error( int ecode, const QString &msg );
    void data( const QString & );
    void putSuccessful( const QString & );
    void urlIsDir();
    void urlIsFile();
    void copyProgress( int step, int total );
    
protected:
    virtual void reset();
    virtual void parse( const QString& url );
    virtual void addEntry( const QUrlInfo &i );
    virtual void clearEntries();

    static char hex2int( char c );
    void getNetworkProtocol();

private:
    QUrlPrivate *d;

};

inline void QUrl::emitEntry( const QUrlInfo &i )
{
    addEntry( i );
    emit entry( i );
}

inline void QUrl::emitFinished( int action )
{
    emit finished( action );
}

inline void QUrl::emitStart( int action )
{
    emit start( action );
}

inline void QUrl::emitCreatedDirectory( const QUrlInfo &i )
{
    emit createdDirectory( i );
}

inline void QUrl::emitRemoved( const QString &s )
{
    emit removed( s );
}

inline void QUrl::emitItemChanged( const QString &oldname, const QString &newname )
{
    emit itemChanged( oldname, newname );
}

inline void QUrl::emitError( int ecode, const QString &msg )
{
    emit error( ecode, msg );
}

inline void QUrl::emitData( const QString &d )
{
    emit data( d );
}

inline void QUrl::emitUrlIsDir()
{
    emit urlIsDir();
}

inline void QUrl::emitUrlIsFile()
{
    emit urlIsFile();
}

inline void QUrl::emitPutSuccessful( const QString &d )
{
    emit putSuccessful( d );
}

inline void QUrl::emitCopyProgress( int step, int total )
{
    emit copyProgress( step, total );
}

#endif
