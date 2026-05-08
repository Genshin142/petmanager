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
