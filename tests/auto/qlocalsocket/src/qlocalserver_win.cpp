/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qlocalserver.h"
#include "qlocalserver_p.h"
#include "qlocalsocket.h"

#include <qdebug.h>
#include <qdatetime.h>
#include <qcoreapplication.h>

#define BUFSIZE 4096

QLocalServerThread::QLocalServerThread(QObject *parent) : QThread(parent),
	handle(INVALID_HANDLE_VALUE)
{
}

QLocalServerThread::~QLocalServerThread()
{
    CloseHandle(handle);
}

void QLocalServerThread::setName(const QString &key)
{
    fullName = QString("\\\\.\\pipe\\%1").arg(key);
    makeHandle();
}

void QLocalServerThread::makeHandle()
{
    if (handle != INVALID_HANDLE_VALUE)
        return;

    handle = CreateNamedPipeW(
                 (TCHAR*)fullName.utf16(), // pipe name
                 PIPE_ACCESS_DUPLEX,       // read/write access
                 PIPE_TYPE_MESSAGE |       // message type pipe
                 PIPE_READMODE_MESSAGE |   // message-read mode
                 PIPE_WAIT,                // blocking mode
                 PIPE_UNLIMITED_INSTANCES, // max. instances
                 BUFSIZE,                  // output buffer size
                 BUFSIZE,                  // input buffer size
                 3000,                     // client time-out
                 NULL);

    if (handle == INVALID_HANDLE_VALUE) {
	emit error();
    }
}

void QLocalServerThread::run()
{
    qDebug() << "Starting server";
    while (true) {
	makeHandle();
        if (handle == INVALID_HANDLE_VALUE)
            return;

	qDebug() << "server: waiting";
        // Wait for a connection, ConnectNamedPipe blocks
	BOOL fConnected = ConnectNamedPipe(handle, NULL) ?
                          TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (fConnected) {
            emit connected((int)handle);
	} else {
	    emit error();
	    CloseHandle(handle);
	}
	qDebug() << "server: resetartin";
        handle = INVALID_HANDLE_VALUE;
    }
}

void QLocalServerPrivate::init()
{
    Q_Q(QLocalServer);
    q->connect(&thread, SIGNAL(connected(int)), q, SLOT(_q_openSocket(int)));
    q->connect(&thread, SIGNAL(finished()), q, SLOT(_q_stoppedListening()));
    q->connect(&thread, SIGNAL(terminated()), q, SLOT(_q_stoppedListening()));
    q->connect(&thread, SIGNAL(error()), q, SLOT(_q_error()));
}

void QLocalServerPrivate::_q_error()
{
    // ### See if this is ever hit when run on all the windows test machines
    QString function = QLatin1String("QLocalServer::listen");
    error = QLocalServer::UnknownError;
    errorString = QLocalServer::tr("%1: unknown error %2").arg(function).arg(GetLastError());
    qDebug() << "error?" << errorString;
}

bool QLocalServerPrivate::listen(const QString &name)
{
    thread.terminate();
    thread.setName(name);
    thread.start();
    return true;
}

void QLocalServerPrivate::_q_stoppedListening()
{
    Q_Q(QLocalServer);
    if (!waiting)
        q->close();
}

void QLocalServerPrivate::_q_openSocket(int handle)
{
    Q_Q(QLocalServer);
    q->incomingConnection(handle);
}

bool QLocalServerPrivate::closeServer()
{
    thread.terminate();
    CloseHandle(thread.handle);
    return true;
}

void QLocalServerPrivate::waitForNewConnection(int msecs, bool *timedOut)
{
    if (!pendingConnections.isEmpty())
	return;

    // For the time being while developing the QLocalSocket class ...
    msecs = 1000;

    // ### Is there no nicer way to do this?
    waiting = true;
    thread.terminate();
    if (thread.handle == INVALID_HANDLE_VALUE)
	thread.makeHandle();
    // The thread might have emited a signal of a new connection
    // before terminating
    QCoreApplication::instance()->processEvents();
    QTime stopWatch;
    stopWatch.start();

    DWORD dwMode = PIPE_NOWAIT;
    SetNamedPipeHandleState(thread.handle, &dwMode, NULL, NULL);
    dwMode = PIPE_WAIT;

    while (pendingConnections.isEmpty()
	 && (msecs == -1 || stopWatch.elapsed() < msecs)) {
	BOOL fConnected = ConnectNamedPipe(thread.handle, NULL) ?
                          TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (fConnected) {
	    SetNamedPipeHandleState(thread.handle, &dwMode, NULL, NULL);
	    _q_openSocket((int)thread.handle);
            thread.handle = INVALID_HANDLE_VALUE;
	    thread.makeHandle();
	    break;
	} else {
          qDebug() << "looping" << GetLastError() << thread.handle;
	  Sleep(10);
	}
    }
    if (timedOut)
        *timedOut = (stopWatch.elapsed() > msecs);
    SetNamedPipeHandleState(thread.handle, &dwMode, NULL, NULL);
    waiting = false;
    thread.start();
}
