#ifndef PLAY_H
#define PLAY_H

#include "qsound.h"
#include <qapp.h>
#include <qmainwindow.h>

class SoundPlayer : public QMainWindow {
    Q_OBJECT
public:
    SoundPlayer();

public slots:
    void doPlay1();
    void doPlay2();
    void doPlay3();
    void doPlay4();

private:
    QSound bucket3;
    QSound bucket4;
};

#endif
