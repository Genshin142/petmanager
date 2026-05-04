#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QMessageLogContext>

namespace Logger {
    void init();
    void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
}

#endif // LOGGER_H
