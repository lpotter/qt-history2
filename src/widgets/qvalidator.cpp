/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qvalidator.cpp#29 $
**
** Implementation of validator classes.
**
** Created : 970610
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Troll Tech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** Licensees holding valid Qt Professional Edition licenses may use this
** file in accordance with the Qt Professional Edition License Agreement
** provided with the Qt Professional Edition.
**
** See http://www.troll.no/pricing.html or email sales@troll.no for
** information about the Professional Edition licensing, or see
** http://www.troll.no/qpl/ for QPL licensing information.
**
*****************************************************************************/

#include "qvalidator.h"
#include "qwidget.h"
#include "qregexp.h"

#include <limits.h> // *_MIN, *_MAX
#include <ctype.h> // isdigit

/*!
  \class QValidator qvalidator.h

  \brief The QValidator class provides validation of input text.

  \ingroup misc

  The class itself is abstract; two subclasses provide rudimentary
  numeric range checking.

  The class includes two virtual functions, validate() and fixup().

  validate() is pure virtual, so it must be implemented by every
  subclass.  It returns Invalid, Valid or Acceptable depending on
  whether its argument is valid (for the class' definition of valid).

  The three states require some explanation.  An \c Invalid string is
  \e clearly invalid.  \c Valid is less obvious - the concept of
  validity is slippery when the string is incomplete (still being
  edited).  QValidator defines \c Valid as the property of a string
  that it is not clearly invalid.  \c Acceptable means that the string
  is acceptable as a final result.  One might say that any string that
  is a plausible intermediate state during entry of an \c Acceptable
  string is \c Valid.

  Here are some examples: <ol>

  <li>For a line edit that accepts integers from 0 to 999 inclusive,
  42 and 666 are \c Acceptable, the empty string and 1114 are \c Valid
  and asdf is \c Invalid.

  <li>For an editable combo box that accepts URLs, any well-formed URL
  is \c Acceptable, "http://www.troll.no/," is \c Valid (it can be a
  cut-and-paste job that accidentally took in a comma at the end), the
  empty string is valid (the user might select and delete all of the
  text in preparation to entering a new URL), and "http:///./moron"
  is \c Invalid.

  <li>For a spin box that accepts lengths, "11cm" and "1in" are \c
  Acceptable, "11" and the empty string are \c Valid, and
  "http://www.troll.no" and "hour" are \c Invalid.

  fixup() is provided for validators that can repair some or all user
  errors.  The default does nothing.  QLineEdit, for example, will
  call fixup() if the user presses Return and the content is not
  currently valid, in case fixup() can do magic.  This allows some \c
  Invalid strings to be made \c Acceptable, too, spoiling the muddy
  definition above even more.

  QValidator is generally used with QLineEdit, QSpinBox and QComboBox.
*/

/*!
  Sets up the internal data structures used by the validator.  At
  the moment there aren't any.
*/

QValidator::QValidator( QWidget * parent, const char *name )
    : QObject( parent, name )
{
}


/*!
  Deletes the validator and frees any storage and other resources
  used.
*/

QValidator::~QValidator()
{
}


/*!
  \fn QValidator::State QValidator::validate( QString & input, int & pos )

  This pure virtual function returns \c Invalid if \a input is invalid
  according to this validator's rules, \c Valid if it is likely that a
  little more editing will make the input acceptable (e.g. the user
  types '4' into a widget which accepts 10-99) and \c Acceptable if
  the input is completely acceptable.

  The function can change \a input and \a pos (the cursor position) if
  it wants to.
*/


/*!
  Attempts to change \a to be valid according to this validator's
  rules.  Need not result in a valid string - callers of this function
  must re-test afterwards.  The default does nothing.

  Reimplementation notes:

  Note that \a input may not be the only QString object referencing
  this string, so it's almost always necessary to detach() the string
  at the start of the code:

  \code
    input.detach();
  \endcode

  You can change \a input even if you aren't able to produce a valid
  string.  For example an ISBN validator might want to delete every
  character except digits and "-", even if the result is not a valid
  ISBN, and a last-name validator might want to remove white space
  from the start and end of the string, even if the resulting string
  is not in the list of known last names.
*/

void QValidator::fixup( QString & input ) const
{
    // just be a little clever to the compiler won't complain
    if ( input.length() )
	;
    else
	;
    // hopefully the compiler will think "oh, #ifdeffed out".
}


/*!
  \class QIntValidator qvalidator.h

  \brief The QIntValidator class provides range checking of integers.

  \ingroup misc

  QIntValidator provides a lower and an upper bound.  It does not
  provide a fixup() function.

  \sa QDoubleValidator
*/


/*!
  Creates a validator object which accepts all integers.
*/

QIntValidator::QIntValidator( QWidget * parent, const char *name )
    : QValidator( parent, name )
{
    b = INT_MIN;
    t = INT_MAX;
}


/*!
  Creates a validator object which accepts all integers from \a
  bottom up to and including \a top.
*/

QIntValidator::QIntValidator( int bottom, int top,
			      QWidget * parent, const char* name )
    : QValidator( parent, name )
{
    b = bottom;
    t = top;
}


/*!
  Deletes the validator and frees up any storage used.
*/

QIntValidator::~QIntValidator()
{
    // nothing
}


/*!
  Returns \a Acceptable if \a input contains a number in the legal
  range, \a Valid if it contains another integer or is empty, and \a
  Invalid if \a input is not an integer.
*/

QValidator::State QIntValidator::validate( QString & input, int & ) const
{
    QRegExp empty( QString::fromLatin1("^ *-? *$") );
    if ( empty.match( input ) >= 0 )
	return QValidator::Valid;
    bool ok;
    long int tmp = input.toLong( &ok );
    if ( !ok )
	return QValidator::Invalid;
    else if ( tmp < b || tmp > t )
	return QValidator::Valid;
    else
	return QValidator::Acceptable;
}


/*!
  Sets the validator to accept only number from \a bottom up to an
  including \a top.
*/

void QIntValidator::setRange( int bottom, int top )
{
    b = bottom;
    t = top;
}


/*!
  \fn int QIntValidator::bottom() const

  Returns the lowest valid number according to this validator.

  \sa top() setRange()
*/


/*!
  \fn int QIntValidator::top() const

  Returns the highest valid number according to this validator.

  \sa bottom() setRange()
*/


/*!
  \class QDoubleValidator qvalidator.h

  \brief The QDoubleValidator class provides range checking of
  floating-point numbers.

  \ingroup misc

  QDoubleValidator provides an upper bound, a lower bound, and a limit
  on the number of digits after the decimal point.  It does not
  provide a fixup() function.

  \sa QIntValidator
*/

/*!
  Creates a validator object which accepts all double from 2.7182818
  to 3.1415926 (please, no bug reports) with at most seven digits after
  the decimal point.

  This constructor is not meant to be useful; it is provided for
  completeness.
*/

QDoubleValidator::QDoubleValidator( QWidget * parent, const char *name )
    : QValidator( parent, name )
{
    b = 2.7182818;
    t = 3.1415926;
    d = 7;
}


/*!
  Creates a validator object which accepts all doubles from \a
  bottom up to and including \a top with at most \a decimals digits
  after the decimal point.
*/

QDoubleValidator::QDoubleValidator( double bottom, double top, int decimals,
				    QWidget * parent, const char* name )
    : QValidator( parent, name )
{
    b = bottom;
    t = top;
    d = decimals;
}


/*!
  Deletes the validator and frees any storage and other resources
  used.
*/

QDoubleValidator::~QDoubleValidator()
{
    // nothing
}


/*!
  Returns \a Acceptable if \a input contains a number in the legal
  range and format, \a Valid if it contains another number, a number
  with too many digits after the decimal point or is empty, and \a
  Invalid if \a input is not a number.
*/

QValidator::State QDoubleValidator::validate( QString & input, int & ) const
{
    QRegExp empty( QString::fromLatin1("^ *-? *$") );
    if ( empty.match( input ) >= 0 )
	return QValidator::Valid;
    bool ok = TRUE;
    double tmp = input.toDouble( &ok );
    if ( !ok )
	return QValidator::Invalid;

    int i = input.find( '.' );
    if ( i >= 0 ) {
	// has decimal point, now count digits after that
	i++;
	int j = i;
	while( isdigit( input[j] ) )
	    j++;
	if ( j - i > d )
	    return QValidator::Valid;
    }

    if ( tmp < b || tmp > t )
	return QValidator::Valid;
    else
	return QValidator::Acceptable;
}


/*!
  Sets the validator to accept numbers from \a bottom up to and
  including \a top with at most \a decimals digits after the decimal
  point.
*/

void QDoubleValidator::setRange( double bottom, double top, int decimals )
{
    b = bottom;
    t = top;
    d = decimals;
}


/*!
  \fn double QDoubleValidator::bottom() const

  Returns the lowest valid number according to this validator.

  \sa top() decimals() setRange()
*/


/*!
  \fn double QDoubleValidator::top() const

  Returns the highest valid number according to this validator.

  \sa bottom() decimals setRange()
*/


/*!
  \fn int QDoubleValidator::decimals() const

  Returns the largest number of digits a valid number can have after
  its decimal point.

  \sa bottom() top() setRange()
*/
