#include <stdio.h>
#include <stdlib.h>

#include "qapplication.h"
#include "qprocess.h"


/*!
  \class QProcess qprocess.h

  \brief The QProcess class provides means to start external programs and
  control their behaviour.

  \ingroup ?

  A QProcess allows you to start a external program, control its input and
  output, etc.
*/

/*!
  Constructs a QProcess.
*/
QProcess::QProcess( QObject *parent, const char *name )
    : QObject( parent, name )
{
    d = new QProcessPrivate;
}

/*!
  Constructs a QProcess with the given command (but does not start it).
*/
QProcess::QProcess( const QString& com, QObject *parent, const char *name )
    : QObject( parent, name )
{
    d = new QProcessPrivate;
    setCommand( com );
}

/*!
  Constructs a QProcess with the given command and arguments (but does not
  start it).
*/
QProcess::QProcess( const QString& com, const QStringList& args, QObject *parent, const char *name )
    : QObject( parent, name )
{
    d = new QProcessPrivate;
    setCommand( com );
    setArguments( args );
}

/*!
  Destructor; if the process is running it is NOT terminated!
*/
QProcess::~QProcess()
{
    delete d;
}


/*!
  Set the command that should be executed.
*/
void QProcess::setCommand( const QString& com )
{
    d->command = com;
}

/*!
  Set the arguments for the command. Previous set arguments will get deleted
  first.
*/
void QProcess::setArguments( const QStringList& args )
{
    d->arguments = args;
}

/*!
  Add a argument to the end of the existing list of arguments.
*/
void QProcess::addArgument( const QString& arg )
{
    d->arguments.append( arg );
}

/*!
  Set a working directory in which the command is executed.
*/
void QProcess::setWorkingDirectory( const QDir& dir )
{
    d->workingDir = dir;
}


/*!
  \fn bool QProcess::start()

  Start the program.

  Return TRUE on success, otherwise FALSE.
*/
/*!
  \fn bool QProcess::hangUp()

  Ask the process to terminate. If this does not work you can try \l kill()
  instead.

  Return TRUE on success, otherwise FALSE.
*/
/*!
  \fn bool QProcess::kill()

  Terminate the process. This is not a safe way to end a process; you should
  try \l hangUp() first and use this function only if it failed.

  Return TRUE on success, otherwise FALSE.
*/
/*!
  \fn bool QProcess::isRunning()

  Return TRUE if the process is running, otherwise FALSE.
*/


/*!
  Return TRUE if the process has exited normally, otherwise FALSE.
*/
bool QProcess::normalExit()
{
    return d->exitNormal;
}

/*!
  Return the exit status of the process. This value is only valid if
  normalExit() is TRUE.
*/
int QProcess::exitStatus()
{
    return d->exitStat;
}


/*!
  \fn void QProcess::dataStdout( const QByteArray& buf )
  This signal is emitted if the process wrote data to stdout.
*/
/*!
  \fn void QProcess::dataStderr( const QByteArray& buf )
  This signal is emitted if the process wrote data to stderr.
*/
/*!
  \fn void QProcess::processExited()
  This signal is emitted if the process has exited.
*/
/*!
  \fn void QProcess::wroteStdin()
  This signal is emitted if the data send to stdin (via dataStdin()) was
  actually read by the process.
*/


/*!
  \fn void QProcess::dataStdin( const QByteArray& buf )
  Write data to the stdin of the process. The process may or may not read this
  data. If the data gets read, the signal wroteStdin() is emitted.
*/
/*!
  Write data to the stdin of the process. The string is handled as a text. So
  what is written to the stdin is the QString::latin1(). The process may or may
  not read this data. If the data gets read, the signal wroteStdin() is
  emitted.
*/
void QProcess::dataStdin( const QString& buf )
{
    QByteArray bbuf;
    bbuf.duplicate( buf.latin1(), buf.length() );
    dataStdin( bbuf );
}

/*!
  \fn void QProcess::closeStdin( )
  Close stdin.
*/


/*!
  \fn void QProcess::socketRead( int fd )
  The process has output data to either stdout or sderr.
*/

/*!
  \fn void QProcess::socketWrite( int fd )
  The process tries to read data from stdin.
*/
