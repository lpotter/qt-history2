#ifndef RESOURCEFILE_H
#define RESOURCEFILE_H

#include "shared_global.h"

#include <QString>
#include <QStringList>
#include <QMap>
#include <QFile>

class QT_SHARED_EXPORT ResourceFile
{
public:
    ResourceFile(const QString &file_name);

    bool load();
    bool save();

    QString resolvePath(const QString &path) const;

    QStringList prefixList() const;
    QStringList fileList(const QString &prefix);
    void addPrefix(const QString &prefix);
    void addFile(const QString &prefix, const QString &file);
    void removePrefix(const QString &prefix);
    void removeFile(const QString &prefix, const QString &file);
    bool contains(const QString &prefix) const;
    bool contains(const QString &prefix, const QString &file) const;
    void changePrefix(const QString &old_prefix, const QString &new_prefix);

    QString relativePath(const QString &abs_path) const;
    QString absolutePath(const QString &rel_path) const;

    static QString fixPrefix(const QString &prefix);

private:
    typedef QMap<QString, QStringList> ResourceMap;
    ResourceMap m_resource_map;
    QString m_file_name;
};

#endif // RESOURCEFILE_H
