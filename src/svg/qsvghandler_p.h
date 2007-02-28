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

#ifndef QSVGHANDLER_P_H
#define QSVGHANDLER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtXml/qxmlstream.h"
#include "QtCore/qhash.h"
#include "QtCore/qstack.h"
#include "qsvgstyle_p.h"

class QSvgNode;
class QSvgTinyDocument;
class QSvgHandler;
class QColor;
class QSvgStyleSelector;

typedef QSvgNode *(*FactoryMethod)(QSvgNode *,
                                   const QXmlStreamAttributes &,
                                   QSvgHandler *);
typedef bool (*ParseMethod)(QSvgNode *,
                            const QXmlStreamAttributes &,
                            QSvgHandler *);

typedef QSvgStyleProperty *(*StyleFactoryMethod)(QSvgNode *,
                                                 const QXmlStreamAttributes &,
                                                 QSvgHandler *);
typedef bool (*StyleParseMethod)(QSvgStyleProperty *,
                                 const QXmlStreamAttributes &,
                                 QSvgHandler *);

class QSvgHandler
{
public:
    enum LengthType {
        PERCENT,
        PX,
        PC,
        PT,
        MM,
        CM,
        IN,
        OTHER
    };

public:
    QSvgHandler(QIODevice *device);
    QSvgHandler(const QByteArray &data);
    ~QSvgHandler();

    QSvgTinyDocument *document() const;

    inline bool ok() const {
        return document() != 0; // ### TODO xml error checking
    }

    void setDefaultCoordinateSystem(LengthType type);
    LengthType defaultCoordinateSystem() const;

    void pushColor(const QColor &color);
    QColor currentColor() const;

    void setInStyle(bool b);
    bool inStyle() const;

    QSvgStyleSelector *selector() const;

    void setAnimPeriod(int start, int end);
    int animationDuration() const;
public:
    bool startElement(const QString &localName, const QXmlStreamAttributes &attributes);
    bool endElement(const QStringRef &localName);
    bool characters(const QStringRef &str);
    bool processingInstruction(const QString &target, const QString &data);

private:
    void init();

    QSvgTinyDocument *m_doc;
    QStack<QSvgNode*> m_nodes;

    QList<QSvgNode*>  m_resolveNodes;

    enum CurrentNode
    {
        Unknown,
        Graphics,
        Style
    };
    QStack<CurrentNode> m_skipNodes;

    QSvgRefCounter<QSvgStyleProperty> m_style;

    LengthType m_defaultCoords;

    QStack<QColor> m_colorStack;
    QStack<int>    m_colorTagCount;

    bool m_inStyle;

    QSvgStyleSelector *m_selector;

    int m_animEnd;
private:
    QXmlStreamReader xml;
    void parse();
    static QHash<QString, FactoryMethod> s_groupFactory;
    static QHash<QString, FactoryMethod> s_graphicsFactory;
    static QHash<QString, ParseMethod>   s_utilFactory;

    static QHash<QString, StyleFactoryMethod>   s_styleFactory;
    static QHash<QString, StyleParseMethod>     s_styleUtilFactory;
};

#endif // QSVGHANDLER_P_H
