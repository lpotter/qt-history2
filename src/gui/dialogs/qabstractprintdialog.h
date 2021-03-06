/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QABSTRACTPRINTDIALOG_H
#define QABSTRACTPRINTDIALOG_H

#include <QtGui/qdialog.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_PRINTER

class QAbstractPrintDialogPrivate;
class QPrinter;

// ### Qt 5: remove this class
class Q_GUI_EXPORT QAbstractPrintDialog : public QDialog
{
    Q_DECLARE_PRIVATE(QAbstractPrintDialog)
    Q_OBJECT

public:
    enum PrintRange {
        AllPages,
        Selection,
        PageRange
    };

    enum PrintDialogOption {
        None                    = 0x0000,
        PrintToFile             = 0x0001,
        PrintSelection          = 0x0002,
        PrintPageRange          = 0x0004,
        PrintCollateCopies      = 0x0010,
        DontUseSheet            = 0x0020
    };

    Q_DECLARE_FLAGS(PrintDialogOptions, PrintDialogOption)

#ifndef QT_NO_PRINTDIALOG

    explicit QAbstractPrintDialog(QPrinter *printer, QWidget *parent = 0);

    virtual int exec() = 0;

    void addEnabledOption(PrintDialogOption option);
    void setEnabledOptions(PrintDialogOptions options);
    PrintDialogOptions enabledOptions() const;
    bool isOptionEnabled(PrintDialogOption option) const;

    void setPrintRange(PrintRange range);
    PrintRange printRange() const;

    void setMinMax(int min, int max);
    int minPage() const;
    int maxPage() const;

    void setFromTo(int fromPage, int toPage);
    int fromPage() const;
    int toPage() const;

    QPrinter *printer() const;

protected:
    QAbstractPrintDialog(QAbstractPrintDialogPrivate &ptr, QPrinter *printer, QWidget *parent = 0);

private:
    Q_DISABLE_COPY(QAbstractPrintDialog)

#endif // QT_NO_PRINTDIALOG
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QAbstractPrintDialog::PrintDialogOptions)

#endif // QT_NO_PRINTER

QT_END_NAMESPACE

QT_END_HEADER

#endif // QABSTRACTPRINTDIALOG_H
