/****************************************************************************
** $Id: //depot/qt/main/extensions/network/src/qsocket.cpp#11 $
**
** Implementation of Network Extension Library
**
** Created : 970521
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

#include "qsocket.h"
#include "qlist.h"
#include "qsocketdevice.h"
#include "qdns.h"

#if defined(UNIX)
// gethostbyname
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#endif

#if defined(_OS_WIN32_)
#include "qt_windows.h"
#endif

//#define QSOCKET_DEBUG


// Private class for QSocket

class QSocketPrivate {
public:
    QSocketPrivate();
   ~QSocketPrivate();

    QSocket::State	state;			// connection state
    QSocket::Mode	mode;			// mode for reading
    QString		host;			// host name
    int			port;			// host port
    QSocketDevice      *socket;			// connection socket
    QSocketNotifier    *rsn, *wsn;		// socket notifiers
    QList<QByteArray>	rba, wba;		// list of read/write bufs
    QHostAddress	addr;			// connection address
    int			rsize, wsize;		// read/write total buf size
    int			rindex, windex;		// read/write index
    bool		newline;		// has newline/can read line
    int			ready_read_timer;	// timer for emit read signals
    QDns	       *dns;
    bool		firstTime;		// workaround for Windows
};

QSocketPrivate::QSocketPrivate()
    : state(QSocket::Idle), mode(QSocket::Binary),
      host(QString::fromLatin1("")), port(0),
      socket(0), rsn(0), wsn(0), rsize(0), wsize(0), rindex(0), windex(0),
      newline(FALSE), ready_read_timer(0), dns(0), firstTime(TRUE)
{
    rba.setAutoDelete( TRUE );
    wba.setAutoDelete( TRUE );
}

QSocketPrivate::~QSocketPrivate()
{
    delete rsn;
    delete wsn;
    delete socket;
}



/*!
  \class QSocket qsocket.h
  \brief The QSocket class provides a buffered socket connection.

  \ingroup kernel

  This class provides a buffered TCP connection over a socket.
  Both read and write operations are buffered.

  \sa QSocketDevice, QHostAddress, QSocketNotifier
*/


/*!
  Creates a QSocket object in \c QSocket::Idle state.

  This socket can be used to make a connection to a host using
  the connectToHost() function.
*/

QSocket::QSocket( QObject *parent, const char *name )
    : QObject( parent, name )
{
    d = new QSocketPrivate;
    setFlags( IO_Direct );
    setStatus( IO_Ok );
}


/*!
  Creates a QSocket object for an existing connection using \a socket.

*/

QSocket::QSocket( int socket, QObject *parent, const char *name )
    : QObject( parent, name )
{
#if defined(QSOCKET_DEBUG)
    qDebug( "QSocket: Attach to socket %x", socket );
#endif
    d = new QSocketPrivate;
    d->socket = new QSocketDevice( socket, QSocketDevice::Stream );
    d->socket->setBlocking( FALSE );
    d->socket->setAddressReusable( TRUE );
    d->state = Connection;
    d->mode = Binary;
    d->rsn = new QSocketNotifier(d->socket->socket(), QSocketNotifier::Read);
    d->wsn = new QSocketNotifier(d->socket->socket(), QSocketNotifier::Write);
    connect( d->rsn, SIGNAL(activated(int)), SLOT(sn_read()) );
    d->rsn->setEnabled( TRUE );
    connect( d->wsn, SIGNAL(activated(int)), SLOT(sn_write()) );
    d->wsn->setEnabled( FALSE );
    // Initialize the IO device flags
    setFlags( IO_Direct );
    setStatus( IO_Ok );
    open( IO_ReadWrite );
}


/*!
  Destroys the socket.  Closes the connection if necessary.
  \sa close()
*/

QSocket::~QSocket()
{
#if defined(QSOCKET_DEBUG)
    qDebug( "QSocket: Destroy" );
#endif
    if ( state() != Idle )
	close();
    ASSERT( d != 0 );
    delete d;
}


/*!
  Returns a pointer to the internal socket device.  The returned pointer
  is null if there is no connection or pending connection.

  There is normally no need to manipulate the socket device directly
  since this class does all the necessary setup for most client or
  server socket applications.
*/

QSocketDevice *QSocket::socketDevice()
{
    return d->socket;
}


/*!
  Processes timer events for QSocket.  Emits the readyRead() signal
  if there if there is buffered input available for reading.
*/

void QSocket::timerEvent( QTimerEvent *e )
{
    if ( e->timerId() == d->ready_read_timer ) {
	if ( d && d->rsize > 0 ) {		// data in read buffer
	    emit readyRead();
	} else {				// empty read buffer
	    killTimer( d->ready_read_timer );	// then reset the timer
	    d->ready_read_timer = 0;
	}
    }
}


/*!
  \enum QSocket::State

  This enum contains the connection states:
  <ul>
  <li> \c QSocket::Idle if there is no connection,
  <li> \c QSocket::HostLookup during a host lookup,
  <li> \c QSocket::Connecting during an attempt to connect to a host, and
  <li> \c QSocket::Connection when there is a connection.
  </ul>
*/

/*!
  Returns the current state of the socket connection.
*/

QSocket::State QSocket::state() const
{
    return d->state;
}


/*!
  Returns the current communication mode, either \c QSocket::Binary
  or \c QSocket::Ascii.

  The default mode is \c QSocket::Binary.

  The documentation of setMode() explains this mode setting.

  \sa setMode()
*/

QSocket::Mode QSocket::mode() const
{
    return d->mode;
}

/*!
  \enum QSocket::Mode

  This enum specifies the communication mode:
  <ul>
  <li> \c QSocket::Binary (default)
  <li> \c QSocket::Ascii
  </ul>
*/

/*!
  Sets the
    \link QSocket::Mode communication mode\endlink to either binary or ascii.

  QSocket::Binary is the default.

  The mode only relates to incoming data. If binary mode is set,
  you read data using the readBlock() function.  bytesAvailable()
  tells you how much buffered data that can be read.

  If ascii mode set, QSocket will look for newline characters
  (\n) in the incoming data.  You can call the canReadLine()
  function to check if there is a line of text to be read.
  If canReadLine() returns TRUE, you can read a complete line of
  incoming text using the readLine() function.
  The readBlock() and bytesAvailable() functions work just like
  in binary mode.

  The close() function sets the mode to \c QSocket::Binary.

  \sa mode(), readBlock(), bytesAvailable(), canReadLine(), readLine(),
  close()
*/

void QSocket::setMode( Mode mode )
{
#if defined(QSOCKET_DEBUG)
    qDebug( "QSocket: Set mode %s", (mode == Binary ? "Binary" : "Ascii") );
#endif
    if ( d->mode == mode )
	return;
    d->mode = mode;
    if ( d->mode == Ascii  )
	d->newline = scanNewline();
}


/*!
  Attempts to make a connection to \a host on the specified \a port.

  Any connection or pending connection is closed immediately
  when you call this function.

  When making a connection, QSocket goes into the \c QSocket::HostLookup
  state while looking up the host.  When the host has been found, QSocket
  will try to connect to the host and it goes into the
  \c QSocket::Connecting state.  Finally, when a connection has been
  made, the state becomes \c QSocket::Connection.

  TODO### No real error handling so far. Blocking host lookup.

  \sa state()
*/

void QSocket::connectToHost( const QString &host, int port )
{
#if defined(QSOCKET_DEBUG)
    qDebug( "QSocket::connectToHost: host %s, port %d", host.ascii(), port );
#endif
    if ( d->state != Idle )
	close();
    // Re-initialize
    delete d;
    d = new QSocketPrivate;
    d->state = HostLookup;
    d->host = host;
    d->port = port;
    d->dns = new QDns( host, QDns::A );
    // Initialize the IO device flags
    setFlags( IO_Sequential );
    setStatus( IO_Ok );
    open( IO_ReadWrite );
    // First connection attempt, more to follow
    tryConnecting();
};



/*!  This private slots continues the connection process where
connectToHost() leaves off.
*/

void QSocket::tryConnecting()
{
    QValueList<QHostAddress> l = d->dns->addresses();
    debug( "l.count %d (for %s)", l.count(), d->dns->label().ascii() );
    if ( l.isEmpty() ) {
	if ( d->dns->queryStatus() == QDns::Active ) {
	    connect( d->dns, SIGNAL(statusChanged()),
		     this, SLOT(tryConnecting()) );
	    return;
	}
	d->state = Idle;
	return;
    }

    debug( "l.count %d", l.count() );
    // ### hack: just use the first address
    d->state = Connecting;
    d->socket = new QSocketDevice;
    if ( d->socket->connect( l[0], d->port ) == FALSE ) {
	// uhnm?
    }
#if defined(QSOCKET_DEBUG)
    qDebug( "QSocket::connectToHost: Connect to IP address %s",
	    l[0].ip4AddrString().ascii() );
#endif

    // Create and setup read/write socket notifiers
    // The socket write notifier will fire when the connection succeeds
    d->rsn = new QSocketNotifier(d->socket->socket(), QSocketNotifier::Read);
    d->rsn->setEnabled( TRUE );
    d->wsn = new QSocketNotifier(d->socket->socket(), QSocketNotifier::Write);
    d->wsn->setEnabled( TRUE );
    connect( d->rsn, SIGNAL(activated(int)), SLOT(sn_read()) );
    connect( d->wsn, SIGNAL(activated(int)), SLOT(sn_write()) );
    return;
}


/*!
  Returns the host name as specified to the connectToHost() function.
  If none has been set, the returned host name is "".
*/

QString QSocket::host() const
{
    return d->host;
}


/*!
  Returns the host port as specified to the connectToHost() function.
  If none has been set, the returned port is 0.
*/

int QSocket::port() const
{
    return d->port;
}


/*!
  \fn void QSocket::hostFound()

  This signal is emitted after connectToHost() has been called and a
  host has been looked up.

  \sa connected()
*/


/*!
  \fn void QSocket::connected()

  This signal is emitted after connectToHost() has been called and a
  connection has been successfully established.

  \sa connectToHost(), closed()
*/


/*!
  \fn void QSocket::closed()

  This signal is emitted when the other end has closed the connection.
  The read buffers may contain buffered input data which you can read
  after the connection was closed.

  \sa connectToHost(), close()
*/


/*!
  \fn void QSocket::delayedCloseFinished()

  This signal is emitted when a delayed close is finished. If you call
  close() and there is buffered output data to be written, QSocket goes
  into the \c QSocket::Closing state and returns immediately.  It will
  then keep writing to the socket until all the data has been written.
  Then, the delayCloseFinished() signal is emitted.

  \sa close()
*/


/*!
  \fn void QSocket::readyRead()

  This signal is emitted when there is incoming data to be read.

  When new data comes into the socket device, this signal is emitted
  immediately.  If QSocket has old incoming data that you have not read,
  the signal is emitted once every second until you read everything.

  \sa readBlock(), readLine(), bytesAvailable()
*/


/*!
  \fn void QSocket::bytesWritten( int nbytes )

  This signal is emitted when data actually has been written to the
  network.  The \a nbytes parameter says how many bytes were written.

  The bytesToWrite() function is often used in the same context, and
  it tells how many buffered bytes there are left to write.

  \sa writeBlock(), bytesToWrite()
*/


/*!
  Opens the socket using the specified QIODevice file mode.  This function
  is called automatically when needed and you should not call it yourself.
  \sa close()
*/

bool QSocket::open( int m )
{
    if ( isOpen() ) {
#if defined(CHECK_STATE)
	qWarning( "QSocket::open: Already open" );
#endif
	return FALSE;
    }
    QIODevice::setMode( m & IO_ReadWrite );
    setState( IO_Open );
    return TRUE;
}


/*!
  Closes the socket.
  The mode to \c QSocket::Binary and the read buffer is cleared.

  If the output buffer is empty, the state is set to \ QSocket::Idle
  and the connection is terminated immediately.  If the output buffer
  still contains data to be written, QSocket goes into the
  \c QSocket::Closing state and the rest of the data will be written.
  When all of the outgoing data have been written, the state is set
  to \c QSocket::Idle and the connection is terminated.  At this
  point, the delayedCloseFinished() signal is emitted.

  \sa state(), setMode(), bytesToWrite()
*/

void QSocket::close()
{
    if ( !isOpen() || d->state != Idle )	// already closed
	return;
#if defined(QSOCKET_DEBUG)
    qDebug( "QSocket: close socket" );
#endif
    d->mode = Binary;
    if ( d->socket && d->wsize ) {		// there's data to be written
	d->state = Closing;
	d->rsn->setEnabled( FALSE );
	d->wsn->setEnabled( TRUE );
	d->rba.clear();				// clear incoming data
	d->rindex = d->rsize = 0;
	return;
    }
    setFlags( IO_Sequential );
    setStatus( IO_Ok );
    if ( d->socket ) {
	// We must disable the socket notifiers before the socket
	// disappears
	d->rsn->setEnabled( FALSE );
	d->wsn->setEnabled( FALSE );
	d->socket->close();
    }
    delete d;
    d = new QSocketPrivate;
    d->state = Idle;
}


/*!
  This function consumes data from the read buffer and copies
  it into \a copyInto if it is a valid pointer.
*/

bool QSocket::skipReadBuf( int nbytes, char *copyInto )
{
    if ( nbytes <= 0 || nbytes > d->rsize )
	return FALSE;
#if defined(QSOCKET_DEBUG)
    qDebug( "QSocket: skipReadBuf %d bytes", nbytes );
#endif
    d->rsize -= nbytes;
    while ( TRUE ) {
	QByteArray *a = d->rba.first();
	if ( d->rindex + nbytes >= (int)a->size() ) {
	    // Here we skip the whole byte array and get the next later
	    int len = a->size() - d->rindex;
	    if ( copyInto ) {
		memcpy( copyInto, a->data()+d->rindex, len );
		copyInto += len;
	    }
	    nbytes -= len;
	    d->rba.remove();
	    d->rindex = 0;
	    if ( nbytes == 0 ) {		// nothing more to skip
		if ( d->mode == Ascii )
		    d->newline = scanNewline();
		return TRUE;
	    }
	} else {
	    // Here we skip only a part of the first byte array
	    if ( copyInto )
		memcpy( copyInto, a->data()+d->rindex, nbytes );
	    d->rindex += nbytes;
	    if ( d->mode == Ascii )
		d->newline = scanNewline();
	    return TRUE;
	}
    }
    return FALSE;				// should never be reached
}


/*!
  This function consumes data from the write buffer.  It is similar
  to skipReadBuf() above, except that it does not copy the data
  into another buffer.
*/

bool QSocket::skipWriteBuf( int nbytes )
{
    if ( nbytes <= 0 || nbytes > d->wsize )
	return FALSE;
#if defined(QSOCKET_DEBUG)
    qDebug( "QSocket: skipWriteBuf %d bytes", nbytes );
#endif
    d->wsize -= nbytes;
    while ( TRUE ) {
	QByteArray *a = d->wba.first();
	if ( d->windex + nbytes >= (int)a->size() ) {
	    nbytes -= a->size() - d->windex;
	    d->wba.remove();
	    d->windex = 0;
	    if ( nbytes == 0 )
		return TRUE;
	} else {
	    d->windex += nbytes;
	    return TRUE;
	}
    }
    return FALSE;
}



/*!
  Scans for any occurrence of \n in the read buffer.
  Stores the text in the byte array \a store if it is non-null.
*/

bool QSocket::scanNewline( QByteArray *store )
{
    if ( d->rsize == 0 )
	return FALSE;
    if ( store && store->size() < 128 )
	store->resize( 128 );
    int i = 0;					// index into 'store'
    QByteArray *a = 0;
    char *p;
    int   n;
    while ( TRUE ) {
	if ( !a ) {
	    a = d->rba.first();
	    if ( !a || a->size() == 0 )
		return FALSE;
	    p = a->data() + d->rindex;
	    n = a->size() - d->rindex;
	} else {
	    a = d->rba.next();
	    if ( !a || a->size() == 0 )
		return FALSE;
	    p = a->data();
	    n = a->size();
	}
	if ( store ) {
	    while ( n-- > 0 ) {
		*(store->data()+i) = *p;
		if ( ++i == (int)store->size() )
		    store->resize( store->size()*2 );
		switch ( *p ) {
		    case '\0':
#if defined(QSOCKET_DEBUG)
			qDebug( "QSocket::scanNewline: Oops, unexpected "
			       "0-terminated text read %s", store->data() );
#endif
			store->resize( i );
			return FALSE;
		    case '\n':
			*(store->data()+i) = '\0';
			store->resize( i );
			return TRUE;
		}
		p++;
	    }
	} else {
	    while ( n-- > 0 ) {
		switch ( *p++ ) {
		    case '\0':
			return FALSE;
		    case '\n':
			return TRUE;
		}
	    }
	}
    }
    return FALSE;
}


/*!
  Implementation of the abstract virtual QIODevice::flush() function.
  This implementation is a no-op.
*/

void QSocket::flush()
{
    if ( (d->state == Connection || d->state == Closing) && d->wsize > 0 ) {
#if defined(QSOCKET_DEBUG)
	qDebug( "QSocket: sn_write: Write data to the socket" );
#endif
	QByteArray *a = d->wba.first();
	int nwritten;
	if ( (int)a->size() - d->windex < 1480 ) {
	    // Concatenate many smaller block
	    QByteArray out(1480);
	    int i = 0;
	    int j = d->windex;
	    int s = a->size() - j;
	    while ( a && i+s < (int)out.size() ) {
		memcpy( out.data()+i, a->data()+j, s );
		j = 0;
		i += s;
		a = d->wba.next();
		s = a ? a->size() : 0;
	    }
	    nwritten = d->socket->writeBlock( out.data(), i );
	} else {
	    // Big block, write it immediately
	    nwritten = d->socket->writeBlock( a->data() + d->windex,
					      a->size() - d->windex );
	}
	skipWriteBuf( nwritten );
	if ( nwritten > 0 )
	    emit bytesWritten( nwritten );
#if defined(QSOCKET_DEBUG)
	qDebug( "QSocket: sn_write: wrote %d bytes, %d left", nwritten,
		d->wsize );
#endif
	if ( d->state == Closing && d->wsize == 0 ) {
#if defined(QSOCKET_DEBUG)
	    qDebug( "QSocket: sn_write: Delayed close done. Terminating now" );
#endif
	    setFlags( IO_Sequential );
	    setStatus( IO_Ok );
	    // We must disable the socket notifiers before the socket
	    // disappears
	    d->rsn->setEnabled( FALSE );
	    d->wsn->setEnabled( FALSE );
	    d->socket->close();
	    delete d;
	    d = new QSocketPrivate;
	    d->state = Idle;
	    emit delayedCloseFinished();
	    return;
	}
    }
    d->wsn->setEnabled( d->wsize > 0 );		// write if there's data
}


/*!
  Returns the number of incoming bytes that can be read, same as
  bytesAvailable().
*/

uint QSocket::size() const
{
    return d->rsize;
}


/*!
  Returns the current read index.  Since QSocket is a sequential
  device, the current read index is always zero.
*/

int QSocket::at() const
{
    return 0;
}


/*!
  Moves the read index forward and returns TRUE if the operation
  was successful.  Moving the index forward means skipping incoming
  data.
*/

bool QSocket::at( int index )
{
    if ( index < 0 || index > d->rsize )
	return FALSE;
    skipReadBuf( index, 0 );			// throw away data 0..index-1
    return TRUE;
}


/*!
  Returns TRUE if there is no more data to read, otherwise FALSE.
*/

bool QSocket::atEnd() const
{
    return d->rsize == 0;
}


/*!
  Returns the number of incoming bytes that can be read, i.e. the
  size of the input buffer.  Equivalent to size().
  \sa bytesToWrite();
*/

int QSocket::bytesAvailable() const
{
    return d->rsize;
}


/*!
  Returns the number of bytes that are waiting to be written, i.e. the
  size of the output buffer.
  \sa bytesAvailable.
*/

int QSocket::bytesToWrite() const
{
    return d->wsize;
}


/*!
  Reads max \a maxlen bytes from the socket into \a data and returns
  the number of bytes read.  Returns -1 if an error occurred.
*/

int QSocket::readBlock( char *data, uint maxlen )
{
    if ( data == 0 && maxlen != 0 ) {
#if defined(CHECK_NULL)
	qWarning( "QSocket::readBlock: Null pointer error" );
#endif
	return -1;
    }
    if ( !isOpen() ) {
#if defined(CHECK_STATE)
	qWarning( "QSocket::readBlock: Socket is not open" );
#endif
	return -1;
    }
    if ( (int)maxlen >= d->rsize )
	maxlen = d->rsize;
#if defined(QSOCKET_DEBUG)
    qDebug( "QSocket: readBlock %d bytes", maxlen );
#endif
    skipReadBuf( maxlen, data );
    if ( d->mode == Ascii )
	d->newline = scanNewline();
    return maxlen;
}


/*!
  Writes \a len bytes to the socket from \a data and returns
  the number of bytes written.  Returns -1 if an error occurred.
*/

int QSocket::writeBlock( const char *data, uint len )
{
#if defined(CHECK_NULL)
    if ( data == 0 && len != 0 ) {
	qWarning( "QSocket::writeBlock: Null pointer error" );
    }
#endif
#if defined(CHECK_STATE)
    if ( !isOpen() ) {
	qWarning( "QSocket::writeBlock: Socket is not open" );
	return -1;
    }
#endif
#if defined(CHECK_STATE)
    if ( d->state == Closing ) {
	qWarning( "QSocket::writeBlock: Cannot write, socket is closing" );
    }
#endif
    if ( len == 0 || d->state == Closing )
	return 0;
    QByteArray *a = d->wba.last();

    // next bit is sensitive.  here's what it ought to do.  if we're
    // writing really small chunks, try to buffer up since system
    // calls are expensive.  but if anything even remotely large is
    // being written, try to issue a write at once.  so, we should try
    // a write immediately if there isn't data queued up already, and
    // the write notifier is off, and this blob of data isn't
    // ridiculously small.

    if ( a && a->size() + len < 128 ) {		// small buffer, resize
	int i = a->size();
	a->resize( i+len );
	memcpy( a->data()+i, data, len );
    } else {					// append new buffer
	a = new QByteArray( len );
	memcpy( a->data(), data, len );
	d->wba.append( a );
    }
    d->wsize += len;
    if ( d->wsn )
	d->wsn->setEnabled( TRUE );		// there's data to write
#if defined(QSOCKET_DEBUG)
    qDebug( "QSocket: writeBlock %d bytes", len );
#endif
    return len;
}


/*!
  Reads a single byte/character from the internal read buffer.
  Returns the byte/character read, or -1 if there is nothing
  to be read.

  \sa bytesAvailable(), putch()
*/

int QSocket::getch()
{
    if ( isOpen() && d->rsize > 0 ) {
	uchar c;
	skipReadBuf( 1, (char*)&c );
	if ( d->mode == Ascii && c == '\n' )
	    d->newline = scanNewline();
	return c;
    }
    return -1;
}


/*!
  Writes the character \e ch into the output buffer.

  Returns \e ch, or -1 if some error occurred.

  \sa getch()
*/

int QSocket::putch( int ch )
{
    char buf[2];
    buf[0] = ch;
    return writeBlock(buf, 1) == 1 ? ch : -1;
}


/*!
  This implementation of the virtual function QIODevice::ungetch() always
  returns -1 (error) because a QSocket is a sequential device and does not
  allow any ungetch operation.
*/

int QSocket::ungetch( int )
{
    return -1;
}


/*!
  Returns TRUE if the socket is in ascii mode and readLine() can be
  called to read a line of text.  Otherwise FALSE is returned.
  \sa setMode(), readLine()
*/

bool QSocket::canReadLine() const
{
    return d->mode == Ascii && d->newline;
}


/*!
  Returns a line of text including a terminating newline character (\n).
  Returns "" if canReadLine() returns FALSE.
  \sa canReadLine()
*/

QString QSocket::readLine()
{
    if ( !canReadLine() )
	return QString::fromLatin1("");
    QByteArray a(256);
    scanNewline( &a );
    at( a.size() );				// skips the data read
    QString s( a );
    return s;
}


/*!
  Internal slot for handling socket read notifications.
*/

void QSocket::sn_read()
{
    int nbytes = d->socket->bytesAvailable();
    // Here, nbytes can often be zero on Microsoft Windows the first
    // time sn_read is invoked where there actually is data on the
    // socket.  This is because of a bug in ioctlsocket().
    // We use the firstTime variable to avoid this problem.
    bool connectionClosed = !d->firstTime && nbytes == 0;
    d->firstTime = FALSE;

    if ( connectionClosed ) {			// connection closed
#if defined(QSOCKET_DEBUG)
	qDebug( "QSocket: sn_read: Connection closed" );
#endif
	// We keep the open state in case there's unread incoming data
	d->state = Idle;
	d->rsn->setEnabled( FALSE );
	d->wsn->setEnabled( FALSE );
	d->socket->close();
	d->wba.clear();				// clear write buffer
	d->windex = d->wsize = 0;
	emit closed();
    } else if ( nbytes > 0 ) {			// data to be read
#if defined(QSOCKET_DEBUG)
	qDebug( "QSocket: sn_read: %d incoming bytes", nbytes );
#endif
	QByteArray *a = new QByteArray( nbytes );
	int nread = d->socket->readBlock( a->data(), nbytes );
	if ( nread != nbytes ) {		// unexpected
#if defined(CHECK_RANGE)
	    qWarning( "QSocket::sn_read: Unexpected short read" );
#endif
	    a->resize( nread );
	}
	d->rba.append( a );
	d->rsize += nread;
	if ( d->mode == Ascii )
	    d->newline = scanNewline();
	if ( d->ready_read_timer == 0 )
	    d->ready_read_timer = startTimer( 1000 );
	emit readyRead();
    }
}


/*!
  Internal slot for handling socket write notifications.
*/

void QSocket::sn_write()
{
    if ( d->state == Connecting ) {		// connection established?
	if ( d->socket->connect( d->addr, d->port ) ) {
	    d->state = Connection;
	    emit connected();
	} else {
	    d->state = Idle;
	    emit error();
	    return;
	}
#if defined(QSOCKET_DEBUG)
	qDebug( "QSocket: sn_write: Got connection!" );
#endif
	d->firstTime = TRUE;
    }
    flush();
}
