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

/*! \class QMovie

    \brief The QMovie class is a convenience class for playing movies
    with QImageReader.

    \ingroup multimedia
    \module gui

    QMovie is a convenience class that uses QImageReader internally to play
    movies.

    First, create a QMovie object by passing either the name of a file or a
    pointer to a QIODevice containing an animated image format to QMovie's
    constructor. You can call isValid() to check if the image data is valid,
    before starting the movie. To start the movie, call start(). QMovie will
    enter \l Running state, and emit started() and stateChanged(). To get the
    current state of the movie, call state().

    To display the movie in your application, you can pass your QMovie object
    to QLabel::setMovie(). Example:

    \code
        QLabel label;
        QMovie *movie = new QMovie("animations/fire.gif");

        label.setMovie(movie);
        movie->start();
    \endcode

    Whenever a new frame is available in the movie, QMovie will emit
    updated(). If the size of the frame changes, resized() is emitted. You can
    call frameImage() or framePixmap() to get a copy of the current
    frame. When the movie is done, QMovie emits finished(). If any error
    occurs during playback (i.e, the image file is corrupt), QMovie will emit
    error().

    You can control the speed of the movie playback by calling setSpeed(),
    which takes the percentage of the original speed as an argument. Pause the
    movie by calling pause(). QMovie will then enter \l Paused state and emit
    stateChanged(). If you call unpause(), QMovie will reenter \l Running
    state and start the movie again. To stop the movie, call stop().

    Certain animation formats allow you to set the background color. You can
    call setBackgroundColor() to set the color, or backgroundColor() to
    retrieve the current background color.

    currentFrameNumber() returns the sequence number of the current frame. The
    first frame in the animation has the sequence number 0. frameCount()
    returns the total number of frames in the animation, if the image format
    supports this. You can call loopCount() to get the number of times the
    movie should loop before finishing. nextFrameDelay() returns the number of
    milliseconds the current frame should be displayed.

    
   \sa QLabel, QImageReader
*/

/*! \enum QMovie::MovieState

    This enum describes the different states of QMovie.

    \value NotRunning The movie is not running. This is QMovie's initial
    state, and the state it enters after stop() has been called or the movie
    is finished.

    \value Paused The movie is paused, and QMovie stops emitting updated() or
    resized(). This state is entered after calling pause() or
    setPaused(true). The current frame number it kept, and the movie will
    continue with the next frame when unpause() or setPaused(false) is called.

    \value Running The movie is running.
*/

#include "qmovie.h"

#include <qimage.h>
#include <qimagereader.h>
#include <qpixmap.h>
#include <qrect.h>
#include <qtimer.h>
#include <private/qobject_p.h>

/*! \internal
 */
class QMoviePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QMovie)
public:
    QMoviePrivate(QMovie *qq);

    // private slots
    void loadNextFrame();

    QImageReader *reader;
    int speed;
    QMovie::MovieState movieState;
    QRect frameRect;
    QImage frameImage;
    QPixmap framePixmap;
    QColor backgroundColor;

    QTimer nextImageTimer;
};

/*! \internal
 */
QMoviePrivate::QMoviePrivate(QMovie *qq)
{
    Q_Q(QMovie);
    q = qq;

    reader = 0;
    movieState = QMovie::NotRunning;
    nextImageTimer.setSingleShot(true);
}

/*! \internal
 */
void QMoviePrivate::loadNextFrame()
{
    Q_Q(QMovie);
    if (reader->canRead()) {
        frameImage = reader->read();
        if (!frameImage.isNull()) {
            if (movieState == QMovie::NotRunning) {
                movieState = QMovie::Running;
                emit q->stateChanged(movieState);
                emit q->started();
            }

            framePixmap = frameImage;
            if (frameRect.size() != frameImage.rect().size()) {
                frameRect = frameImage.rect();
                emit q->resized(frameRect.size());
            }

            emit q->updated(frameRect);

            nextImageTimer.start(reader->nextImageDelay());
        } else {
            emit q->error();
            if (movieState != QMovie::NotRunning) {
                movieState = QMovie::NotRunning;
                emit q->stateChanged(movieState);
                emit q->finished();
            }
        }
        return;
    }

    movieState = QMovie::NotRunning;
    emit q->stateChanged(movieState);
    emit q->finished();
}

/*! 
    Constructs a QMovie object, passing the \a parent object to QObject's
    constructor.

    \sa setFileName(), setDevice(), setFormat()
 */
QMovie::QMovie(QObject *parent)
    : QObject(*new QMoviePrivate(this), parent)
{
    Q_D(QMovie);
    d->reader = new QImageReader;
}

/*! 
    Constructs a QMovie object. QMovie will use read image data from \a
    device, which it assumes is open and readable. If \a format is not empty,
    QMovie will use the image format \a format for decoding the image
    data. Otherwise, QMovie will attempt to guess the format.

    The \a parent object is passed to QObject's constructor.
 */
QMovie::QMovie(QIODevice *device, const QByteArray &format, QObject *parent)
    : QObject(*new QMoviePrivate(this), parent)
{
    Q_D(QMovie);
    d->reader = new QImageReader(device, format);
}

/*! 
    Constructs a QMovie object. QMovie will use read image data from \a
    fileName. If \a format is not empty, QMovie will use the image format \a
    format for decoding the image data. Otherwise, QMovie will attempt to
    guess the format.

    The \a parent object is passed to QObject's constructor.
 */
QMovie::QMovie(const QString &fileName, const QByteArray &format, QObject *parent)
    : QObject(*new QMoviePrivate(this), parent)
{
    Q_D(QMovie);
    d->reader = new QImageReader(fileName, format);
}

/*!
    Destructs the QMovie object.
*/
QMovie::~QMovie()
{
    Q_D(QMovie);
    delete d->reader;
}

/*!
    Sets the current device to \a device. QMovie will read image data from
    this device when the movie is running.

    \sa device(), setFormat()
*/
void QMovie::setDevice(QIODevice *device)
{
    Q_D(QMovie);
    d->reader->setDevice(device);
}

/*!
    Returns the device QMovie reads image data from. If no device has
    currently been assigned, 0 is returned.

    \sa setDevice(), fileName()
*/
QIODevice *QMovie::device() const
{
    Q_D(const QMovie);
    return d->reader->device();
}

/*!
    Sets the name of the file that QMovie reads image data from, to \a
    fileName.

    \sa fileName(), setDevice(), setFormat()
*/
void QMovie::setFileName(const QString &fileName)
{
    Q_D(QMovie);
    d->reader->setFileName(fileName);
}

/*!
    Returns the name of the file that QMovie reads image data from. If no file
    name has been assigned, or if the assigned device is not a file, an empty
    QString is returned.

    \sa setFileName(), device()
*/
QString QMovie::fileName() const
{
    Q_D(const QMovie);
    return d->reader->fileName();
}

/*!
    Sets the format that QMovie will use when decoding image data. By default,
    QMovie will attempt to guess the format of the image data.

    \sa QImageReader::supportedImageFormats()
*/
void QMovie::setFormat(const QByteArray &format)
{
    Q_D(QMovie);
    d->reader->setFormat(format);
}

/*!
    Returns the format that QMovie uses when decoding image data. If no format
    has been assigned, an empty QByteArray() is returned.

    \sa setFormat()
*/
QByteArray QMovie::format() const
{
    Q_D(const QMovie);
    return d->reader->format();
}

/*!
    For image formats that support it, this function sets the background color
    to \a color.

    \sa backgroundColor()
*/
void QMovie::setBackgroundColor(const QColor &color)
{
    Q_D(QMovie);
    d->backgroundColor = color;
}

/*!
    Returns the background color of the movie. If no background color has been
    assigned, an invalid QColor is returned.

    \sa setBackgroundColor()
*/
QColor QMovie::backgroundColor() const
{
    Q_D(const QMovie);
    return d->backgroundColor;
}

/*!
    Returns the current state of QMovie.

    \sa MovieState, stateChanged()
*/
QMovie::MovieState QMovie::state() const
{
    Q_D(const QMovie);
    return d->movieState;
}

/*!
    Returns the rect of the last frame. If no frame has yet been updated, an
    invalid QRect is returned.

    \sa frameImage(), framePixmap()
*/
QRect QMovie::frameRect() const
{
    Q_D(const QMovie);
    return d->frameRect;
}

/*!
    Returns the current frame as a QPixmap.

    \sa frameImage(), updated()
*/
QPixmap QMovie::framePixmap() const
{
    Q_D(const QMovie);
    return d->framePixmap;
}

/*!
    Returns the current frame as a QImage.

    \sa framePixmap(), updated()
*/
QImage QMovie::frameImage() const
{
    Q_D(const QMovie);
    return d->frameImage;
}

/*!
    Returns true if the movie is valid (e.g., the image data is readable and
    the image format is supported); otherwise returns false.
*/
bool QMovie::isValid() const
{
    Q_D(const QMovie);
    return d->reader->canRead();
}

/*!
    Returns true if QMovie is in \l Running state.

    Same as state() == \l QMovie::Running.
*/
bool QMovie::isRunning() const
{
    Q_D(const QMovie);
    return d->movieState == Running;
}

/*!
    Returns the number of frames in the movie.

    Certain animation formats do not support this feature, in which
    case -1 is returned.
*/
int QMovie::frameCount() const
{
    Q_D(const QMovie);
    return d->reader->imageCount();
}

/*!
    Returns the number of milliseconds QMovie will wait before updating the
    next frame in the animation.
*/
int QMovie::nextFrameDelay() const
{
    Q_D(const QMovie);
    return d->reader->nextImageDelay();
}

/*!
    Returns the sequence number of the current frame. The number of the first
    frame in the movie is 0.
*/
int QMovie::currentFrameNumber() const
{
    Q_D(const QMovie);
    return d->reader->currentImageNumber();
}

/*!
    Returns the number of times the movie will loop before it finishes.
*/
int QMovie::loopCount() const
{
    Q_D(const QMovie);
    return d->reader->loopCount();
}

/*!
    If \a paused is true, QMovie will enter \l Paused state and emit
    stateChanged(Paused); otherwise it will enter \l Running state and emit
    stateChanged(Running).

    \sa isPaused(), state()
*/
void QMovie::setPaused(bool paused)
{
    if (paused)
        pause();
    else
        unpause();
}

/*!
    Returns true if the movie is paused; otherwise returns false.

    Same as state() == \l QMovie::Paused.

    \sa isRunning()
 */
bool QMovie::isPaused() const
{
    Q_D(const QMovie);
    return d->movieState == Paused;
}

/*!
    Sets the speed of the movie to \a percentSpeed, in percentage of the
    original speed. Example:

    \code
        QMovie movie("racecar.gif");
        movie.setSpeed(200); // 2x speed
    \endcode

    \sa speed()
*/
void QMovie::setSpeed(int percentSpeed)
{
    Q_D(QMovie);
    d->speed = percentSpeed;
}

/*!
    Returns the current speed of the movie, in percentage of the original
    movie speed. By default, 100 is returned (i.e., 100% of the original
    speed).

    \sa setSpeed()
*/
int QMovie::speed() const
{
    Q_D(const QMovie);
    return d->speed;
}

/*!
    Starts the movie. QMovie will enter \l Running state, and start emitting
    updated() and resized() as the movie progresses.

    \sa stop(), pause()
*/
void QMovie::start()
{
    Q_D(QMovie);
    if (d->movieState == Running) {
        qWarning("QMovie::start() called when state is Running");
        return;
    }

    connect(&d->nextImageTimer, SIGNAL(timeout()), this, SLOT(loadNextFrame()));
    d->loadNextFrame();
}

/*!
    Pauses the movie. QMovie will enter \l Paused state, and stop emitting
    updated() and resized() until the movie is unpaused.

    \sa unpause(), setPaused(), state()
*/
void QMovie::pause()
{
    Q_D(QMovie);
    if (d->movieState == NotRunning) {
        qWarning("QMovie::pause() called when state is NotRunning");
        return;
    }
    emit stateChanged(Paused);
    d->movieState = Paused;
    d->nextImageTimer.stop();
}

/*!
    Unpauses the movie. QMovie will reenter \l Running state, and start
    emitting updated() and resized() as the movie progresses.

    \sa pause(), stop()
*/
void QMovie::unpause()
{
    Q_D(QMovie);
    if (d->movieState == Running) {
        qWarning("QMovie::unpause() called when state is NotRunning");
        return;
    }
    emit stateChanged(Running);
    d->movieState = Running;
    d->nextImageTimer.start(d->reader->nextImageDelay());
}

/*!
    Stops the movie. QMovie enters \l NotRunning state, and stops emitting
    updated() and resized(). If start() is called again, the movie will
    restart from the beginning.

    \sa start(), pause()
*/
void QMovie::stop()
{
    Q_D(QMovie);
    if (d->movieState == NotRunning) {
        qWarning("QMovie::stop() called when state is NotRunning");
        return;
    }
    emit stateChanged(NotRunning);
    d->movieState = NotRunning;
    d->nextImageTimer.stop();
}

#define d d_func()
#include "moc_qmovie.cpp"
