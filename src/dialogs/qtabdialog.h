/****************************************************************************
** $Id: //depot/qt/main/src/dialogs/qtabdialog.h#1 $
**
** Definition of tab dialog
**
** Copyright (C) 1996 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#ifndef QTABDLG_H
#define QTABDLG_H

#include "qdialog.h"

class QTab;

class QTabDialog : public QDialog
{
    Q_OBJECT

public:
    QTabDialog( QWidget *parent=0, const char *name=0, WFlags f=0 );
    ~QTabDialog();

    void show();

    void addTab( QWidget *, const char * );

    void setDefaultButton( bool enable );
    bool hasDefaultButton() { return db; }

    void setCancelButton( bool enable );
    bool hasCancelButton() { return cb; }

    bool eventFilter( QObject *, QEvent * );

protected:
    void paintEvent( QPaintEvent * );
    void resizeEvent( QResizeEvent * );

signals:
    void apply();
    void collect();
    void defaultButtonPressed();
    void cancelButtonPressed();
    void okButtonPressed();

private:
    void showTab();
    void setButtonSizes();

    QTab * tabs;
    QTab * currentTab;
    QPushButton * ok;
    QPushButton * cb;
    QPushButton * db;
    QRect childRect;

    friend class QTab;
};



#endif
