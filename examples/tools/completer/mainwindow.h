/****************************************************************************
**
** Copyright (C) 2006-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QAbstractItemModel;
class QComboBox;
class QCompleter;
class QLabel;
class QLineEdit;
class QProgressBar;
class QCheckBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);

private slots:
    void about();
    void changeCase(int);
    void changeMode(int);
    void changeModel();

private:
    void createMenu();
    QAbstractItemModel *modelFromFile(const QString& fileName);

    QComboBox *caseCombo;
    QComboBox *modeCombo;
    QComboBox *modelCombo;
    QCheckBox *wrapCheckBox;
    QCompleter *completer;
    QLabel *contentsLabel;
    QComboBox *comboBox;
    QLineEdit *lineEdit;
};

#endif // MAINWINDOW_H
