/****************************************************************************
** $Id: //depot/qt/main/src/tools/qregexp.h#1 $
**
** Definition of QRegExp class
**
** Author  : Haavard Nord
** Created : 950126
**
** Copyright (C) 1995 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#ifndef QREGEXP_H
#define QREGEXP_H

#include "qstring.h"


class QRegExp
{
public:
    QRegExp();
    QRegExp( const char *pattern, bool wildcardMode=FALSE );
    QRegExp( const QRegExp & );
   ~QRegExp();
    QRegExp    &operator=( const QRegExp & );
    QRegExp    &operator=( const char *pattern );

    bool	isEmpty() const	     { return rxdata != 0; }
    bool	isValid() const	     { return valid; }

    bool 	wildcardMode() const { return wcmode; }
    void	setWildcardMode( bool );

    int		match( const char *str, int index=0, int *len=0 ) const;

protected:
    void	wc2rx();
    void	compile();
    char       *matchsub( ushort *, char *, char * ) const;

private:
    QString	rxstring;			// regular expression pattern
    ushort     *rxdata;				// compiled regexp pattern
    bool	valid;
    bool	wcmode;
};


#endif // QREGEXP_H
