/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qasyncimageio.h#17 $
**
**		      ***   INTERNAL HEADER FILE   ***
**
**		This file is NOT a part of the Qt interface!
**
** Definition of asynchronous image/movie loading classes
**
** Created : 970617
**
** Copyright (C) 1994-1997 by Troll Tech AS.  All rights reserved.
**
** --------------------------------------------------------------------------
** Internal header file containing private classes used by QMovie.
*****************************************************************************/

#ifndef QASYNCIMAGEIO_H
#define QASYNCIMAGEIO_H

#ifndef QT_H
#include "qimage.h"
#endif // QT_H



class QImageConsumer {
public:
    virtual void changed(const QRect&)=0;
    virtual void end()=0;
    virtual void frameDone()=0;
    virtual void setLooping(int)=0;
    virtual void setFramePeriod(int)=0;
    virtual void setSize(int, int)=0;
};

class QImageFormat {
public:
    virtual ~QImageFormat();
    virtual int decode(QImage& img, QImageConsumer* consumer,
	    const uchar* buffer, int length)=0;
};

class QImageFormatType {
public:
    virtual ~QImageFormatType();
    virtual QImageFormat* decoderFor(const uchar* buffer, int length)=0;
    virtual const char* formatName() const=0;
protected:
    QImageFormatType();
};

struct QImageDecoderPrivate;

class QImageDecoder {
public:
    QImageDecoder(QImageConsumer* c);
    ~QImageDecoder();

    const QImage& image() { return img; }
    int decode(const uchar* buffer, int length);

    static const char* formatName(const uchar* buffer, int length);

    static QStrList inputFormats();
    static void registerDecoderFactory(QImageFormatType*);
    static void unregisterDecoderFactory(QImageFormatType*);

private:
    QImageFormat* actual_decoder;
    QImageConsumer* consumer;
    QImage img;
    QImageDecoderPrivate *d;
};


class QGIFFormat : public QImageFormat {
public:
    QGIFFormat();
    virtual ~QGIFFormat();

    int decode(QImage& img, QImageConsumer* consumer,
	    const uchar* buffer, int length);

private:
    void fillRect(QImage&, int x, int y, int w, int h, uchar col);

    // GIF specific stuff
    QRgb* globalcmap;
    QImage backingstore;
    unsigned char hold[16];
    bool gif89;
    int count;
    int ccount;
    int expectcount;
    enum State {
	Header,
	LogicalScreenDescriptor,
	GlobalColorMap,
	LocalColorMap,
	Introducer,
	ImageDescriptor,
	TableImageLZWSize,
	ImageDataBlockSize,
	ImageDataBlock,
	ExtensionLabel,
	GraphicControlExtension,
	ApplicationExtension,
	NetscapeExtensionBlockSize,
	NetscapeExtensionBlock,
	SkipBlockSize,
	SkipBlock,
	Done,
	Error
    } state;
    int gncols;
    int ncols;
    int lzwsize;
    bool lcmap;
    int swidth, sheight;
    int left, top, right, bottom;
    enum Disposal { NoDisposal, DoNotChange, RestoreBackground, RestoreImage };
    Disposal disposal;
    bool disposed;
    int trans;
    bool preserve_trans;
    bool gcmap;
    int bgcol;
    int interlace;
    int accum;
    int bitcount;

    enum { max_lzw_bits=12 }; // (poor-compiler's static const int)

    int code_size, clear_code, end_code, max_code_size, max_code;
    int firstcode, oldcode, incode;
    short table[2][1<< max_lzw_bits];
    short stack[(1<<(max_lzw_bits))*2];
    short *sp;
    bool needfirst;
    int x, y;
    int frame;
    bool out_of_bounds;
    bool digress;
    void nextY(QImage& img, QImageConsumer* consumer);
    void disposePrevious( QImage& img, QImageConsumer* consumer );
};

class QGIFFormatType : public QImageFormatType
{
    QImageFormat* decoderFor(const uchar* buffer, int length);
    const char* formatName() const;
};


#endif // QASYNCIMAGEIO_H
