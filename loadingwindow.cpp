#include "loadingwindow.h"
#include "mainwindow.h"
#include "modules/memberdatamanager.h"
#include "modules/productdatamanager.h"
#include "modules/servicedatamanager.h"
#include "modules/staffdatamanager.h"
#include "modules/petdatamanager.h"
#include <QGraphicsDropShadowEffect>
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QPainterPath>

LoadingWindow::LoadingWindow(UserRole role, const QString &userName, QWidget *parent)
    : QWidget(parent), m_role(role), m_userName(userName), m_loadedCount(0)
{
    setupUI();

    // 连接各个管理器的信号
    connect(MemberDataManager::instance(), &MemberDataManager::dataChanged, this, [this](){
        if (!m_memberLoaded) { m_memberLoaded = true; m_loadedCount++; onDataLoaded(); }
    });
    connect(ProductDataManager::instance(), &ProductDataManager::productDataChanged, this, [this](){
        if (!m_productLoaded) { m_productLoaded = true; m_loadedCount++; onDataLoaded(); }
    });
    connect(ServiceDataManager::instance(), &ServiceDataManager::serviceDataChanged, this, [this](){
        if (!m_serviceLoaded) { m_serviceLoaded = true; m_loadedCount++; onDataLoaded(); }
    });
    connect(ProductDataManager::instance(), &ProductDataManager::inboundListReceived, this, [this](){
        if (!m_inboundLoaded) { m_inboundLoaded = true; m_loadedCount++; onDataLoaded(); }
    });
    connect(StaffDataManager::instance(), &StaffDataManager::staffDataChanged, this, [this](){
        if (!m_staffLoaded) { m_staffLoaded = true; m_loadedCount++; onDataLoaded(); }
    });
    connect(PetDataManager::instance(), &PetDataManager::globalDataChanged, this, [this](){
        if (!m_petLoaded) { m_petLoaded = true; m_loadedCount++; onDataLoaded(); }
    });

    // 强制触发一次请求（以防管理器还没开始加载）
    MemberDataManager::instance()->requestMemberList();
    ProductDataManager::instance()->requestProductList();
    ProductDataManager::instance()->requestInboundList(); // 新增：入库列表
    ServiceDataManager::instance()->requestServiceList();
    StaffDataManager::instance()->requestStaffList();
    PetDataManager::instance()->requestPetList();

    // 设置安全超时，防止某些请求失败导致死等
    QTimer::singleShot(5000, this, &LoadingWindow::onTimeout);
}

void LoadingWindow::setupUI()
{
    setFixedSize(450, 300);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // 居中显示
    QScreen *screen = QApplication::primaryScreen();
    move(screen->geometry().center() - rect().center());

    QFrame *mainFrame = new QFrame(this);
    mainFrame->setObjectName("MainFrame");
    mainFrame->setGeometry(10, 10, 430, 280);
    mainFrame->setStyleSheet(
        "#MainFrame { "
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #ffffff, stop:1 #f8fafc); "
        "  border-radius: 20px; "
        "  border: 1px solid #e2e8f0; "
        "}"
    );

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(30);
    shadow->setColor(QColor(0, 0, 0, 40));
    shadow->setOffset(0, 4);
    mainFrame->setGraphicsEffect(shadow);

    QVBoxLayout *layout = new QVBoxLayout(mainFrame);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(20);

    // 顶部 Logo/Icon 占位 (用一个漂亮的渐变圆圈代替)
    QWidget *logoCircle = new QWidget();
    logoCircle->setFixedSize(60, 60);
    logoCircle->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3b82f6, stop:1 #2563eb); "
        "border-radius: 30px;"
    );
    layout->addWidget(logoCircle, 0, Qt::AlignCenter);

    m_statusLabel = new QLabel("正在同步云端数据...");
    m_statusLabel->setStyleSheet("font-size: 18px; color: #1e293b; font-weight: bold;");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_statusLabel);

    m_progressBar = new QProgressBar();
    m_progressBar->setFixedHeight(8);
    m_progressBar->setRange(0, m_totalTasks);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar { background: #f1f5f9; border-radius: 4px; border: none; } "
        "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3b82f6, stop:1 #60a5fa); border-radius: 4px; }"
    );
    layout->addWidget(m_progressBar);

    m_detailLabel = new QLabel("准备建立安全连接...");
    m_detailLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    m_detailLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_detailLabel);
}

void LoadingWindow::onDataLoaded()
{
    m_progressBar->setValue(m_loadedCount);
    
    QString detail;
    if (m_loadedCount == 1) detail = "已获取会员基础档案...";
    else if (m_loadedCount == 2) detail = "商品与库存数据校验中...";
    else if (m_loadedCount == 3) detail = "服务项目配置同步完成...";
    else if (m_loadedCount == 4) detail = "员工排班数据加载完毕...";
    else if (m_loadedCount == 5) detail = "订单与资产数据就绪，正在准备主界面...";
    
    m_detailLabel->setText(detail);

    if (m_loadedCount >= m_totalTasks && !m_isTransitioning) {
        m_isTransitioning = true;
        // 稍微延迟一下，给用户一个“加载完成”的视觉反馈
        QTimer::singleShot(500, this, [this](){
            MainWindow *mw = new MainWindow(m_role, m_userName);
            mw->show();
            this->close();
        });
    }
}

void LoadingWindow::onTimeout()
{
    if (m_loadedCount < m_totalTasks && !m_isTransitioning) {
        m_isTransitioning = true;
        qDebug() << "[LOADING] Timeout reached, entering app with partial data.";
        m_detailLabel->setText("连接响应较慢，正在尝试进入系统...");
        QTimer::singleShot(1000, this, [this](){
            MainWindow *mw = new MainWindow(m_role, m_userName);
            mw->show();
            this->close();
        });
    }
}
