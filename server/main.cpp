#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QProcess>
#include "../utils/logger.h"
#include "network/server_core.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

#ifdef Q_OS_WIN
    // 自动清理后台残留的旧版 PetServer.exe 进程，确保端口 8080 能够成功绑定
    qint64 currentPid = QCoreApplication::applicationPid();
    qDebug() << "[INIT] Checking and cleaning other leftover PetServer instances...";
    QString killCmd = QString("taskkill /F /IM PetServer.exe /FI \"PID ne %1\"").arg(currentPid);
    QProcess::execute("cmd.exe", QStringList() << "/c" << killCmd);
#endif

    // 1. 初始化自定义日志 (单文件，启动清空)
    Logger::init("server.log");
    
    qDebug() << "==========================================";
    qDebug() << "   PetManager Backend Server Engine v2.1  ";
    qDebug() << "==========================================";

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
