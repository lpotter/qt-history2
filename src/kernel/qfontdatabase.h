/****************************************************************************
**
** Definition of the QFontDatabase class.
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QFONTDATABASE_H
#define QFONTDATABASE_H

#ifndef QT_H
#include "qwindowdefs.h"
#include "qstring.h"
#include "qstringlist.h"
#include "qfont.h"
#include "qlist.h"
#endif // QT_H


#ifndef QT_NO_FONTDATABASE

class QFontStylePrivate; /* Don't touch! */
struct QtFontStyle;
struct QtFontFamily;
struct QtFontFoundry;
struct QFontDef;
class QFontEngine;
#ifdef Q_WS_QWS
class QDiskFont;
#endif

class QFontDatabasePrivate;

class Q_GUI_EXPORT QFontDatabase
{
public:
    static QList<int> standardSizes();

    QFontDatabase();

    QStringList families() const;
    QStringList families( QFont::Script ) const;
    QStringList styles( const QString & ) const;
    QList<int> pointSizes( const QString &, const QString & = QString::null);
    QList<int> smoothSizes( const QString &, const QString &);
    QString styleString( const QFont &);

    QFont font( const QString &, const QString &, int);

    bool isBitmapScalable( const QString &, const QString & = QString::null) const;
    bool isSmoothlyScalable( const QString &, const QString & = QString::null) const;
    bool isScalable( const QString &, const QString & = QString::null) const;
    bool isFixedPitch( const QString &, const QString & = QString::null) const;

    bool italic( const QString &, const QString &) const;
    bool bold( const QString &, const QString &) const;
    int weight( const QString &, const QString &) const;

    static QString scriptName(QFont::Script);
    static QString scriptSample(QFont::Script);

#ifdef Q_WS_QWS
    static void qwsAddDiskFont( QDiskFont *qdf );
#endif

    // For source compatibility with < 3.0
#ifndef QT_NO_COMPAT

    QStringList families(bool) const;
    QStringList styles( const QString &, const QString & ) const;
    QList<int> pointSizes( const QString &, const QString &, const QString & );
    QList<int> smoothSizes( const QString &, const QString &, const QString & );

    QFont font( const QString &, const QString &, int, const QString &);

    bool isBitmapScalable( const QString &, const QString &, const QString & ) const;
    bool isSmoothlyScalable( const QString &, const QString &, const QString & ) const;
    bool isScalable( const QString &, const QString &, const QString & ) const;
    bool isFixedPitch( const QString &, const QString &, const QString & ) const;

    bool italic( const QString &, const QString &, const QString & ) const;
    bool bold( const QString &, const QString &, const QString & ) const;
    int weight( const QString &, const QString &, const QString & ) const;

#endif // QT_NO_COMPAT

private:
#if defined(Q_WS_X11) || defined(Q_WS_WIN)
    static QFontEngine *findFont( QFont::Script script, const QFontPrivate *fp,
				  const QFontDef &request, int force_encoding_id = -1 );
#endif // Q_WS_X11

    static void createDatabase();

    static void parseFontName(const QString &name, QString &foundry, QString &family);

    friend struct QFontDef;
    friend class QFontPrivate;
    friend class QFontDialog;
    friend class QFontEngineLatinXLFD;

    QFontDatabasePrivate *d;
};


#ifndef QT_NO_COMPAT

inline QStringList QFontDatabase::families( bool ) const
{
    return families();
}

inline QStringList QFontDatabase::styles( const QString &family,
					  const QString & ) const
{
    return styles(family);
}

inline QList<int> QFontDatabase::pointSizes( const QString &family,
						  const QString &style ,
						  const QString & )
{
    return pointSizes(family, style);
}

inline QList<int> QFontDatabase::smoothSizes( const QString &family,
						   const QString &style,
						   const QString & )
{
    return smoothSizes(family, style);
}

inline QFont QFontDatabase::font( const QString &familyName,
				  const QString &style,
				  int pointSize,
				  const QString &)
{
    return font(familyName, style, pointSize);
}

inline bool QFontDatabase::isBitmapScalable( const QString &family,
					     const QString &style,
					     const QString & ) const
{
    return isBitmapScalable(family, style);
}

inline bool QFontDatabase::isSmoothlyScalable( const QString &family,
					       const QString &style,
					       const QString & ) const
{
    return isSmoothlyScalable(family, style);
}

inline bool QFontDatabase::isScalable( const QString &family,
				       const QString &style,
				       const QString & ) const
{
    return isScalable(family, style);
}

inline bool QFontDatabase::isFixedPitch( const QString &family,
					 const QString &style,
					 const QString & ) const
{
    return isFixedPitch(family, style);
}

inline bool QFontDatabase::italic( const QString &family,
				   const QString &style,
				   const QString & ) const
{
    return italic(family, style);
}

inline bool QFontDatabase::bold( const QString &family,
				 const QString &style,
				 const QString & ) const
{
    return bold(family, style);
}

inline int QFontDatabase::weight( const QString &family,
				  const QString &style,
				  const QString & ) const
{
    return weight(family, style);
}

#endif // QT_NO_COMPAT

#endif // QT_NO_FONTDATABASE

#endif // QFONTDATABASE_H
