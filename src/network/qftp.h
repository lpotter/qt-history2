/****************************************************************************
** $Id: //depot/qt/main/src/network/qftp.h#6 $
**
** Definition of QFtp class.
**
** Created : 970521
**
** Copyright (C) 1997-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the network module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition licenses may use this
** file in accordance with the Qt Commercial License Agreement provided
** with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef QFTP_H
#define QFTP_H

#ifndef QT_H
#include "qstring.h" // char*->QString conversion
#include "qurlinfo.h"
#include "qnetworkprotocol.h"
#endif // QT_H

#ifndef QT_NO_NETWORKPROTOCOL_FTP


class QSocket;


class Q_EXPORT QFtp : public QNetworkProtocol
{
    Q_OBJECT

public:
    QFtp();
    virtual ~QFtp();
    int supportedOperations() const;

protected:
    void parseDir( const QString &buffer, QUrlInfo &info );
    void operationListChildren( QNetworkOperation *op );
    void operationMkDir( QNetworkOperation *op );
    void operationRemove( QNetworkOperation *op );
    void operationRename( QNetworkOperation *op );
    void operationGet( QNetworkOperation *op );
    void operationPut( QNetworkOperation *op );

    QSocket *commandSocket, *dataSocket;
    bool connectionReady, passiveMode;
    int getTotalSize, getDoneSize;
    bool startGetOnFail;
    int putToWrite, putWritten, putOffset;
    bool errorInListChildren;

private:
    bool checkConnection( QNetworkOperation *op );
    void close();
    void reinitCommandSocket();
    void okButTryLater( int code, const QCString &data );
    void okGoOn( int code, const QCString &data );
    void okButNeedMoreInfo( int code, const QCString &data );
    void errorForNow( int code, const QCString &data );
    void errorForgetIt( int code, const QCString &data );

protected slots:
    void hostFound();
    void connected();
    void closed();
    void readyRead();
    void dataHostFound();
    void dataConnected();
    void dataClosed();
    void dataReadyRead();
    void dataBytesWritten( int nbytes );
    void error( int );

};

#endif // QT_NO_NETWORKPROTOCOL_FTP

#endif // QFTP_H
