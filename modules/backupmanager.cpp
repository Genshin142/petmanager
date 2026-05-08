#include "backupmanager.h"
#include <QThread>
#include <QDir>
#include <QDateTime>
#include <QFile>

BackupManager::BackupManager(QObject *parent) : QObject(parent) {}

void BackupManager::performBackup() {
    emit progressUpdated(5, "正在初始化备份环境...");
    
    // 1. 创建目标目录
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString baseDestDir = "D:/PetManager_Backup/Backup_" + timestamp;
    
    QDir dir;
    if (!dir.mkpath(baseDestDir)) {
        emit backupFailed("无法在 D 盘创建备份目录，请检查权限。");
        emit finished();
        return;
    }

    // 模拟耗时过程
    QThread::msleep(500); 
    emit progressUpdated(20, "正在导出核心数据库文件...");

    // 2. 模拟拷贝数据库 (当前只生成一个 dummy 文件)
    QFile dbFile(baseDestDir + "/database_snapshot.db");
    if (dbFile.open(QIODevice::WriteOnly)) {
        dbFile.write("DUMMY_DB_DATA");
        dbFile.close();
    }
    
    QThread::msleep(800);
    emit progressUpdated(50, "正在打包多媒体附件库...");

    // 3. 模拟拷贝大量图片文件
    QDir mediaDir(baseDestDir + "/media_assets");
    dir.mkpath(baseDestDir + "/media_assets");
    for (int i = 1; i <= 10; ++i) {
        QFile img(baseDestDir + QString("/media_assets/pet_img_%1.jpg").arg(i));
        if (img.open(QIODevice::WriteOnly)) img.close();
        
        emit progressUpdated(50 + (i * 4), QString("正在复制照片 (%1/10)...").arg(i));
        QThread::msleep(150); // 模拟每个文件耗时
    }

    emit progressUpdated(95, "正在验证备份完整性...");
    QThread::msleep(400);

    emit progressUpdated(100, "备份完成！");
    emit backupCompleted(baseDestDir);
    emit finished();
}
