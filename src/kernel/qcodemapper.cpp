/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qcodemapper.cpp#4 $
**
** Implementation of QCodeMapper class
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

#include "qlist.h"
#include "qcodemapper.h"
#include <stdlib.h>
#include <ctype.h>

static QList<QCodeMapper> all;

/*!
  \class QCodeMapper
  \brief Provides conversion between text encodings.

  By making objects of subclasses of QCodeMapper, support for
  new encodings can be added to Qt.
*/

QCodeMapper::QCodeMapper()
{
    all.append(this);
}

QCodeMapper::~QCodeMapper()
{
    all.remove(this);
}

/*!
  Returns a value indicating how likely this decoder is
  for decoding some format that has the given name.  The
  default implementation does some very minor character-skipping
  so that almost-extact matches score high.

  A good match returns a positive number around the length of
  the string.  A bad match is negative.
*/
int QCodeMapper::heuristicNameMatch(const char* hint) const
{
    const char* actual = name();
    int r = -10;
    int toggle = 0;
    while ( *hint && *actual ) {
	// Skip punctuation
	while ( hint[1] && !isalnum(*hint) )
	    hint++;
	while ( actual[1] && !isalnum(*actual) )
	    actual++;

	if ( tolower(*hint) == tolower(*actual) ) {
	    hint++;
	    actual++;
	    r+=3;
	} else if ( tolower(hint[1]) == tolower(*actual) ) {
	    hint++;
	    r+=1;
	} else if ( tolower(*hint) == tolower(actual[1]) ) {
	    actual++;
	    r+=1;
	} else {
	    if ( toggle ) {
		actual++;
		r--;
	    } else {
		hint++;
		r--;
	    }
	    toggle = !toggle;
	}
    }
    return r;
}

/*!
  Returns the QCodeMapper which matches the MIBenum \a mib.
*/
QCodeMapper* QCodeMapper::mapperForMib(int mib)
{
    QListIterator<QCodeMapper> i(all);
    QCodeMapper* result;
    for ( ; (result=i); ++i ) {
	if ( result->mib()==mib )
	    break;
    }
    return result;
}

/*!
  Searches all installed QCodeMapper objects, returning the one
  which best matches given name.  May return NULL.

  \sa heuristicNameMatch()
*/
QCodeMapper* QCodeMapper::mapperForName(const char* hint)
{
    QListIterator<QCodeMapper> i(all);
    QCodeMapper* result;
    int best=0;
    for ( QCodeMapper* cursor; (cursor=i); ++i ) {
	int s = cursor->heuristicNameMatch(hint);
	if ( s > best ) {
	    best = s;
	    result = cursor;
	}
    }
    return result;
}

/*!
  Searches all installed QCodeMapper objects, returning the one
  which most recognizes the given content.  May return NULL.

  \sa heuristicContentMatch()
*/
QCodeMapper* QCodeMapper::mapperForContent(const char* chars, int len)
{
    QListIterator<QCodeMapper> i(all);
    QCodeMapper* result;
    int best=0;
    for ( QCodeMapper* cursor; (cursor=i); ++i ) {
	int s = cursor->heuristicContentMatch(chars,len);
	if ( s > best ) {
	    best = s;
	    result = cursor;
	}
    }
    return result;
}

/*!
  \fn const char* QCodeMapper::name() const
  Subclasses of QCodeMapper must override this function.  It returns
  the name of the encoding supported by the subclass.
*/

/*!
  \fn int QCodeMapper::mib() const
  Subclasses of QCodeMapper must override this function.  It returns
  the MIBenum (see ######) for this encoding.
*/

/*!
  \fn QString QCodeMapper::toUnicode(const char* chars, int len) const
  Subclasses of QCodeMapper must override this function.  It converts
  the first \a len characters of \a chars to Unicode.
*/

/*!
  \fn char* QCodeMapper::fromUnicode(const QString& uc, int& len_in_out) const
  Subclasses of QCodeMapper must override this function.  It converts
  the first \a len_in_out characters of \a uc from Unicode to
  the encoding of the subclass.  The value returns is the property
  of the caller, which is responsible for deleting it with "delete []".
  The length of the resulting character sequence is returned in \a len_in_out.
*/

/*!
  \fn int QCodeMapper::heuristicContentMatch(const char* chars, int len) const

  Subclasses of QCodeMapper must override this function.  It examines
  the first \a len bytes of \a chars and returns a value indicating how
  likely it is that the string is a prefix of text encoded in the
  encoding of the subclass.  Any negative return value indicates that the text
  is detectably not in the encoding (eg. it contains undefined characters).
  A return value of 0 indicates that the text should be decoded with this
  mapper rather than as ASCII, but there
  is no particular evidence.  The value should range up to \a len.  Thus,
  most decoders will return -1, 0, or -\a len.

  The characters are not null terminated.

  \sa mapperForContent().
*/

