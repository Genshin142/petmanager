#include <QCoreApplication>
#include <QDebug>
#include "network/server_core.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    qDebug() << "==========================================";
    qDebug() << "   PetManager Backend Server Engine v1.0  ";
    qDebug() << "==========================================";

    ServerCore core;
    quint16 port = 8080;

    if (core.start(port)) {
        qDebug() << "[INIT] Server started successfully.";
        qDebug() << "[INIT] Listening on port:" << port;
        qDebug() << "[INIT] Press Ctrl+C to terminate.";
    } else {
        qCritical() << "[ERR] Failed to start server on port" << port;
        return -1;
    }

    return a.exec();
}
