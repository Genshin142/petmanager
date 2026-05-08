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
