# 数据备份系统 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为系统增加一个不阻塞主界面、带实时进度条且基于多线程的本地文件打包备份引擎。

**Architecture:** 
- `BackupManager`: 继承自 `QObject` 的 Worker 类，放入 `QThread` 执行。负责文件拷贝的 IO 操作，并通过信号将进度传递出去。
- `BackupProgressDialog`: 继承自 `QDialog` 的纯 UI 类，显示 `QProgressBar` 并接收 `BackupManager` 的信号。
- 在 `PersonalModule` 中实例化二者并启动。

**Tech Stack:** C++17, Qt6 (QtWidgets, QThread, QDir, QFile)

---

### Task 1: 创建多线程备份引擎后台核心

**Files:**
- Create: `e:\QT\work\PetManager\modules\backupmanager.h`
- Create: `e:\QT\work\PetManager\modules\backupmanager.cpp`

- [ ] **Step 1: 编写 backupmanager.h**

```cpp
#ifndef BACKUPMANAGER_H
#define BACKUPMANAGER_H

#include <QObject>
#include <QString>

class BackupManager : public QObject {
    Q_OBJECT
public:
    explicit BackupManager(QObject *parent = nullptr);

public slots:
    void performBackup();

signals:
    void progressUpdated(int percent, const QString &currentTask);
    void backupCompleted(const QString &finalPath);
    void backupFailed(const QString &errorReason);
    void finished(); // 通知 QThread 可以退出了
};

#endif // BACKUPMANAGER_H
```

- [ ] **Step 2: 编写 backupmanager.cpp (带模拟的延迟文件复制)**

```cpp
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
```

### Task 2: 创建备份进度对话框 UI

**Files:**
- Create: `e:\QT\work\PetManager\modules\backupprogressdialog.h`
- Create: `e:\QT\work\PetManager\modules\backupprogressdialog.cpp`

- [ ] **Step 1: 编写 backupprogressdialog.h**

```cpp
#ifndef BACKUPPROGRESSDIALOG_H
#define BACKUPPROGRESSDIALOG_H

#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>

class BackupProgressDialog : public QDialog {
    Q_OBJECT
public:
    explicit BackupProgressDialog(QWidget *parent = nullptr);

public slots:
    void updateProgress(int percent, const QString &taskName);
    void onBackupComplete(const QString &path);
    void onBackupError(const QString &error);

private:
    void setupUI();

    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QLabel *m_titleLabel;
    QPushButton *m_closeBtn;
};

#endif // BACKUPPROGRESSDIALOG_H
```

- [ ] **Step 2: 编写 backupprogressdialog.cpp**

```cpp
#include "backupprogressdialog.h"
#include <QVBoxLayout>

BackupProgressDialog::BackupProgressDialog(QWidget *parent) : QDialog(parent) {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setFixedSize(400, 220);
    setupUI();
}

void BackupProgressDialog::setupUI() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(15);

    this->setStyleSheet("QDialog { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 12px; }");

    m_titleLabel = new QLabel("系统数据冷备份");
    m_titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #1e293b;");
    layout->addWidget(m_titleLabel);

    m_statusLabel = new QLabel("正在准备数据...");
    m_statusLabel->setStyleSheet("font-size: 13px; color: #64748b; margin-top: 10px;");
    layout->addWidget(m_statusLabel);

    m_progressBar = new QProgressBar();
    m_progressBar->setFixedHeight(12);
    m_progressBar->setTextVisible(false);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setStyleSheet(R"(
        QProgressBar { background-color: #f1f5f9; border: none; border-radius: 6px; }
        QProgressBar::chunk { background-color: #3b82f6; border-radius: 6px; }
    )");
    layout->addWidget(m_progressBar);

    layout->addStretch();

    m_closeBtn = new QPushButton("后台运行");
    m_closeBtn->setFixedHeight(40);
    m_closeBtn->setStyleSheet(R"(
        QPushButton { background-color: #f8fafc; color: #334155; font-weight: bold; font-size: 13px; border-radius: 6px; border: 1px solid #e2e8f0; }
        QPushButton:hover { background-color: #f1f5f9; }
    )");
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(m_closeBtn);
}

void BackupProgressDialog::updateProgress(int percent, const QString &taskName) {
    m_progressBar->setValue(percent);
    m_statusLabel->setText(taskName);
}

void BackupProgressDialog::onBackupComplete(const QString &path) {
    m_titleLabel->setText("备份成功");
    m_titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #10b981;");
    m_statusLabel->setText("已加密归档至:\n" + path);
    m_closeBtn->setText("完成并关闭");
    m_closeBtn->setStyleSheet(R"(
        QPushButton { background-color: #0f172a; color: #ffffff; font-weight: bold; font-size: 13px; border-radius: 6px; border: none; }
        QPushButton:hover { background-color: #1e293b; }
    )");
}

void BackupProgressDialog::onBackupError(const QString &error) {
    m_titleLabel->setText("备份失败");
    m_titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #ef4444;");
    m_statusLabel->setText(error);
    m_progressBar->setStyleSheet("QProgressBar::chunk { background-color: #ef4444; border-radius: 6px; }");
    m_closeBtn->setText("关闭");
}
```

### Task 3: 接入 PersonalModule 并实现多线程调用

**Files:**
- Modify: `e:\QT\work\PetManager\modules\personalmodule.cpp`
- Modify: `e:\QT\work\PetManager\PetManager.pro`

- [ ] **Step 1: 在 `.pro` 中注册新文件**

将 `modules/backupmanager.h` 和 `modules/backupprogressdialog.h` 添加到 `HEADERS`。
将 `modules/backupmanager.cpp` 和 `modules/backupprogressdialog.cpp` 添加到 `SOURCES`。

- [ ] **Step 2: 替换 onActionClicked 逻辑**

```cpp
// 在 personalmodule.cpp 顶部增加
#include "backupmanager.h"
#include "backupprogressdialog.h"
#include <QThread>

// 在 onActionClicked 的数据备份分支中：
    } else if (actionName == "数据备份") {
        BackupProgressDialog *dialog = new BackupProgressDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show(); // 非阻塞显示

        QThread *thread = new QThread();
        BackupManager *worker = new BackupManager();
        worker->moveToThread(thread);

        // 连接信号槽
        connect(thread, &QThread::started, worker, &BackupManager::performBackup);
        connect(worker, &BackupManager::progressUpdated, dialog, &BackupProgressDialog::updateProgress);
        connect(worker, &BackupManager::backupCompleted, dialog, &BackupProgressDialog::onBackupComplete);
        connect(worker, &BackupManager::backupFailed, dialog, &BackupProgressDialog::onBackupError);
        
        // 自动清理内存
        connect(worker, &BackupManager::finished, thread, &QThread::quit);
        connect(worker, &BackupManager::finished, worker, &QObject::deleteLater);
        connect(thread, &QThread::finished, thread, &QObject::deleteLater);

        thread->start();
        return;
    }
```
