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

/*
  doc.h
*/

#ifndef DOC_H
#define DOC_H

#include <QSet>
#include <QString>

#include "location.h"

QT_BEGIN_NAMESPACE

class Atom;
class CodeMarker;
class Config;
class DocPrivate;
class Quoter;
class Text;

class Doc
{
public:
    // the order is important
    enum SectioningUnit { Book = -2, Part, Chapter, Section1, Section2, Section3, Section4 };

    Doc() : priv(0) {}
    Doc(const Location &loc, const QString &source, const QSet<QString> &metaCommandSet);
    Doc(const Doc &doc);
    ~Doc();

    Doc& operator=( const Doc& doc );

    void renameParameters(const QStringList &oldNames, const QStringList &newNames);
    void simplifyEnumDoc();
    void setBody(const Text &body);

    const Location &location() const;
    bool isEmpty() const;
    const QString& source() const;
    const Text& body() const;
    Text briefText() const;
    Text trimmedBriefText(const QString &className) const;
    Text legaleseText() const;
    const QString& baseName() const;
    SectioningUnit granularity() const;
    const QSet<QString> &parameterNames() const;
    const QStringList &enumItemNames() const;
    const QStringList &omitEnumItemNames() const;
    const QSet<QString> &metaCommandsUsed() const;
    QStringList metaCommandArgs( const QString& metaCommand ) const;
    const QList<Text> &alsoList() const;
    bool hasTableOfContents() const;
    bool hasKeywords() const;
    bool hasTargets() const;
    const QList<Atom *> &tableOfContents() const;
    const QList<Atom *> &keywords() const;
    const QList<Atom *> &targets() const;
    const QMap<QString, QString> &metaTagMap() const;

    static void initialize( const Config &config );
    static void terminate();
    static QString alias( const QString &english );
    static void trimCStyleComment( Location& location, QString& str );
    static CodeMarker *quoteFromFile(const Location &location, Quoter &quoter,
                                     const QString &fileName);
    static QString canonicalTitle(const QString &title);

private:
    void detach();

    DocPrivate *priv;
};

QT_END_NAMESPACE

#endif
