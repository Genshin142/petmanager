#include "financemodule.h"
#include "performancemodule.h"
#include "salarymodule.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

FinanceModule::FinanceModule(QWidget *parent) : QWidget(parent)
{
    setupUI();
}

void FinanceModule::setupUI()
{
    setObjectName("FinanceModule");
    setStyleSheet("#FinanceModule { background-color: #f8fafc; }");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // --- 1. 顶部胶囊切换器区域 ---
    QWidget *topBar = new QWidget();
    topBar->setFixedHeight(70);
    topBar->setStyleSheet("background-color: white; border-bottom: 1px solid #e2e8f0;");
    
    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(25, 0, 25, 0);

    QLabel *titleLbl = new QLabel("财务结算中心");
    titleLbl->setStyleSheet("font-size: 20px; font-weight: 800; color: #0f172a; margin-right: 20px;");
    topLayout->addWidget(titleLbl);

    // 胶囊切换背景
    QWidget *tabCapsule = new QWidget();
    tabCapsule->setFixedSize(280, 52); // 再次增大尺寸
    tabCapsule->setStyleSheet("background-color: #f1f5f9; border-radius: 26px;");
    
    QHBoxLayout *tabLayout = new QHBoxLayout(tabCapsule);
    tabLayout->setContentsMargins(6, 6, 6, 6); // 均匀边距
    tabLayout->setSpacing(4);

    m_perfTabBtn = new QPushButton("业绩核销");
    m_salaryTabBtn = new QPushButton("薪资发放");
    
    // 增加 padding: 0 10px 确保水平空间，不设置垂直 padding 避免挤压
    QString activeStyle = "QPushButton { background-color: white; color: #3b82f6; border-radius: 20px; font-weight: 800; font-size: 13px; border: none; padding: 0 10px; }";
    QString inactiveStyle = "QPushButton { background-color: transparent; color: #64748b; border-radius: 20px; font-weight: bold; font-size: 13px; border: none; padding: 0 10px; } QPushButton:hover { color: #334155; }";

    m_perfTabBtn->setFixedHeight(40); // 显式固定高度
    m_salaryTabBtn->setFixedHeight(40);
    
    m_perfTabBtn->setStyleSheet(activeStyle);
    m_salaryTabBtn->setStyleSheet(inactiveStyle);

    tabLayout->addWidget(m_perfTabBtn);
    tabLayout->addWidget(m_salaryTabBtn);
    topLayout->addWidget(tabCapsule);
    
    topLayout->addStretch(); 

    mainLayout->addWidget(topBar);

    // --- 2. 堆栈内容区 ---
    m_stack = new QStackedWidget();
    m_perfModule = new PerformanceModule();
    m_salaryModule = new SalaryModule();

    m_stack->addWidget(m_perfModule);
    m_stack->addWidget(m_salaryModule);

    mainLayout->addWidget(m_stack);

    // 信号连接
    connect(m_perfTabBtn, &QPushButton::clicked, this, [this, activeStyle, inactiveStyle](){
        m_stack->setCurrentIndex(0);
        m_perfTabBtn->setStyleSheet(activeStyle);
        m_salaryTabBtn->setStyleSheet(inactiveStyle);
    });

    connect(m_salaryTabBtn, &QPushButton::clicked, this, [this, activeStyle, inactiveStyle](){
        m_stack->setCurrentIndex(1);
        m_perfTabBtn->setStyleSheet(inactiveStyle);
        m_salaryTabBtn->setStyleSheet(activeStyle);
    });
}

void FinanceModule::onTabChanged(int index)
{
    m_stack->setCurrentIndex(index);
}
