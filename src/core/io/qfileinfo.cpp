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

#include "qfileinfo.h"
#include <qdatetime.h>
#include <qfileengine.h>
#include <qplatformdefs.h>
#include <qglobal.h>
#include <qatomic.h>
#include "qdir.h"

#define d d_func()
#define q q_func()

//************* QFileInfoPrivate
class QFileInfoPrivate
{
    QFileInfo *q_ptr;
    Q_DECLARE_PUBLIC(QFileInfo)

protected:
    QFileInfoPrivate(QFileInfo *, const QFileInfo *copy=0);
    ~QFileInfoPrivate();

    void initFileEngine(const QString &);

    uint getFileFlags(QFileEngine::FileFlags) const;
    QDateTime &getFileTime(QFileEngine::FileTime) const;
private:
    enum { CachedPerms=0x01, CachedTypes=0x02, CachedFlags=0x04,
           CachedMTime=0x10, CachedCTime=0x20, CachedATime=0x40,
           CachedSize =0x08 };
    struct Data {
        inline Data() : fileEngine(0), cache_enabled(1) { ref = 1; clear(); }
        inline ~Data() { delete fileEngine; }
        inline void clear() {
            fileFlags = 0;
            cached = 0;
        }
        mutable QAtomic ref;

        QFileEngine *fileEngine;
        mutable QString fileName;

        mutable uint cache_enabled : 1;
        mutable uint fileSize;
        mutable QDateTime fileTimes[3];
        mutable uint fileFlags;
        mutable uchar cached;
        inline bool getCached(uchar c) const { return cache_enabled ? (cached&c) : 0; }
        inline void setCached(uchar c) { if(cache_enabled) cached |= c; }
    } *data;
    inline void reset() {
        detach();
        data->clear();
    }
    void detach();
};

QFileInfoPrivate::QFileInfoPrivate(QFileInfo *qq, const QFileInfo *copy) : q_ptr(qq)
{
    if(copy) {
        ++copy->d->data->ref;
        data = copy->d->data;
    } else {
        data = new QFileInfoPrivate::Data;
        data->clear();
    }
}

QFileInfoPrivate::~QFileInfoPrivate()
{
    if (!--data->ref)
        delete data;
    data = 0;
    q_ptr = 0;
}

void
QFileInfoPrivate::initFileEngine(const QString &file)
{
    detach();
    delete data->fileEngine;
    data->fileEngine = 0;
    data->clear();
    data->fileEngine = QFileEngine::createFileEngine(file);
    data->fileName = file;
}

void
QFileInfoPrivate::detach()
{
    if (data->ref != 1) {
        QFileInfoPrivate::Data *x = data;
        data = new QFileInfoPrivate::Data;
        initFileEngine(x->fileName);
        --x->ref;
    }
}

uint
QFileInfoPrivate::getFileFlags(QFileEngine::FileFlags request) const
{
    QFileEngine::FileFlags flags = 0;
    if((request & QFileEngine::TypesMask) && !data->getCached(CachedTypes)) {
        data->setCached(CachedTypes);
        flags |= QFileEngine::TypesMask;
    }
    if((request & QFileEngine::PermsMask) && !data->getCached(CachedPerms)) {
        data->setCached(CachedPerms);
        flags |= QFileEngine::PermsMask;
    }
    if((request & QFileEngine::FlagsMask) && !data->getCached(CachedFlags)) {
        data->setCached(CachedFlags);
        flags |= QFileEngine::FlagsMask;
    }
    if(flags)
        data->fileFlags |= (data->fileEngine->fileFlags(flags) & flags);
    return data->fileFlags & request;
}

QDateTime
&QFileInfoPrivate::getFileTime(QFileEngine::FileTime request) const
{
    if(request == QFileEngine::CreationTime) {
        if(data->getCached(CachedCTime))
            return data->fileTimes[request];
        return (data->fileTimes[request] = data->fileEngine->fileTime(request));
    }
    if(request == QFileEngine::ModificationTime) {
        if(data->getCached(CachedMTime))
            return data->fileTimes[request];
        return (data->fileTimes[request] = data->fileEngine->fileTime(request));
    }
    if(request == QFileEngine::AccessTime) {
        if(data->getCached(CachedATime))
            return data->fileTimes[request];
        return (data->fileTimes[request] = data->fileEngine->fileTime(request));
    }
    return data->fileTimes[0]; //cannot really happen
}

//************* QFileInfo

/*!
    \class QFileInfo
    \reentrant
    \brief The QFileInfo class provides system-independent file information.

    \ingroup io

    QFileInfo provides information about a file's name and position
    (path) in the file system, its access rights and whether it is a
    directory or symbolic link, etc. The file's size and last
    modified/read times are also available.

    A QFileInfo can point to a file with either a relative or an
    absolute file path. Absolute file paths begin with the directory
    separator "/" (or with a drive specification on Windows). Relative
    file names begin with a directory name or a file name and specify
    a path relative to the current working directory. An example of an
    absolute path is the string "/tmp/quartz". A relative path might
    look like "src/fatlib". You can use the function isRelative() to
    check whether a QFileInfo is using a relative or an absolute file
    path. You can call the function makeAbsolute() to convert a
    relative QFileInfo's path to an absolute path.

    The file that the QFileInfo works on is set in the constructor or
    later with setFile(). Use exists() to see if the file exists and
    size() to get its size.

    To speed up performance, QFileInfo caches information about the
    file. Because files can be changed by other users or programs, or
    even by other parts of the same program, there is a function that
    refreshes the file information: refresh(). If you want to switch
    off a QFileInfo's caching and force it to access the file system
    every time you request information from it call setCaching(false).

    The file's type is obtained with isFile(), isDir() and
    isSymLink(). The readLink() function provides the name of the file
    the symlink points to.

    Elements of the file's name can be extracted with dirPath() and
    fileName(). The fileName()'s parts can be extracted with
    baseName() and extension().

    The file's dates are returned by created(), lastModified() and
    lastRead(). Information about the file's access permissions is
    obtained with isReadable(), isWritable() and isExecutable(). The
    file's ownership is available from owner(), ownerId(), group() and
    groupId(). You can examine a file's permissions and ownership in a
    single statement using the permission() function.

    If you need to read and traverse directories, see the QDir class.
*/

/*!
    Constructs a new empty QFileInfo.
*/

QFileInfo::QFileInfo() : d_ptr(new QFileInfoPrivate(this))
{
}

/*!
    Constructs a new QFileInfo that gives information about the given
    file. The \a file can also include an absolute or relative path.

    \sa setFile(), isRelative(), QDir::setCurrent(), QDir::isRelativePath()
*/

QFileInfo::QFileInfo(const QString &file) : d_ptr(new QFileInfoPrivate(this))
{
    d->initFileEngine(file);
}

/*!
    Constructs a new QFileInfo that gives information about file \a
    file.

    If the \a file has a relative path, the QFileInfo will also have a
    relative path.

    \sa isRelative()
*/

QFileInfo::QFileInfo(const QFile &file) : d_ptr(new QFileInfoPrivate(this))
{
    d->initFileEngine(file.fileName());
}

/*!
    Constructs a new QFileInfo that gives information about the given
    \a file in the directory \a dir.

    If \a dir has a relative path, the QFileInfo will also have a
    relative path.

    \sa isRelative()
*/

#ifndef QT_NO_DIR
QFileInfo::QFileInfo(const QDir &dir, const QString &file) : d_ptr(new QFileInfoPrivate(this))
{
    d->initFileEngine(dir.filePath(file));
}
#endif

/*!
    Constructs a new QFileInfo that is a copy of the given \a fileinfo.
*/

QFileInfo::QFileInfo(const QFileInfo &fileinfo) : d_ptr(new QFileInfoPrivate(this, &fileinfo))
{

}

/*!
    Destroys the QFileInfo and frees its resources.
*/


QFileInfo::~QFileInfo()
{
    delete d_ptr;
    d_ptr = 0;
}

/*!
    Compares two QFileInfo's for equality based on the two file
    locations pointing to the same place.

    \warning This will not compare two different symbolic links
    pointing to the same file.
*/

bool
QFileInfo::operator==(const QFileInfo &fileinfo)
{
    if(fileinfo.d->data == d->data)
        return true;
    if(!d->data->fileEngine || !fileinfo.d->data->fileEngine)
        return false;
    if(d->data->fileEngine->type() != fileinfo.d->data->fileEngine->type() ||
       d->data->fileEngine->caseSensitive() != fileinfo.d->data->fileEngine->caseSensitive())
        return false;
    if(fileinfo.size() == size()) { //if the size isn't the same...
        QString file1 = absoluteFilePath(),
                file2 = fileinfo.absoluteFilePath();
        if(file1.length() == file2.length()) {
            if(!fileinfo.d->data->fileEngine->caseSensitive()) {
                for(int i = 0; i < file1.length(); i++) {
                    if(file1.at(i).toLower() != file2.at(i).toLower())
                        return false;
                }
                return true;
            }
            return (file1 == file2);
        }
    }
    return false;
}

/*!
    Makes a copy of the given \a fileinfo and assigns it to this QFileInfo.
*/

QFileInfo
&QFileInfo::operator=(const QFileInfo &fileinfo)
{
    qAtomicAssign(d->data, fileinfo.d->data);
    return *this;
}

/*!
    Sets the file that the QFileInfo provides information about to \a
    file.

    The \a file can also include an absolute or relative file path.
    Absolute paths begin with the directory separator (e.g. "/" under
    Unix) or a drive specification (under Windows). Relative file
    names begin with a directory name or a file name and specify a
    path relative to the current directory.

    Example:
    \code
    QString absolute = "/local/bin";
    QString relative = "local/bin";
    QFileInfo absFile(absolute);
    QFileInfo relFile(relative);

    QDir::setCurrent(QDir::rootPath());
    // absFile and relFile now point to the same file

    QDir::setCurrent("/tmp");
    // absFile now points to "/local/bin",
    // while relFile points to "/tmp/local/bin"
    \endcode

    \sa isRelative(), QDir::setCurrent(), QDir::isRelativePath()
*/

void
QFileInfo::setFile(const QString &file)
{
    d->initFileEngine(file);
}

/*!
    \overload

    Sets the file that the QFileInfo provides information about to \a
    file.

    If \a file includes a relative path, the QFileInfo will also have
    a relative path.

    \sa isRelative()
*/

void
QFileInfo::setFile(const QFile &file)
{
    d->initFileEngine(file.fileName());
}

#ifndef QT_NO_DIR
/*!
    \overload

    Sets the file that the QFileInfo provides information about to \a
    file in directory \a dir.

    If \a file includes a relative path, the QFileInfo will also
    have a relative path.

    \sa isRelative()
*/

void
QFileInfo::setFile(const QDir &dir, const QString &file)
{
    d->initFileEngine(dir.filePath(file));
}
#endif

/*!
    Returns the absolute path including the file name.

    The absolute path name consists of the full path and the file
    name. On Unix this will always begin with the root, '/',
    directory. On Windows this will always begin 'D:/' where D is a
    drive letter, except for network shares that are not mapped to a
    drive letter, in which case the path will begin '//sharename/'.

    This function returns the same as filePath(), unless isRelative()
    is true.

    If the QFileInfo is empty it returns QDir::currentPath().

    This function can be time consuming under Unix (in the order of
    milliseconds).

    \sa isRelative(), filePath()
*/

QString
QFileInfo::absoluteFilePath() const
{
    if(!d->data->fileEngine)
        return QLatin1String("");
    return d->data->fileEngine->fileName(QFileEngine::AbsoluteName);
}

/*!
    Returns the canonical path, i.e. a path without symbolic links or
    redundant "." or ".." elements.

    On systems that do not have symbolic links this function will
    always return the same string that absoluteFilePath() returns. If
    the canonical path does not exist (normally due to dangling
    symbolic links) canonicalFilePath() returns an empty string.

    \sa filePath(), absoluteFilePath(), QString::isNull()
*/

QString
QFileInfo::canonicalFilePath() const
{
    if(!d->data->fileEngine)
        return QLatin1String("");
    return d->data->fileEngine->fileName(QFileEngine::CanonicalName);
}


/*!
    Returns the file's path absolute path.

    This does not include the filename. ###

    \sa dir(), filePath(), fileName(), isRelative(), path()
*/

QString
QFileInfo::absolutePath() const
{
    if(!d->data->fileEngine)
        return QLatin1String("");
    return d->data->fileEngine->fileName(QFileEngine::AbsolutePathName);
}

/*!
    Returns the canonical path, i.e. a path without symbolic links or
    redundant "." or ".." elements.

    On systems that do not have symbolic links this function will
    always return the same string that absoluteFilePath() returns. If the
    canonical path does not exist (normally due to dangling symbolic
    links) canonicalPath() returns an empty string.

    \sa absoluteFilePath(), QString::isNull()
*/

QString
QFileInfo::canonicalPath() const
{
    if(!d->data->fileEngine)
        return QLatin1String("");
    return d->data->fileEngine->fileName(QFileEngine::CanonicalPathName);
}


/*!
    Returns the file's path.

y    \sa dir(), filePath(), fileName(), isRelative(), absolutePath()
*/

QString
QFileInfo::path() const
{
    if(!d->data->fileEngine)
        return QLatin1String("");
    return d->data->fileEngine->fileName(QFileEngine::PathName);
}

/*!
    \fn bool QFileInfo::isAbsolute() const

    Returns true if the file path name is absolute, otherwise returns
    false if the path is relative.

    \sa isRelative()
*/

/*!
    Returns true if the file path name is relative, otherwise returns
    false if the path is absolute (e.g. under Unix a path is absolute
    if it begins with a "/").

    \sa isAbsolute()
*/

bool
QFileInfo::isRelative() const
{
    if(!d->data->fileEngine)
        return true;
    return d->data->fileEngine->isRelativePath();
}


/*!
    Converts the file's path to an absolute path.

    If it is already absolute, nothing is done.

    \sa filePath(), isRelative()
*/

bool
QFileInfo::makeAbsolute()
{
    if(!d->data->fileEngine || !d->data->fileEngine->isRelativePath())
        return false;
    QString absFileName = d->data->fileEngine->fileName(QFileEngine::AbsoluteName);
    d->detach();
    d->data->fileName = absFileName;
    d->data->fileEngine->setFileName(absFileName);
    return true;
}

/*!
    Returns true if the file exists; otherwise returns false.
*/

bool
QFileInfo::exists() const
{
    if(!d->data->fileEngine)
        return false;
    return d->getFileFlags(QFileEngine::ExistsFlag);
}

/*!
    Refreshes the information about the file, i.e. reads in information
    from the file system the next time a cached property is fetched.
*/

void
QFileInfo::refresh()
{
    d->reset();
}

/*!
    Returns the file name, including the path (which may be absolute
    or relative).

    \sa isRelative(), absoluteFilePath()
*/

QString
QFileInfo::filePath() const
{
    if(!d->data->fileEngine)
        return QLatin1String("");
    return d->data->fileEngine->fileName(QFileEngine::DefaultName);
}

/*!
    Returns the name of the file, excluding the path.

    Example:
    \code
        QFileInfo fi("/tmp/archive.tar.gz");
        QString name = fi.fileName();                // name = "archive.tar.gz"
    \endcode

    \sa isRelative(), filePath(), baseName(), extension()
*/

QString
QFileInfo::fileName() const
{
    if(!d->data->fileEngine)
        return QLatin1String("");
    return d->data->fileEngine->fileName(QFileEngine::BaseName);
}

/*!
    Returns the base name of the file without the path.

    The base name consists of all characters in the file up to (but
    not including) the \e first '.' character.

    Example:
    \code
        QFileInfo fi("/tmp/archive.tar.gz");
        QString base = fi.baseName();  // base = "archive"
    \endcode

    \sa fileName(), suffix(), completeSuffix(), completeBaseName()
*/

QString
QFileInfo::baseName() const
{
    if(!d->data->fileEngine)
        return QLatin1String("");
    return d->data->fileEngine->fileName(QFileEngine::BaseName).section(QLatin1Char('.'), 0, 0);
}

/*!
    Returns the complete base name of the file without the path.

    The complete base name consists of all characters in the file up
    to (but not including) the \e last '.' character.

    Example:
    \code
        QFileInfo fi("/tmp/archive.tar.gz");
        QString base = fi.completeBaseName();  // base = "archive.tar"
    \endcode

    \sa fileName(), suffix(), completeSuffix(), baseName()
*/

QString
QFileInfo::completeBaseName() const
{
    if(!d->data->fileEngine)
        return QLatin1String("");
    return d->data->fileEngine->fileName(QFileEngine::BaseName).section(QLatin1Char('.'), 0, -2);
}

/*!
    Returns the complete suffix of the file.

    The complete suffix consists of all characters in the file after
    (but not including) the first '.'.

    Example:
    \code
        QFileInfo fi("/tmp/archive.tar.gz");
        QString ext = fi.completeSuffix();  // ext = "tar.gz"
    \endcode

    \sa fileName(), suffix(), baseName(), completeBaseName()
*/

QString
QFileInfo::completeSuffix() const
{
    if(!d->data->fileEngine)
        return QLatin1String("");
    return d->data->fileEngine->fileName(QFileEngine::BaseName).section(QLatin1Char('.'), 1, -1);
}

/*!
    Returns the suffix of the file.

    The suffix consists of all characters in the file after (but not
    including) the last '.'.

    Example:
    \code
        QFileInfo fi("/tmp/archive.tar.gz");
        QString ext = fi.suffix();  // ext = "gz"
    \endcode

    \sa fileName(), completeSuffix(), baseName(), completeBaseName()
*/

QString
QFileInfo::suffix() const
{
    if(!d->data->fileEngine)
        return QLatin1String("");
    return d->data->fileEngine->fileName(QFileEngine::BaseName).section(QLatin1Char('.'), -1);
}

#ifndef QT_NO_DIR

/*!
    Returns the file's path as a QDir object.

    \sa dirPath(), filePath(), fileName(), isRelative(), absoluteDir()
*/

QDir
QFileInfo::dir() const
{
    return QDir(path());
}

/*!
    Returns the file's absolute path as a QDir object.

    \sa filePath(), fileName(), isRelative(), dir()
*/

QDir
QFileInfo::absoluteDir() const
{
    return QDir(absolutePath());
}

#ifdef QT_COMPAT
/*!
    Use absoluteDir() or the dir() overload that takes no parameters
    instead.
*/
QDir QFileInfo::dir(bool absPath) const
{
    if(absPath)
        return absoluteDir();
    return dir();
}
#endif //QT_COMPAT
#endif //QT_NO_DIR

/*!
    Returns true if the user can read the file; otherwise returns false.

    \sa isWritable(), isExecutable(), permission()
*/

bool
QFileInfo::isReadable() const
{
    if(!d->data->fileEngine)
        return false;
    return d->getFileFlags(QFileEngine::ReadUserPerm);
}

/*!
    Returns true if the user can write to the file; otherwise returns false.

    \sa isReadable(), isExecutable(), permission()
*/


bool
QFileInfo::isWritable() const
{
    if(!d->data->fileEngine)
        return false;
    return d->getFileFlags(QFileEngine::WriteUserPerm);
}

/*!
    Returns true if the file is executable; otherwise returns false.

    \sa isReadable(), isWritable(), permission()
*/

bool
QFileInfo::isExecutable() const
{
    if(!d->data->fileEngine)
        return false;
    return d->getFileFlags(QFileEngine::ExeUserPerm);
}

bool
QFileInfo::isHidden() const
{
    if(!d->data->fileEngine)
        return false;
    return d->getFileFlags(QFileEngine::HiddenFlag);
}

/*!
    Returns true if this object points to a file. Returns false if the
    object points to something which isn't a file, e.g. a directory or
    a symlink.

    \sa isDir(), isSymLink()
*/

bool
QFileInfo::isFile() const
{
    if(!d->data->fileEngine)
        return false;
    return d->getFileFlags(QFileEngine::FileType);
}

/*!
    Returns true if this object points to a directory or to a symbolic
    link to a directory; otherwise returns false.

    \sa isFile(), isSymLink()
*/

bool
QFileInfo::isDir() const
{
    if(!d->data->fileEngine)
        return false;
    return d->getFileFlags(QFileEngine::DirectoryType);
}

/*!
    Returns true if this object points to a symbolic link (or to a
    shortcut on Windows); otherwise returns false.

    \sa isFile(), isDir(), readLink()
*/

bool
QFileInfo::isSymLink() const
{
    if(!d->data->fileEngine)
        return false;
    return d->getFileFlags(QFileEngine::LinkType);
}

/*!
  Returns true if the object points to a directory or to a symbolic
  link to a directory, and that directory is the root directory; otherwise
  returns false.
*/

bool
QFileInfo::isRoot() const
{
    if (!d->data->fileEngine)
        return true;
    return d->getFileFlags(QFileEngine::RootFlag);
}

/*!
    Returns the name a symlink (or shortcut on Windows) points to, or
    a an empty string if the object isn't a symbolic link.

    This name may not represent an existing file; it is only a string.
    QFileInfo::exists() returns true if the symlink points to an
    existing file.

    \sa exists(), isSymLink(), isDir(), isFile()
*/

QString
QFileInfo::readLink() const
{
    if(!d->data->fileEngine)
        return QLatin1String("");
    return d->data->fileEngine->fileName(QFileEngine::LinkName);
}

/*!
    Returns the owner of the file. On systems where files
    do not have owners, or if an error occurs, an empty string is
    returned.

    This function can be time consuming under Unix (in the order of
    milliseconds).

    \sa ownerId(), group(), groupId()
*/

QString
QFileInfo::owner() const
{
    if(!d->data->fileEngine)
        return QLatin1String("");
    return d->data->fileEngine->owner(QFileEngine::OwnerUser);
}

/*!
    Returns the id of the owner of the file.

    On Windows and on systems where files do not have owners this
    function returns ((uint) -2).

    \sa owner(), group(), groupId()
*/

uint
QFileInfo::ownerId() const
{
    if(!d->data->fileEngine)
        return 0;
    return d->data->fileEngine->ownerId(QFileEngine::OwnerUser);
}

/*!
    Returns the group of the file. On Windows, on systems where files
    do not have groups, or if an error occurs, an empty string is
    returned.

    This function can be time consuming under Unix (in the order of
    milliseconds).

    \sa groupId(), owner(), ownerId()
*/

QString
QFileInfo::group() const
{
    if(!d->data->fileEngine)
        return QLatin1String("");
    return d->data->fileEngine->owner(QFileEngine::OwnerGroup);
}

/*!
    Returns the id of the group the file belongs to.

    On Windows and on systems where files do not have groups this
    function always returns (uint) -2.

    \sa group(), owner(), ownerId()
*/

uint
QFileInfo::groupId() const
{
    if(!d->data->fileEngine)
        return 0;
    return d->data->fileEngine->ownerId(QFileEngine::OwnerGroup);
}

/*!
    Tests for file permissions. The \a permissions argument can be
    several flags of type \c QFile::Permissions OR-ed together to check
    for permission combinations.

    On systems where files do not have permissions this function
    always returns true.

    Example:
    \code
        QFileInfo fi("/tmp/archive.tar.gz");
        if (fi.permission(QFile::WriteUser | QFile::ReadGroup))
            qWarning("I can change the file; my group can read the file");
        if (fi.permission(QFile::WriteGroup | QFile::WriteOther))
            qWarning("The group or others can change the file");
    \endcode

    \sa isReadable(), isWritable(), isExecutable(), QFile::Permissions
*/

bool
QFileInfo::permission(QFile::Permissions permissions) const
{
    if(!d->data->fileEngine)
        return false;
    return d->getFileFlags(QFileEngine::FileFlags((int)permissions)) == (uint)permissions;
}

/*!
    Returns the complete OR-ed together combination of
    QFile::Permissions for the file.

    \sa QFileInfo::permission(), QFile::Permissions
*/

QFile::Permissions
QFileInfo::permissions() const
{
    if(!d->data->fileEngine)
        return 0;
    return QFile::Permissions(d->getFileFlags(QFileEngine::PermsMask) & QFileEngine::PermsMask);
}


/*!
    Returns the file size in bytes, or 0 if the file does not exist or
    if the size is 0 or if the size cannot be fetched.
*/

QIODevice::Offset
QFileInfo::size() const
{
    if(!d->data->fileEngine)
        return 0;
    if(!d->data->getCached(QFileInfoPrivate::CachedSize))
        d->data->fileSize = d->data->fileEngine->size();
    return d->data->fileSize;
}

/*!
    Returns the date and time when the file was created.

    On platforms where this information is not available, returns the
    same as lastModified().

    \sa created() lastModified() lastRead()
*/

QDateTime
QFileInfo::created() const
{
    if(!d->data->fileEngine)
        return QDateTime();
    return d->getFileTime(QFileEngine::CreationTime);
}

/*!
    Returns the date and time when the file was last modified.

    \sa created() lastModified() lastRead()
*/

QDateTime
QFileInfo::lastModified() const
{
    if(!d->data->fileEngine)
        return QDateTime();
    return d->getFileTime(QFileEngine::ModificationTime);
}

/*!
    Returns the date and time when the file was last read (accessed).

    On platforms where this information is not available, returns the
    same as lastModified().

    \sa created() lastModified() lastRead()
*/

QDateTime
QFileInfo::lastRead() const
{
    if(!d->data->fileEngine)
        return QDateTime();
    return d->getFileTime(QFileEngine::AccessTime);
}

/*! \internal
    Detaches all internal data.
*/

void
QFileInfo::detach()
{
    d->detach();
}

/*!
    Returns TRUE if caching is enabled; otherwise returns FALSE.

    \sa setCaching(), refresh()
*/

bool QFileInfo::caching() const
{
    return d->data->cache_enabled;
}

/*!
    If \a enable is TRUE, enables caching of file information. If \a
    enable is FALSE caching is disabled.

    When caching is enabled, QFileInfo reads the file information from
    the file system the first time it's needed, but generally not
    later.

    Caching is enabled by default.

    \sa refresh(), caching()
*/

void
QFileInfo::setCaching(bool enable)
{
    detach();
    d->data->cache_enabled = enable;
}

/*!
    \fn QString QFileInfo::baseName(bool complete)

    Use completeBaseName() or the baseName() overload that takes no
    parameters instead.
*/

/*!
    \fn QString QFileInfo::extension(bool complete = true) const

    Use completeSuffix() or suffix() instead.
*/

/*!
    \fn QString QFileInfo::absFilePath() const

    Use absoluteFilePath() instead.
*/

/*!
    \fn bool QFileInfo::convertToAbs()

    Use makeAbsolute() instead.
*/

/*!
    \enum QFileInfo::PermissionSpec

    \compat

    \value ReadOwner
    \value WriteOwner
    \value ExeOwner
    \value ReadUser
    \value WriteUser
    \value ExeUser
    \value ReadGroup
    \value WriteGroup
    \value ExeGroup
    \value ReadOther
    \value WriteOther
    \value ExeOther
*/
