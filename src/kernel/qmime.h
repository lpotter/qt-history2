/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qmime.h#9 $
**
** Definition of mime classes
**
** Created : 981204
**
** Copyright (C) 1998-2000 Trolltech AS.  All rights reserved.
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

#ifndef QMIME_H
#define QMIME_H

#ifndef QT_H
#include "qwindowdefs.h"
#include "qmap.h"
#endif // QT_H

#ifndef QT_NO_MIME

class QImageDrag;
class QTextDrag;

class Q_EXPORT QMimeSource
{
    friend class QClipboardData;

public:
    QMimeSource();
    virtual ~QMimeSource();
    virtual const char* format( int n = 0 ) const = 0;
    virtual bool provides( const char* ) const;
    virtual QByteArray encodedData( const char* ) const = 0;
    int serialNumber() const;

private:
    int ser_no;
    enum { NoCache, Text, Graphics } cacheType;
    union
    {
	struct
	{
	    QString *str;
	    QCString *subtype;
	} txt;
	struct
	{
	    QImage *img;
	    QPixmap *pix;
	} gfx;
    } cache;
    void clearCache();

    // friends for caching
    friend class QImageDrag;
    friend class QTextDrag;

};

inline int QMimeSource::serialNumber() const
{ return ser_no; }

class QStringList;
class QMimeSourceFactoryData;

class Q_EXPORT QMimeSourceFactory {
public:
    QMimeSourceFactory();
    virtual ~QMimeSourceFactory();

    static QMimeSourceFactory* defaultFactory();
    static void setDefaultFactory( QMimeSourceFactory* );
    static QMimeSourceFactory* takeDefaultFactory();
    static void addFactory( QMimeSourceFactory *f );
    static void removeFactory( QMimeSourceFactory *f );

    virtual const QMimeSource* data(const QString& abs_name) const;
    virtual QString makeAbsolute(const QString& abs_or_rel_name, const QString& context) const;
    const QMimeSource* data(const QString& abs_or_rel_name, const QString& context) const;

    virtual void setText( const QString& abs_name, const QString& text );
    virtual void setImage( const QString& abs_name, const QImage& im );
    virtual void setPixmap( const QString& abs_name, const QPixmap& pm );
    virtual void setData( const QString& abs_name, QMimeSource* data );
    virtual void setFilePath( const QStringList& );
    virtual QStringList filePath() const;
    void addFilePath( const QString& );
    virtual void setExtensionType( const QString& ext, const char* mimetype );

private:
    QMimeSource *dataInternal(const QString& abs_name, const QMap<QString, QString> &extensions ) const;
    QMimeSourceFactoryData* d;
};

#ifdef Q_WS_WIN

#ifndef QT_H
#include "qptrlist.h" // down here for GCC 2.7.* compatibility
#endif // QT_H

/*
  Encapsulation of conversion between MIME and Windows CLIPFORMAT.
  Not need on X11, as the underlying protocol uses the MIME standard
  directly.
*/

class Q_EXPORT QWindowsMime {
public:
    QWindowsMime();
    virtual ~QWindowsMime();

    static void initialize();

    static QPtrList<QWindowsMime> all();
    static QWindowsMime* convertor( const char* mime, int cf );
    static const char* cfToMime(int cf);

    static int registerMimeType(const char *mime);

    virtual const char* convertorName()=0;
    virtual int countCf()=0;
    virtual int cf(int index)=0;
    virtual bool canConvert( const char* mime, int cf )=0;
    virtual const char* mimeFor(int cf)=0;
    virtual int cfFor(const char* )=0;
    virtual QByteArray convertToMime( QByteArray data, const char* mime, int cf )=0;
    virtual QByteArray convertFromMime( QByteArray data, const char* mime, int cf )=0;
};

#endif // Q_WS_WIN

#endif // QT_NO_MIME

#endif // QMIME_H
