/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qptd_win.cpp#2 $
**
** Implementation of QPaintDevice class for Windows + NT
**
** Author  : Haavard Nord
** Created : 940801
**
** Copyright (C) 1994 by Troll Tech as.  All rights reserved.
**
*****************************************************************************/

#include "qpaintd.h"

#if defined(DEBUG)
static char ident[] = "$Id: //depot/qt/main/src/kernel/qptd_win.cpp#2 $";
#endif


QPaintDevice::QPaintDevice()
{
    devFlags = PDT_UNDEF;
}

QPaintDevice::~QPaintDevice()
{
}
