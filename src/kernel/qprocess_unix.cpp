/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qprocess_unix.cpp#73 $
**
** Implementation of QProcess class for Unix
**
** Created : 20000905
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
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
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
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

#ifndef QT_H
#  include <qfeatures.h>
#endif // QT_H

#ifndef QT_NO_PROCESS

#include "qplatformdefs.h"
#include "qprocess.h"
#include "qapplication.h"
#include "qptrqueue.h"
#include "qptrlist.h"
#include "qsocketnotifier.h"
#include "qtimer.h"
#include "qcleanuphandler.h"


//#define QT_QPROCESS_DEBUG


#ifdef Q_C_CALLBACKS
extern "C" {
#endif // Q_C_CALLBACKS

    QT_SIGNAL_RETTYPE qt_C_sigchldHnd(QT_SIGNAL_ARGS);

#ifdef Q_C_CALLBACKS
}
#endif // Q_C_CALLBACKS


class QProc;
class QProcessManager;
class QProcessPrivate
{
public:
    QProcessPrivate();
    ~QProcessPrivate();

    void closeOpenSocketsForChild();
    void newProc( pid_t pid, QProcess *process );

    QByteArray bufStdout;
    QByteArray bufStderr;

    QPtrQueue<QByteArray> stdinBuf;

    QSocketNotifier *notifierStdin;
    QSocketNotifier *notifierStdout;
    QSocketNotifier *notifierStderr;

    ssize_t stdinBufRead;
    QProc *proc;

    bool exitValuesCalculated;

    static QProcessManager *procManager;
};


/***********************************************************************
 *
 * QProc
 *
 **********************************************************************/
/*
  The class QProcess does not necessarily map exactly to the running
  child processes: if the process is finished, the QProcess class may still be
  there; furthermore a user can use QProcess to start more than one process.

  The helper-class QProc has the semantics that one instance of this class maps
  directly to a running child process.
*/
class QProc
{
public:
    QProc( pid_t p, QProcess *proc=0 ) : pid(p), process(proc)
    {
#if defined(QT_QPROCESS_DEBUG)
	qDebug( "QProc: Constructor for pid %d and QProcess %p", pid, process );
#endif
	socketStdin = 0;
	socketStdout = 0;
	socketStderr = 0;
    }
    ~QProc()
    {
#if defined(QT_QPROCESS_DEBUG)
	qDebug( "QProc: Destructor for pid %d and QProcess %p", pid, process );
#endif
	if ( process != 0 ) {
	    if ( process->d->notifierStdin )
		process->d->notifierStdin->setEnabled( FALSE );
	    if ( process->d->notifierStdout )
		process->d->notifierStdout->setEnabled( FALSE );
	    if ( process->d->notifierStderr )
		process->d->notifierStderr->setEnabled( FALSE );
	    process->d->proc = 0;
	}
	if( socketStdin != 0 )
	    ::close( socketStdin );
	// ### close these sockets even on parent exit or is it better only on
	// sigchld (but what do I have to do with them on exit then)?
	if( socketStdout != 0 )
	    ::close( socketStdout );
	if( socketStderr != 0 )
	    ::close( socketStderr );
    }

    pid_t pid;
    int socketStdin;
    int socketStdout;
    int socketStderr;
    QProcess *process;
};

/***********************************************************************
 *
 * QProcessManager
 *
 **********************************************************************/
class QProcessManager : public QObject
{
    Q_OBJECT

public:
    QProcessManager();
    ~QProcessManager();

    void append( QProc *p );
    void remove( QProc *p );

public slots:
    void removeMe();
    void sigchldHnd( int );

public:
    struct sigaction oldactChld;
    struct sigaction oldactPipe;
    QPtrList<QProc> *procList;
    int sigchldFd[2];
};

QCleanupHandler<QProcessManager> qprocess_cleanup_procmanager;

QProcessManager::QProcessManager()
{
    procList = new QPtrList<QProc>;
    procList->setAutoDelete( TRUE );

    // The SIGCHLD handler writes to a socket to tell the manager that
    // something happened. This is done to get the processing in sync with the
    // event reporting.
    if ( ::socketpair( AF_UNIX, SOCK_STREAM, 0, sigchldFd ) ) {
	sigchldFd[0] = 0;
	sigchldFd[1] = 0;
    } else {
#if defined(QT_QPROCESS_DEBUG)
	qDebug( "QProcessManager: install socket notifier (%d)", sigchldFd[1] );
#endif
	QSocketNotifier *sn = new QSocketNotifier( sigchldFd[1],
		QSocketNotifier::Read, this );
	connect( sn, SIGNAL(activated(int)),
		this, SLOT(sigchldHnd(int)) );
	sn->setEnabled( TRUE );
    }

    // install a SIGCHLD handler and ignore SIGPIPE
    struct sigaction act;

#if defined(QT_QPROCESS_DEBUG)
    qDebug( "QProcessManager: install a sigchild handler" );
#endif
    act.sa_handler = qt_C_sigchldHnd;
    sigemptyset( &(act.sa_mask) );
    sigaddset( &(act.sa_mask), SIGCHLD );
    act.sa_flags = SA_NOCLDSTOP;
#if defined(SA_RESTART)
    act.sa_flags |= SA_RESTART;
#endif
    if ( sigaction( SIGCHLD, &act, &oldactChld ) != 0 )
	qWarning( "Error installing SIGCHLD handler" );

#if defined(QT_QPROCESS_DEBUG)
    qDebug( "QProcessManager: install a sigpipe handler (SIG_IGN)" );
#endif
    act.sa_handler = QT_SIGNAL_IGNORE;
    sigemptyset( &(act.sa_mask) );
    sigaddset( &(act.sa_mask), SIGPIPE );
    act.sa_flags = 0;
    if ( sigaction( SIGPIPE, &act, &oldactPipe ) != 0 )
	qWarning( "Error installing SIGPIPE handler" );

    qprocess_cleanup_procmanager.add( this );
}

QProcessManager::~QProcessManager()
{
    delete procList;

    if ( sigchldFd[0] != 0 )
	::close( sigchldFd[0] );
    if ( sigchldFd[1] != 0 )
	::close( sigchldFd[1] );

    // restore SIGCHLD handler
#if defined(QT_QPROCESS_DEBUG)
    qDebug( "QProcessManager: restore old sigchild handler" );
#endif
    if ( sigaction( SIGCHLD, &oldactChld, 0 ) != 0 )
	qWarning( "Error restoring SIGCHLD handler" );

#if defined(QT_QPROCESS_DEBUG)
    qDebug( "QProcessManager: restore old sigpipe handler" );
#endif
    if ( sigaction( SIGPIPE, &oldactPipe, 0 ) != 0 )
	qWarning( "Error restoring SIGPIPE handler" );
}

void QProcessManager::append( QProc *p )
{
    procList->append( p );
#if defined(QT_QPROCESS_DEBUG)
    qDebug( "QProcessManager: append process (procList.count(): %d)", procList->count() );
#endif
}

void QProcessManager::remove( QProc *p )
{
    procList->remove( p );
#if defined(QT_QPROCESS_DEBUG)
    qDebug( "QProcessManager: remove process (procList.count(): %d)", procList->count() );
#endif
    if ( procList->count() == 0 ) {
	QTimer::singleShot( 0, this, SLOT(removeMe()) );
    }
}

void QProcessManager::removeMe()
{
    QProcessPrivate::procManager = 0;
    qprocess_cleanup_procmanager.remove( this );
    delete this;
}

void QProcessManager::sigchldHnd( int fd )
{
    char tmp;
    ::read( fd, &tmp, sizeof(tmp) );
#if defined(QT_QPROCESS_DEBUG)
    qDebug( "QProcessManager::sigchldHnd()" );
#endif
    QProc *proc;
    QProcess *process;
    bool removeProc;
    proc = procList->first();
    while ( proc != 0 ) {
	removeProc = FALSE;
	process = proc->process;
	if ( process != 0 ) {
	    if ( !process->isRunning() ) {
#if defined(QT_QPROCESS_DEBUG)
		qDebug( "QProcessManager::sigchldHnd(): process exited (QProcess available)" );
#endif
		// read pending data
		process->socketRead( proc->socketStdout );
		process->socketRead( proc->socketStderr );

		if ( process->notifyOnExit )
		    emit process->processExited();

		removeProc = TRUE;
	    }
	} else {
	    int status;
	    if ( ::waitpid( proc->pid, &status, WNOHANG ) == proc->pid ) {
#if defined(QT_QPROCESS_DEBUG)
		qDebug( "QProcessManager::sigchldHnd(): process exited (QProcess not available)" );
#endif
		removeProc = TRUE;
	    }
	}
	if ( removeProc ) {
	    QProc *oldproc = proc;
	    proc = procList->next();
	    remove( oldproc );
	} else {
	    proc = procList->next();
	}
    }
}

#include "qprocess_unix.moc"


/***********************************************************************
 *
 * QProcessPrivate
 *
 **********************************************************************/
QProcessManager *QProcessPrivate::procManager = 0;

QProcessPrivate::QProcessPrivate()
{
#if defined(QT_QPROCESS_DEBUG)
    qDebug( "QProcessPrivate: Constructor" );
#endif
    stdinBufRead = 0;

    notifierStdin = 0;
    notifierStdout = 0;
    notifierStderr = 0;

    exitValuesCalculated = FALSE;

    proc = 0;
}

QProcessPrivate::~QProcessPrivate()
{
#if defined(QT_QPROCESS_DEBUG)
    qDebug( "QProcessPrivate: Destructor" );
#endif

    if ( proc != 0 ) {
	if ( proc->socketStdin != 0 ) {
	    ::close( proc->socketStdin );
	    proc->socketStdin = 0;
	}
	proc->process = 0;
    }

    while ( !stdinBuf.isEmpty() ) {
	delete stdinBuf.dequeue();
    }
    delete notifierStdin;
    delete notifierStdout;
    delete notifierStderr;
}

/*
  Closes all open sockets in the child process that are not needed by the child
  process. Otherwise one child may have an open socket on standard input, etc.
  of another child.
*/
void QProcessPrivate::closeOpenSocketsForChild()
{
    if ( procManager != 0 ) {
	if ( procManager->sigchldFd[0] != 0 )
	    ::close( procManager->sigchldFd[0] );
	if ( procManager->sigchldFd[1] != 0 )
	    ::close( procManager->sigchldFd[1] );

	// close also the sockets from other QProcess instances
	QProc *proc;
	for ( proc=procManager->procList->first(); proc!=0; proc=procManager->procList->next() ) {
	    ::close( proc->socketStdin );
	    ::close( proc->socketStdout );
	    ::close( proc->socketStderr );
	}
    }
}

void QProcessPrivate::newProc( pid_t pid, QProcess *process )
{
    proc = new QProc( pid, process );
    if ( procManager == 0 ) {
	procManager = new QProcessManager;
    }
    // the QProcessManager takes care of deleting the QProc instances
    procManager->append( proc );
}

/***********************************************************************
 *
 * sigchld handler callback
 *
 **********************************************************************/
QT_SIGNAL_RETTYPE qt_C_sigchldHnd( QT_SIGNAL_ARGS )
{
    if ( QProcessPrivate::procManager == 0 )
	return;
    if ( QProcessPrivate::procManager->sigchldFd[0] == 0 )
	return;

    char a = 1;
    ::write( QProcessPrivate::procManager->sigchldFd[0], &a, sizeof(a) );
}


/***********************************************************************
 *
 * QProcess
 *
 **********************************************************************/
/*!
  This private class does basic initialization.
*/
void QProcess::init()
{
    d = new QProcessPrivate();
    exitStat = 0;
    exitNormal = FALSE;
}

/*!
  This private class resets the process variables, etc. so that it can be used
  for another process to start.
*/
void QProcess::reset()
{
    delete d;
    d = new QProcessPrivate();
    exitStat = 0;
    exitNormal = FALSE;
    d->bufStdout.resize( 0 );
    d->bufStderr.resize( 0 );
}

QByteArray* QProcess::bufStdout()
{
    if ( d->proc && d->proc->socketStdout ) {
	socketRead( d->proc->socketStdout );
    }
    return &d->bufStdout;
}

QByteArray* QProcess::bufStderr()
{
    if ( d->proc && d->proc->socketStderr ) {
	socketRead( d->proc->socketStderr );
    }
    return &d->bufStderr;
}

void QProcess::consumeBufStdout( int consume )
{
    uint n = d->bufStdout.size();
    if ( consume==-1 || (uint)consume >= n ) {
	d->bufStdout.resize( 0 );
    } else {
	QByteArray tmp( n - consume );
	memcpy( tmp.data(), d->bufStdout.data()+consume, n-consume );
	d->bufStdout = tmp;
    }
}

void QProcess::consumeBufStderr( int consume )
{
    uint n = d->bufStderr.size();
    if ( consume==-1 || (uint)consume >= n ) {
	d->bufStderr.resize( 0 );
    } else {
	QByteArray tmp( n - consume );
	memcpy( tmp.data(), d->bufStderr.data()+consume, n-consume );
	d->bufStderr = tmp;
    }
}

/*!
  Destroys the class.

  If the process is running, it is NOT terminated! Standard input, standard
  output and standard error of the process are closed.

  \sa hangUp() kill()
*/
QProcess::~QProcess()
{
    delete d;
}

/*!
  Tries to run a process for the command and arguments that were specified with
  setArguments(), addArgument() or that were specified in the constructor. The
  command is searched in the path for executable programs; you can also use an
  absolute path to the command.

  Returns TRUE if the process could be started, otherwise FALSE.

  You can write data to standard input of the process with
  writeToStdin(), you can close standard input with closeStdin() and you can
  terminate the process hangUp() resp. kill().

  You can call this function even when there already is a running
  process in this object. In this case, QProcess closes standard input
  of the old process and deletes pending data, i.e., you loose all
  control over that process, but the process is not terminated. This applies
  also if the process could not be started. (On operating systems that have
  zombie processes, Qt will also wait() on the old process.)

  \sa launch() closeStdin()
*/
bool QProcess::start()
{
#if defined(QT_QPROCESS_DEBUG)
    qDebug( "QProcess::start()" );
#endif
    reset();

    int sStdin[2];
    int sStdout[2];
    int sStderr[2];

    // open sockets for piping
    if ( ::socketpair( AF_UNIX, SOCK_STREAM, 0, sStdin ) ) {
	return FALSE;
    }
    if ( ::socketpair( AF_UNIX, SOCK_STREAM, 0, sStderr ) ) {
	return FALSE;
    }
    if ( ::socketpair( AF_UNIX, SOCK_STREAM, 0, sStdout ) ) {
	return FALSE;
    }

    // the following pipe is only used to determine if the process could be
    // started
    int fd[2];
    if ( pipe( fd ) < 0 ) {
	// non critical error, go on
	fd[0] = 0;
	fd[1] = 0;
    }

    // construct the arguments for exec
    QCString *arglistQ = new QCString[ _arguments.count() + 1 ];
    const char** arglist = new const char*[ _arguments.count() + 1 ];
    int i = 0;
    for ( QStringList::Iterator it = _arguments.begin(); it != _arguments.end(); ++it ) {
	arglistQ[i] = (*it).local8Bit();
	arglist[i] = arglistQ[i];
#if defined(QT_QPROCESS_DEBUG)
	qDebug( "QProcess::start(): arg %d = %s", i, arglist[i] );
#endif
	i++;
    }
    arglist[i] = 0;

    // fork and exec
    QApplication::flushX();
    pid_t pid = fork();
    if ( pid == 0 ) {
	// child
	d->closeOpenSocketsForChild();
	::close( sStdin[1] );
	::close( sStdout[0] );
	::close( sStderr[0] );
	::dup2( sStdin[0], STDIN_FILENO );
	::dup2( sStdout[1], STDOUT_FILENO );
	::dup2( sStderr[1], STDERR_FILENO );
#ifndef QT_NO_DIR
	::chdir( workingDir.absPath().latin1() );
#endif
	if ( fd[0] )
	    ::close( fd[0] );
	if ( fd[1] )
	    ::fcntl( fd[1], F_SETFD, FD_CLOEXEC ); // close on exec shows sucess
	::execvp( arglist[0], (char*const*)arglist ); // ### cast not nice
	if ( fd[1] ) {
	    char buf = 0;
	    ::write( fd[1], &buf, 1 );
	    ::close( fd[1] );
	}
	::exit( -1 );
    } else if ( pid == -1 ) {
	// error forking
	goto error;
    }
    // test if exec was successful
    if ( fd[1] )
	close( fd[1] );
    if ( fd[0] ) {
	char buf;
	while ( TRUE ) {
	    int n = ::read( fd[0], &buf, 1 );
	    if ( n==1 ) {
		// socket was not closed => error
		goto error;
	    } else if ( n==-1 ) {
		if ( errno==EAGAIN || errno==EINTR )
		    // try it again
		    continue;
	    }
	    break;
	}
    }


    ::close( sStdin[0] );
    ::close( sStdout[1] );
    ::close( sStderr[1] );

    d->newProc( pid, this );
    d->proc->socketStdin = sStdin[1];
    d->proc->socketStdout = sStdout[0];
    d->proc->socketStderr = sStderr[0];

    // setup notifiers for the sockets
    d->notifierStdin = new QSocketNotifier( sStdin[1], QSocketNotifier::Write );
    d->notifierStdout = new QSocketNotifier( sStdout[0], QSocketNotifier::Read );
    d->notifierStderr = new QSocketNotifier( sStderr[0], QSocketNotifier::Read );
    connect( d->notifierStdin, SIGNAL(activated(int)),
	    this, SLOT(socketWrite(int)) );
    connect( d->notifierStdout, SIGNAL(activated(int)),
	    this, SLOT(socketRead(int)) );
    connect( d->notifierStderr, SIGNAL(activated(int)),
	    this, SLOT(socketRead(int)) );
    if ( !d->stdinBuf.isEmpty() ) {
	d->notifierStdin->setEnabled( TRUE );
    }
    if ( ioRedirection ) {
	d->notifierStdout->setEnabled( TRUE );
	d->notifierStderr->setEnabled( TRUE );
    }

    // cleanup and return
    delete[] arglistQ;
    delete[] arglist;
    return TRUE;

error:
#if defined(QT_QPROCESS_DEBUG)
    qDebug( "QProcess::start(): error starting process" );
#endif
    ::close( sStdin[1] );
    ::close( sStdout[0] );
    ::close( sStderr[0] );
    ::close( sStdin[0] );
    ::close( sStdout[1] );
    ::close( sStderr[1] );
    ::close( fd[0] );
    ::close( fd[1] );
    delete[] arglistQ;
    delete[] arglist;
    return FALSE;
}


/*!
  Asks the process to terminate. Processes can ignore this wish. If you want to
  be sure that the process really terminates, you must use kill()
  instead.

  The function returns immediately: it does not wait until the process has
  finished. When the process really exited, the signal processExited() is
  emitted.

  \sa kill() processExited()
*/
void QProcess::hangUp() const
{
    if ( d->proc != 0 )
	::kill( d->proc->pid, SIGHUP );
}

/*!
  Terminates the process. This is not a safe way to end a process since the
  process will not be able to do cleanup. hangUp() is the saver way to do it,
  but processes might ignore a hangUp().

  The function returns immediately: it does not wait until the process has
  finished. When the process really exited, the signal processExited() is
  emitted.

  \sa hangUp() processExited()
*/
void QProcess::kill() const
{
    if ( d->proc != 0 )
	::kill( d->proc->pid, SIGKILL );
}

/*!
  Returns TRUE if the process is running, otherwise FALSE.

  \sa normalExit() exitStatus() processExited()
*/
bool QProcess::isRunning() const
{
    if ( d->exitValuesCalculated ) {
#if defined(QT_QPROCESS_DEBUG)
	qDebug( "QProcess::isRunning(): FALSE (already computed)" );
#endif
	return FALSE;
    }
    if ( d->proc == 0 )
	return FALSE;
    int status;
    if ( ::waitpid( d->proc->pid, &status, WNOHANG ) == d->proc->pid )
    {
	// compute the exit values
	QProcess *that = (QProcess*)this; // mutable
	that->exitNormal = WIFEXITED( status ) != 0;
	if ( exitNormal ) {
	    that->exitStat = WEXITSTATUS( status );
	}
	d->exitValuesCalculated = TRUE;
#if defined(QT_QPROCESS_DEBUG)
	qDebug( "QProcess::isRunning(): FALSE" );
#endif
	return FALSE;
    }
#if defined(QT_QPROCESS_DEBUG)
    qDebug( "QProcess::isRunning(): TRUE" );
#endif
    return TRUE;
}

/*!
  Writes the data \a buf to the standard input of the process. The process may
  or may not read this data.

  This function returns immediately; the QProcess class might write the data at
  a later point (you have to enter the event loop for that). When all the data
  is written to the process, the signal wroteToStdin() is emitted. This does
  not mean that the process really read the data, since this class only detects
  when it was able to write the data to the operating system.

  \sa wroteToStdin() closeStdin() readStdout() readStderr()
*/
void QProcess::writeToStdin( const QByteArray& buf )
{
#if defined(QT_QPROCESS_DEBUG)
//    qDebug( "QProcess::writeToStdin(): write to stdin (%d)", d->socketStdin );
#endif
    d->stdinBuf.enqueue( new QByteArray(buf) );
    if ( d->notifierStdin != 0 )
	d->notifierStdin->setEnabled( TRUE );
}


/*!
  Closes standard input of the process.

  This function also deletes pending data that is not written to standard input
  yet.

  \sa wroteToStdin()
*/
void QProcess::closeStdin()
{
    if ( d->proc == 0 )
	return;
    if ( d->proc->socketStdin !=0 ) {
	while ( !d->stdinBuf.isEmpty() ) {
	    delete d->stdinBuf.dequeue();
	}
	delete d->notifierStdin;
	d->notifierStdin = 0;
	if ( ::close( d->proc->socketStdin ) != 0 ) {
	    qWarning( "Could not close stdin of child process" );
	}
#if defined(QT_QPROCESS_DEBUG)
	qDebug( "QProcess::closeStdin(): stdin (%d) closed", d->proc->socketStdin );
#endif
	d->proc->socketStdin = 0;
    }
}


/*
  This private slot is called when the process has outputted data to either
  standard output or standard error.
*/
void QProcess::socketRead( int fd )
{
#if defined(QT_QPROCESS_DEBUG)
    qDebug( "QProcess::socketRead(): %d", fd );
#endif
    if ( fd == 0 )
	return;
    const int bufsize = 4096;
    QByteArray *buffer = 0;
    uint oldSize;
    int n;
    if ( fd == d->proc->socketStdout ) {
	buffer = &d->bufStdout;
    } else if ( fd == d->proc->socketStderr ) {
	buffer = &d->bufStderr;
    } else {
	// this case should never happen, but just to be safe
	return;
    }

    // read data
    oldSize = buffer->size();
    buffer->resize( oldSize + bufsize );
    n = ::read( fd, buffer->data()+oldSize, bufsize );
    if ( n > 0 )
	buffer->resize( oldSize + n );
    else
	buffer->resize( oldSize );
    // eof or error?
    if ( n == 0 || n == -1 ) {
	if ( fd == d->proc->socketStdout ) {
#if defined(QT_QPROCESS_DEBUG)
	    qDebug( "QProcess::socketRead(): stdout (%d) closed", fd );
#endif
	    d->notifierStdout->setEnabled( FALSE );
	    delete d->notifierStdout;
	    d->notifierStdout = 0;
	    ::close( d->proc->socketStdout );
	    d->proc->socketStdout = 0;
	    return;
	} else if ( fd == d->proc->socketStderr ) {
#if defined(QT_QPROCESS_DEBUG)
	    qDebug( "QProcess::socketRead(): stderr (%d) closed", fd );
#endif
	    d->notifierStderr->setEnabled( FALSE );
	    delete d->notifierStderr;
	    d->notifierStderr = 0;
	    ::close( d->proc->socketStderr );
	    d->proc->socketStderr = 0;
	    return;
	}
    }
    // read all data that is available
    while ( n == bufsize ) {
	oldSize = buffer->size();
	buffer->resize( oldSize + bufsize );
	n = ::read( fd, buffer->data()+oldSize, bufsize );
	if ( n > 0 )
	    buffer->resize( oldSize + n );
    }

    if ( fd == d->proc->socketStdout ) {
#if defined(QT_QPROCESS_DEBUG)
	qDebug( "QProcess::socketRead(): %d bytes read from stdout (%d)",
		buffer->size()-oldSize, fd );
#endif
	emit readyReadStdout();
    } else if ( fd == d->proc->socketStderr ) {
#if defined(QT_QPROCESS_DEBUG)
	qDebug( "QProcess::socketRead(): %d bytes read from stderr (%d)",
		buffer->size()-oldSize, fd );
#endif
	emit readyReadStderr();
    }
}


/*
  This private slot is called when the process tries to read data from standard
  input.
*/
void QProcess::socketWrite( int fd )
{
    if ( fd != d->proc->socketStdin || d->proc->socketStdin == 0 )
	return;
    if ( d->stdinBuf.isEmpty() ) {
	d->notifierStdin->setEnabled( FALSE );
	return;
    }
#if defined(QT_QPROCESS_DEBUG)
    qDebug( "QProcess::socketWrite(): write to stdin (%d)", fd );
#endif
    ssize_t ret = ::write( fd,
	    d->stdinBuf.head()->data() + d->stdinBufRead,
	    d->stdinBuf.head()->size() - d->stdinBufRead );
    if ( ret > 0 )
	d->stdinBufRead += ret;
    if ( d->stdinBufRead == (ssize_t)d->stdinBuf.head()->size() ) {
	d->stdinBufRead = 0;
	delete d->stdinBuf.dequeue();
	if ( wroteToStdinConnected && d->stdinBuf.isEmpty() )
	    emit wroteToStdin();
	socketWrite( fd );
    }
}

/*
  This private slot is only used under Windows (but moc does not know about #if
  defined()).
*/
void QProcess::timeout()
{
}


/*
  This private function is used by connectNotify() and disconnectNotify() to
  change the value of ioRedirection (and related behaviour)
*/
void QProcess::setIoRedirection( bool value )
{
    ioRedirection = value;
    if ( ioRedirection ) {
	if ( d->notifierStdout )
	    d->notifierStdout->setEnabled( TRUE );
	if ( d->notifierStderr )
	    d->notifierStderr->setEnabled( TRUE );
    } else {
	if ( d->notifierStdout )
	    d->notifierStdout->setEnabled( FALSE );
	if ( d->notifierStderr )
	    d->notifierStderr->setEnabled( FALSE );
    }
}

/*
  This private function is used by connectNotify() and
  disconnectNotify() to change the value of notifyOnExit (and related
  behaviour)
*/
void QProcess::setNotifyOnExit( bool value )
{
    notifyOnExit = value;
}

/*
  This private function is used by connectNotify() and disconnectNotify() to
  change the value of wroteToStdinConnected (and related behaviour)
*/
void QProcess::setWroteStdinConnected( bool value )
{
    wroteToStdinConnected = value;
}

#endif // QT_NO_PROCESS
