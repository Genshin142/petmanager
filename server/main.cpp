#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <glog/logging.h>
#include "network/server_core.h"

int main(int argc, char *argv[])
{
    // 1. 初始化 glog
    google::InitGoogleLogging(argv[0]);
    
    // 基础配置
    FLAGS_logtostderr = true;
    FLAGS_stderrthreshold = 0;
    
    // 自定义日志存放路径
    QDir logDir;
    if (!logDir.exists("logs")) {
        logDir.mkdir("logs");
    }
    FLAGS_log_dir = "logs";
    
    // 强化控制台输出配置
    FLAGS_stderrthreshold = 0; 
    FLAGS_alsologtostderr = true; 
    FLAGS_colorlogtostderr = true;
    FLAGS_minloglevel = 0; 
    FLAGS_logtostderr = false; // 关键：设为 false 才能同时写入日志文件
    
    // 增加原生输出对比，确认控制台本身是否正常
    qDebug() << "[DEBUG] System starting...";
    
    QCoreApplication a(argc, argv);

    LOG(INFO) << "==========================================";
    LOG(INFO) << "   PetManager Backend Server Engine v1.0  ";
    LOG(INFO) << "==========================================";

    ServerCore core;
    quint16 port = 8080;

    if (core.start(port)) {
        LOG(INFO) << "[INIT] Server started successfully.";
        LOG(INFO) << "[INIT] Listening on port: " << port;
        LOG(INFO) << "[INIT] Press Ctrl+C to terminate.";
    } else {
        LOG(ERROR) << "[ERR] Failed to start server on port " << port;
        return -1;
    }

    return a.exec();
}
