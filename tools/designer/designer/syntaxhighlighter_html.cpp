/****************************************************************************
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of Qt Designer.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "syntaxhighlighter_html.h"
#include "qstring.h"
#include "qmap.h"
#include "qapplication.h"


SyntaxHighlighter_HTML::SyntaxHighlighter_HTML()
    : QTextPreProcessor(), lastFormat( 0 ), lastFormatId( -1 )
{
    QFont f( qApp->font() );

    addFormat( Standard, new QTextFormat( f, Qt::black ) );
    addFormat( Keyword, new QTextFormat( f, Qt::darkRed ) );
    addFormat( Attribute, new QTextFormat( f, Qt::darkGreen ) );
    addFormat( AttribValue, new QTextFormat( f, Qt::darkYellow ) );
}

SyntaxHighlighter_HTML::~SyntaxHighlighter_HTML()
{
}

void SyntaxHighlighter_HTML::process( QTextDocument *doc, QTextParagraph *string, int, bool invalidate )
{

    QTextFormat *formatStandard = format( Standard );
    QTextFormat *formatKeyword = format( Keyword );
    QTextFormat *formatAttribute = format( Attribute );
    QTextFormat *formatAttribValue = format( AttribValue );

    const int StateStandard  = 0;
    const int StateTag       = 1;
    const int StateAttribute = 2;
    const int StateAttribVal = 3;

    QString buffer = "";

    int state = StateStandard;


    if ( string->prev() ) {
	if ( string->prev()->endState() == -1 )
	    process( doc, string->prev(), 0, FALSE );
	state = string->prev()->endState();
    }


    int i = 0;
    for ( ;; ) {
	QChar c = string->at( i )->c;
	
	if ( c == '<' ) {
	    if ( state != StateStandard  )
		string->setFormat( i - buffer.length(), buffer.length(), formatStandard, FALSE );
	    buffer = c;
	    state = StateTag;
	    string->setFormat( i, 1, formatKeyword, FALSE );
	}
	else if ( c == '>' && ( state != StateStandard ) ) {
	    string->setFormat( i, 1, formatKeyword, FALSE );
	    buffer = "";
	    state = StateStandard;
	}
	else if ( c == ' ' && state == StateTag ) {
	    buffer += c;
	    string->setFormat( i, 1, formatStandard, FALSE );
	    state = StateAttribute;
	}
	else if ( c == '=' && state == StateAttribute ) {
	    buffer += c;
	    string->setFormat( i, 1, formatStandard, FALSE );
	    state = StateAttribute;	
	}
	else if ( c == '\"' && state == StateAttribute ) {
	    buffer += c;
	    string->setFormat( i, 1, formatStandard, FALSE );
	    state = StateAttribVal;
	}
	else if ( c == '\"' && state == StateAttribVal ) {
	    buffer += c;
	    string->setFormat( i, 1, formatStandard, FALSE );
	    state = StateAttribute;
	}
	else if ( state == StateAttribute ) {
	    buffer += c;
	    string->setFormat( i, 1, formatAttribute, FALSE );	
	}
	else if ( state == StateAttribVal ) {
	    buffer += c;
	    string->setFormat( i, 1, formatAttribValue, FALSE );
	}
	else if ( state == StateTag ) {
	    string->setFormat( i, 1, formatKeyword, FALSE );
	    buffer += c;
	}
	else if ( state == StateStandard ) {
	    string->setFormat( i, 1, formatStandard, FALSE );
	}
	
	i++;	
	if ( i >= string->length() )
	    break;
    }

    string->setEndState( state );
    string->setFirstPreProcess( FALSE );

    if ( invalidate && string->next() &&
	 !string->next()->firstPreProcess() && string->next()->endState() != -1 ) {
	QTextParagraph *p = string->next();
	while ( p ) {
	    if ( p->endState() == -1 )
		return;
	    p->setEndState( -1 );
	    p = p->next();
	}
    }
}

QTextFormat *SyntaxHighlighter_HTML::format( int id )
{
    if ( lastFormatId == id  && lastFormat )
	return lastFormat;

    QTextFormat *f = formats[ id ];
    lastFormat = f ? f : formats[ 0 ];
    lastFormatId = id;
    return lastFormat;
}

void SyntaxHighlighter_HTML::addFormat( int id, QTextFormat *f )
{
    formats.insert( id, f );
}
