#include "worker.h"
#include "tools.h"

Worker::Worker()
{
    status = "Idle";
}

void Worker::setStatusString(const QString &string)
{
    status = string;
    emit statusStringChanged(status);
}

QString Worker::statusString() const
{
    return status;
}
