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
    m_titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #1e293b; border: none; background: transparent;");
    layout->addWidget(m_titleLabel);

    m_statusLabel = new QLabel("正在准备数据...");
    m_statusLabel->setStyleSheet("font-size: 13px; color: #64748b; margin-top: 10px; border: none; background: transparent;");
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
    m_closeBtn->setFixedHeight(48);
    m_closeBtn->setStyleSheet(R"(
        QPushButton { background-color: #f8fafc; color: #334155; font-weight: bold; font-size: 14px; border-radius: 8px; border: 1px solid #e2e8f0; padding: 0px 24px; text-align: left; }
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
    m_titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #10b981; border: none; background: transparent;");
    m_statusLabel->setText("已加密归档至:\n" + path);
    m_closeBtn->setText("完成并关闭");
    m_closeBtn->setStyleSheet(R"(
        QPushButton { background-color: #3b82f6; color: #ffffff; font-weight: bold; font-size: 14px; border-radius: 8px; border: none; padding: 0px 24px; text-align: left; }
        QPushButton:hover { background-color: #2563eb; }
    )");
}

void BackupProgressDialog::onBackupError(const QString &error) {
    m_titleLabel->setText("备份失败");
    m_titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #ef4444; border: none; background: transparent;");
    m_statusLabel->setText(error);
    m_progressBar->setStyleSheet("QProgressBar::chunk { background-color: #ef4444; border-radius: 6px; }");
    m_closeBtn->setText("关闭");
}
