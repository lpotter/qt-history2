#ifndef HELPMAINWINDOW_H
#define HELPMAINWINDOW_H

#include <qmainwindow.h>

class HelpView;
class HelpNavigation;
class QPopupMenu;

class HelpMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    HelpMainWindow();

private:
    HelpView *viewer;
    HelpNavigation *navigation;

private slots:
    void slotFilePrint();
    void slotEditCopy();
    void slotEditSelectAll();
    void slotEditFind();
    void slotViewContents();
    void slotViewIndex();
    void slotViewSearch();
    void slotViewBookmarks();
    void slotGoBack();
    void slotGoForward();
    void slotGoHome();
    void slotHelpAbout();
    void slotHelpAboutQt();

    void newSource( const QString &name );
    
private:
    QPopupMenu *history;

};

#endif
