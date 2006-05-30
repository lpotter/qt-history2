/****************************************************************************
**
** Copyright (C) 2005-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QtGui>

#include "tictactoe.h"

TicTacToe::TicTacToe(QWidget *parent)
    : QWidget(parent)
{
}

QSize TicTacToe::minimumSizeHint() const
{
    return QSize(200, 200);
}

QSize TicTacToe::sizeHint() const
{
    return QSize(200, 200);
}

void TicTacToe::setState(const QString &newState)
{
    turnNumber = 0;
    for (int position = 0; position < 9; ++position) {
        if (newState.at(position) != Empty)
            ++turnNumber;
    }
    myState = newState;
    update();
}

QString TicTacToe::state() const
{
    return myState;
}

void TicTacToe::clearBoard()
{
    myState = "---------";
    turnNumber = 0;
    update();
}

void TicTacToe::mousePressEvent(QMouseEvent *event)
{
    if (turnNumber == 9) {
        clearBoard();
        update();
    } else {
        for (int position = 0; position < 9; ++position) {
            QRect cell = cellRect(position / 3, position % 3);
            if (cell.contains(event->pos())) {
                if (myState.at(position) == Empty) {
                    if (turnNumber % 2 == 0)
                        myState.replace(position, 1, Cross);
                    else
                        myState.replace(position, 1, Nought);
                    ++turnNumber;
                    update();
                }
            }
        }
    }
}

void TicTacToe::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(QPen(Qt::darkGreen, 1));
    painter.drawLine(cellWidth(), 0, cellWidth(), height());
    painter.drawLine(2 * cellWidth(), 0, 2 * cellWidth(), height());
    painter.drawLine(0, cellHeight(), width(), cellHeight());
    painter.drawLine(0, 2 * cellHeight(), width(), 2 * cellHeight());

    painter.setPen(QPen(Qt::darkBlue, 2));

    for (int position = 0; position < 9; ++position) {
        QRect cell = cellRect(position / 3, position % 3);

        if (myState.at(position) == Cross) {
            painter.drawLine(cell.topLeft(), cell.bottomRight());
            painter.drawLine(cell.topRight(), cell.bottomLeft());
        } else if (myState.at(position) == Nought) {
            painter.drawEllipse(cell);
        }
    }

    painter.setPen(QPen(Qt::yellow, 3));

    for (int position = 0; position < 9; position = position + 3) {
        if (myState.at(position) != Empty
                && myState.at(position + 1) == myState.at(position)
                && myState.at(position + 2) == myState.at(position)) {
            int y = cellRect((position / 3), 0).center().y();
            painter.drawLine(0, y, width(), y);
            turnNumber = 9;
        }
    }

    for (int position = 0; position < 3; ++position) {
        if (myState.at(position) != Empty
                && myState.at(position + 3) == myState.at(position)
                && myState.at(position + 6) == myState.at(position)) {
            int x = cellRect(0, position).center().x();
            painter.drawLine(x, 0, x, height());
            turnNumber = 9;
        }
    }
    if (myState.at(0) != Empty && myState.at(4) == myState.at(0)
            && myState.at(8) == myState.at(0)) {
        painter.drawLine(0, 0, width(), height());
        turnNumber = 9;
    }
    if (myState.at(2) != Empty && myState.at(4) == myState.at(2)
            && myState.at(6) == myState.at(2)) {
        painter.drawLine(0, height(), width(), 0);
        turnNumber = 9;
    }
}

QRect TicTacToe::cellRect(int row, int column) const
{
    const int HMargin = width() / 30;
    const int VMargin = height() / 30;
    return QRect(column * cellWidth() + HMargin,
                 row * cellHeight() + VMargin,
                 cellWidth() - 2 * HMargin,
                 cellHeight() - 2 * VMargin);
}
