#include <QApplication>

#include "receiver.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Receiver receiver;
    app.setMainWidget(&receiver);
    receiver.show();
    return app.exec();
}
