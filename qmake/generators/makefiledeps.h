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

#ifndef __MAKEFILEDEPS_H__
#define __MAKEFILEDEPS_H__

#include "qstringlist.h"

struct SourceFile;
struct SourceDependChildren;
class SourceFiles;

class QMakeLocalFileName {
    uint is_null : 1;
    QString real_name, local_name;
public:
    QMakeLocalFileName() : is_null(1) { }
    QMakeLocalFileName(const QString &);
    bool isNull() const { return is_null; }
    const QString &real() const { return real_name; }
    const QString &local() const { return local_name; }
};

class QMakeSourceFileInfo
{
private:
    //quick project lookups
    SourceFiles *files;
    bool files_changed;
    QList<QMakeLocalFileName> depdirs;

    //sleezy buffer code
    char *spare_buffer;
    int   spare_buffer_size;
    char *getBuffer(int s);

    //actual guts
    bool findMocs(SourceFile *);
    bool findDeps(SourceFile *);
    void dependTreeWalker(SourceFile *, SourceDependChildren *);

    //cache
    QString cachefile;

protected:
    virtual QMakeLocalFileName fixPathForFile(const QMakeLocalFileName &, bool forOpen=false);
    virtual QMakeLocalFileName findFileForDep(const QMakeLocalFileName &, const QMakeLocalFileName &);
    virtual QMakeLocalFileName findFileForMoc(const QMakeLocalFileName &);

public:
    QMakeSourceFileInfo(const QString &cachefile="");
    virtual ~QMakeSourceFileInfo();

    QList<QMakeLocalFileName> dependencyPaths() const { return depdirs; }
    void setDependencyPaths(const QList<QMakeLocalFileName> &);

    enum SourceFileType { TYPE_UNKNOWN, TYPE_C, TYPE_UI, TYPE_QRC };
    enum SourceFileSeek { SEEK_DEPS=0x01, SEEK_MOCS=0x02, ADD_MOC=0x04 };
    void addSourceFiles(const QStringList &, uchar, SourceFileType type=TYPE_C);

    int included(const QString &file);
    QStringList dependencies(const QString &file);

    QString mocFile(const QString &file);
    QString mocSource(const QString &mocfile);
    bool mocable(const QString &file);

    virtual QMap<QString, QStringList> getCacheVerification();
    virtual bool verifyCache(const QMap<QString, QStringList> &);
    void setCacheFile(const QString &cachefile); //auto caching
    void loadCache(const QString &cf);
    void saveCache(const QString &cf);
};

#endif /* __MAKEFILEDEPS_H__ */
