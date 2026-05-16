#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include "../utils/logger.h"
#include "network/server_core.h"

int main(int argc, char *argv[])
{
    // 1. 初始化自定义日志 (单文件，启动清空)
    Logger::init("server.log");
    
    qDebug() << "==========================================";
    qDebug() << "   PetManager Backend Server Engine v2.1  ";
    qDebug() << "==========================================";

    QCoreApplication a(argc, argv);

    ServerCore core;
    quint16 port = 8080;

    if (core.start(port)) {
        qDebug() << "[INIT] Server started successfully.";
        qDebug() << "[INIT] Listening on port: " << port;
        qDebug() << "[INIT] Press Ctrl+C to terminate.";
    } else {
        qDebug() << "[ERR] Failed to start server on port " << port;
        return -1;
    }

    return a.exec();
}
