/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qprocess.cpp#20 $
**
** Implementation of QProcess class
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

#include <stdio.h>
#include <stdlib.h>

#include "qprocess.h"

#ifndef QT_NO_PROCESS

#include "qapplication.h"


//#define QPROCESS_DEBUG


/*!
  \class QProcess qprocess.h

  \brief The QProcess class is used to start external programs and control
  their behavior.

  \ingroup misc

  You can write to standard input of the started program. You can read the
  output of the program on standard output and standard error. You get notified
  when the program exits.

  There are two different ways to run a process: If you use start(), you have
  full control over the process; you can write to the standard input via the
  writeToStdin() slots whenever you want, and you can close standard input via
  the closeStdin() slot.

  If you know the data that should be written to the standard input of the
  process already when you want to run the process, you can use the launch()
  functions instead. These functions take the data that should be written to
  standard input as an argument, write it to standard input and automatically
  close standard input if all data was written.

  If you use a launch() function to run the process, you should not use the
  slots writeToStdin() and closeStdin(), since the result is not well-defined.

  It is possible to use one instance to start more than one process. If the
  first process is not finished yet, you loose control of that process: all
  pending data that is not written yet is lost, you get not notified when the
  process finishes, etc. All signals, slots and functions affect only the
  process that was started last.

  Example: If you want to start the \c uic command (Qt commandline tool used
  with the Qt Designer) and make some operations on the output (the \c uic
  outputs the code it generates to standard output by default). Consider the
  case, that you want to start it with the command line options "-tr i18n" on
  the file "help dialog.ui" (notice the space in the filename). On the command
  line you would do that with 
  \code
  uic -tr i18n "help dialog.ui"
  \endcode

  A code snippet for this with the QProcess class could look like this:

  \code
  UicManager::UicManager()
  {
      proc = new QProcess( this );
      proc->addArgument( "uic" );
      proc->addArgument( "-tr" );
      proc->addArgument( "i18n" );
      proc->addArgument( "help dialog.ui" );
      connect( proc, SIGNAL(readyReadStdout()), this, SLOT(readFromStdout()) );
      proc->start();                                                          
  }

  void UicManager::readFromStdout()
  {
      // Do some processing.
      // Keep in mind that the data might be reported in chunks.
  }                                                                           
  \endcode

  The readyReadStdout() signal is emitted when there is new data on standard
  output. This happens asynchronous: you don't know if more data will arrive
  later. In the above example you could connect the processExited() signal to
  the slot UicManager::readFromStdout() instead. If you do so, you are sure
  that all data is available when the slot is called. On the other hand, you
  must wait until the process has finished before doing any processing. Which
  approach is best, depends highly on the requirements of your application.
*/

/*!
  Constructs a QProcess object. The parameters \a parent and \a name are passed
  to the QObject constructor.

  \sa setArguments() addArgument() start()
*/
QProcess::QProcess( QObject *parent, const char *name )
    : QObject( parent, name ), ioRedirection( FALSE ), notifyOnExit( FALSE ),
    wroteStdinConnected( FALSE )
{
    init();
}

/*!
  Constructs a QProcess with \a arg0 as the command to be executed. The
  parameters \a parent and \a name are passed to the QObject constructor.

  The process is not started. You have to call start() explicitly to start the
  process.

  \sa setArguments() addArgument() start()
*/
QProcess::QProcess( const QString& arg0, QObject *parent, const char *name )
    : QObject( parent, name ), ioRedirection( FALSE ), notifyOnExit( FALSE ),
    wroteStdinConnected( FALSE )
{
    init();
    addArgument( arg0 );
}

/*!
  Constructs a QProcess with \a args as the arguments of the process. The first
  element in the list is the command to be executed. The other elements in the
  list are the arguments to this command. The parameters \a parent and \a name
  are passed to the QObject constructor.

  The process is not started. You have to call start() explicitly to start the
  process.

  \sa setArguments() addArgument() start()
*/
QProcess::QProcess( const QStringList& args, QObject *parent, const char *name )
    : QObject( parent, name ), ioRedirection( FALSE ), notifyOnExit( FALSE ),
    wroteStdinConnected( FALSE )
{
    init();
    setArguments( args );
}


/*!
  Sets \a args as the arguments for the process. The first element in the list
  is the command to be executed. The other elements in the list are the
  arguments to this command.

  Arguments that were previously set, are deleted first.

  \sa addArgument()
*/
void QProcess::setArguments( const QStringList& args )
{
    arguments = args;
}

/*!
  Adds \a arg to the end of the existing list of arguments.

  The first element in the list of arguments is the command to be executed; the
  following elements are the arguments to this command.

  \sa setArguments()
*/
void QProcess::addArgument( const QString& arg )
{
    arguments.append( arg );
}

/*!
  Sets \a dir as the working directory for a process. This does not affect
  running processes; only processes that are started afterwards are affected.

  Setting the working directory is especially useful for processes that try to
  access files with relative filenames.

  \sa start()
*/
void QProcess::setWorkingDirectory( const QDir& dir )
{
    workingDir = dir;
}


/*!
  Returns TRUE if the process has exited normally, otherwise FALSE. This
  implies that this function returns FALSE if the process is running.

  \sa isRunning() exitStatus() processExited()
*/
bool QProcess::normalExit()
{
    // isRunning() has the side effect that it determines the exit status!
    if ( isRunning() )
	return FALSE;
    else
	return exitNormal;
}

/*!
  Returns the exit status of the process.

  If normalExit() is FALSE, this function returns 0. So you should check the
  return value of normalExit() before relying on this value.

  \sa normalExit() processExited()
*/
int QProcess::exitStatus()
{
    // isRunning() has the side effect that it determines the exit status!
    if ( isRunning() )
	return 0;
    else
	return exitStat;
}


/*!
  Reads the data that the process has written to standard output. When new data was
  written to standard output, the class emits the signal readyReadStdout().

  \sa readyReadStdout() readStderr() writeToStdin()
*/
QByteArray QProcess::readStdout()
{
    QByteArray buf = bufStdout;
    bufStdout.resize( 0 );
    return buf;
}

/*!
  Reads the data that the process has written to standard error. When new data was
  written to standard error, the class emits the signal readyReadStderr().

  \sa readyReadStderr() readStdout() writeToStdin()
*/
QByteArray QProcess::readStderr()
{
    QByteArray buf = bufStderr;
    bufStderr.resize( 0 );
    return buf;
}

/*!
  Runs the process and writes the data \a buf to standard input of the process.
  If all data is written to standard input, it closes standard input. The
  command is searched in the path for executable programs; you can also use an
  absolute path to the command.

  Returns TRUE if the process could be started, otherwise FALSE.

  Notice that you should not use the slots writeToStdin() and closeStdin() on
  processes started with launch(), since the result is not well-defined. If you
  need these slots, use start() instead.

  The process may or may not read this data.

  You can call this function when a process that was started with this instance
  still runs. In this case, it closes standard input of that process and it
  deletes pending data - you loose all control over that process, but the
  process is not terminated.

  \sa start()
*/
bool QProcess::launch( const QByteArray& buf )
{
    if ( start() ) {
	connect( this, SIGNAL(wroteStdin()),
		this, SLOT(closeStdinLaunch()) );
	writeToStdin( buf );
	return TRUE;
    } else {
	return FALSE;
    }
}

/*! \overload

  The data \a buf is written to standard input with writeToStdin(); so this
  function writes the QString::local8Bit() representation of the string.
*/
bool QProcess::launch( const QString& buf )
{
    if ( start() ) {
	connect( this, SIGNAL(wroteStdin()),
		this, SLOT(closeStdinLaunch()) );
	writeToStdin( buf );
	return TRUE;
    } else {
	return FALSE;
    }
}

/*!
  This slot is used by the launch() functions to close standard input.
*/
void QProcess::closeStdinLaunch()
{
    disconnect( this, SIGNAL(wroteStdin()),
	    this, SLOT(closeStdinLaunch()) );
    closeStdin();
}


/*!
  \fn void QProcess::readyReadStdout()

  This signal is emitted when the process wrote data to standard output.
  You can read the data with readStdout().

  \sa readStdout() readyReadStderr()
*/
/*!
  \fn void QProcess::readyReadStderr()

  This signal is emitted when the process wrote data to standard error.
  You can read the data with readStderr().

  \sa readStderr() readyReadStdout()
*/
/*!
  \fn void QProcess::processExited()

  This signal is emitted when the process has exited.

  \sa isRunning() normalExit() exitStatus()
*/
/*!
  \fn void QProcess::wroteStdin()

  This signal is emitted if the data send to standard input (via writeToStdin()) was
  actually written to the process. This does not imply that the process really
  read the data, but it is now save to close standard input without loosing
  pending data.

  \sa writeToStdin() closeStdin()
*/


/*! \overload

  The string \a buf is handled as a text. So what is written to standard input is the
  QString::local8Bit() representation.
*/
void QProcess::writeToStdin( const QString& buf )
{
    writeToStdin( buf.local8Bit() );
}


/*
 * Under Windows the implementation is not so nice: it is not that easy to
 * detect when one of the signals should be emitted; therefore there are some
 * timers that query the information.
 * To keep it a little efficient, use the timers only when they are needed.
 * They are needed, if you are interested in the signals. So use
 * connectNotify() and disconnectNotify() to keep track of your interest.
 */
/*!  \reimp
*/
void QProcess::connectNotify( const char * signal )
{
#if defined(QPROCESS_DEBUG)
    qDebug( "QProcess::connectNotify(): signal %s has been connected", signal );
#endif
    if ( !ioRedirection )
	if ( qstrcmp( signal, SIGNAL(readyReadStdout()) )==0 ||
		qstrcmp( signal, SIGNAL(readyReadStderr()) )==0
	   ) {
#if defined(QPROCESS_DEBUG)
	    qDebug( "QProcess::connectNotify(): set ioRedirection to TRUE" );
#endif
	    setIoRedirection( TRUE );
	    return;
	}
    if ( !notifyOnExit && qstrcmp( signal, SIGNAL(processExited()) )==0 ) {
#if defined(QPROCESS_DEBUG)
	qDebug( "QProcess::connectNotify(): set notifyOnExit to TRUE" );
#endif
	setNotifyOnExit( TRUE );
	return;
    }
    if ( !wroteStdinConnected && qstrcmp( signal, SIGNAL(wroteStdin()) )==0 ) {
#if defined(QPROCESS_DEBUG)
	qDebug( "QProcess::connectNotify(): set wroteStdinConnected to TRUE" );
#endif
	setWroteStdinConnected( TRUE );
	return;
    }
}

/*!  \reimp
*/
void QProcess::disconnectNotify( const char * )
{
    if ( ioRedirection &&
	    receivers( SIGNAL(readyReadStdout()) ) ==0 &&
	    receivers( SIGNAL(readyReadStderr()) ) ==0
	    ) {
#if defined(QPROCESS_DEBUG)
	qDebug( "QProcess::disconnectNotify(): set ioRedirection to FALSE" );
#endif
	setIoRedirection( FALSE );
    }
    if ( notifyOnExit && receivers( SIGNAL(processExited()) ) == 0 ) {
#if defined(QPROCESS_DEBUG)
	qDebug( "QProcess::disconnectNotify(): set notifyOnExit to FALSE" );
#endif
	setNotifyOnExit( FALSE );
    }
    if ( wroteStdinConnected && receivers( SIGNAL(wroteStdin()) ) == 0 ) {
#if defined(QPROCESS_DEBUG)
	qDebug( "QProcess::disconnectNotify(): set wroteStdinConnected to FALSE" );
#endif
	setWroteStdinConnected( FALSE );
    }
}

#endif // QT_NO_PROCESS
