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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef RESOURCEMIMEDATA_H
#define RESOURCEMIMEDATA_H

#include "shared_global_p.h"
#include <QtCore/QString>

QT_BEGIN_NAMESPACE

class QMimeData;

namespace qdesigner_internal {

// Resources for drag and drop
class QDESIGNER_SHARED_EXPORT ResourceMimeData
{
public:
    enum Type { Image, File };

    ResourceMimeData(Type t = File);

    Type type() const        { return m_type; }

    QString qrcPath() const  { return  m_qrcPath; }
    QString filePath() const { return m_filePath; }

    void setQrcPath(const QString &qrcPath)   {  m_qrcPath  = qrcPath; }
    void setFilePath(const QString &filePath) {  m_filePath = filePath;}
    void setType(Type type)                   {  m_type     = type;}

    QMimeData *toMimeData() const;
    static bool isResourceMimeData(const QMimeData *md);
    static bool isResourceMimeData(const QMimeData *md, Type desiredType);
    bool fromMimeData(const QMimeData *md);

private:
    Type m_type;
    QString m_qrcPath;
    QString m_filePath;
};
}

QT_END_NAMESPACE

#endif // RESOURCEMIMEDATA_H
