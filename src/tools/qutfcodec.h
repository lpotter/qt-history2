/****************************************************************************
** $Id: //depot/qt/main/src/tools/qutfcodec.h#1 $
**
** Definition of QEucCodec class
**
** Created : 981015
**
** Copyright (C) 1998 Troll Tech AS.  All rights reserved.
**
** This file is part of Troll Tech's internal development tree for Qt.
**
** This header text will be replaced by an appropriate text by the
** mkdist script which generates external distributions.
**
** If you are using the Qt Professional Edition or the Qt Free Edition,
** please notify Troll Tech at <info@troll.no> if you see this text.
**
** To Troll Tech developers: This header was generated by the script
** fixcopyright-int. It has the same number of text lines as the free
** and professional editions to avoid line number inconsistency.
**
*****************************************************************************/

#ifndef QUTFCODEC_H
#define QUTFCODEC_H

#include "qtextcodec.h"

class QUtf8Codec : public QTextCodec {
public:
    virtual int mib() const;
    const char* name() const;

    QTextDecoder* makeDecoder() const;

    char* fromUnicode(const QString& uc, int& len_in_out) const;

    int heuristicContentMatch(const char* chars, int len) const;
};

class QUtf16Codec : public QTextCodec {
public:
    virtual int mib() const;
    const char* name() const;

    QTextDecoder* makeDecoder() const;
    QTextEncoder* makeEncoder() const;

    int heuristicContentMatch(const char* chars, int len) const;
};

#endif
