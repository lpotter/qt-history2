/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <q3mainwindow.h>
#include <qmap.h>

class Q3Action;
class QComboBox;
class QTabWidget;
class Q3TextEdit;

class TextEdit : public Q3MainWindow
{
    Q_OBJECT

public:
    TextEdit( QWidget *parent = 0, const char *name = 0 );

    Q3TextEdit *currentEditor() const;
    void load( const QString &f );


public slots:
    void fileNew();
    void fileOpen();
    void fileSave();
    void fileSaveAs();
    void filePrint();
    void fileClose();
    void fileExit();

    void editUndo();
    void editRedo();
    void editCut();
    void editCopy();
    void editPaste();

    void textBold();
    void textUnderline();
    void textItalic();
    void textFamily( const QString &f );
    void textSize( const QString &p );
    void textStyle( int s );
    void textColor();
    void textAlign( Q3Action *a );

    void fontChanged( const QFont &f );
    void colorChanged( const QColor &c );
    void alignmentChanged( int a );
    void editorChanged( QWidget * );


private:
    void setupFileActions();
    void setupEditActions();
    void setupTextActions();
    void doConnections( Q3TextEdit *e );

    Q3Action *actionTextBold,
	*actionTextUnderline,
	*actionTextItalic,
	*actionTextColor,
	*actionAlignLeft,
	*actionAlignCenter,
	*actionAlignRight,
	*actionAlignJustify;
    QComboBox *comboStyle,
	*comboFont,
	*comboSize;
    QTabWidget *tabWidget;
    QMap<Q3TextEdit*, QString> filenames;

};


#endif
