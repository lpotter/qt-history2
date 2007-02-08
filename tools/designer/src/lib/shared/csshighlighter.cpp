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
TRANSLATOR qdesigner_internal::StyleSheetEditorDialog
*/

#include "csshighlighter_p.h"

namespace qdesigner_internal {

CssHighlighter::CssHighlighter(QTextDocument *document)
: QSyntaxHighlighter(document)
{
}

void CssHighlighter::highlightBlock(const QString& text)
{
    enum Token { ALNUM, LBRACE, RBRACE, COLON, SEMICOLON, COMMA, QUOTE, SLASH, STAR };
    static const int transitions[10][9] = {
        { Selector, Property, Selector, Pseudo,    Property, Selector, Quote, MaybeComment, Selector }, // Selector
        { Property, Property, Selector, Value,     Property, Property, Quote, MaybeComment, Property }, // Property
        { Value,    Property, Selector, Value,     Property, Value,    Quote, MaybeComment, Value }, // Value
        { Pseudo1, Property, Selector, Pseudo2,    Selector, Selector, Quote, MaybeComment, Pseudo }, // Pseudo
        { Pseudo1, Property, Selector, Pseudo,    Selector, Selector, Quote, MaybeComment, Pseudo1 }, // Pseudo1
        { Pseudo2, Property, Selector, Pseudo,    Selector, Selector, Quote, MaybeComment, Pseudo2 }, // Pseudo2
        { Quote,    Quote,    Quote,    Quote,     Quote,    Quote,   -1, Quote, Quote }, // Quote
        { -1, -1, -1, -1, -1, -1, -1, -1, Comment }, // MaybeComment
        { Comment, Comment, Comment, Comment, Comment, Comment, Comment, Comment, MaybeCommentEnd }, // Comment
        { Comment, Comment, Comment, Comment, Comment, Comment, Comment, -1, MaybeCommentEnd } // MaybeCommentEnd
    };

    int lastIndex = 0;
    bool lastWasSlash = false;
    int state = previousBlockState(), save_state;
    if (state == -1) {
        state = save_state = Selector;
    } else {
        save_state = state>>16;
        state &= 0x00ff;
    }

    if (state == MaybeCommentEnd) {
        state = Comment;
    } else if (state == MaybeComment) {
        state = save_state;
    }

    for (int i = 0; i < text.length(); i++) {
        int token = ALNUM;
        const QChar c = text.at(i);
        const char a = c.toAscii();

        if (state == Quote) {
            if (a == '\\') {
                lastWasSlash = true;
            } else {
                if (a == '\"' && !lastWasSlash) {
                    token = QUOTE;
                }
                lastWasSlash = false;
            }
        } else {
            switch (a) {
            case '{': token = LBRACE; break;
            case '}': token = RBRACE; break;
            case ':': token = COLON; break;
            case ';': token = SEMICOLON; break;
            case ',': token = COMMA; break;
            case '\"': token = QUOTE; break;
            case '/': token = SLASH; break;
            case '*': token = STAR; break;
            default: break;
            }
        }

        int new_state = transitions[state][token];

        if (new_state != state) {
            bool include_token = new_state == MaybeCommentEnd || (state == MaybeCommentEnd && new_state!= Comment)
                                 || state == Quote;
            highlight(text, lastIndex, i-lastIndex+include_token, state);

            if (new_state == Comment) {
                lastIndex = i-1; // include the slash and star
            } else {
                lastIndex = i + ((token == ALNUM || new_state == Quote) ? 0 : 1);
            }
        }

        if (new_state == -1) {
            state = save_state;
        } else if (state <= Pseudo2) {
            save_state = state;
            state = new_state;
        } else {
            state = new_state;
        }
    }

    highlight(text, lastIndex, text.length() - lastIndex, state);
    setCurrentBlockState(state + (save_state<<16));
}

void CssHighlighter::highlight(const QString &text, int start, int length, int state)
{
    if (start >= text.length() || length <= 0)
        return;

    QTextCharFormat format;

    switch (state) {
    case Selector:
        setFormat(start, length, Qt::darkRed);
        break;
    case Property:
        setFormat(start, length, Qt::blue);
        break;
    case Value:
        setFormat(start, length, Qt::black);
        break;
    case Pseudo1:
        setFormat(start, length, Qt::darkRed);
        break;
    case Pseudo2:
        setFormat(start, length, Qt::darkRed);
        break;
    case Quote:
        setFormat(start, length, Qt::darkMagenta);
        break;
    case Comment:
    case MaybeCommentEnd:
        format.setForeground(Qt::darkGreen);
        setFormat(start, length, format);
        break;
    default:
        break;
    }
}

} // namespace qdesigner_internal
