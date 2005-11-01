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
#ifndef QVFB_SHM_PROTOCOL_H
#define QVFB_SHM_PROTOCOL_H

#include "qvfbprotocol.h"

class QVFbHeader;
class QTimer;
class QShMemViewProtocol : public QVFbViewProtocol
{
public:
    QShMemViewProtocol(int display_id, const QSize &size, int depth, bool mmap, QObject *parent = 0);
    ~QShMemViewProtocol();

    int width() const;
    int height() const;
    int depth() const;
    int linestep() const;
    int  numcols() const;
    QRgb *clut() const;
    unsigned char *data() const;

    void setRate(int);
    int rate() const;

protected slots:
    void flushChanges();

protected:
    QVFbKeyProtocol *keyHandler() const { return kh; }
    QVFbMouseProtocol *mouseHandler() const { return mh; }

private:
    QVFbKeyPipeProtocol *kh;
    QVFbMousePipeProtocol *mh;
    QVFbHeader *hdr;
    size_t displaySize;
    unsigned char *dataCache;
    bool mUseMmap;
    int lockId;
    int shmId;
    QTimer *mRefreshTimer;
};

#endif
