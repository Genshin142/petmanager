#ifndef LOADINGWINDOW_H
#define LOADINGWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QPropertyAnimation>
#include <QTimer>
#include "common_types.h"

class LoadingWindow : public QWidget
{
    Q_OBJECT
public:
    explicit LoadingWindow(UserRole role, const QString &userName, QWidget *parent = nullptr);

private slots:
    void onDataLoaded();
    void onTimeout();

private:
    void setupUI();
    void checkProgress();

    UserRole m_role;
    QString m_userName;
    
    QLabel *m_statusLabel;
    QLabel *m_detailLabel;
    QProgressBar *m_progressBar;
    
    int m_loadedCount;
    const int m_totalTasks = 6; // 会员、商品、入库、服务、员工、订单/宠物
    
    bool m_memberLoaded = false;
    bool m_productLoaded = false;
    bool m_inboundLoaded = false;
    bool m_serviceLoaded = false;
    bool m_staffLoaded = false;
    bool m_petLoaded = false;
    bool m_isTransitioning = false;
};

#endif // LOADINGWINDOW_H
