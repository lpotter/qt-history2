/**********************************************************************
**
** Copyright (C) 2000 Trolltech AS.  All rights reserved.
**
** This file is part of Qt Designer
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef KDEVDLG2UI_H
#define KDEVDLG2UI_H

#include <qtextstream.h>
#include <qfile.h>
#include <qstringlist.h>

class KDEVDLG2UI
{
public:
    KDEVDLG2UI( QTextStream* input, const QString& name = "dialog" );
    ~KDEVDLG2UI();

    bool parse();
    bool parse ( QStringList& get );

    QStringList targetFiles;

protected:

    bool writeDialog( const QString& name );

    QString className;
    QString line;
    QTextStream *in;
    QStringList target;

    void cleanString( QString* text );

    void indent();
    void undent();
    void wi();

    void writeClass( const QString& name );
    void writeCString( const QString& name, const QString& value );
    void writeString( const QString& name, const QString& value );
    void writeRect( const QString& name, int x, int y, int w, int h );
    void writeFont( const QString& family, int pointsize );
    void writeBool( const QString& name, bool value );
    void writeNumber( const QString& name, int value );
    void writeEnum( const QString& name, const QString& value );
    void writeSet( const QString& name, const QString& value );
    void writeStyles( const QStringList styles, bool isFrame );
    void writeWidgetStart( const QString& qclass );
    void writeWidgetEnd();
    
private:
    int indentation;
    bool writeToFile;
    QTextStream* out;
};

#endif
