/****************************************************************************
**
** Copyright (C) 2004-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of an example program for Qt.
** EDITIONS: NOLIMITS
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

/*
screenwidget.cpp

A widget to display colour components from an image using independently
selected colors. Controls are provided to allow the image to be inverted, and
the color to be selection via a standard dialog. The image is displayed in a
label widget.
*/

#include <QApplication>
#include <QColorDialog>
#include <QDrag>
#include <QGridLayout>
#include <QImage>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPixmap>
#include <QPushButton>
#include <QWidget>

#include "screenwidget.h"

/*!
Initializes the paint color, the mask color (cyan, magenta,
or yellow), connects the color selector and invert checkbox to functions,
and creates a two-by-two grid layout.
*/

ScreenWidget::ScreenWidget(QWidget *parent, QColor initialColor,
                           const QString &name, Separation mask)
    : QFrame(parent)
{
    paintColor = initialColor;
    maskColor = mask;
    inverted = false;

    imageLabel = new QLabel(this);

    nameLabel = new QLabel(name, this);
    colorButton = new QPushButton(this);
    colorButton->setBackgroundRole(QPalette::Button);
    colorButton->setMinimumSize(32, 32);

    QPalette palette(colorButton->palette());
    palette.setColor(QPalette::Button, initialColor);
    colorButton->setPalette(palette);

    invertButton = new QPushButton(tr("Invert"), this);
    //invertButton->setToggleButton(true);
    //invertButton->setOn(inverted);
    invertButton->setEnabled(false);

    connect(colorButton, SIGNAL(clicked()), this, SLOT(setColor()));
    connect(invertButton, SIGNAL(clicked()), this, SLOT(invertImage()));

    grid = new QGridLayout(this);
    grid->addWidget(imageLabel, 0, 0, 1, 2);
    grid->addWidget(nameLabel, 1, 0);
    grid->addWidget(colorButton, 1, 1);
    grid->addWidget(invertButton, 2, 1, 1, 1);
}

/*!
    Creates a new image by separating out the cyan, magenta, or yellow
    component, depending on the mask color specified in the constructor.

    The amount of the component found in each pixel of the image is used
    to determine how much of a user-selected ink is used for each pixel
    in the new image for the label widget.
*/

void ScreenWidget::createImage()
{
    newImage = originalImage.copy();

    // Create CMY components for the ink being used.
    float cyanInk = (255 - paintColor.red())/255.0;
    float magentaInk = (255 - paintColor.green())/255.0;
    float yellowInk = (255 - paintColor.blue())/255.0;

    int (*convert)(QRgb);

    switch (maskColor) {
        case Cyan:
            convert = qRed;
            break;
        case Magenta:
            convert = qGreen;
            break;
        case Yellow:
            convert = qBlue;
            break;
    }

    for (int y = 0; y < newImage.height(); ++y) {
        for (int x = 0; x < newImage.width(); ++x) {
            QRgb p(originalImage.pixel(x, y));

            // Separate the source pixel into its cyan component.
            int amount;

            if (inverted)
                amount = convert(p);
            else
                amount = 255 - convert(p);

            QColor newColor(
                255 - qMin(int(amount * cyanInk), 255),
                255 - qMin(int(amount * magentaInk), 255),
                255 - qMin(int(amount * yellowInk), 255));

            newImage.setPixel(x, y, newColor.rgb());
        }
    }

    imageLabel->setPixmap(QPixmap(newImage));
}

/*!
    Returns a pointer to the modified image.
*/

QImage* ScreenWidget::image()
{
    return &newImage;
}

/*!
    Sets whether the amount of ink applied to the canvas is to be inverted
    (subtracted from the maximum value) before the ink is applied.
*/

void ScreenWidget::invertImage()
{
    //inverted = invertButton->isOn();
    inverted = !inverted;
    createImage();
    emit imageChanged();
}

/*!
    Separate the current image into cyan, magenta, and yellow components.
    Create a representation of how each component might appear when applied
    to a blank white piece of paper.
*/

void ScreenWidget::setColor()
{
    QColor newColor = QColorDialog::getColor(paintColor);

    if (newColor.isValid()) {
        paintColor = newColor;
        QPalette palette(colorButton->palette());
        palette.setColor(QPalette::Button, paintColor);
        colorButton->setPalette(palette);
        createImage();
        emit imageChanged();
    }
}

/*!
    Records the original image selected by the user, creates a color
    separation, and enables the invert image checkbox.
*/

void ScreenWidget::setImage(QImage &image)
{
    originalImage = image;
    createImage();
    invertButton->setEnabled(true);
}
