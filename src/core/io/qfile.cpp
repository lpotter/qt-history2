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

#include "qfile.h"
#include <qplatformdefs.h>
#include <qglobal.h>
#include <private/qiodevice_p.h>
#include <qfileengine.h>
#include <qfileinfo.h>
#include <qtemporaryfile.h>
#include <qlist.h>

#include <errno.h>

#define d d_func()
#define q q_func()

//************* QFilePrivate
class QFilePrivate : public QIODevicePrivate
{
    Q_DECLARE_PUBLIC(QFile)

protected:
    QFilePrivate();
    ~QFilePrivate();

    bool openExternalFile(int flags, int fd);
    inline QFileEngine *getFileEngine() const { return static_cast<QFileEngine*>(q->ioEngine()); }

private:
    inline static QByteArray locale_encode(const QString &f) 
           { return f.toLocal8Bit(); }
    static QFile::EncoderFn encoder;
    inline static QString locale_decode(const QByteArray &f) 
           { return QString::fromLocal8Bit(f); }
    static QFile::DecoderFn decoder;
    
    QString fileName;
    mutable QFileEngine *fileEngine;
};

QFile::EncoderFn QFilePrivate::encoder = QFilePrivate::locale_encode;
QFile::DecoderFn QFilePrivate::decoder = QFilePrivate::locale_decode;

QFilePrivate::QFilePrivate() : fileEngine(0)
{ 
}

QFilePrivate::~QFilePrivate()
{
    delete fileEngine;
    fileEngine = 0;
}

bool
QFilePrivate::openExternalFile(int flags, int fd)
{
    Q_ASSERT(!fileEngine || !q->isOpen());
    delete fileEngine;
    QFSFileEngine *fe = new QFSFileEngine;
    fileEngine = fe;
    return fe->open(flags, fd);
}

//************* QFile

/*!
    \class QFile
    \reentrant
    \brief The QFile class is an I/O device that operates on files.

    \ingroup io
    \mainclass

    QFile is an I/O device for reading and writing binary and text
    files. A QFile may be used by itself or, more conveniently, with a
    QDataStream or QTextStream.

    The file name is usually passed in the constructor, but it can be
    changed with setFileName(). You can check for a file's existence with
    exists(), and remove a file with remove().

    The file is opened with open(), closed with close(), and flushed
    with flush(). Data is usually read and written using QDataStream
    or QTextStream, but you can read with readBlock() and readLine(),
    and write with writeBlock(). QFile also supports getch(), ungetch(),
    and putch().

    The size of the file is returned by size(). You can get the
    current file position or move to a new file position using the
    at() functions. If you've reached the end of the file, atEnd()
    returns true. The file handle is returned by handle().

    The following example uses QTextStream to read a text file
    line by line, printing each line with a line number:
    \code
    QStringList lines;
    QFile file("file.txt");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        QString line;
        int i = 1;
        while (!stream.atEnd()) {
            line = stream.readLine(); // line of text excluding '\n'
            printf("%3d: %s\n", i++, line.latin1());
            lines += line;
        }
        file.close();
    }
    \endcode

    Writing text is just as easy. The following example shows how to
    write the data we read in the previous example to a file:
    \code
    QFile file("file.txt");
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        QStringList::ConstIterator i = lines.constBegin();
        for (; i != lines.constEnd(); ++i)
            stream << *i << "\n";
        file.close();
    }
    \endcode

    The QFileInfo class holds detailed information about a file, such
    as access permissions, file dates and file types.

    The QDir class manages directories and lists of file names.

    When you use QFile, QFileInfo, and QDir to access the file system
    with Qt, you can use Unicode file names. On Unix, these file names
    are converted to an 8-bit encoding. If you want to do your own
    file I/O on Unix, you should convert file names using the
    encodeName() and decodeName() functions to convert the file name
    into the local encoding.

    The conversion scheme can be changed using setEncodingFunction().
    This might be useful if you wish to give the user an option to
    store file names in UTF-8, for example, but be aware that such file
    names would probably then be unrecognizable when seen by other
    programs.

    On Windows NT/2000, Unicode file names are supported
    directly in the file system and this function should be avoided.
    On Windows 95, non-Latin1 locales are not supported.

    \sa QDataStream, QTextStream
*/

/*!
    \enum QFile::PermissionSpec

    This enum is used by the permission() function to report the
    permissions and ownership of a file. The values may be OR-ed
    together to test multiple permissions and ownership values.

    \value ReadOwner The file is readable by the owner of the file.
    \value WriteOwner The file is writable by the owner of the file.
    \value ExeOwner The file is executable by the owner of the file.
    \value ReadUser The file is readable by the user.
    \value WriteUser The file is writable by the user.
    \value ExeUser The file is executable by the user.
    \value ReadGroup The file is readable by the group.
    \value WriteGroup The file is writable by the group.
    \value ExeGroup The file is executable by the group.
    \value ReadOther The file is readable by anyone.
    \value WriteOther The file is writable by anyone.
    \value ExeOther The file is executable by anyone.

    \warning The semantics of \c ReadUser, \c WriteUser and \c ExeUser are
    unfortunately not platform independent: on Unix, the rights of the owner of
    the file are returned and on Windows the rights of the current user are
    returned. This behavior might change in a future Qt version. If you want to
    find the rights of the owner of the file, you should use the flags \c
    ReadOwner, \c WriteOwner and \c ExeOwner. If you want to find out the
    rights of the current user, you should use isReadable(), isWritable() and
    isExecutable().
*/

/*!
    Constructs a QFile with no name.
*/
QFile::QFile() : QIODevice(*new QFilePrivate)
{
    setFlags(QIODevice::Direct);
    resetStatus();
}

/*!
    Constructs a QFile with a file name \a name.

    \sa setFileName()
*/
QFile::QFile(const QString &name) : QIODevice(*new QFilePrivate)
{
    d->fileName = name;
    setFlags(QIODevice::Direct);
    resetStatus();
}

/*!
    Destroys the file object, closing it if necessary.
*/
QFile::~QFile()
{
    close();
}

/*!
    Returns the name set by setFileName().

    \sa setFileName(), QFileInfo::fileName()
*/
QString
QFile::fileName() const
{
    return d->fileName;
}

/*!
    Sets the \a name of the file. The name can have no path, a
    relative path, or an absolute absolute path.

    Do not call this function if the file has already been opened.

    If the file name has no path or a relative path, the path used
    will be the application's current directory path
    \e{at the time of the open()} call.

    Example:
    \code
        QFile file;
        QDir::setCurrent("/tmp");
        file.setFileName("readme.txt");
        QDir::setCurrent("/home");
        file.open(QIODevice::ReadOnly);      // opens "/home/readme.txt" under Unix
    \endcode

    Note that the directory separator "/" works for all operating
    systems supported by Qt.

    \sa fileName(), QFileInfo, QDir
*/
void
QFile::setFileName(const QString &name)
{
    if(d->fileEngine)
        d->fileEngine->setFileName(name);
    d->fileName = name;
}

/*!
    \fn QString QFile::decodeName(const char *localFileName)

    \overload

    Returns the Unicode version of the given \a localFileName. See
    encodeName() for details.
*/

/*!
    By default, this function converts \a fileName to the local 8-bit
    encoding determined by the user's locale. This is sufficient for
    file names that the user chooses. File names hard-coded into the
    application should only use 7-bit ASCII filename characters.

    \sa decodeName() setEncodingFunction()
*/

QByteArray
QFile::encodeName(const QString &fileName)
{
    return (*QFilePrivate::encoder)(fileName);
}

/*!
    \enum QFile::EncoderFn

    This is used by QFile::setEncodingFunction() to specify how Unicode
    file names are converted to the appropriate local encoding.
*/


/*!
    This does the reverse of QFile::encodeName() using \a localFileName.

    \sa setDecodingFunction()
*/

QString
QFile::decodeName(const QByteArray &localFileName)
{
    return (*QFilePrivate::decoder)(localFileName);
}

/*!
    \fn void QFile::setEncodingFunction(EncoderFn function)

    \nonreentrant

    Sets the \a function for encoding Unicode file names. The
    default encodes in the locale-specific 8-bit encoding.

    \sa encodeName()
*/

void
QFile::setEncodingFunction(EncoderFn f)
{
    QFilePrivate::encoder = f;
}

/*!
    \enum QFile::DecoderFn

    This is used by QFile::setDecodingFunction() to specify how file names
    are converted from the local encoding to Unicode.
*/

/*!
    \fn void QFile::setDecodingFunction(DecoderFn function)

    \nonreentrant

    Sets the \a function for decoding 8-bit file names. The
    default uses the locale-specific 8-bit encoding.

    \sa encodeName(), decodeName()
*/

void
QFile::setDecodingFunction(DecoderFn f)
{
    QFilePrivate::decoder = f;
}

/*!
    \overload

    Returns true if the file specified by fileName() exists; otherwise
    returns false.

    \sa fileName() setFileName()
*/

bool
QFile::exists() const
{
    return (d->getFileEngine()->fileFlags(QFileEngine::FlagsMask) & QFileEngine::ExistsFlag);
}

/*!
    Returns true if the file specified by \a fileName exists; otherwise
    returns false.
*/

bool
QFile::exists(const QString &fileName)
{
    return QFileInfo(fileName).exists();    
}

/*!
    Removes the file specified by fileName(). Returns true if successful;
    otherwise returns false.

    The file is closed before it is removed.

    \sa setFileName()
*/

bool
QFile::remove()
{
    if (d->fileName.isEmpty()) {
        qWarning("QFile::remove: Empty or null file name");
        return false;
    }
    close();
    if(status() == QIODevice::Ok) {
        if(d->getFileEngine()->remove()) {
            resetStatus();
            return true;
        }
        setStatus(QIODevice::RemoveError, errno);
    }
    return false;
}

/*!
    \overload

    Removes the file specified by the \a fileName given.

    Returns true if successful; otherwise returns false.

    \sa remove()
*/

bool
QFile::remove(const QString &fileName)
{
    return QFile(fileName).remove();
}

/*!
    Renames the file currently specified by fileName() to \a newName.
    Returns true if successful; otherwise returns false.

    The file is closed before it is renamed.

    \sa setFileName()
*/

bool
QFile::rename(const QString &newName)
{
    if (d->fileName.isEmpty()) {
        qWarning("QFile::remove: Empty or null file name");
        return false;
    }
    close();
    if(status() == QIODevice::Ok) {
        if(d->getFileEngine()->rename(newName)) {
            resetStatus();
            return true;
        }
        setStatus(QIODevice::RenameError, errno);
    }
    return false;
}

/*!
    \overload

    Renames the file \a oldName to \a newName. Returns true if
    successful; otherwise returns false.

    \sa rename()
*/

bool
QFile::rename(const QString &oldName, const QString &newName)
{
    return QFile(oldName).rename(newName);
}


/*!
    Copies the file currently specified by fileName() to \a newName.
    Returns true if successful; otherwise returns false.

    The file is closed before it is copied.

    \sa setFileName()
*/

bool 
QFile::copy(const QString &newName)
{
    if (d->fileName.isEmpty()) {
        qWarning("QFile::copy: Empty or null file name");
        return false;
    }
    close();
    if(status() == QIODevice::Ok) {
        bool error = false;
        if(!open(QFile::ReadOnly)) {
            error = true;
            setStatus(QIODevice::CopyError, QString("Cannot open %1 for input").arg(d->fileName));
        } else {
            QTemporaryFile out;
            if(!out.open()) {
                close();
                error = true;
                setStatus(QIODevice::CopyError, "Cannot open for output");
            } else {
                char block[1024];
                while(!atEnd()) {
                    Q_LONG in = readBlock(block, 1024);
                    if(in == -1)
                        break;
                    if(in != out.writeBlock(block, in)) {
                        setStatus(QIODevice::CopyError, "Failure to write block");
                        error = true;
                        break;
                    }
                }
                if(!error && !QFile::rename(out.fileName(), newName)) {
                    error = true;
                    setStatus(QIODevice::CopyError, QString("Cannot create %1 for output").arg(newName));
                }
            }
        }
        if(!error) {
            QFile::setPermissions(newName, permissions());
            resetStatus();
            return true;
        }
    }
    return false;
}

/*!
    \overload

    Copies the file \a fileName to \a newName. Returns true if successful;
    otherwise returns false.

    \sa rename()
*/

bool 
QFile::copy(const QString &fileName, const QString &newName)
{
    return QFile(fileName).copy(newName);
}

/*!
    \overload

    Opens the existing file handle \a fh in the given \a mode.
    Returns true if successful; otherwise returns false.

    Example:
    \code
        #include <stdio.h>

        void printError(const char* msg)
        {
            QFile file;
            file.open(QIODevice::WriteOnly, stderr);
            file.writeBlock(msg, qstrlen(msg));        // write to stderr
            file.close();
        }
    \endcode

    When a QFile is opened using this function, close() does not actually
    close the file, but only flushes it.

    \warning If \a fh is \c stdin, \c stdout, or \c stderr, you may not be
    able to seek(). See QIODevice::isSequentialAccess() for more
    information.

    \sa close()
*/

bool
QFile::open(int mode, FILE *fh)
{
    return open(mode, QT_FILENO(fh));
}

/*!
    \overload

    Opens the existing file descripter \a fd in the given \a mode.
    Returns true if successful; otherwise returns false.

    When a QFile is opened using this function, close() does not
    actually close the file.

    The QFile that is opened using this function is automatically set
    to be in raw mode; this means that the file input/output functions
    are slow. If you run into performance issues, you should try to
    use one of the other open functions.

    \warning If \a fd is 0 (stdin), 1 (stdout), or 2 (stderr),
    you may not be able to seek(). size() is set to \c LLONG_MAX (in
    \c limits.h).

    \sa close()
*/

bool
QFile::open(int mode, int fd)
{
    if (isOpen()) {
        qWarning("QFile::open: File already open");
        return false;
    }
    if(mode & (Append|WriteOnly)) //append implies write
        mode |= WriteOnly;
    setFlags(QIODevice::Direct);
    resetStatus();
    setMode(mode);
    if (!(isReadable() || isWritable())) {
        qWarning("QFile::open: File access not specified");
        return false;
    }
    if(d->openExternalFile(flags(), fd)) {
        setState(QIODevice::Open);
        setMode(mode | QIODevice::Raw);
        if(d->getFileEngine()->isSequential())
            setType(QIODevice::Sequential);
        return true;
    }
    return false;
}


/*!
    \fn Q_LONG QFile::readLine(QString &string, Q_LONG maximum)

    Reads a line of text.

    Reads bytes from the file into the \a string until end-of-line or
    the \a maximum number bytes have been read, whichever occurs first.
    Returns the number of bytes read, or -1 if there was an error
    (e.g. end of file). Any terminating newline is not stripped.

    This function is only efficient for buffered files. Avoid using
    readLine() for files that have been opened with the \c QIODevice::Raw
    flag.

    Note that the string is read as plain Latin1 bytes, not Unicode.

    \sa readBlock(), QTextStream::readLine()
*/

Q_LONG
QFile::readLine(QString &s, Q_LONG maxlen)
{
    QByteArray ba;
    ba.resize(maxlen);
    Q_LONG l = readLine(ba.data(), maxlen);
    if (l >= 0)
        s = QString::fromLatin1(ba);
    return l;
}

/*!
  Returns the file handle of the file.

  This is a small positive integer, suitable for use with C library
  functions such as fdopen() and fcntl(). On systems that use file
  descriptors for sockets (i.e. Unix systems, but not Windows) the handle
  can be used with QSocketNotifier as well.

  If the file is not open, or there is an error, handle() returns -1.

  \sa QSocketNotifier
*/

int
QFile::handle() const
{
    if (!isOpen())
        return -1;
    QFileEngine *engine = d->getFileEngine();
    if(engine->type() == QFileEngine::File)
        return static_cast<QFSFileEngine*>(engine)->handle();
    return -1;
}

/*!
  \reimp
*/
QIOEngine 
*QFile::ioEngine() const
{
    if(!d->fileEngine) 
        d->fileEngine = QFileEngine::createFileEngine(d->fileName);
    return d->fileEngine;
}

/*!
    \fn QString QFile::name() const

    Use fileName() instead.
*/

/*!
    \fn void QFile::setName(const QString &name)

    Use setFileName() instead.
*/

/*!
    Sets the file size (in bytes) \a sz. Returns true if the file if the
    resize succeeds; false otherwise. If \a sz is larger than the file
    currently is the new bytes will be set to 0, if \a sz is smaller the
    file is simply truncated.

    \sa QFile::size(), setFileName()
*/

bool 
QFile::resize(QIODevice::Offset sz)
{
    if(d->getFileEngine()->setSize(sz)) {
        resetStatus();
        return true;
    }
    setStatus(QIODevice::ResizeError, errno);
    return false;
}

/*!
    \overload

    Sets \a fileName to size (in bytes) \a sz. Returns true if the file if
    the resize succeeds; false otherwise. If \a sz is larger than \a
    fileName currently is the new bytes will be set to 0, if \a sz is
    smaller the file is simply truncated.

    \sa resize()
*/

bool 
QFile::resize(const QString &fileName, QIODevice::Offset sz)
{
    return QFile(fileName).resize(sz);
}

/*!
    Returns the complete OR-ed together combination of
    QFile::PermissionSpec for the file.

    \sa QFile::setPermissions, QFile::PermissionSpec, setFileName()
*/

uint 
QFile::permissions() const
{
    return (d->getFileEngine()->fileFlags(QFileEngine::PermsMask) & QFileEngine::PermsMask);
}

/*!
    \overload

    Returns the complete OR-ed together combination of
    QFile::PermissionSpec for \a fileName.

    \sa permissions(), QFile::PermissionSpec
*/

uint 
QFile::permissions(const QString &fileName)
{
    return QFile(fileName).permissions();
}

/*!
    Sets the permissions for the file to \a permissionSpec. The
    permissionSpec argument can be several flags of type \c
    QFile::PermissionSpec OR-ed together to set the file to.

    \sa permissions(), QFile::PermissionSpec, setFileName()
*/

bool 
QFile::setPermissions(uint permissionSpec)
{
    if(d->getFileEngine()->chmod(permissionSpec)) {
        resetStatus();
        return true;
    }
    setStatus(QIODevice::PermissionsError, errno);
    return false;
}

/*!
    \overload

    Sets the permissions for \a fileName file to \a permissionSpec. The
    permissionSpec argument can be several flags of type \c
    QFile::PermissionSpec OR-ed together to set the file to.

    \sa setPermissions()
*/

bool 
QFile::setPermissions(const QString &fileName, uint permissionSpec)
{
    return QFile(fileName).setPermissions(permissionSpec);
}

