/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qnetworkprotocol.cpp#12 $
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

#include "qnetworkprotocol.h"
#include "qlocalfs.h"
#include "qtimer.h"

QNetworkProtocolDict *qNetworkProtocolRegister = 0;

/*!
  \class QNetworkProtocol qnetworkprotocol.h

  This is a baseclass which should be used for implementations
  of network protocols which can then be used in Qt (e.g.
  in the filedialog).
*/

/*!
  \fn void QNetworkProtocol::finished( int action )

  This signal is emitted when a data transfer of some sort finished.
  \a action gives more information about it, this can be one of
	ActListDirectory
	ActCopyFile
	ActMoveFiles
	ActGet
*/

/*!
  \fn void QNetworkProtocol::start( int action )

  This signal is emitted when a data transfer of some sort started.
  \a action gives more information about it, this can be one of
	ActListDirectory
	ActCopyFile
	ActMoveFiles
	ActGet
*/

/*!
  \fn void QNetworkProtocol::error( int ecode, const QString &msg )

  This signal is emitted whenever an error occures. \a ecode
  is the error code, and \a msg an error message which can be
  e.g. displayed to the user.

  \a ecode is one of
	ErrDeleteFile
	ErrRenameFile
	ErrCopyFile
	ErrReadDir
	ErrCreateDir
	ErrUnknownProtocol
	ErrParseError
*/

/*!
  \fn void QNetworkProtocol::data( const QCString &data )

  This signal is emitted when new \a data has been received.
*/


/*!
  \fn void QNetworkProtocol::connectionStateChanged( int state, const QString &data )

  #### todo
*/

/*!
  \fn void QNetworkProtocol::entry( const QUrlInfo &i )

  This signal is emitted after listEntries() was called and
  a new entry (file) has been read from the list of files. \a i
  holds the information about the new etry.
*/

/*!
  \fn void QNetworkProtocol::createdDirectory( const QUrlInfo &i )

  This signal is emitted when mkdir() has been succesful
  and the directory has been created. \a i holds the information
  about the new directory.
*/

/*!
  \fn void QNetworkProtocol::removed( const QString &name )

  This signal is emitted when remove() has been succesful
  and the file has been removed. \a name is the filename
  of the removed file.
*/

/*!
  \fn void QNetworkProtocol::itemChanged( const QString &oldname, const QString &newname )

  This signal is emitted whenever a file, which is a child of this URL,
  has been changed e.g. by successfully calling rename(). \a oldname is
  the original name of the file and \a newname is the name which the file
  go now.
*/

/*!
  \fn void QNetworkProtocol::copyProgress( const QString &from, const QString &to, int step, int total )

  When copying a file this signal is emitted. \a from is the file which
  is copied, \a to the destination. \a step is the progress
  (always <= \a total) or -1, if copying just started. \a total is the
  number of steps needed to copy the file.

  This signal can be used to show the progress when copying files.
*/

/*!
  #### todo
*/

QNetworkProtocol::QNetworkProtocol()
    : url( 0 ), opInProgress( 0 )
{
    connect( this, SIGNAL( data( const QCString &, QNetworkOperation * ) ),
	     this, SLOT( emitData( const QCString &, QNetworkOperation * ) ) );
    connect( this, SIGNAL( finished( QNetworkOperation * ) ),
	     this, SLOT( emitFinished( QNetworkOperation * ) ) );
    connect( this, SIGNAL( start( QNetworkOperation * ) ),
	     this, SLOT( emitStart( QNetworkOperation * ) ) );
    connect( this, SIGNAL( newChild( const QUrlInfo &, QNetworkOperation * ) ),
	     this, SLOT( emitNewChild( const QUrlInfo &, QNetworkOperation * ) ) );
    connect( this, SIGNAL( createdDirectory( const QUrlInfo &, QNetworkOperation * ) ),
	     this, SLOT( emitCreatedDirectory( const QUrlInfo &, QNetworkOperation * ) ) );
    connect( this, SIGNAL( removed( QNetworkOperation * ) ),
	     this, SLOT( emitRemoved( QNetworkOperation * ) ) );
    connect( this, SIGNAL( itemChanged( QNetworkOperation * ) ),
	     this, SLOT( emitItemChanged( QNetworkOperation * ) ) );
    connect( this, SIGNAL( copyProgress( int, int, QNetworkOperation * ) ),
	     this, SLOT( emitCopyProgress( int, int, QNetworkOperation * ) ) );

    connect( this, SIGNAL( finished( QNetworkOperation * ) ),
	     this, SLOT( processNextOperation( QNetworkOperation * ) ) );
    opStartTimer = new QTimer( this );
    connect( opStartTimer, SIGNAL( timeout() ),
	     this, SLOT( startOps() ) );
    
}

/*!
  #### todo
*/

QNetworkProtocol::~QNetworkProtocol()
{
    url = 0;
    if ( opInProgress )
	delete opInProgress;
    operationQueue.setAutoDelete( TRUE );
}

/*!
  #### todo
*/

void QNetworkProtocol::setUrl( QUrlOperator *u )
{
    url = u;
    if ( !opInProgress && !operationQueue.isEmpty() )
	opStartTimer->start( 0, TRUE );
}

/*!
  #### todo
*/

QNetworkOperation *QNetworkProtocol::get( const QCString &d )
{
    QNetworkOperation *res = new QNetworkOperation( QNetworkProtocol::OpGet,
						    QString::fromLatin1( d ),
						    QString::null, QString::null );
    addOperation( res );
    return res;
}

/*!
  #### todo
*/

QNetworkOperation *QNetworkProtocol::listChildren()
{
    QNetworkOperation *res = new QNetworkOperation( QNetworkProtocol::OpListChildren,
						    QString::null,
						    QString::null, QString::null );
    addOperation( res );
    return res;
}

/*!
  #### todo
*/

QNetworkOperation *QNetworkProtocol::mkdir( const QString &d )
{
    QNetworkOperation *res = new QNetworkOperation( QNetworkProtocol::OpMkdir,
						    d,
						    QString::null, QString::null );
    addOperation( res );
    return res;
}

/*!
  #### todo
*/

QNetworkOperation *QNetworkProtocol::remove( const QString &d )
{
    QNetworkOperation *res = new QNetworkOperation( QNetworkProtocol::OpRemove,
						    d,
						    QString::null, QString::null );
    addOperation( res );
    return res;
}

/*!
  #### todo
*/

QNetworkOperation *QNetworkProtocol::rename( const QString &on, const QString &nn )
{
    QNetworkOperation *res = new QNetworkOperation( QNetworkProtocol::OpRename,
						    on, nn, QString::null );
    addOperation( res );
    return res;
}

/*!
  #### todo
*/

QNetworkOperation *QNetworkProtocol::copy( const QString &from, const QString &to, bool move )
{
    QNetworkOperation *res = new QNetworkOperation( QNetworkProtocol::OpCopy,
						    from, to, QString::null );
    addOperation( res );
    if ( move ) {
	QNetworkOperation *m = new QNetworkOperation( QNetworkProtocol::OpRemove, from,
						      QString::null, QString::null );
	addOperation( m );
    }
    return res;
}

/*!
 */

bool QNetworkProtocol::checkConnection( QNetworkOperation * )
{
    return TRUE;
}

/*!
  #### todo
*/

int QNetworkProtocol::supportedOperations() const
{
    return 0;
}

/*!
 */

void QNetworkProtocol::addOperation( QNetworkOperation *op )
{
    operationQueue.enqueue( op );
    if ( !opInProgress )
	opStartTimer->start( 0, TRUE );
}

/*!
  #### todo
*/

void QNetworkProtocol::registerNetworkProtocol( const QString &protocol,
						QNetworkProtocolFactoryBase *protocolFactory )
{
    if ( !qNetworkProtocolRegister ) {
	qNetworkProtocolRegister = new QNetworkProtocolDict;
	QNetworkProtocol::registerNetworkProtocol( "file", new QNetworkProtocolFactory< QLocalFs > );
    }

    qNetworkProtocolRegister->insert( protocol, protocolFactory );
}

/*!
  #### todo
*/

QNetworkProtocol *QNetworkProtocol::getNetworkProtocol( const QString &protocol )
{
    if ( !qNetworkProtocolRegister ) {
	qNetworkProtocolRegister = new QNetworkProtocolDict;
	QNetworkProtocol::registerNetworkProtocol( "file", new QNetworkProtocolFactory< QLocalFs > );
    }
    
    if ( protocol.isNull() )
	return 0;

    QNetworkProtocolFactoryBase *factory = qNetworkProtocolRegister->find( protocol );
    if ( factory )
	return factory->createObject();

    return 0;
}

/*!
 */

bool QNetworkProtocol::hasOnlyLocalFileSystem()
{
    if ( !qNetworkProtocolRegister )
	return FALSE;
    
    QDictIterator< QNetworkProtocolFactoryBase > it( *qNetworkProtocolRegister );
    for ( ; it.current(); ++it )
	if ( it.currentKey() != "file" )
	    return FALSE;
    return TRUE;
}

/*!
 */

void QNetworkProtocol::startOps()
{
    processNextOperation( 0 );
}

/*!
 */

void QNetworkProtocol::processOperation( QNetworkOperation *op )
{
    if ( !op )
	return;
    
    switch ( op->operation() ) {
    case OpListChildren:
	operationListChildren( op );
	break;
    case OpMkdir:
	operationMkDir( op );
	break;
    case OpRemove:
	operationRemove( op );
	break;
    case OpRename:
	operationRename( op );
	break;
    case OpCopy: case OpMove:
	operationCopy( op );
	break;
    case OpGet:
	operationGet( op );
	break;
    }
}

/*!
 */

void QNetworkProtocol::operationListChildren( QNetworkOperation * )
{
}

/*!
 */

void QNetworkProtocol::operationMkDir( QNetworkOperation * )
{
}

/*!
 */

void QNetworkProtocol::operationRemove( QNetworkOperation * )
{
}

/*!
 */

void QNetworkProtocol::operationRename( QNetworkOperation * )
{
}

/*!
 */

void QNetworkProtocol::operationCopy( QNetworkOperation * )
{
}

/*!
 */

void QNetworkProtocol::operationGet( QNetworkOperation * )
{
}

/*!
 */

void QNetworkProtocol::processNextOperation( QNetworkOperation *old )
{
    if ( old )
	delete old;
    
    if ( operationQueue.isEmpty() ) {
	opInProgress = 0;
	return;
    }

    QNetworkOperation *op = operationQueue.head();
    opInProgress = 0;
 
    if ( !checkConnection( op ) ) {
	if ( op->state() != QNetworkProtocol::StFailed )
	    opStartTimer->start( 0, TRUE );
	else {
	    emit finished( op );
	    operationQueue.clear();
	}
	    
	return;
    }
    
    opInProgress = op;
    operationQueue.dequeue();
    processOperation( op );
}


/*!
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */


struct QNetworkOperationPrivate
{
    QNetworkProtocol::Operation operation;
    QNetworkProtocol::State state;
    QString arg1, arg2, arg3;
    QString protocolDetail;
    QNetworkProtocol::Error errorCode;
};

QNetworkOperation::QNetworkOperation( QNetworkProtocol::Operation operation,
				      const QString &arg1, const QString &arg2,
				      const QString &arg3 )
{
    d = new QNetworkOperationPrivate;
    d->operation = operation;
    d->state = QNetworkProtocol::StWaiting;
    d->arg1 = arg1;
    d->arg2 = arg2;
    d->arg3 = arg3;
    d->protocolDetail = QString::null;
    d->errorCode = QNetworkProtocol::NoError;
}

QNetworkOperation::~QNetworkOperation()
{
    delete d;
}

void QNetworkOperation::setState( QNetworkProtocol::State state )
{
    d->state = state;
}

void QNetworkOperation::setProtocolDetail( const QString &detail )
{
    d->protocolDetail = detail;
}

void QNetworkOperation::setErrorCode( QNetworkProtocol::Error ec )
{
    d->errorCode = ec;
}

QNetworkProtocol::Operation QNetworkOperation::operation() const
{
    return d->operation;
}

QNetworkProtocol::State QNetworkOperation::state() const
{
    return d->state;
}

QString QNetworkOperation::arg1() const
{
    return d->arg1;
}

QString QNetworkOperation::arg2() const
{
    return d->arg2;
}

QString QNetworkOperation::arg3() const
{
    return d->arg3;
}

QString QNetworkOperation::protocolDetail() const
{
    return d->protocolDetail;
}

QNetworkProtocol::Error QNetworkOperation::errorCode() const
{
    return d->errorCode;
}
