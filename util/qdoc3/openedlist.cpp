/*
  openedlist.cpp
*/

#include <qregexp.h>

#include "atom.h"
#include "openedlist.h"

static const char roman[] = "m\2d\5c\2l\5x\2v\5i";

OpenedList::OpenedList( Style style )
    : sty( style ), ini( 1 ), nex( 0 )
{
}

OpenedList::OpenedList( const Location& location, const QString& hint )
    : sty( Bullet ), ini( 1 )
{
    QRegExp hintSyntax( "(\\W*)([0-9]+|[A-Z]+|[a-z]+)(\\W*)" );

    if ( hintSyntax.exactMatch(hint) ) {
	bool ok;
	int asNumeric = hint.toInt( &ok );
	int asRoman = fromRoman( hintSyntax.cap(2) );
	int asAlpha = fromAlpha( hintSyntax.cap(2) );

	if ( ok ) {
            sty = Numeric;
	    ini = asNumeric;
	} else if ( asRoman > 0 && asRoman != 100 && asRoman != 500 ) {
	    sty = ( hint == hint.toLower() ) ? LowerRoman : UpperRoman;
	    ini = asRoman;
	} else {
	    sty = ( hint == hint.toLower() ) ? LowerAlpha : UpperAlpha;
	    ini = asAlpha;
	}
	pref = hintSyntax.cap( 1 );
	suff = hintSyntax.cap( 3 );
    } else if ( !hint.isEmpty() ) {
	location.warning( tr("Unrecognized list style '%1'").arg(hint) );
    }
    nex = ini - 1;
}

QString OpenedList::styleString() const
{
    switch ( style() ) {
    case Bullet:
    default:
	return ATOM_LIST_BULLET;
    case Tag:
	return ATOM_LIST_TAG;
    case Value:
	return ATOM_LIST_VALUE;
    case Numeric:
	return ATOM_LIST_NUMERIC;
    case UpperAlpha:
	return ATOM_LIST_UPPERALPHA;
    case LowerAlpha:
	return ATOM_LIST_LOWERALPHA;
    case UpperRoman:
	return ATOM_LIST_UPPERROMAN;
    case LowerRoman:
	return ATOM_LIST_LOWERROMAN;
    }
}

QString OpenedList::numberString() const
{
    return QString::number( number() );
/*
    switch ( style() ) {
    case Numeric:
	return QString::number( number() );
    case UpperAlpha:
	return toAlpha( number() ).toUpper();
    case LowerAlpha:
	return toAlpha( number() );
    case UpperRoman:
	return toRoman( number() ).toUpper();
    case LowerRoman:
	return toRoman( number() );
    case Bullet:
    default:
	return "*";
    }*/
}

QString OpenedList::toAlpha( int n )
{
    QString str;

    while ( n > 0 ) {
	n--;
	str.prepend( (n % 26) + 'a' );
	n /= 26;
    }
    return str;
}

int OpenedList::fromAlpha( const QString& str )
{
    int n = 0;
    int u;

    for ( int i = 0; i < (int) str.length(); i++ ) {
	u = str[i].toLower().unicode();
	if ( u >= 'a' && u <= 'z' ) {
	    n *= 26;
	    n += u - 'a' + 1;
	} else {
	    return 0;
	}
    }
    return n;
}

QString OpenedList::toRoman( int n )
{
    /*
      See p. 30 of Donald E. Knuth's "TeX: The Program".
    */
    QString str;
    int j = 0;
    int k;
    int u;
    int v = 1000;

    for ( ;; ) {
	while ( n >= v ) {
	    str += roman[j];
	    n -= v;
	}

	if ( n <= 0 )
	    break;

	k = j + 2;
	u = v / roman[k - 1];
	if ( roman[k - 1] == 2 ) {
	    k += 2;
	    u /= 5;
	}
	if ( n + u >= v ) {
	    str += roman[k];
	    n += u;
	} else {
	    j += 2;
	    v /= roman[j - 1];
	}
    }
    return str;
}

int OpenedList::fromRoman( const QString& str )
{
    int n = 0;
    int j;
    int u;
    int v = 0;

    for ( int i = str.length() - 1; i >= 0; i-- ) {
	j = 0;
	u = 1000;
	while ( roman[j] != 'i' && roman[j] != str[i].toLower() ) {
	    j += 2;
	    u /= roman[j - 1];
	}
	if ( u < v ) {
	    n -= u;
	} else {
	    n += u;
	}
	v = u;
    }

    if ( str.toLower() == toRoman(n) ) {
	return n;
    } else {
	return 0;
    }
}
