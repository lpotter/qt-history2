/****************************************************************************
** $Id$
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#include <qapplication.h>
#include <qevent.h>
#include <qsqldatabase.h>
#include <qsqltable.h>
#include <qsqlcursor.h>
#include "book.h"
#include "editbook.h"

class QSqlRecord;


class AuthorCursor : public QSqlCursor
{
    public:
	AuthorCursor();
	QSqlRecord *primeInsert();
};


class BookForm : public BookFormBase
{
    Q_OBJECT
public:
    BookForm( QWidget * parent = 0, const char * name = 0, bool modal = FALSE, WFlags f = 0 );
public slots:
    void slotNewAuthor( const QSqlRecord *authorRecord );
    void slotInsertBook();
    void slotUpdateBook();
    void slotDeleteBook();
private:
    AuthorCursor authorCursor;
};




