/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

/*! \class QTcpServer

    \brief The QTcpServer class provides a TCP-based server.
\if defined(commercial)
    It is part of the <a href="commercialeditions.html">Qt Enterprise Edition</a>.
\endif
    \reentrant
    \ingroup io
    \module network

    This class is a server which accepts incoming TCP connections. You
    can specify the port or have QTcpServer pick one, and listen on
    just one address, or on all the machine's addresses.

    Call listen() to have the server listen for incoming
    connections. isListening() can be called to check whether or not
    the server is listening for incoming connections. The
    newConnection() signal is emitted when a connection is
    available. hasPendingConnection() returns true if an incoming
    connection is currently pending. Call nextPendingConnection() to
    accept the pending connection as a connected QTcpSocket. Call
    close() to close the server.

    When listening for connections, the server's address and port can
    be fetched by calling serverAddress() and serverPort().

    If an error occurs, serverError() returns the type of error, and
    errorString() can be called to get a human readable description of
    what happened.

    This class supports blocking operations for when the event loop is
    not available. waitForConnection() is a blocking function call
    which suspends the execution of the program until either a
    connection is available or a timeout expires.

    For low level access, socketDescriptor() returns the native socket
    descriptor that the server uses to listen for connections. This
    socket can be set by calling setSocketDescriptor().
*/

//#define QTCPSERVER_DEBUG

#include "qtcpserver.h"
#include "qsocketlayer.h"
#include "qhostaddress.h"
#include "qlist.h"
#include "qtcpsocket.h"
#include "private/qobject_p.h"
#include "qsocketnotifier.h"

#define d d_func()
#define q q_func()

class QTcpServerPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QTcpServer);
public:
    QTcpServerPrivate();
    ~QTcpServerPrivate();

    QList<QTcpSocket *> pendingConnections;

    Q_UINT16 port;
    QHostAddress address;

    Qt::SocketState state;
    QSocketLayer socketLayer;

    Qt::SocketError serverSocketError;
    QString serverSocketErrorString;

    int maxConnections;

    QSocketNotifier *readSocketNotifier;

    // private slots
    void processIncomingConnection(int socketDescriptor);
};

/*! \internal
*/
QTcpServerPrivate::QTcpServerPrivate()
{
    port = 0;
    maxConnections = 30;
    readSocketNotifier = 0;
}

/*! \internal
*/
QTcpServerPrivate::~QTcpServerPrivate()
{
}

/*! \internal
*/
void QTcpServerPrivate::processIncomingConnection(int)
{
    d->readSocketNotifier->setEnabled(false);
    if (pendingConnections.count() >= maxConnections) {
#if defined (QTCPSERVER_DEBUG)
        qDebug("QTcpServerPrivate::processIncomingConnection() too many connections");
#endif
        return;
    }

    int descriptor;
    while ((descriptor = socketLayer.accept()) != -1) {
#if defined (QTCPSERVER_DEBUG)
        qDebug("QTcpServerPrivate::processIncomingConnection() accepted socket %i", descriptor);
#endif
        q->incomingConnection(descriptor);
    }

    emit q->newConnection();
}

/*!
    Constructs a QTcpServer. the \a parent argument is passed to
    QObject's constructor.

    \sa listen()
*/
QTcpServer::QTcpServer(QObject *parent)
    : QObject(*new QTcpServerPrivate, parent)
{
    d->state = Qt::UnconnectedState;
}

/*!
    Destructs the QTcpServer. If the server is listening for
    connections, it is closed before it is destroyed.

    Any clients that are still connected must either disconnect, or be
    reparented before the server is deleted. Otherwise, the clients
    will be deleted together with the server.
*/
QTcpServer::~QTcpServer()
{
    close();
}

/*!
    Makes the server listen for incoming connections on address \a
    address and port \a port. If \a port is 0, a port is chosed
    automatically. If QHostAddress::AnyAddress is passed to \a
    address, the server will listen on all network interfaces.

    Returns true on success; otherwise returns false.

    \sa isListening()
*/
bool QTcpServer::listen(const QHostAddress &address, Q_UINT16 port)
{
    if (d->state == Qt::ListeningState) {
        qWarning("QTcpServer::listen() called when already listening");
        return false;
    }

    Qt::NetworkLayerProtocol proto;
    if (address.isIPv4Address()) {
        proto = Qt::IPv4Protocol;
    } else {
#if defined(Q_NO_IPv6)
        // If we have no IPv6 support, then we will not be able to
        // listen on an IPv6 interface.
        // ### Report the error somehow.
        return false;
#endif
        proto = Qt::IPv6Protocol;
    }

    if (!d->socketLayer.initialize(Qt::TcpSocket, proto)) {
        d->serverSocketError = d->socketLayer.socketError();
        d->serverSocketErrorString = d->socketLayer.errorString();
        return false;
    }

    if (!d->socketLayer.bind(address, port)) {
        d->serverSocketError = d->socketLayer.socketError();
        d->serverSocketErrorString = d->socketLayer.errorString();
        return false;
    }

    if (!d->socketLayer.listen()) {
        d->serverSocketError = d->socketLayer.socketError();
        d->serverSocketErrorString = d->socketLayer.errorString();
        return false;
    }

    d->readSocketNotifier = new QSocketNotifier(d->socketLayer.socketDescriptor(),
                                                QSocketNotifier::Read, this);
    connect(d->readSocketNotifier, SIGNAL(activated(int)), SLOT(processIncomingConnection(int)));

    d->state = Qt::ListeningState;
    d->address = address;
    d->port = port;

#if defined (QTCPSERVER_DEBUG)
    qDebug("QTcpServer::listen(%i, \"%s\") == true (listening on port %i)", port,
           address.toString().latin1(), d->socketLayer.localPort());
#endif
    return true;
}

/*! \overload

    Listens for connections on all network interfaces. Equivalent to
    calling listen(QHostAddress::AnyAddress, \a port).
*/
bool QTcpServer::listen(Q_UINT16 port)
{
    return listen(QHostAddress::AnyAddress, port);
}

/*!
    Returns true if the server is currently listening for incoming
    connections; otherwise returns false.
*/
bool QTcpServer::isListening() const
{
    return d->socketLayer.socketState() == Qt::ListeningState;
}

/*!
    Closes the server; the server will no longer listen for incoming
    connections.
*/
void QTcpServer::close()
{
    if (d->readSocketNotifier && d->readSocketNotifier->isEnabled())
        d->readSocketNotifier->setEnabled(false);
    delete d->readSocketNotifier;
    d->readSocketNotifier = 0;

    if (d->socketLayer.isValid())
        d->socketLayer.close();
    d->state = Qt::UnconnectedState;
}

/*!
    Returns the native socket descriptor the server uses to listen for
    incoming instructions. If the server is not listening, -1 is
    returned.
*/
int QTcpServer::socketDescriptor() const
{
    return d->socketLayer.socketDescriptor();
}

/*!
    Sets the socket descriptor this server should use when listening
    for incoming connections.
*/
bool QTcpServer::setSocketDescriptor(int socketDescriptor)
{
    if (isListening()) {
        qWarning("QTcpServer::setSocketDescriptor() can only be called when in NotListeningState");
        return false;
    }

    if (!d->socketLayer.initialize(socketDescriptor)) {
        d->serverSocketError = d->socketLayer.socketError();
        d->serverSocketErrorString = d->socketLayer.errorString();
    }

    return true;
}

/*!
    Returns the server's port if it is listening for connections;
    otherwise 0 is returned.
*/
Q_UINT16 QTcpServer::serverPort() const
{
    return d->socketLayer.localPort();
}

/*!
    Returns the server's address if it is listening for connections;
    otherwise QHostAddress::NullAddress is returned.
*/
QHostAddress QTcpServer::serverAddress() const
{
    return d->socketLayer.localAddress();
}

/*!
    Waits for at most \a msec milliseconds or until an incoming
    connection is available. Returns true if a connection is
    available; otherwise returns false.

    This is a blocking function call; its use is disadvised in a
    single threaded application, as the whole thread will stop
    responding until the function returns. waitForConnection() is most
    useful when there is no event loop available. The general approach
    is to connect to the newConnection() signal.

    \sa hasPendingConnection(), nextPendingConnection()
*/
bool QTcpServer::waitForConnection(int msec, bool *timedOut)
{
    if (d->state != Qt::ListeningState)
        return false;

    if (!d->socketLayer.waitForRead(msec, timedOut)) {
        d->serverSocketError = d->socketLayer.socketError();
        d->serverSocketErrorString = d->socketLayer.errorString();
        return false;
    }

    if (timedOut && *timedOut)
        return false;

    d->processIncomingConnection(0);

    emit newConnection();
    return true;
}

/*!
    Returns true if the server has a pending connection; otherwise
    returns false.

    \sa nextPendingConnection()
*/
bool QTcpServer::hasPendingConnection() const
{
    return !d->pendingConnections.isEmpty();
}

/*!
    Returns the next pending connection as a connected QTcpSocket. The
    socket is created as a child of the server.

    \sa hasPendingConnection()
*/
QTcpSocket *QTcpServer::nextPendingConnection()
{
    if (d->pendingConnections.isEmpty())
        return 0;

    if (!d->readSocketNotifier->isEnabled())
        d->readSocketNotifier->setEnabled(true);
    return d->pendingConnections.takeFirst();
}

/*!
    This function is called by QTcpServer when a new connection is
    available. The \a socketDescriptor argument is the native socket
    descriptor for the accepted connection.

    The base implementation creates a QTcpSocket, sets the socket
    descriptor and then stores the QTcpSocket in an internal list of
    pending connections. Finally newConnection() is emitted.

    Reimplement this function to alter the server's behavior when a
    connection is available.
*/
void QTcpServer::incomingConnection(int socketDescriptor)
{
#if defined (QTCPSERVER_DEBUG)
    qDebug("QTcpServer::incomingConnection(%i)", socketDescriptor);
#endif

    QTcpSocket *socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);
    d->pendingConnections.append(socket);
}

/*!
    Sets the maximum number of pending accepted connections to \a
    numConnections. QTcpServer will accept no more than \a
    numConnections incoming connections before nextPendingConnection()
    is called. By default, no more than 30 pending connections are
    allowed.

    Clients that attempt to connect to the server after it has reached
    its maximum number of pending connections will either immediately
    fail to connect, or they will time out.
*/
void QTcpServer::setMaxPendingConnections(int numConnections)
{
    d->maxConnections = numConnections;
}

/*!
    Returns the maximum number of pending accepted connections.

    By default, no more than 30 pending connections are allowed.

    \sa setMaxPendingConnections()
*/
int QTcpServer::maxPendingConnections() const
{
    return d->maxConnections;
}

/*!
    Returns the type error that last occurred.
*/
Qt::SocketError QTcpServer::serverError() const
{
    return d->serverSocketError;
}

/*!
    Returns a human readable description of the last error that
    occurred.
*/
QString QTcpServer::errorString() const
{
    return d->serverSocketErrorString;
}

#include "moc_qtcpserver.cpp"

