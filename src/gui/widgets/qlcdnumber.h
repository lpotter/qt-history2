/****************************************************************************
**
** Definition of QLCDNumber class.
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the widgets module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QLCDNUMBER_H
#define QLCDNUMBER_H

#ifndef QT_H
#include "qframe.h"
#include "qbitarray.h"
#endif // QT_H

#ifndef QT_NO_LCDNUMBER


class QLCDNumberPrivate;

class Q_GUI_EXPORT QLCDNumber : public QFrame                // LCD number widget
{
    Q_OBJECT
    Q_ENUMS(Mode SegmentStyle)
    Q_PROPERTY(bool smallDecimalPoint READ smallDecimalPoint WRITE setSmallDecimalPoint)
    Q_PROPERTY(int numDigits READ numDigits WRITE setNumDigits)
    Q_PROPERTY(Mode mode READ mode WRITE setMode)
    Q_PROPERTY(SegmentStyle segmentStyle READ segmentStyle WRITE setSegmentStyle)
    Q_PROPERTY(double value READ value WRITE display)
    Q_PROPERTY(int intValue READ intValue WRITE display)

public:
#ifdef QT_COMPAT
    QT_COMPAT_CONSTRUCTOR QLCDNumber(QWidget* parent, const char* name);
    QT_COMPAT_CONSTRUCTOR QLCDNumber(uint numDigits, QWidget* parent, const char* name);
#endif
    QLCDNumber(QWidget* parent = 0);
    QLCDNumber(uint numDigits, QWidget* parent = 0);
    ~QLCDNumber();

    enum Mode { Hex, Dec, Oct, Bin, HEX = Hex, DEC = Dec, OCT = Oct,
                BIN = Bin };
    enum SegmentStyle { Outline, Filled, Flat };

    bool    smallDecimalPoint() const;

    int            numDigits() const;
    virtual void setNumDigits(int nDigits);

    bool    checkOverflow(double num) const;
    bool    checkOverflow(int          num) const;

    Mode mode() const;
    virtual void setMode(Mode);

    SegmentStyle segmentStyle() const;
    virtual void setSegmentStyle(SegmentStyle);

    double  value() const;
    int            intValue() const;

    QSize sizeHint() const;

public slots:
    void    display(const QString &str);
    void    display(int num);
    void    display(double num);
    virtual void setHexMode();
    virtual void setDecMode();
    virtual void setOctMode();
    virtual void setBinMode();
    virtual void setSmallDecimalPoint(bool);

signals:
    void    overflow();

protected:
    void    paintEvent(QPaintEvent *);

private:
    void    init();
    void    internalDisplay(const QString &);
    void    internalSetString(const QString& s);
    void    drawString(const QString& s, QPainter &, QBitArray * = 0,
                        bool = true);
    //void    drawString(const QString &, QPainter &, QBitArray * = 0) const;
    void    drawDigit(const QPoint &, QPainter &, int, char,
                       char = ' ');
    void    drawSegment(const QPoint &, char, QPainter &, int, bool = false);

    int            ndigits;
    double  val;
    uint    base        : 2;
    uint    smallPoint        : 1;
    uint    fill        : 1;
    uint    shadow        : 1;
    QString digitStr;
    QBitArray points;
    QLCDNumberPrivate * d;

private:        // Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QLCDNumber(const QLCDNumber &);
    QLCDNumber &operator=(const QLCDNumber &);
#endif
};

inline bool QLCDNumber::smallDecimalPoint() const
{ return (bool)smallPoint; }

inline int QLCDNumber::numDigits() const
{ return ndigits; }


#endif // QT_NO_LCDNUMBER

#endif // QLCDNUMBER_H
