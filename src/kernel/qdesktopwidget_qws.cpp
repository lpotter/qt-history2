//depot/qt/main/src/kernel/qdesktopwidget_qws.cpp#3 - edit change 33507 (text)
#include "qdesktopwidget.h"

class QDesktopWidgetPrivate
{
public:
    QDesktopWidgetPrivate();

    int appScreen;
    int screenCount;

    QArray<QRect> rects;
};

QDesktopWidgetPrivate::QDesktopWidgetPrivate()
{
    appScreen = 0;
    screenCount = 1;

    rects.resize( screenCount );
    //### Get the rects for the different screens and put them into rects
}

QDesktopWidget::QDesktopWidget()
: QWidget( 0, "desktop", WType_Desktop )
{
    d = new QDesktopWidgetPrivate;
}

QDesktopWidget::~QDesktopWidget()
{
    delete d;
}

int QDesktopWidget::primaryScreen() const
{
    return d->appScreen;
}

int QDesktopWidget::numScreens() const
{
    return d->screenCount;
}

QWidget *QDesktopWidget::screen( int )
{
    return this;
}

const QRect& QDesktopWidget::screenGeometry( int ) const
{
    return frameGeometry();
}

int QDesktopWidget::screenNumber( QWidget * ) const
{
    return d->appScreen;
}

int QDesktopWidget::screenNumber( const QPoint & ) const
{
    return d->appScreen;
}
