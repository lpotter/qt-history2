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

#ifndef DEFS_H
#define DEFS_H

#include <qsizepolicy.h>
#include <qstring.h>

#define POINTER_TOOL 32000
#define CONNECT_TOOL 32001
#define ORDER_TOOL 32002
#define BUDDY_TOOL 32004

int size_type_to_int( QSizePolicy::SizeType t );
QString size_type_to_string( QSizePolicy::SizeType t );
QSizePolicy::SizeType int_to_size_type( int i );

#endif
