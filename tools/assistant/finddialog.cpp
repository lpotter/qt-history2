
#include "finddialog.h"
#include "mainwindow.h"
#include "tabbedbrowser.h"
#include "helpwindow.h"

#include <qtextbrowser.h>
#include <qstatusbar.h>
#include <qlineedit.h>

FindDialog::FindDialog(MainWindow *parent)
    : QDialog(parent)
{
    gui.setupUI(this);

    lastBrowser = 0;
    onceFound = false;
    findExpr.clear();
    sb = new QStatusBar(this);

    if (layout())
        layout()->addWidget(sb);

    sb->message(tr("Enter the text you are looking for."));
}

FindDialog::~FindDialog()
{
}

void FindDialog::on_findButton_clicked()
{
    doFind(gui.radioForward->isChecked());
}

void FindDialog::on_closeButton_clicked()
{
    reject();
}

void FindDialog::doFind(bool forward)
{
    QTextBrowser *browser = static_cast<QTextBrowser*>(mainWindow()->browsers()->currentBrowser());
    sb->clear();

    if (gui.comboFind->currentText() != findExpr || lastBrowser != browser)
        onceFound = false;
    findExpr = gui.comboFind->currentText();

    bool found;
    if (browser->hasSelectedText()) { // Search either forward or backward from cursor.
        found = browser->find(findExpr, gui.checkCase->isChecked(), gui.checkWords->isChecked(),
                              forward);
    } else {
        int para = forward ? 0 : INT_MAX;
        int index = forward ? 0 : INT_MAX;
        found = browser->find(findExpr, gui.checkCase->isChecked(), gui.checkWords->isChecked(),
                              forward, &para, &index);
    }

    if (!found) {
        if (onceFound) {
            if (forward)
                statusMessage(tr("Search reached end of the document"));
            else
                statusMessage(tr("Search reached start of the document"));
        } else {
            statusMessage(tr( "Text not found" ));
        }
    }
    onceFound |= found;
    lastBrowser = browser;
}

bool FindDialog::hasFindExpression() const
{
    return !findExpr.isEmpty();
}

void FindDialog::statusMessage(const QString &message)
{
    if (isVisible())
        sb->message(message);
    else
        static_cast<MainWindow*>(parent())->statusBar()->message(message, 2000);
}

MainWindow *FindDialog::mainWindow() const
{
    return static_cast<MainWindow*>(parentWidget());
}

void FindDialog::reset()
{
    gui.comboFind->setFocus();
    gui.comboFind->lineEdit()->setSelection(
        0, gui.comboFind->lineEdit()->text().length());
}

