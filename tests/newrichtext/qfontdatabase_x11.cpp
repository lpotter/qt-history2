/****************************************************************************
** $Id$
**
** Implementation of platform specific QFontDatabase
**
** Created : 970521
**
** Copyright (C) 1992-2002 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition licenses may use this
** file in accordance with the Qt Commercial License Agreement provided
** with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include <qplatformdefs.h>
#include "qfontdata_p.h"

#include "fontengine.h"
#include "fontenginexlfd.h"
#include "fontenginexft.h"

#include "qt_x11.h"

#include <ctype.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#define QFONTDATABASE_DEBUG
#ifdef QFONTDATABASE_DEBUG
#  include <qdatetime.h>
#endif // QFONTDATABASE_DEBUG

// ----- begin of generated code -----

#define make_tag( c1, c2, c3, c4 ) \
( (((unsigned int)c1)<<24) | (((unsigned int)c2)<<16) | \
(((unsigned int)c3)<<8) | ((unsigned int)c4) )

struct XlfdEncoding {
    const char *name;
    int id;
    int mib;
    unsigned int hash1;
    unsigned int hash2;
};

static const XlfdEncoding xlfd_encoding[] = {
    { "iso8859-1", 0, 0, make_tag('i','s','o','8'), make_tag('5','9','-','1') },
    { "iso8859-2", 1, 5, make_tag('i','s','o','8'), make_tag('5','9','-','2') },
    { "iso8859-3", 2, 6, make_tag('i','s','o','8'), make_tag('5','9','-','3') },
    { "iso8859-4", 3, 7, make_tag('i','s','o','8'), make_tag('5','9','-','4') },
    { "iso8859-14", 4, 110, make_tag('i','s','o','8'), make_tag('9','-','1','4') },
    { "iso8859-15", 5, 111, make_tag('i','s','o','8'), make_tag('9','-','1','5') },
    { "iso8859-5", 6, 8, make_tag('i','s','o','8'), make_tag('5','9','-','5') },
    { "*-cp1251", 7, 2251, 0, make_tag('1','2','5','1') },
    { "koi8-ru", 8, 2084, make_tag('k','o','i','8'), make_tag('8','-','r','u') },
    { "koi8-u", 9, 2088, make_tag('k','o','i','8'), make_tag('i','8','-','u') },
    { "koi8-r", 10, 2084, make_tag('k','o','i','8'), make_tag('i','8','-','r') },
    { "iso8859-7", 11, 10, make_tag('i','s','o','8'), make_tag('5','9','-','7') },
    { "iso8859-6", 12, 82, make_tag('i','s','o','8'), make_tag('5','9','-','6') },
    { "iso8859-8", 13, 85, make_tag('i','s','o','8'), make_tag('5','9','-','8') },
    { "gb18030-0", 14, -114, make_tag('g','b','1','8'), make_tag('3','0','-','0') },
    { "gb18030.2000-0", 15, -113, make_tag('g','b','1','8'), make_tag('0','0','-','0') },
    { "gbk-0", 16, -113, make_tag('g','b','k','-'), make_tag('b','k','-','0') },
    { "gb2312.*-0", 17, 57, make_tag('g','b','2','3'), 0 },
    { "jisx0201*-0", 18, 15, make_tag('j','i','s','x'), 0 },
    { "jisx0208*-0", 19, 63, make_tag('j','i','s','x'), 0 },
    { "ksc5601.1987-0", 20, 36, make_tag('k','s','c','5'), make_tag('8','7','-','0') },
    { "big5hkscs-0", 21, -2101, make_tag('b','i','g','5'), make_tag('c','s','-','0') },
    { "hkscs-1", 22, -2101, make_tag('h','k','s','c'), make_tag('c','s','-','1') },
    { "big5*-*", 23, -2026, make_tag('b','i','g','5'), 0 },
    { "tscii-*", 24, 2028, make_tag('t','s','c','i'), 0 },
    { "tis620*-*", 25, 2259, make_tag('t','i','s','6'), 0 },
    { "iso8859-11", 26, 2259, make_tag('i','s','o','8'), make_tag('9','-','1','1') },
    { "mulelao-1", 27, -4242, make_tag('m','u','l','e'), make_tag('a','o','-','1') },
    { "ethiopic-unicode", 28, 0, make_tag('e','t','h','i'), make_tag('c','o','d','e') },
    { "iso10646-1", 29, 0, make_tag('i','s','o','1'), make_tag('4','6','-','1') },
    { "unicode-*", 30, 0, make_tag('u','n','i','c'), 0 },
    { "*-symbol", 31, 0, 0, make_tag('m','b','o','l') },
    { "*-fontspecific", 32, 0, 0, make_tag('i','f','i','c') },
    { 0, 0, 0, 0, 0 }
};

static const char scripts_for_xlfd_encoding[33][49] = {
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
      0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
      1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
      0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 1 }

};

// ----- end of generated code -----
const int numEncodings = sizeof( xlfd_encoding ) / sizeof( XlfdEncoding ) - 1;

int qt_xlfdEncoding_Id( const char *encoding )
{
    // qDebug("looking for encoding id for '%s'", encoding );
    int len = strlen( encoding );
    if ( len < 4 )
	return -1;
    unsigned int hash1 = make_tag( encoding[0], encoding[1], encoding[2], encoding[3] );
    const char *ch = encoding + len - 4;
    unsigned int hash2 = make_tag( ch[0], ch[1], ch[2], ch[3] );

    const XlfdEncoding *enc = xlfd_encoding;
    for ( ; enc->name; ++enc ) {
	if ( (enc->hash1 && enc->hash1 != hash1) ||
	     (enc->hash2 && enc->hash2 != hash2) )
	    continue;
	// hashes match, do a compare if strings match
	// the enc->name can contain '*'s we have to interpret correctly
	const char *n = enc->name;
	const char *e = encoding;
	while ( 1 ) {
 	    // qDebug("bol: *e='%c', *n='%c'", *e,  *n );
	    if ( *e == '\0' ) {
		if ( *n )
		    break;
		// qDebug( "found encoding id %d", enc->id );
		return enc->id;
	    }
	    if ( *e == *n ) {
		++e;
		++n;
		continue;
	    }
	    if ( *n != '*' )
		break;
	    ++n;
 	    // qDebug("skip: *e='%c', *n='%c'", *e,  *n );
	    while ( *e && *e != *n )
		++e;
	}
    }
    //    qDebug( "couldn't find encoding %s", encoding );
    return -1;
}

int qt_mibForXlfd( const char * encoding )
{
    int id = qt_xlfdEncoding_Id( encoding );
    if ( id != -1 )
	return xlfd_encoding[id].mib;
    return 0;
};

static const char * xlfd_for_id( int id )
{
    // special case: -1 returns the "*-*" encoding, allowing us to do full
    // database population in a single X server round trip.
    if ( id < 0 || id > (signed) ( numEncodings ) )
	return "*-*";
    return xlfd_encoding[id].name;
}



// returns a sample unicode character for the specified script
static QChar sampleCharacter(QFont::Script script)
{
    ushort ch;

    switch (script) {
    case QFont::Latin:                     ch = 0x00c0; break;
    case QFont::Greek:                     ch = 0x0390; break;
    case QFont::Cyrillic:                  ch = 0x0410; break;
    case QFont::Armenian:                  ch = 0x0540; break;
    case QFont::Georgian:                  ch = 0x10a0; break;
    case QFont::Runic:                     ch = 0x16a0; break;
    case QFont::Ogham:                     ch = 0x1680; break;
    case QFont::CombiningMarks:            ch = 0x0300; break;

    case QFont::Hebrew:                    ch = 0x05d0; break;
    case QFont::Arabic:                    ch = 0x0630; break;
    case QFont::Syriac:                    ch = 0x0710; break;
    case QFont::Thaana:                    ch = 0x0780; break;

    case QFont::Devanagari:                ch = 0x0910; break;
    case QFont::Bengali:                   ch = 0x0990; break;
    case QFont::Gurmukhi:                  ch = 0x0a10; break;
    case QFont::Gujarati:                  ch = 0x0a90; break;
    case QFont::Oriya:                     ch = 0x0b10; break;
    case QFont::Tamil:                     ch = 0x0b90; break;
    case QFont::Telugu:                    ch = 0x0c10; break;
    case QFont::Kannada:                   ch = 0x0c90; break;
    case QFont::Malayalam:                 ch = 0x0d10; break;
    case QFont::Sinhala:                   ch = 0x0d90; break;
    case QFont::Thai:                      ch = 0x0e10; break;
    case QFont::Lao:                       ch = 0x0e81; break;
    case QFont::Tibetan:                   ch = 0x0f00; break;
    case QFont::Myanmar:                   ch = 0x1000; break;
    case QFont::Khmer:                     ch = 0x1780; break;

    case QFont::Han:                       ch = 0x4e00; break;
    case QFont::Hiragana:                  ch = 0x3050; break;
    case QFont::Katakana:                  ch = 0x30b0; break;
    case QFont::Hangul:                    ch = 0xac00; break;
    case QFont::Bopomofo:                  ch = 0x3110; break;
    case QFont::Yi:                        ch = 0xa000; break;

    case QFont::Ethiopic:                  ch = 0x1200; break;
    case QFont::Cherokee:                  ch = 0x13a0; break;
    case QFont::CanadianAboriginal:        ch = 0x1410; break;
    case QFont::Mongolian:                 ch = 0x1800; break;

    case QFont::CurrencySymbols:           ch = 0x20aa; break;
    case QFont::LetterlikeSymbols:         ch = 0x2122; break;
    case QFont::NumberForms:               ch = 0x215b; break;
    case QFont::MathematicalOperators:     ch = 0x222b; break;
    case QFont::TechnicalSymbols:          ch = 0x2440; break;
    case QFont::GeometricSymbols:          ch = 0x2500; break;
    case QFont::MiscellaneousSymbols:      ch = 0x2600; break;
    case QFont::EnclosedAndSquare:         ch = 0x2460; break;
    case QFont::Braille:                   ch = 0x2800; break;

    case QFont::Unicode:		   ch = 0xfffd; break;

    case QFont::LatinExtendedA_2:          ch = 0x0102; break;
    case QFont::LatinExtendedA_3:          ch = 0x0108; break;
    case QFont::LatinExtendedA_4:          ch = 0x0100; break;
    case QFont::LatinExtendedA_14:         ch = 0x0174; break;
    case QFont::LatinExtendedA_15:         ch = 0x0152; break;

    default:				   ch = 0x0000; break;
    }

    return QChar(ch);
}

static inline bool canRender( FontEngineIface *fe, const QChar &sample )
{
    if ( !fe ) return FALSE;

    QChar chs[2] = { QChar(0xfffe), sample };
    bool hasChar = !fe->canRender( chs, 1 ) && fe->canRender( chs+1, 1 );

#ifdef QFONTLOADER_DEBUG_VERBOSE
    if (hasChar) {
	qDebug("QFontLoader: unicode font has char 0x%04x", sample.unicode() );
    }
#endif

    return hasChar;
}

static QtFontStyle::Key getStyle( char ** tokens )
{
    QtFontStyle::Key key;

    char slant0 = tolower( (uchar) tokens[QFontPrivate::Slant][0] );

    if ( slant0 == 'r' ) {
        if ( tokens[QFontPrivate::Slant][1]) {
            char slant1 = tolower( (uchar) tokens[QFontPrivate::Slant][1] );

            if ( slant1 == 'o' )
                key.oblique = TRUE;
            else if ( slant1 == 'i' )
		key.italic = TRUE;
        }
    } else if ( slant0 == 'o' )
	key.oblique = TRUE;
    else if ( slant0 == 'i' )
	key.italic = TRUE;

    key.weight = QFontPrivate::getFontWeight( tokens[QFontPrivate::Weight] );

    return key;
}



static inline void capitalize ( char *s )
{
    bool space = TRUE;
    while( *s ) {
	if ( space )
	    *s = toupper( *s );
	space = ( *s == ' ' );
	++s;
    }
}

static inline bool isZero(char *x)
{
    return (x[0] == '0' && x[1] == 0);
}

static inline bool isScalable( char **tokens )
{
    return (isZero(tokens[QFontPrivate::PixelSize]) &&
	    isZero(tokens[QFontPrivate::PointSize]) &&
	    isZero(tokens[QFontPrivate::AverageWidth]));
}

static inline bool isSmoothlyScalable( char **tokens )
{
    return (isZero(tokens[QFontPrivate::ResolutionX]) &&
	    isZero(tokens[QFontPrivate::ResolutionY]));
}

static inline bool isFixedPitch( char **tokens )
{
    return (tokens[QFontPrivate::Spacing][0] == 'm' ||
	    tokens[QFontPrivate::Spacing][0] == 'c' ||
	    tokens[QFontPrivate::Spacing][0] == 'M' ||
	    tokens[QFontPrivate::Spacing][0] == 'C');
}

extern bool qt_has_xft; // defined in qfont_x11.cpp


bool xlfdsFullyLoaded = FALSE;
unsigned char encodingLoaded[numEncodings];

static void loadXlfds( const char *reqFamily, int encoding_id )
{
    QtFontFamily *fontFamily = reqFamily ? db->family( reqFamily ) : 0;

    // make sure we don't load twice
    if ( (encoding_id == -1 && xlfdsFullyLoaded) || (encoding_id != -1 && encodingLoaded[encoding_id]) )
	return;
    if ( fontFamily && fontFamily->xlfdLoaded )
	return;

    int fontCount;
    // force the X server to give us XLFDs
    QCString xlfd_pattern = "-*-";
    xlfd_pattern += reqFamily ? reqFamily : "*";
    xlfd_pattern += "-*-*-*-*-*-*-*-*-*-*-";
    xlfd_pattern += xlfd_for_id( encoding_id );

    char **fontList = XListFonts( QPaintDevice::x11AppDisplay(),
				  xlfd_pattern.data(),
				  0xffff, &fontCount );
    qDebug("requesting xlfd='%s', got %d fonts", xlfd_pattern.data(), fontCount );


    char *tokens[QFontPrivate::NFontFields];

    for( int i = 0 ; i < fontCount ; i++ ) {
	if ( !QFontPrivate::parseXFontName( fontList[i], tokens ) ) continue;

	// get the encoding_id for this xlfd.  we need to do this
	// here, since we can pass -1 to this function to do full
	// database population
	*(tokens[QFontPrivate::CharsetEncoding]-1) = '-';
	int encoding_id = qt_xlfdEncoding_Id( tokens[QFontPrivate::CharsetRegistry] );
	if ( encoding_id == -1 )
	    continue;

	char *familyName = tokens[QFontPrivate::Family];
	capitalize( familyName );
	char *foundryName = tokens[QFontPrivate::Foundry];
	capitalize( foundryName );
	QtFontStyle::Key styleKey = getStyle( tokens );

	bool smooth_scalable = FALSE;
	bool bitmap_scalable = FALSE;
	if ( isScalable(tokens) ) {
	    if ( isSmoothlyScalable( tokens ) )
		smooth_scalable = TRUE;
	    else
		bitmap_scalable = TRUE;
	}
	int pixelSize = atoi( tokens[QFontPrivate::PixelSize] );
	bool fixedPitch = isFixedPitch( tokens );


	QtFontFamily *family = fontFamily ? fontFamily : db->family( familyName, TRUE );
	QtFontFoundry *foundry = family->foundry( foundryName, TRUE );
	QtFontStyle *style = foundry->style( styleKey, TRUE );

	style->xlfd_uses_regular = ( qstrcmp( tokens[QFontPrivate::Weight], "regular" ) == 0 );

	if ( smooth_scalable ) {
	    style->smoothScalable = TRUE;
	    style->bitmapScalable = FALSE;
	    pixelSize = SMOOTH_SCALABLE;
	}
	if ( !style->smoothScalable && bitmap_scalable )
	    style->bitmapScalable = TRUE;
	if ( !fixedPitch )
	    family->fixedPitch = FALSE;

	QtFontSize *size = style->pixelSize( pixelSize, TRUE );
	QtFontEncoding *enc = size->encodingID( encoding_id, TRUE );
	enc->pitch = *tokens[QFontPrivate::Spacing];
	if ( !enc->pitch ) enc->pitch = '*';

	for ( int script = 0; script < QFont::NScripts + 1; script++ ) {
	    if ( scripts_for_xlfd_encoding[encoding_id][script] )
		family->scripts[script] = QtFontFamily::Supported;
	    else
		family->scripts[script] |= QtFontFamily::UnSupported_Xlfd;
	}
	if ( encoding_id == -1 )
	    family->xlfdLoaded = TRUE;
    }
    if ( !reqFamily ) {
	// mark encoding as loaded
	if ( encoding_id == -1 )
	    xlfdsFullyLoaded = TRUE;
	else
	    encodingLoaded[encoding_id] = TRUE;
    }

    XFreeFontNames( fontList );
}


#ifndef QT_NO_XFTFREETYPE
static int getXftWeight(int xftweight)
{
    int qtweight = QFont::Black;
    if (xftweight <= (XFT_WEIGHT_LIGHT + XFT_WEIGHT_MEDIUM) / 2)
	qtweight = QFont::Light;
    else if (xftweight <= (XFT_WEIGHT_MEDIUM + XFT_WEIGHT_DEMIBOLD) / 2)
	qtweight = QFont::Normal;
    else if (xftweight <= (XFT_WEIGHT_DEMIBOLD + XFT_WEIGHT_BOLD) / 2)
	qtweight = QFont::DemiBold;
    else if (xftweight <= (XFT_WEIGHT_BOLD + XFT_WEIGHT_BLACK) / 2)
	qtweight = QFont::Bold;

    return qtweight;
}

static void loadXft()
{
    if (!qt_has_xft)
	return;

    XftFontSet  *fonts;

    QString familyName;
    char *value;
    int weight_value;
    int slant_value;
    int spacing_value;
    char *file_value;
    int index_value;

    fonts =
	XftListFonts(QPaintDevice::x11AppDisplay(),
		     QPaintDevice::x11AppScreen(),
		     (const char *)0,
		     XFT_FAMILY, XFT_WEIGHT, XFT_SLANT, XFT_SPACING, XFT_FILE, XFT_INDEX,
		     (const char *)0);

    for (int i = 0; i < fonts->nfont; i++) {
	if (XftPatternGetString(fonts->fonts[i],
				XFT_FAMILY, 0, &value) != XftResultMatch )
	    continue;
	// 	capitalize( value );
	familyName = value;

	slant_value = XFT_SLANT_ROMAN;
	weight_value = XFT_WEIGHT_MEDIUM;
	spacing_value = XFT_PROPORTIONAL;
	XftPatternGetInteger (fonts->fonts[i], XFT_SLANT, 0, &slant_value);
	XftPatternGetInteger (fonts->fonts[i], XFT_WEIGHT, 0, &weight_value);
	XftPatternGetInteger (fonts->fonts[i], XFT_SPACING, 0, &spacing_value);
	XftPatternGetString (fonts->fonts[i], XFT_FILE, 0, &file_value);
	XftPatternGetInteger (fonts->fonts[i], XFT_INDEX, 0, &index_value);

	QtFontFamily *family = db->family( familyName, TRUE );
	family->hasXft = TRUE;

	QCString file = file_value;
	QCString ext = file.mid( file.findRev( '.' ) ).lower();
	if ( index_value == 0 && ( ext == ".ttf" || ext == ".otf" ) )
	    family->xftFilename = file;
	else
	    qDebug("invalid xft file %s", file_value );

	QtFontStyle::Key styleKey;
	styleKey.italic = (slant_value == XFT_SLANT_ITALIC);
	styleKey.oblique = (slant_value == XFT_SLANT_OBLIQUE);
	styleKey.weight = getXftWeight( weight_value );

	QtFontFoundry *foundry = family->foundry( QString::null,  TRUE );
	QtFontStyle *style = foundry->style( styleKey,  TRUE );

	style->smoothScalable = TRUE;
	family->fixedPitch = ( spacing_value >= XFT_MONO );

	QtFontSize *size = style->pixelSize( SMOOTH_SCALABLE, TRUE );
	QtFontEncoding *enc = size->encodingID( -1, TRUE );
	enc->pitch = ( spacing_value >= XFT_CHARCELL ? 'c' :
		       ( spacing_value >= XFT_MONO ? 'm' : 'p' ) );
    }

    XftFontSetDestroy (fonts);
}

#define MAKE_TAG( _x1, _x2, _x3, _x4 ) \
          ( ( (Q_UINT32)_x1 << 24 ) |     \
            ( (Q_UINT32)_x2 << 16 ) |     \
            ( (Q_UINT32)_x3 <<  8 ) |     \
              (Q_UINT32)_x4         )

#ifdef _POSIX_MAPPED_FILES
static inline Q_UINT32 getUInt(unsigned char *p)
{
    Q_UINT32 val;
    val = *p++ << 24;
    val |= *p++ << 16;
    val |= *p++ << 8;
    val |= *p;

    return val;
}

static inline Q_UINT16 getUShort(unsigned char *p)
{
    Q_UINT16 val;
    val = *p++ << 8;
    val |= *p;

    return val;
}

static inline void tag_to_string( char *string, Q_UINT32 tag )
{
    string[0] = (tag >> 24)&0xff;
    string[1] = (tag >> 16)&0xff;
    string[2] = (tag >> 8)&0xff;
    string[3] = tag&0xff;
    string[4] = 0;
}

static Q_UINT16 getGlyphIndex( unsigned char *table, Q_UINT16 format, unsigned short unicode )
{
    if ( format == 0 ) {
	if ( unicode < 256 )
	    return (int) *(table+6+unicode);
    } else if ( format == 2 ) {
	qWarning("format 2 encoding table for Unicode, not implemented!");
    } else if ( format == 4 ) {
	Q_UINT16 segCountX2 = getUShort( table + 6 );
	unsigned char *ends = table + 14;
	Q_UINT16 endIndex = 0;
	int i = 0;
	for ( ; i < segCountX2/2 && (endIndex = getUShort( ends + 2*i )) < unicode; i++ );

	unsigned char *idx = ends + segCountX2 + 2 + 2*i;
	Q_UINT16 startIndex = getUShort( idx );

	if ( startIndex > unicode )
	    return 0;

	idx += segCountX2;
	Q_INT16 idDelta = (Q_INT16)getUShort( idx );
	idx += segCountX2;
	Q_UINT16 idRangeOffset = (Q_UINT16)getUShort( idx );

	Q_UINT16 glyphIndex;
	if ( idRangeOffset ) {
	    Q_UINT16 id = getUShort( idRangeOffset + 2*(unicode - startIndex) + idx);
	    if ( id )
		glyphIndex = ( idDelta + id ) % 0x10000;
	    else
		glyphIndex = 0;
	} else {
	    glyphIndex = (idDelta + unicode) % 0x10000;
	}
	return glyphIndex;
    }

    return 0;
}
#endif

// #### get rid of the X11 screen somehow.
static inline void checkXftCoverage( QtFontFamily *family, int x11Screen )
{
//     qDebug("checking Xft coverage of family %s",  family->name.latin1() );

#ifdef _POSIX_MAPPED_FILES
    if ( !family->xftFilename.isEmpty() ) {
	qDebug("not using Xft for coverage checking of '%s'!", family->name.latin1() );
	int fd = open( family->xftFilename.data(), O_RDONLY );
	if ( fd == -1 )
	    goto xftCheck;
	void *map;
	size_t pagesize = getpagesize();
	off_t offset = 0;
	size_t length = (8192 / pagesize + 1) * pagesize;
	if ( (map = mmap( 0, length, PROT_READ, MAP_SHARED, fd, offset ) ) == MAP_FAILED )
	    goto xftCheck;

	unsigned char *ttf = (unsigned char *)map;
	Q_UINT32 version = getUInt( ttf );
	if ( version != 0x00010000 ) {
	    qDebug("file has wrong version %x",  version );
	    goto xftCheck;
	}
	Q_UINT16 numTables =  getUShort( ttf+4 );

	unsigned char *table_dir = ttf + 12;
	Q_UINT32 cmap_offset = 0;
	Q_UINT32 cmap_length = 0;
	for ( int n = 0; n < numTables; n++ ) {
	    Q_UINT32 tag = getUInt( table_dir + 16*n );
	    if ( tag == MAKE_TAG( 'c', 'm', 'a', 'p' ) ) {
		cmap_offset = getUInt( table_dir + 16*n + 8 );
		cmap_length = getUInt( table_dir + 16*n + 12 );
		break;
	    }
	}
	if ( !cmap_offset ) {
	    qDebug("no cmap found" );
	    goto xftCheck;
	}

	if ( cmap_offset + cmap_length > length ) {
	    munmap( map, length );
	    offset = cmap_offset / pagesize * pagesize;
	    cmap_offset -= offset;
	    length = (cmap_offset + cmap_length);
	    if ( (map = mmap( 0, length, PROT_READ, MAP_SHARED, fd, offset ) ) == MAP_FAILED )
		goto xftCheck;
	}

	unsigned char *cmap = ((unsigned char *)map) + cmap_offset;

	version = getUShort( cmap );
	if ( version != 0 ) {
	    qDebug("wrong cmap version" );
	    goto xftCheck;
	}
	numTables = getUShort( cmap + 2 );
	unsigned char *unicode_table = 0;
	for ( int n = 0; n < numTables; n++ ) {
	    Q_UINT32 version = getUInt( cmap + 4 + 8*n );
	    // accept both symbol and Unicode encodings. prefer unicode.
	    if ( version == 0x00030001 || version == 0x00030000 ) {
		unicode_table = cmap + getUInt( cmap + 4 + 8*n + 4 );
		if ( version == 0x00030001 )
		    break;
	    }
	}

	if ( !unicode_table ) {
	    qDebug("no unicode table found" );
	    goto xftCheck;
	}

	Q_UINT16 format = getUShort( unicode_table );
	if ( format != 4 )
	    goto xftCheck;

	for ( int i = 0; i < QFont::NScripts+1; i++ ) {
	    QChar ch = sampleCharacter( (QFont::Script)i );
	    if ( getGlyphIndex( unicode_table, format, ch.unicode() ) ) {
// 		qDebug("font can render script %d",  i );
		family->scripts[i] = QtFontFamily::Supported;
	    } else {
		family->scripts[i] |= QtFontFamily::UnSupported_Xft;
	    }
	}


	return;
    }
#endif

 xftCheck:

    qDebug("using Xft for checking of '%s'", family->name.latin1() );

    XftPattern *pattern = XftPatternCreate();
    if ( !pattern )
	return;

#ifndef QT_XFT2
    XftPatternAddString (pattern, XFT_ENCODING, "iso10646-1");
#endif
    XftPatternAddString( pattern, XFT_FAMILY, family->name.local8Bit().data() );

    XftResult res;
    XftPattern *result = XftFontMatch( QPaintDevice::x11AppDisplay(),
				       x11Screen, pattern, &res );
    XftPatternDestroy(pattern);
    XftPattern *dup = XftPatternDuplicate( result );
    XftFont *xftfs = XftFontOpenPattern(QPaintDevice::x11AppDisplay(), dup);

    if ( xftfs ) {
	FontEngineIface *fe = new FontEngineXft( xftfs, result, 0 );

	for ( int i = 0; i < QFont::NScripts+1; i++ ) {
	    if ( ::canRender( fe,  sampleCharacter( (QFont::Script)i ) ) ) {
// 		qDebug("font can render script %d",  i );
		family->scripts[i] = QtFontFamily::Supported;
	    } else {
		family->scripts[i] |= QtFontFamily::UnSupported_Xft;
	    }
	}
	delete fe;
    } else {
	qDebug("could not load Xft font for family %s",  family->name.latin1() );
    }
    family->xftScriptCheck = TRUE;
}

#endif

static void load( const QString &family, QFont::Script script, int x11Screen )
{
    if ( family.isNull() ) {
	for ( int i = 0; i < numEncodings; i++ ) {
	    if ( scripts_for_xlfd_encoding[i][script] )
		loadXlfds( 0, i );
	}
    } else {
	QtFontFamily *f = db->family( family, TRUE );
	if ( f->fullyLoaded )
	    return;

#ifndef QT_NO_XFTFREETYPE
	// need to check Xft coverage
	if ( f->hasXft && !f->xftScriptCheck ) {
	    checkXftCoverage( f, x11Screen );
	}
#endif
	// could reduce this further with some more magic:
	// would need to remember the encodings loaded for the family.
	if ( !f->hasXft && !(f->scripts[script] & QtFontFamily::Supported) &&
	     !(f->scripts[script] & QtFontFamily::UnSupported_Xlfd) ) {
	    loadXlfds( family, -1 );
	    f->fullyLoaded = TRUE;
	}

	// set Unknown script status to UnSupported
	if ( !(f->scripts[script] & QtFontFamily::Supported) )
	     f->scripts[script] = QtFontFamily::UnSupported;
    }
}

static void loadFully()
{
    loadXlfds( 0, -1 );
}

void QFontDatabase::createDatabase()
{
    if ( db ) return;
    db = new QFontDatabasePrivate;

    memset( encodingLoaded, FALSE, sizeof( encodingLoaded ) );

#ifdef QFONTDATABASE_DEBUG
    QTime t;
    t.start();
#endif // QFONTDATABASE_DEBUG

#ifndef QT_NO_XFTFREETYPE
    loadXft();
#endif

#ifdef QFONTDATABASE_DEBUG
    qDebug("QFontDatabase: loaded Xft: %d ms",  t.elapsed() );
    t.start();
#endif // QFONTDATABASE_DEBUG
//     /*
//       when the time comes, we need to change the font database to do
//       incremental loading, instead of fully creating the database all
//       at once.
//     */
//     loadXlfds( 0, -1 ); // full load

    for ( int i = 0; i < db->count; i++ ) {
	checkXftCoverage( db->families[i], 0 );
    }

#ifdef QFONTDATABASE_DEBUG
    qDebug("QFontDatabase: xft coverage check: %d ms",  t.elapsed() );
#endif // QFONTDATABASE_DEBUG

#ifdef QFONTDATABASE_DEBUG
    // print the database
    for ( int f = 0; f < db->count; f++ ) {
	QtFontFamily *family = db->families[f];
	qDebug("'%s' %s  hasXft=%s", family->name.latin1(), (family->fixedPitch ? "fixed" : ""),
	       (family->hasXft ? "yes" : "no") );
	for ( int i = 0; i < QFont::NScripts; i++ ) {
	    qDebug( "\t%s: %s", QFontDatabase::scriptName( (QFont::Script)i ).latin1(),
		    ( (family->scripts[i] & QtFontFamily::Supported) ? "Supported" :
		      (family->scripts[i] & QtFontFamily::UnSupported) == QtFontFamily::UnSupported ?
		      "UnSupported" : "Unknown" ) );
	}

	for ( int fd = 0; fd < family->count; fd++ ) {
	    QtFontFoundry *foundry = family->foundries[fd];
	    qDebug("\t\t'%s'", foundry->name.latin1() );
	    for ( int s = 0; s < foundry->count; s++ ) {
		QtFontStyle *style = foundry->styles[s];
		qDebug("\t\t\tstyle: italic=%d oblique=%d weight=%d",
		       style->key.italic, style->key.oblique, style->key.weight );
		if ( style->smoothScalable )
		    qDebug("\t\t\t\tsmooth scalable" );
		else if ( style->bitmapScalable )
		    qDebug("\t\t\t\tbitmap scalable" );
		if ( style->pixelSizes ) {
		    qDebug("\t\t\t\t%d pixel sizes",  style->count );
		    for ( int z = 0; z < style->count; ++z ) {
			QtFontSize *size = style->pixelSizes + z;
			for ( int e = 0; e < size->count; ++e ) {
			    qDebug( "\t\t\t\t  size %5d pitch %c encoding %s",
				    size->pixelSize,
				    size->encodings[e].pitch,
				    xlfd_for_id( size->encodings[e].encoding ) );
			}
		    }
		}
	    }
	}
    }
#endif // QFONTDATABASE_DEBUG
}


static inline FontEngineIface *loadEngine( int styleStrategy, int styleHint,
					   const QString &family,
					   const QString &foundry,
					   int weight, bool italic,
					   bool oblique, int pixelSize,
					   char pitch, bool use_regular,
					   const QCString &encoding,
					   int x11Screen )
{
#ifndef QT_NO_XFTFREETYPE
    if ( encoding == "*-*" ) {
	int slant_value;
	double size_value;

	if ( weight == 0 )
	    weight = XFT_WEIGHT_MEDIUM;
	else if ( weight < (QFont::Light + QFont::Normal) / 2 )
	    weight = XFT_WEIGHT_LIGHT;
	else if ( weight < (QFont::Normal + QFont::DemiBold) / 2 )
	    weight = XFT_WEIGHT_MEDIUM;
	else if ( weight < (QFont::DemiBold + QFont::Bold) / 2 )
	    weight = XFT_WEIGHT_DEMIBOLD;
	else if ( weight < (QFont::Bold + QFont::Black) / 2 )
	    weight = XFT_WEIGHT_BOLD;
	else
	    weight = XFT_WEIGHT_BLACK;

	if ( italic )
	    slant_value = XFT_SLANT_ITALIC;
	else if ( oblique )
	    slant_value = XFT_SLANT_OBLIQUE;
	else
	    slant_value = XFT_SLANT_ROMAN;

	size_value = pixelSize;

#if 0
	// ### FIX ME Lars :)
	if ( size_value > MAXFONTSIZE ) {
	    *scale = (double)size_value/(double)MAXFONTSIZE;
	    size_value = MAXFONTSIZE;
	} else {
	    *scale = 1.;
	}
#endif // 0


	const char *generic_value = 0;
	switch ( styleHint ) {
	case QFont::SansSerif:
	default:
	    generic_value = "sans";
	    break;
	case QFont::Serif:
	    generic_value = "serif";
	    break;
	case QFont::TypeWriter:
	    generic_value = "mono";
	    break;
	}

	XftPattern *pattern = XftPatternCreate();
	if ( !pattern ) return 0;

#ifndef QT_XFT2
	XftPatternAddString (pattern, XFT_ENCODING, "iso10646-1");
#endif
	if ( !foundry.isNull() )
	    XftPatternAddString( pattern, XFT_FOUNDRY, foundry.local8Bit().data() );
	if ( !family.isNull() )
	    XftPatternAddString( pattern, XFT_FAMILY, family.local8Bit().data() );
	XftPatternAddString( pattern, XFT_FAMILY, generic_value );

	XftPatternAddInteger( pattern, XFT_SPACING,
			      ( pitch == 'c' ? XFT_CHARCELL :
				( pitch == 'm' ? XFT_MONO : XFT_PROPORTIONAL ) ) );

	XftPatternAddInteger( pattern, XFT_WEIGHT, weight );
	XftPatternAddInteger( pattern, XFT_SLANT, slant_value );
	XftPatternAddDouble( pattern, XFT_PIXEL_SIZE, size_value );

	extern bool qt_use_antialiasing; // defined in qfont_x11.cpp
	if ( !qt_use_antialiasing || styleStrategy & ( QFont::PreferAntialias |
						       QFont::NoAntialias) ) {
	    bool requestAA = ( qt_use_antialiasing &&
			       !( styleStrategy & QFont::NoAntialias ) );
	    XftPatternAddBool( pattern, XFT_ANTIALIAS, requestAA );
	}

	XftResult res;
	XftPattern *result = XftFontMatch( QPaintDevice::x11AppDisplay(),
					   x11Screen, pattern, &res );
	XftPatternDestroy(pattern);

	// We pass a duplicate to XftFontOpenPattern because either xft font
	// will own the pattern after the call or the pattern will be
	// destroyed.
	XftPattern *dup = XftPatternDuplicate( result );
	XftFont *xftfs = XftFontOpenPattern(QPaintDevice::x11AppDisplay(), dup);

	return new FontEngineXft( xftfs, result, 0 );
    }
#endif // QT_NO_XFTFREETYPE

    QCString xlfd = "-";
    xlfd += foundry.isEmpty() ? "*" : foundry.latin1();
    xlfd += "-";
    xlfd += family.isEmpty() ? "*" : family.latin1();

    xlfd += "-";
    if ( weight > 0 && weight <= QFont::Light )
	xlfd += "light";
    else if ( weight <= QFont::Normal )
	xlfd += use_regular ? "regular" : "medium";
    else if ( weight <= QFont::DemiBold )
	xlfd += "demibold";
    else if ( weight <= QFont::Bold )
	xlfd += "bold";
    else if ( weight <= QFont::Black )
	xlfd += "black";
    else
	xlfd += "*";

    xlfd += "-";
    xlfd += ( italic ? "i" : ( oblique ? "o" : "r" ) );

    xlfd += "-*-*-";
    xlfd += QString::number( pixelSize ).latin1();
    xlfd += "-*-*-*-";
    // ### handle cell spaced fonts
    xlfd += pitch;
    xlfd += "-*-" + encoding;

    // qDebug( "xlfd: '%s'", xlfd.data() );

    XFontStruct *xfs;
    if (! (xfs = XLoadQueryFont(QPaintDevice::x11AppDisplay(), xlfd.data() ) ) )
	return 0;

    return new FontEngineXLFD( xfs, xlfd.data(), encoding.data(), 0 );
}



static unsigned int bestFoundry( unsigned int score, int styleStrategy,
				 QtFontFamily *fam, const QString &foundry,
				 QtFontStyle::Key styleKey, int pixelSize, char pitch,
				 QtFontFoundry **best_fnd, QtFontStyle **best_sty,
				 int &best_px, char &best_pt, int &best_encoding_id )
{
    for ( int x = 0; x < fam->count; ++x ) {
	QtFontFoundry *fnd = fam->foundries[x];
	if ( ! foundry.isEmpty() &&
	     ucstricmp( fnd->name, foundry ) != 0 )
	    continue;
	// qDebug( "  foundry '%s'", fnd->name.latin1() );

	QtFontStyle *sty = fnd->style( styleKey );
	if ( !sty && styleKey.italic ) {
	    styleKey.italic = FALSE;
	    styleKey.oblique = TRUE;
	    sty = fnd->style( styleKey );
	}
	if ( !sty ) {
	    // ### do some matching here
	    sty = fnd->styles[0];
	}

	int px = pixelSize;
	if ( sty->smoothScalable )
	    px = SMOOTH_SCALABLE;
	else if ( sty->bitmapScalable && ( styleStrategy & QFont::PreferMatch ) )
	    px = 0;

	QtFontSize *size = sty->pixelSize( px );
	if ( !size ) {
	    // find closest size match
	    unsigned int distance = ~0;
	    for ( int x = 0; x < sty->count; ++x ) {
		unsigned int d = QABS( sty->pixelSizes[x].pixelSize - pixelSize);
		if ( d < distance ) {
		    distance = d;
		    size = sty->pixelSizes + x;
		}
	    }

	    if ( sty->bitmapScalable &&
		 ! ( styleStrategy & QFont::PreferQuality ) &&
		 ( distance * 10 / pixelSize ) > 2 ) {
		px = 0;
		size = sty->pixelSize( px );
	    }
	    Q_ASSERT( size != 0 );
	}

	if ( sty->smoothScalable )
	    px = pixelSize;
	else if ( sty->bitmapScalable && px == 0 )
	    px = pixelSize;
	else
	    px = size->pixelSize;

	int encoding_id = size->encodings[0].encoding;
	char pt = size->encodings[0].pitch;
	for ( int i = 1; i < size->count; i++ ) {
	    // Xft is preferred
	    if ( size->encodings[i].encoding == -1 ) {
		pt = size->encodings[i].pitch;
		encoding_id = -1;
		break;
	    }
	}

	unsigned int this_score = 0;
	if ( ! ( pitch == 'm' && pt == 'c' ) || pitch != pt )
	    this_score += 200;
	if ( ! ( styleKey == sty->key ) )
	    this_score += 100;
	if ( px != size->pixelSize && !sty->smoothScalable ) // bitmap scaled
	    this_score += 50;
	if ( encoding_id != -1 )
	    this_score += 1;

	if ( this_score < score ) {
	    // qDebug( "found a match: this_score %u score %u", this_score, score );

	    score = this_score;
	    *best_fnd = fnd;
	    *best_sty = sty;
	    best_px = px;
	    best_pt = pt;
	    best_encoding_id = encoding_id;

	    // qDebug( "  has weight %d italic %d oblique %d",
	    // (*best_sty)->key.weight,
	    // (*best_sty)->key.italic,
	    // (*best_sty)->key.oblique );
	    // qDebug( "  has size %d pitch '%c' encoding id %d '%s'",
	    // best_px, best_pt, best_encoding_id,
	    // xlfd_for_id( best_encoding_id ) );
	}
    }

    return score;
}



FontEngineIface *QFontDatabase::findFont( QFont::Script script,
					  int styleStrategy, int styleHint,
					  const QString &family, const QString &foundry,
					  int weight, bool italic,
					  int pixelSize, char pitch, int x11Screen )
{
    qDebug( "---> QFontDatabase::findFont: looking for script %d '%s'",
	    script, scriptName( script ).latin1() );

    FontEngineIface *fe = 0;

    QtFontFamily *best_fam = 0;
    QtFontFoundry *best_fnd = 0;
    QtFontStyle *best_sty = 0;
    int best_px = 0;
    char best_pt = '*';
    int best_encoding_id = -2;

    QtFontStyle::Key styleKey;
    styleKey.italic = italic;
    styleKey.weight = weight;

    // qDebug( "  trying script %d (index %d)",
    // scriptlist[script_index], script_index );

    load( family, script, x11Screen );

    unsigned int score = ~0;
    for ( int x = 0; x < db->count; ++x ) {
	QtFontFamily *fam = db->families[x];
	if ( !family.isEmpty() && ucstricmp( fam->name, family ) != 0 )
	    continue;

	if ( family.isNull() )
	    load( fam->name, script, x11Screen );

	if ( !(fam->scripts[script] & QtFontFamily::Supported) ) {
	    // should never get here with partial loads
	    Q_ASSERT( fam->scripts[script] == QtFontFamily::UnSupported );
	    if ( !(fam->scripts[QFont::UnknownScript] & QtFontFamily::Supported) )
		continue;
	}

	// as we know the script is supported, we can be sure to find a matching font here.

	unsigned int newscore =
	    bestFoundry( score, styleStrategy,
			 fam, foundry, styleKey, pixelSize, pitch,
			 &best_fnd, &best_sty, best_px, best_pt,
			 best_encoding_id );
	if ( best_fnd == 0 ) {
	    newscore =  bestFoundry( score, styleStrategy,
				     fam, QString::null, styleKey, pixelSize, pitch,
				     &best_fnd, &best_sty, best_px, best_pt,
				     best_encoding_id );
	}

	if ( newscore < score ) {
	    score = newscore;
	    best_fam = fam;
	}
	if ( newscore < 10 )
	    break;
    }
    if ( best_fam == 0 || best_fnd == 0 || best_sty == 0 )
	return 0;

    qDebug( "BEST: family '%s' foundry '%s'",
	    best_fam->name.latin1(), best_fnd->name.latin1() );
    // qDebug( "  using weight %d italic %d oblique %d",
    // best_sty->key.weight, best_sty->key.italic, best_sty->key.oblique );
    // qDebug( "  using size %d pitch '%c' encoding id %d '%s'",
    // best_px, best_pt, best_encoding_id, xlfd_for_id( best_encoding_id ) );

    fe = loadEngine( styleStrategy, styleHint, best_fam->name, best_fnd->name,
		     best_sty->key.weight, best_sty->key.italic,
		     best_sty->key.oblique,
		     best_px, best_pt, best_sty->xlfd_uses_regular,
		     xlfd_for_id( best_encoding_id ), x11Screen );

    // qDebug( "  fontengine %p", fe );

    QChar sample = sampleCharacter( script );
    if ( fe && !canRender( fe, sample ) ) {
	qWarning( "font loaded can't render sample 0x%04x", sample.unicode() );
	delete fe;
	fe = 0;
    }

    return fe;
}
