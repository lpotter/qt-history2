/****************************************************************************
** $Id: //depot/qt/main/src/tools/qregexp.cpp#8 $
**
** Implementation of QRegExp class
**
** Author  : Haavard Nord
** Created : 950126
**
** Copyright (C) 1995 by Troll Tech AS.  All rights reserved.
**
** --------------------------------------------------------------------------
** This regular expression implementation is inspired by Ozan Yigit's regexp.
*****************************************************************************/

#include "qregexp.h"
#include <ctype.h>
#if defined(_OS_MAC_) && defined(VXWORKS)
#include <stdlib.h>
#else
#include <malloc.h>
#endif

#if defined(DEBUG)
static char ident[] = "$Id: //depot/qt/main/src/tools/qregexp.cpp#8 $";
#endif


//
// The regexp pattern is internally represented as an array of ushorts,
// each element containing an 8-bit character or a 16-bit code (listed below).
// Character classes are encoded as 256 bits, i.e. 16*16 bits.
//

const ushort CHR	= 0x4000;		// character
const ushort BOL	= 0x8001;		// beginning of line 	^
const ushort EOL	= 0x8002;		// end of line		$
const ushort BOW	= 0x8003;		// beginning of word	\<
const ushort EOW	= 0x8004;		// end of word		\>
const ushort ANY	= 0x8005;		// any character	.
const ushort CCL	= 0x8006;		// character class	[]
const ushort CLO	= 0x8007;		// Kleene closure	*
const ushort OPT	= 0x8008;		// Optional closure	?
const ushort END	= 0x0000;

//
// QRegExp::error codes (internal)
//

const PatOk		= 0;			// pattern ok
const PatNull		= 1;			// no pattern defined
const PatSyntax		= 2;			// pattern syntax error
const PatOverflow	= 4;			// pattern too long

// ---------------------------------------------------------------------------
// QRegExp member functions
//

QRegExp::QRegExp()
{
    rxdata = 0;
    cs = TRUE;
    wc = FALSE;
    error = PatOk;
}

QRegExp::QRegExp( const char *pattern, bool caseSensitive, bool wildcard )
{
    rxstring = pattern;
    rxdata = 0;
    cs = caseSensitive;
    wc = wildcard;
    compile();
}

QRegExp::QRegExp( const QRegExp &r )
{
    rxstring = r.pattern();
    rxdata = 0;
    cs = r.caseSensitive();
    wc = r.wildcard();
    compile();
}

QRegExp::~QRegExp()
{
    delete rxdata;
}

QRegExp &QRegExp::operator=( const QRegExp &r )
{
    rxstring = (const char *)r.rxstring;
    compile();
    return *this;
}

QRegExp &QRegExp::operator=( const char *pattern )
{
    rxstring = pattern;
    compile();
    return *this;
}


bool QRegExp::operator==( const QRegExp &r ) const
{
    return rxstring == r.rxstring && cs == r.cs && wc == r.wc;
}


void QRegExp::setWildcard( bool wildcard )
{
    if ( wildcard != wc ) {
	wc = wildcard;
	compile();
    }
}

void QRegExp::setCaseSensitive( bool cases )
{
    if ( cases != cs ) {
	cs = cases;
	compile();
    }
}

//
// Find the first instance of the regexp pattern in a string.
//

int QRegExp::match( const char *str, int index, int *len ) const
{
    if ( error )				// not fit for fight
	return -1;
    register char *p = (char *)str + index;
    ushort *d  = rxdata;
    char   *ep = 0;

    if ( *d == BOL )				// match from beginning of line
	ep = matchstr( d, p, p );
    else {
	if ( *d & CHR ) {
	    if ( cs ) {				// case sensitive
		while ( *p && *p != (char)*d )
		    p++;
	    }
	    else {				// case insensitive
		while ( *p && tolower(*p) != (char)*d )
		    p++;
	    }
	}
	while ( *p ) {				// regular match
	    if ( (ep=matchstr(d,p,(char*)str+index)) )
		break;
	    p++;
	}
    }
    if ( ep ) {					// match
	if ( len )
	    *len = ep - p;
	return (int)(p - str);			// return index
    }
    else {					// no match
	if ( len )
	    *len = 0;
	return -1;
    }
}

inline bool iswordchar( int x )
{
    return isalnum(x) || x == '_';
}

char *QRegExp::matchstr( ushort *rxd, char *str, char *bol ) const
{						// recursively match string
    register char *p = str;
    ushort *d = rxd;
    while ( *d ) {
	if ( *d & CHR ) {			// match char
	    if ( cs ) {				// case sensitive
		if ( *p++ != (char)*d )
		    return 0;
	    }
	    else {				// case insensitive
		if ( tolower(*p) != (char)*d )
		    return 0;
		p++;
	    }
	    d++;
	}
	else switch ( *d++ ) {
	    case ANY:				// match anything
		if ( !*p++ )
		    return 0;
		break;
	    case CCL:				// match char class
		if ( (d[*p >> 4] & (1 << (*p & 0xf))) == 0 )
		    return 0;
		p++;
		d += 16;
		break;
	    case BOL:				// match beginning of line
		if ( p != bol )
		    return 0;
		break;
	    case EOL:				// match end of line
		if ( *p )
		    return 0;
		break;
	    case BOW:				// match beginning of word
		if ( !iswordchar(*p) || (p > bol && iswordchar(*(p-1)) ) )
		    return 0;
		break;
	    case EOW:				// match end of word
		if ( iswordchar(*p) || p == bol || !iswordchar(*(p-1)) )
		    return 0;
		break;
	    case CLO:				// Kleene closure
		{
		char *first_p = p;
		if ( *d & CHR ) {		// match char
		    if ( cs ) {			// case sensitive
			while ( *p && *p == (char)*d )
			    p++;
		    }
		    else {			// case insensitive
			while ( *p && tolower(*p) == (char)*d )
			    p++;
		    }
		}
		else switch ( *d ) {
		    case ANY:
			while ( *p )
			    p++;
			break;
		    case CCL:
			d++;
			while ( *p && d[*p >> 4] & (1 << (*p & 0xf)) )
			    p++;
			d += 15;
			break;
		    default:			// error
			return 0;
		}
		d++;
		d++;
		char *end;
		while ( p >= first_p ) {	// go backwards
		    if ( (end = matchstr(d,p,bol)) )
			return end;
		    --p;
		}
		}
		return 0;
	    case OPT:				// optional closure
		{
		char *first_p = p;
		if ( *d & CHR ) {		// match char
		    if ( cs ) {			// case sensitive
			if ( *p && *p == (char)*d )
			    p++;
		    }
		    else {			// case insensitive
			if ( *p && tolower(*p) == (char)*d )
			    p++;
		    }
		}
		else switch ( *d ) {
		    case ANY:
			if ( *p )
			    p++;
			break;
		    case CCL:
			d++;
			if ( *p && d[*p >> 4] & (1 << (*p & 0xf)) )
			    p++;
			d += 15;
			break;
		    default:			// error
			return 0;
		}
		d++;
		d++;
		char *end;
		while ( p >= first_p ) {	// go backwards
		    if ( (end = matchstr(d,p,bol)) )
			return end;
		    --p;
		}
		}
		return 0;
	    default:				// error
		return 0;
	}
    }
    return p;
}


//
// Translate wildcard pattern to standard regexp pattern.
// Ex:   *.cpp	==> ^.*\.cpp$
//

static QString wc2rx( const char *pattern )
{
    register char *p = (char *)pattern;
    QString wcpattern = "^";
    char c;
    while ( (c=*p++) ) {
	switch ( c ) {
	    case '*':				// '*' ==> '.*'
		wcpattern += '.';
		break;
	    case '?':				// '?' ==> '.'
		c = '.';
		break;
	    case '.':				// quote special regexp chars
	    case '+':
	    case '\\':
	    case '^':
	    case '$':
	    case '[':
		wcpattern += '\\';
		break;
	}
	wcpattern += c;
    }
    wcpattern += '$';
    return wcpattern;				// return new regexp pattern
}


//
// Internal: Get char value and increment pointer.
//

static int char_val( char **str )		// get char value
{
    register char *p = *str;
    int len = 1;
    int v = 0;
    if ( *p == '\\' ) {				// escaped code
	p++;
	if ( *p == 0 ) {			// it is just a '\'
	    (*str)++;
	    return '\\';
	}
	len++;					// length at least 2
	switch ( tolower(*p) ) {
	    case 'b':  v = '\b';  break;	// bell
	    case 'f':  v = '\f';  break;	// form feed
	    case 'n':  v = '\n';  break;	// newline
	    case 'r':  v = '\r';  break;	// return
	    case 't':  v = '\t';  break;	// tab

	    case 'x': {				// hex code
		p++;
		int  c = tolower(*p);
		bool a = c >= 'a' && c <= 'f';
		if ( isdigit(c) || a ) {	// hex digit?
		    v = a ? 10 + c - 'a' : c - '0';
		    len++;
		}
		p++;
		c = tolower(*p);
		a = c >= 'a' && c <= 'f';
		if ( isdigit(c) || a ) {	// another hex digit?
		    v *= 16;
		    v += a ? 10 + c - 'a' : c - '0';
		    len++;
		}
	        }
		break;

	    default: {
		int i;
		--len;				// first check if octal
		for ( i=0; i<3 && *p >= '0' && *p <= '7'; i++ ) {
		    v *= 8;
		    v += *p++ - '0';
		    len++;
		}
		if ( i == 0 ) {			// not an octal number
		    v = *p;
		    len++;
		}
	    }
	}
    }
    else
	v = *p;
    *str += len;
    return v;
}


#if defined(DEBUG)
ushort *dump( ushort *p )			// DEBUG !!!
{
    while ( *p != END ) {
	if ( *p & CHR ) {
	    debug( "\tCHR\t%c (%d)", *p&0xff, *p&0xff );
	    p++;
	} else switch ( *p++ ) {
	    case BOL:
	        debug( "\tBOL" );
		break;
	    case EOL:
	        debug( "\tEOL" );
		break;
	    case BOW:
	        debug( "\tBOW" );
		break;
	    case EOW:
	        debug( "\tEOW" );
		break;
	    case ANY:
	        debug( "\tANY" );
		break;
	    case CCL: {
		QString s = "";
		QString buf;
		for ( int n=0; n<256; n++ ) {
		    if ( p[n >> 4] & (1 << (n & 0xf)) ) {
			if ( isprint(n) )
			    s += (char)n;
			else {
			    buf.sprintf( "\\X%.2X", n );
			    s += buf;
			}
		    }
		}
	        debug( "\tCCL\t%s", (char *)s );
		p += 16;
	        }
		break;
	    case CLO:
		debug( "\tCLO" );
		p = dump( p );
		break;
	    case OPT:
		debug( "\tOPT" );
		p = dump( p );
		break;
	}
    }
    debug( "\tEND" );
    return p+1;
}
#endif // DEBUG


//
// Compile the regexp pattern and store the result in rxdata.
// The 'error' flag is set to non-zero if an error is detected.
// NOTE: The compile function is not reentrant!!!

const maxlen = 1024;				// max length of regexp array
static ushort rxarray[ maxlen ];		// tmp regexp array

void QRegExp::compile()
{
    if ( rxdata ) {				// delete old data
	delete rxdata;
	rxdata = 0;
    }
    if ( rxstring.isEmpty() ) {			// no regexp pattern set
	error = PatNull;
	return;
    }

    error = PatOk;				// assume pattern is ok

    QString pattern = wc ? wc2rx(rxstring) : rxstring;
    char   *p = pattern;			// pattern pointer
    ushort *d = rxarray;			// data pointer
    ushort *prev_d = 0;

#define GEN(x)	*d++ = (x)

    while ( *p ) {
	switch ( *p ) {

	    case '^':				// beginning of line
		prev_d = d;
		GEN( p == pattern.data() ? BOL : *p );
		p++;
		break;

	    case '$':				// end of line
		prev_d = d;
		GEN( *(p+1) == 0 ? EOL : *p );
		p++;
		break;

	    case '.':				// any char
		prev_d = d;
		GEN( ANY );
		p++;
		break;

	    case '[':				// character class
		{
		char cc[256];
		char neg;			// mask for CCL
		prev_d = d;
		GEN( CCL );
		p++;
		memset( cc, 0, 256 );		// reset char class array
		if ( *p == '^' ) {		// negate!
		    neg = 1;
		    p++;
		}
		else
		    neg = 0;
		if ( *p == ']' )		// bracket, not end
		    cc[*p++] = 1;
		int prev_c = -1;
		while ( *p && *p != ']' ) {	// scan the char set
		    if ( *p == '-' && *(p+1) && *(p+1) != ']' ) {
			p++;			// range!
			if ( prev_c == -1 )	// no previous char
			    cc['-'] = 1;
			else {
			    int start = prev_c;
			    int stop = char_val( &p );
			    if ( start > stop ) { // swap start and stop
				int tmp = start;
				start = stop;
				stop = tmp;
			    }
			    while ( start++ < stop )
				cc[start] = 1;
			}
		    }
		    else			// normal char
			cc[(prev_c=char_val(&p))] = 1;
		}
		if ( *p != ']' ) {		// missing close bracket
		    error = PatSyntax;
		    return;
		}
		if ( d + 16 >= rxarray + maxlen ) {
		    error = PatOverflow;	// pattern too long
		    return;
		}
		memset( d, 0, 16*sizeof(ushort) );
		for ( int i=0; i<256; i++ ) {	// set bits
		    if ( cc[i] ^ neg ) {
			d[i >> 4] |= (1 << (i & 0xf));
			if ( !cs && isalpha(i) ) {
			    int j = islower(i) ? toupper(i) : tolower(i);
			    d[j >> 4] |= (1 << (j & 0xf));
			}
		    }
		}
		d += 16;
		p++;
		}
		break;

	    case '*':				// Kleene closure, or
	    case '+':				// positive closure, or
	    case '?':				// optional closure
		{
		if ( prev_d == 0 ) {		// no previous expression
		    error = PatSyntax;		// empty closure
		    return;
		}
		switch ( *prev_d ) {		// test if invalid closure
		    case BOL:
		    case BOW:
		    case EOW:
		    case CLO:
		    case OPT:
			error = PatSyntax;
			return;
		}
		int ddiff = d - prev_d;
		if ( *p == '+' ) {		// convert to Kleene closure
		    if ( d + ddiff >= rxarray + maxlen ) {
			error = PatOverflow;	// pattern too long
			return;
		    }
		    memcpy( d, prev_d, ddiff*sizeof(ushort) );
		    d += ddiff;
		    prev_d += ddiff;
		}
		memmove( prev_d+1, prev_d, ddiff*sizeof(ushort) );
		*prev_d = *p == '?' ? OPT : CLO;
		d++;
		GEN( END );
		p++;
		}
		break;

	    default:
		prev_d = d;
		if ( *p == '\\' && (*(p+1) == '<' || *(p+1) == '>') ) {
		    GEN( *++p == '<' ? BOW : EOW );
		    p++;
		}
		else {
		    int c = char_val(&p);
		    if ( !cs )
			c = tolower( c );
		    GEN( CHR | c );
		}
	}
	if ( d >= rxarray + maxlen ) {		// oops!
	    error = PatOverflow;		// pattern too long	    
	    return;
	}
    }
    GEN( END );
    int len = d - rxarray;
    rxdata = new ushort[ len ];			// copy from rxarray to rxdata
    CHECK_PTR( rxdata );
    memcpy( rxdata, rxarray, len*sizeof(ushort) );
#if defined(DEBUG)
//  dump( rxdata );	// uncomment this line for debugging!!!
#endif
}


// ---------------------------------------------------------------------------
// QString member functions that use QRegExp
//

int QString::find( const QRegExp &r, int index ) const
{						// find substring
    return (uint)index >= size() ? -1 : r.match( data(), index );
}

int QString::findRev( const QRegExp &r, int index ) const
{						// reverse find substring
    if ( index < 0 ) {				// neg index ==> start from end
	if ( size() )
	    index = strlen( data() );
	else					// empty string
	    return -1;
    }
    else if ( (uint)index >= size() )		// bad index
	return -1;
    while( index >= 0 ) {
	if ( r.match(data(),index) == index )
	    return index;
	index--;
    }
    return -1;
}

int QString::contains( const QRegExp &r ) const
{						// get # substrings
    int count = 0;
    register char *d = data();
    if ( !d )					// null string
	return 0;
    int index = -1;
    while ( TRUE ) {				// count overlapping matches
	index = r.match( d, index+1 );
	if ( index >= 0 )
	    count++;
	else
	    break;
    }
    return count;
}
