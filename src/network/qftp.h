/****************************************************************************
**
** Definition of QFtp class.
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of the network module of the Qt GUI Toolkit.
** EDITIONS: FREE, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QFTP_H
#define QFTP_H

#ifndef QT_H
#include "qstring.h" // char*->QString conversion
#include "qurlinfo.h"
#include "qnetworkprotocol.h"
#endif // QT_H

#if !defined( QT_MODULE_NETWORK ) || defined( QT_LICENSE_PROFESSIONAL ) || defined( QT_INTERNAL_NETWORK )
#define QM_EXPORT_FTP
#else
#define QM_EXPORT_FTP Q_NETWORK_EXPORT
#endif

#ifndef QT_NO_NETWORKPROTOCOL_FTP

class QFtpPrivate;

class QM_EXPORT_FTP QFtp : public QObject
{
    Q_OBJECT

public:
    QFtp( QObject *parent = 0, const char *name = 0 );
    virtual ~QFtp();

    enum State {
	Unconnected,
	HostLookup,
	Connecting,
	Connected,
	LoggedIn,
	Closing
    };
    enum Error {
	NoError,
	UnknownError,
	HostNotFound,
	ConnectionRefused,
	NotConnected
    };
    enum Command {
	None,
	ConnectToHost,
	Login,
	Close,
	List,
	Cd,
	Get,
	Put,
	Remove,
	Mkdir,
	Rmdir,
	Rename,
	RawCommand
    };

    int connectToHost( const QString &host, Q_UINT16 port=21 );
    int login( const QString &user=QString::null, const QString &password=QString::null );
    int close();
    int list( const QString &dir=QString::null );
    int cd( const QString &dir );
    int get( const QString &file, QIODevice *dev=0 );
    int put( const QByteArray &data, const QString &file );
    int put( QIODevice *dev, const QString &file );
    int remove( const QString &file );
    int mkdir( const QString &dir );
    int rmdir( const QString &dir );
    int rename( const QString &oldname, const QString &newname );

    int rawCommand( const QString &command );

    Q_ULONG bytesAvailable() const;
    Q_LONG readBlock( char *data, Q_ULONG maxlen );
    QByteArray readAll();

    int currentId() const;
    QIODevice* currentDevice() const;
    Command currentCommand() const;
    bool hasPendingCommands() const;
    void clearPendingCommands();

    State state() const;

    Error error() const;
    QString errorString() const;

public slots:
    void abort();

signals:
    void stateChanged( int );
    void listInfo( const QUrlInfo& );
    void readyRead();
    void dataTransferProgress( int, int );
    void rawCommandReply( int, const QString& );

    void commandStarted( int );
    void commandFinished( int, bool );
    void done( bool );

private slots:
    void startNextCommand();
    void piFinished( const QString& );
    void piError( int, const QString& );
    void piConnectState( int );
    void piFtpReply( int, const QString& );

private:
    Q_DECL_PRIVATE(QFtp);
};

#endif // QT_NO_NETWORKPROTOCOL_FTP

#endif // QFTP_H
