/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qtranslator.h#9 $
**
** Definition of the translator class
**
** Created : 980906
**
** Copyright (C) 1998-99 by Troll Tech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Troll Tech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** Licensees holding valid Qt Professional Edition licenses may use this
** file in accordance with the Qt Professional Edition License Agreement
** provided with the Qt Professional Edition.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
** information about the Professional Edition licensing, or see
** http://www.trolltech.com/qpl/ for QPL licensing information.
**
*****************************************************************************/


#ifndef QTRANSLATOR_H
#define QTRANSLATOR_H

#ifndef QT_H
#include "qobject.h"
#include "qintdict.h"
#endif // QT_H

#ifndef QT_NO_TRANSLATION

class QTranslatorPrivate;


class Q_EXPORT QTranslatorMessage {
public:
    enum Type { Unfinished, Finished, Obsolete };

    QTranslatorMessage();
    QTranslatorMessage( const char * context,
			const char * sourceText,
			const char * comment,
			const QString& translation = QString::null,
			Type type = Unfinished );
    QTranslatorMessage( QDataStream & );

    uint hash() const { return h; }
    const char *context() const { return cx; }
    const char *sourceText() const { return st; }
    const char *comment() const { return cm; }

    void setTranslation( const QString & translation ) { tn = translation; }
    QString translation() const { return tn; }

    void setType( Type type ) { ty = type; }
    Type type() const { return ty; }

    enum Prefix { NoPrefix, Hash, HashContext, HashContextSourceText,
    		  HashContextSourceTextComment };
    void write( QDataStream & s, bool strip,
		Prefix prefix = HashContextSourceTextComment ) const;
    Prefix commonPrefix( const QTranslatorMessage& ) const;

    bool operator==( const QTranslatorMessage& m ) const;
    bool operator!=( const QTranslatorMessage& m ) const
    { return !operator==( m ); }
    bool operator<( const QTranslatorMessage& m ) const;
    bool operator<=( const QTranslatorMessage& m ) const
    { return !operator>( m ); }
    bool operator>( const QTranslatorMessage& m ) const
    { return this->operator<( m ); }
    bool operator>=( const QTranslatorMessage& m ) const
    { return !operator<( m ); }

private:
    uint h;
    QCString cx;
    QCString st;
    QCString cm;
    QString tn;
    Type ty;

    enum Tag { Tag_End = 1, Tag_SourceText16, Tag_Translation, Tag_Context16,
	       Tag_Hash, Tag_SourceText, Tag_Context, Tag_Comment, Tag_Type };
};


class Q_EXPORT QTranslator: public QObject
{
    Q_OBJECT
public:
    QTranslator( QObject * parent, const char * name = 0 );
    ~QTranslator();

// ### find( const char *, const char *, const char * ) obsolete in Qt 3.0 ?
    QString find( const char *, const char *, const char * ) const;
// ### find( const char *, const char * ) obsolete in Qt 3.0
    virtual QString find( const char *, const char * ) const;
// ### findMessage made virtual in Qt 3.0
    QTranslatorMessage findMessage( const char *, const char *,
				    const char * ) const;

    bool load( const QString & filename,
	       const QString & directory = QString::null,
	       const QString & search_delimiters = QString::null,
	       const QString & suffix = QString::null );

    enum SaveMode { Everything, Stripped };

    bool save( const QString & filename, SaveMode mode = Everything );

    void clear();

    void insert( const QTranslatorMessage& );
// ### insert() obsolete in Qt 3.0
    void insert( const char *, const char *, const QString & );
    void remove( const QTranslatorMessage& );
// ### first remove obsolete in Qt 3.0
    void remove( const char *, const char * );
    bool contains( const char *, const char *, const char * ) const;
// ### contains removed in Qt 3.0
    bool contains( const char *, const char * ) const;

// ### squeeze() obsolete in Qt 3.0
// ### replaced by squeeze( SaveMode mode = Everything )
    void squeeze( SaveMode );
    void squeeze();
    void unsqueeze();

    QValueList<QTranslatorMessage> messages() const;
    QString toUnicode( const char * ) const;

private:
    QTranslatorPrivate * d;

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QTranslator( const QTranslator & );
    QTranslator &operator=( const QTranslator & );
#endif
};

#endif // QT_NO_TRANSLATION

#endif
