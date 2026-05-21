#include "performancemodule.h"
#include "salarydatamanager.h"
#include "staffdatamanager.h"
#include "common_types.h"
#include "custom_calendar_edit.h"
#include "custommessagedialog.h"
#include "../utils/imageutils.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDateTime>
#include <QFrame>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QScrollArea>
#include <QStyledItemDelegate>
#include <QEvent>
#include <QPainter>
#include <QPainterPath>
#include <QStackedWidget>

// --- 复刻：全行圆角选中委托 ---
class PerformanceRowDelegate : public QStyledItemDelegate {
public:
    private:
    mutable int m_hoveredRow = -1;
public:
    PerformanceRowDelegate(QObject *parent) : QStyledItemDelegate(parent) {
        QAbstractItemView *view = qobject_cast<QAbstractItemView*>(parent);
        if (view) {
            view->setMouseTracking(true);
            connect(view, &QAbstractItemView::entered, this, [this, view](const QModelIndex &index) {
                m_hoveredRow = index.row();
                view->viewport()->update();
            });
            view->viewport()->installEventFilter(this);
        }
    }
    
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (event->type() == QEvent::Leave) {
            m_hoveredRow = -1;
            QAbstractItemView *view = qobject_cast<QAbstractItemView*>(parent());
            if (view) {
                view->viewport()->update();
            }
        }
        return QStyledItemDelegate::eventFilter(watched, event);
    }
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        if (index.row() == m_hoveredRow) {
            painter->fillRect(opt.rect, QColor("#ecf5ff"));
        } else {
            painter->fillRect(opt.rect, Qt::white);
        }

        if (opt.state & QStyle::State_Selected) {
            bool isFirst = (index.column() == 0);
            bool isLast = (index.column() == index.model()->columnCount() - 1);
            QRect rect = opt.rect.adjusted(1, 4, -1, -4);
            int radius = 8;
            QColor borderColor("#3b82f6");
            QColor bgColor("#eff6ff");

            painter->fillRect(opt.rect, bgColor);
            painter->setPen(QPen(borderColor, 2));
            
            if (isFirst) {
                QPainterPath path;
                path.moveTo(opt.rect.right() + 1, rect.top()); 
                path.lineTo(rect.left() + radius, rect.top());
                path.arcTo(QRect(rect.left(), rect.top(), radius*2, radius*2), 90, 90);
                path.lineTo(rect.left(), rect.bottom() - radius);
                path.arcTo(QRect(rect.left(), rect.bottom() - radius*2, radius*2, radius*2), 180, 90);
                path.lineTo(opt.rect.right() + 1, rect.bottom());
                painter->drawPath(path);
            } else if (isLast) {
                QPainterPath path;
                path.moveTo(opt.rect.left() - 1, rect.top());
                path.lineTo(rect.right() - radius, rect.top());
                path.arcTo(QRect(rect.right() - radius*2, rect.top(), radius*2, radius*2), 90, -90);
                path.lineTo(rect.right(), rect.bottom() - radius);
                path.arcTo(QRect(rect.right() - radius*2, rect.bottom() - radius*2, radius*2, radius*2), 0, -90);
                path.lineTo(opt.rect.left() - 1, rect.bottom());
                painter->drawPath(path);
            } else {
                painter->drawLine(QPoint(opt.rect.left() - 1, rect.top()), QPoint(opt.rect.right() + 1, rect.top()));
                painter->drawLine(QPoint(opt.rect.left() - 1, rect.bottom()), QPoint(opt.rect.right() + 1, rect.bottom()));
            }
        } else {
            painter->setPen(QPen(QColor("#f1f5f9"), 1));
            painter->drawLine(opt.rect.bottomLeft(), opt.rect.bottomRight());
        }

        painter->setPen(QColor((opt.state & QStyle::State_Selected) ? "#1e40af" : "#334155"));
        QFont font("Microsoft YaHei", 9);
        if (opt.state & QStyle::State_Selected) font.setBold(true);
        painter->setFont(font);
        
        QRect textRect = opt.rect.adjusted(10, 0, -10, 0);
        painter->drawText(textRect, Qt::AlignCenter, opt.text);
        
        painter->restore();
    }
};

PerformanceModule::PerformanceModule(QWidget *parent) : QWidget(parent)
{
    setupUI();
    refreshData();
    
    if (m_perfTable->rowCount() > 0) {
        m_perfTable->selectRow(0);
    }

    // 触发初始数据请求
    onFilterChanged();

    connect(SalaryDataManager::instance(), &SalaryDataManager::performanceDataChanged, this, &PerformanceModule::refreshData);
    // 👈 实时响应来自服务端的财务刷新指令
    connect(SalaryDataManager::instance(), &SalaryDataManager::financeRefreshRequested, this, &PerformanceModule::onFilterChanged);
}

void PerformanceModule::setupUI()
{
    setObjectName("PerformanceModule");
    setStyleSheet("#PerformanceModule { background-color: #f8fafc; font-family: 'Microsoft YaHei'; }");

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 20, 20, 20);
    rootLayout->setSpacing(15);

    // --- 1. 顶部概览卡片 ---
    QFrame *headerGroup = new QFrame();
    headerGroup->setObjectName("headerGroup");
    headerGroup->setFixedHeight(130);
    headerGroup->setStyleSheet("QFrame#headerGroup { background-color: white; border: 1px solid #e2e8f0; border-radius: 12px; }");
    QVBoxLayout *headerGroupLayout = new QVBoxLayout(headerGroup);
    headerGroupLayout->setContentsMargins(20, 15, 20, 15);
    headerGroupLayout->setSpacing(15);

    QLabel *titleLbl = new QLabel("业绩核销管理看板");
    titleLbl->setStyleSheet("font-size: 16px; font-weight: bold; color: #1e293b; border: none; background: transparent;");
    headerGroupLayout->addWidget(titleLbl);

    QHBoxLayout *statsLayout = new QHBoxLayout();
    statsLayout->setSpacing(15);

    auto createStatsCard = [&](const QString &title, const QString &val, const QString &color) {
        QFrame *card = new QFrame();
        card->setFixedHeight(60);
        card->setStyleSheet("QFrame { background-color: #f8fafc; border: 1px solid #e2e8f0; border-radius: 8px; }");
        QVBoxLayout *cl = new QVBoxLayout(card);
        cl->setContentsMargins(15, 5, 15, 5);
        cl->setSpacing(0);
        QLabel *tl = new QLabel(title); tl->setStyleSheet("color: #64748b; font-size: 11px; font-weight: bold; border: none;");
        QLabel *vl = new QLabel(val); vl->setStyleSheet(QString("color: %1; font-size: 16px; font-weight: 800; border: none;").arg(color));
        cl->addWidget(tl); cl->addWidget(vl);
        return card;
    };

    QFrame *revCard = createStatsCard("服务总营收 (CNY)", "¥ 0.00", "#1e293b");
    m_totalRevenueVal = static_cast<QLabel*>(revCard->layout()->itemAt(1)->widget());
    QFrame *commCard = createStatsCard("预计总提成", "¥ 0.00", "#10b981");
    m_totalCommVal = static_cast<QLabel*>(commCard->layout()->itemAt(1)->widget());
    QFrame *pendCard = createStatsCard("待核销笔数", "0 笔", "#f59e0b");
    m_pendingVerifyCountVal = static_cast<QLabel*>(pendCard->layout()->itemAt(1)->widget());

    statsLayout->addWidget(revCard, 1);
    statsLayout->addWidget(commCard, 1);
    statsLayout->addWidget(pendCard, 1);
    headerGroupLayout->addLayout(statsLayout);
    rootLayout->addWidget(headerGroup);

    // --- 2. 核心内容区域 ---
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(15);

    // 2.1 左侧列
    QVBoxLayout *leftColumn = new QVBoxLayout();
    leftColumn->setSpacing(15);

    QFrame *filterGroup = new QFrame();
    filterGroup->setObjectName("filterGroup");
    filterGroup->setFixedHeight(64);
    filterGroup->setStyleSheet("QFrame#filterGroup { background-color: white; border: 1px solid #e2e8f0; border-radius: 12px; }");
    QHBoxLayout *filterLayout = new QHBoxLayout(filterGroup);
    filterLayout->setContentsMargins(20, 0, 20, 0);
    filterLayout->setSpacing(10);

    QString lineEditStyle = "QLineEdit { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 12px; font-size: 13px; background: white; height: 32px; } QLineEdit:focus { border-color: #409eff; outline: none; }";
    QString comboStyle = "QComboBox { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 10px; background: white; font-size: 13px; height: 32px; } QComboBox:hover { border-color: #409eff; } QComboBox::drop-down { border: none; width: 24px; } QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; } QComboBox QAbstractItemView { border: 1px solid #e2e8f0; border-radius: 8px; background: white; selection-background-color: #f1f5f9; selection-color: #3b82f6; outline: none; padding: 5px; }";

    m_startDateEdit = new CustomCalendarEdit();
    m_startDateEdit->setFixedWidth(100);
    m_startDateEdit->setText(QDate::currentDate().addDays(-30).toString("yyyy-MM-dd"));
    m_startDateEdit->setStyleSheet(lineEditStyle);
    
    m_endDateEdit = new CustomCalendarEdit();
    m_endDateEdit->setFixedWidth(100);
    m_endDateEdit->setText(QDate::currentDate().toString("yyyy-MM-dd"));
    m_endDateEdit->setStyleSheet(lineEditStyle);

    m_employeeCombo = new QComboBox();
    m_employeeCombo->setFixedWidth(100);
    m_employeeCombo->addItem("全部员工", "");
    for (const auto &emp : StaffDataManager::instance()->allStaff()) {
        if (emp.status != "离职") {
            m_employeeCombo->addItem(emp.name, emp.id);
        }
    }
    m_employeeCombo->setStyleSheet(comboStyle);

    m_statusCombo = new QComboBox();
    m_statusCombo->setFixedWidth(110);
    m_statusCombo->addItems({"全部状态", "待核销", "已核销"});
    m_statusCombo->setStyleSheet(comboStyle);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("搜索服务...");
    m_searchEdit->setFixedWidth(130);
    m_searchEdit->setStyleSheet(lineEditStyle);

    filterLayout->addWidget(new QLabel("时段:"));
    filterLayout->addWidget(m_startDateEdit);
    filterLayout->addWidget(new QLabel("-"));
    filterLayout->addWidget(m_endDateEdit);
    filterLayout->addSpacing(5);
    filterLayout->addWidget(new QLabel("员工:"));
    filterLayout->addWidget(m_employeeCombo);
    filterLayout->addWidget(m_statusCombo);
    filterLayout->addSpacing(5);
    filterLayout->addWidget(m_searchEdit);
    filterLayout->addStretch();
    leftColumn->addWidget(filterGroup);

    QFrame *tableCard = new QFrame();
    tableCard->setObjectName("tableCard");
    tableCard->setStyleSheet("QFrame#tableCard { background-color: white; border: 1px solid #e2e8f0; border-radius: 12px; }");
    QVBoxLayout *tableCardLayout = new QVBoxLayout(tableCard);
    tableCardLayout->setContentsMargins(0, 0, 0, 0);
    tableCardLayout->setSpacing(0);

    m_perfTable = new QTableWidget();
    m_perfTable->setColumnCount(8);
    m_perfTable->setHorizontalHeaderLabels({"日期", "执行人", "工号", "岗位", "项目", "金额", "提成", "状态"});
    m_perfTable->setItemDelegate(new PerformanceRowDelegate(m_perfTable));
    m_perfTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_perfTable->verticalHeader()->setVisible(false);
    m_perfTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_perfTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_perfTable->setShowGrid(false);
    m_perfTable->setFocusPolicy(Qt::NoFocus);
    m_perfTable->setStyleSheet(R"(
        QTableWidget { border: none; background: white; outline: none; border-radius: 12px; }
        QHeaderView::section { background-color: white; border: none; border-bottom: 1px solid #f1f5f9; padding: 12px; font-weight: bold; color: #64748b; font-size: 13px; }
        QHeaderView { border: none; border-top-left-radius: 12px; border-top-right-radius: 12px; background-color: white; }
    )");
    tableCardLayout->addWidget(m_perfTable, 1);

    // --- 3. 底部页码栏 (严格复刻会员模块右对齐布局) ---
    QFrame *paginationFrame = new QFrame();
    paginationFrame->setFixedHeight(50);
    paginationFrame->setStyleSheet("QFrame { background: white; border-top: 1px solid #f1f5f9; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; }");
    QHBoxLayout *pageLayout = new QHBoxLayout(paginationFrame);
    pageLayout->setContentsMargins(20, 0, 20, 0);
    
    m_pageInfoLabel = new QLabel("第 1 页 / 共 1 页");
    m_pageInfoLabel->setStyleSheet("color: #64748b; font-size: 13px; font-weight: bold;");
    
    m_prevBtn = new QPushButton("上一页");
    m_nextBtn = new QPushButton("下一页");
    QString pageBtnStyle = "QPushButton { height: 28px; border: 1px solid #e2e8f0; border-radius: 6px; background: white; color: #64748b; font-size: 12px; padding: 0 12px; font-weight: bold; } QPushButton:hover { border-color: #3b82f6; color: #3b82f6; background: #eff6ff; } QPushButton:disabled { background: white; color: #cbd5e1; border-color: #f1f5f9; }";
    m_prevBtn->setStyleSheet(pageBtnStyle);
    m_nextBtn->setStyleSheet(pageBtnStyle);
    
    // 关键：左侧添加弹簧，将所有组件推向右侧
    pageLayout->addStretch(); 
    pageLayout->addWidget(m_prevBtn);
    pageLayout->addSpacing(20); 
    pageLayout->addWidget(m_pageInfoLabel);
    pageLayout->addSpacing(20); 
    pageLayout->addWidget(m_nextBtn);
    
    tableCardLayout->addWidget(paginationFrame);
    leftColumn->addWidget(tableCard, 1);

    connect(m_prevBtn, &QPushButton::clicked, this, &PerformanceModule::onPrevPage);
    connect(m_nextBtn, &QPushButton::clicked, this, &PerformanceModule::onNextPage);

    contentLayout->addLayout(leftColumn, 3);

    QFrame *detailCard = new QFrame();
    detailCard->setObjectName("detailCard");
    detailCard->setFixedWidth(450); 
    detailCard->setStyleSheet("QFrame#detailCard { background-color: white; border: 1px solid #e2e8f0; border-radius: 12px; }");
    QVBoxLayout *cardOuterLayout = new QVBoxLayout(detailCard);
    cardOuterLayout->setContentsMargins(25, 30, 25, 25);
    cardOuterLayout->setSpacing(0);

    m_detailStack = new class QStackedWidget(detailCard);
    m_detailStack->setStyleSheet("background: transparent; border: none;");
    cardOuterLayout->addWidget(m_detailStack);

    // Page 0: 详情界面
    QWidget *detailContentWidget = new QWidget();
    detailContentWidget->setStyleSheet("background: transparent; border: none;");
    QVBoxLayout *detailLayout = new QVBoxLayout(detailContentWidget);
    detailLayout->setContentsMargins(0, 0, 0, 0);

    // Page 1: 暂无数据界面
    QWidget *noDataWidget = new QWidget();
    noDataWidget->setStyleSheet("background: transparent; border: none;");
    QVBoxLayout *noDataLayout = new QVBoxLayout(noDataWidget);
    noDataLayout->setContentsMargins(0, 0, 0, 0);
    noDataLayout->setSpacing(15);
    noDataLayout->setAlignment(Qt::AlignCenter);

    QLabel *noDataIcon = new QLabel();
    noDataIcon->setFixedSize(80, 80);
    noDataIcon->setStyleSheet("background-color: #f8fafc; border-radius: 40px; font-size: 36px; border: none;");
    noDataIcon->setText("📭");
    noDataIcon->setAlignment(Qt::AlignCenter);

    QLabel *noDataText = new QLabel("当前暂无数据");
    noDataText->setStyleSheet("color: #94a3b8; font-size: 15px; font-weight: bold;");
    noDataText->setAlignment(Qt::AlignCenter);

    noDataLayout->addStretch();
    noDataLayout->addWidget(noDataIcon, 0, Qt::AlignCenter);
    noDataLayout->addWidget(noDataText, 0, Qt::AlignCenter);
    noDataLayout->addStretch();

    m_detailStack->addWidget(detailContentWidget);
    m_detailStack->addWidget(noDataWidget);
    m_detailStack->setCurrentIndex(1); // 默认暂无数据
    
    QLabel *detailTitle = new QLabel("核销详情分析");
    detailTitle->setStyleSheet("font-weight: bold; font-size: 16px; color: #1e293b; margin-bottom: 20px; border: none; background: transparent;");
    detailLayout->addWidget(detailTitle);

    // --- 员工头部 (复刻薪资模块) ---
    QWidget *staffHeader = new QWidget();
    staffHeader->setFixedHeight(80);
    QHBoxLayout *headerL = new QHBoxLayout(staffHeader);
    headerL->setContentsMargins(0,0,0,10);
    
    m_detailHeaderAvatar = new QLabel();
    m_detailHeaderAvatar->setFixedSize(60, 60);
    m_detailHeaderAvatar->setStyleSheet("background-color: #f1f5f9; border-radius: 30px; font-weight: 800; font-size: 20px; color: #3b82f6; border: none;");
    m_detailHeaderAvatar->setAlignment(Qt::AlignCenter);
    m_detailHeaderAvatar->setCursor(Qt::PointingHandCursor);
    m_detailHeaderAvatar->installEventFilter(this);
    
    QVBoxLayout *infoL = new QVBoxLayout();
    infoL->setSpacing(4);
    
    QHBoxLayout *nameIdL = new QHBoxLayout();
    nameIdL->setSpacing(10);
    m_detailHeaderName = new QLabel("-"); m_detailHeaderName->setStyleSheet("font-weight: bold; font-size: 18px; color: #1e293b; border: none;");
    m_detailHeaderEmpId = new QLabel("ID: -"); m_detailHeaderEmpId->setStyleSheet("color: #94a3b8; font-size: 13px; border: none;");
    nameIdL->addWidget(m_detailHeaderName);
    nameIdL->addWidget(m_detailHeaderEmpId);
    nameIdL->addStretch();

    m_detailHeaderRole = new QLabel("岗位: -"); m_detailHeaderRole->setStyleSheet("color: #64748b; font-size: 13px; border: none; background: transparent;");
    
    infoL->addLayout(nameIdL);
    infoL->addWidget(m_detailHeaderRole);
    
    headerL->addWidget(m_detailHeaderAvatar);
    headerL->addLayout(infoL);
    headerL->addStretch();
    
    detailLayout->addWidget(staffHeader);

    // 分割线
    QFrame *sep1 = new QFrame(); sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet("background: #f1f5f9; min-height: 1px; max-height: 1px; margin: 10px 0; border: none;");
    detailLayout->addWidget(sep1);
    
    auto createDetailItem = [&](const QString &label, QLabel* valLabel) {
        QWidget *w = new QWidget();
        w->setStyleSheet("background: transparent; border: none;");
        QHBoxLayout *hl = new QHBoxLayout(w);
        hl->setContentsMargins(0, 8, 0, 8);
        QLabel *ll = new QLabel(label); ll->setStyleSheet("color: #64748b; font-size: 14px; border: none;");
        valLabel->setStyleSheet("font-weight: bold; font-size: 14px; border: none; color: #1e293b;");
        valLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        hl->addWidget(ll); hl->addStretch(); hl->addWidget(valLabel);
        return w;
    };
    
    detailLayout->addWidget(createDetailItem("订单编号", m_detailOrderIdVal = new QLabel("-")));
    detailLayout->addWidget(createDetailItem("客户名称", m_detailCustomerVal = new QLabel("-")));
    detailLayout->addWidget(createDetailItem("支付方式", m_detailPaymentVal = new QLabel("-")));
    detailLayout->addWidget(createDetailItem("关联宠物", m_detailPetVal = new QLabel("-")));
    
    // 分割线
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: #f1f5f9; min-height: 1px; max-height: 1px; border: none; margin: 10px 0;");
    detailLayout->addWidget(line);

    detailLayout->addWidget(createDetailItem("订单总额", m_detailOrderTotalVal = new QLabel("-")));
    detailLayout->addWidget(createDetailItem("实付金额", m_detailActualPaidVal = new QLabel("-")));
    detailLayout->addWidget(createDetailItem("计算公式", m_detailCommFormulaVal = new QLabel("-")));
    detailLayout->addStretch();
    
    m_confirmVerifyBtn = new QPushButton("确认当前核销");
    m_confirmVerifyBtn->setFixedHeight(48);
    m_confirmVerifyBtn->setCursor(Qt::PointingHandCursor);
    m_confirmVerifyBtn->setStyleSheet("QPushButton { background: white; border: 1px solid #3b82f6; color: #3b82f6; border-radius: 8px; font-weight: bold; font-size: 15px; } QPushButton:hover { background: #eff6ff; }");
    connect(m_confirmVerifyBtn, &QPushButton::clicked, this, [this](){
        if (m_currentRecordId.isEmpty()) {
            CustomMessageDialog::showWarning(this, "未选中", "请先从表格中选择一笔业绩记录。");
            return;
        }
        onVerifySingle(m_currentRecordId);
    });
    detailLayout->addWidget(m_confirmVerifyBtn);

    contentLayout->addWidget(detailCard, 1);
    rootLayout->addLayout(contentLayout, 1);

    // 监听表格选择
    connect(m_perfTable, &QTableWidget::itemSelectionChanged, this, [this](){
        int row = m_perfTable->currentRow();
        if (row < 0) return;
        QString recordId = m_perfTable->item(row, 0)->data(Qt::UserRole).toString();
        m_currentRecordId = recordId;
        
        QString dateRange = QString("%1,%2").arg(m_startDateEdit->text(), m_endDateEdit->text());
        auto records = SalaryDataManager::instance()->getPerformanceRecords(dateRange, m_employeeCombo->currentData().toString());
        for (const auto &r : records) {
            if (r.id == recordId) {
                // 更新员工头部
                m_detailHeaderAvatar->setText(r.employeeName.left(1));
                m_detailHeaderName->setText(r.employeeName);
                
                // 格式化 ID 以匹配 StaffDataManager 的格式 (E00X)
                QString staffKey = QString("E%1").arg(r.employeeId.toInt(), 3, 10, QChar('0'));
                EmployeeInfo emp = StaffDataManager::instance()->getStaff(staffKey);
                
                m_detailHeaderEmpId->setText("ID: " + emp.id); // 显示工号而不是纯数字
                m_currentStaffImgPath = emp.imgPath; // 存储当前路径用于点击放大
                
                // 加载真实头像
                if (!emp.imgPath.isEmpty()) {
                    QPixmap pix = ImageUtils::loadPixmap(emp.imgPath);
                    m_detailHeaderAvatar->setPixmap(ImageUtils::getCircularPixmap(pix, 60));
                    m_detailHeaderAvatar->setText("");
                } else {
                    m_detailHeaderAvatar->setPixmap(QPixmap());
                    m_detailHeaderAvatar->setText(r.employeeName.left(1));
                }
                
                m_detailHeaderRole->setText(emp.role.isEmpty() ? "普通员工" : emp.role);

                m_detailOrderIdVal->setText(r.orderId);
                m_detailCustomerVal->setText(QString("%1 (ID: %2)").arg(r.customerName).arg(r.customerId));
                m_detailPaymentVal->setText(r.payMethod.isEmpty() ? "现金" : r.payMethod);
                m_detailPetVal->setText(QString("%1 (ID: %2 / %3)").arg(r.petName).arg(r.petId).arg(r.petBreed));
                m_detailOrderTotalVal->setText(QString("¥%1").arg(r.orderAmount, 0, 'f', 2));
                m_detailActualPaidVal->setText(QString("¥%1").arg(r.finalAmount, 0, 'f', 2));
                
                if (r.commissionType == "固定提成") {
                    m_detailCommFormulaVal->setText(QString("固定提成 = ¥%1").arg(r.commission, 0, 'f', 2));
                } else {
                    m_detailCommFormulaVal->setText(QString("¥%1 × %2% = ¥%3")
                                                    .arg(r.finalAmount, 0, 'f', 2)
                                                    .arg(r.commissionRate * 100, 0, 'f', 0)
                                                    .arg(r.commission, 0, 'f', 2));
                }
                
                // 状态联动：已核销的记录隐藏核销按钮
                m_confirmVerifyBtn->setVisible(r.status == "待核销");
                break;
            }
        }
    });

    // 事件连接
    connect(m_startDateEdit, &CustomCalendarEdit::textChanged, this, &PerformanceModule::onFilterChanged);
    connect(m_endDateEdit, &CustomCalendarEdit::textChanged, this, &PerformanceModule::onFilterChanged);
    connect(m_employeeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PerformanceModule::onFilterChanged);
    connect(m_statusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PerformanceModule::onFilterChanged);
}

void PerformanceModule::refreshData()
{
    // 该方法由 performanceDataChanged 信号触发，仅负责 UI 更新
    updateStats();
    updateTable();
}

void PerformanceModule::updateStats()
{
    QString dateRange = QString("%1,%2").arg(m_startDateEdit->text(), m_endDateEdit->text());
    auto records = SalaryDataManager::instance()->getPerformanceRecords(dateRange);
    double totalRev = 0, totalComm = 0;
    int pending = 0;
    
    for (const auto &r : records) {
        totalRev += r.orderAmount;
        totalComm += r.commission;
        if (r.status == "待核销") pending++;
    }
    
    m_totalRevenueVal->setText(QString("¥ %1").arg(totalRev, 0, 'f', 2));
    m_totalCommVal->setText(QString("¥ %1").arg(totalComm, 0, 'f', 2));
    m_pendingVerifyCountVal->setText(QString("%1 笔").arg(pending));
}

void PerformanceModule::updateTable()
{
    m_perfTable->setRowCount(0);
    QString dateRange = QString("%1,%2").arg(m_startDateEdit->text(), m_endDateEdit->text());
    QString filterEmp = m_employeeCombo->currentData().toString();
    QString filterStatus = m_statusCombo->currentText();

    auto allRecords = SalaryDataManager::instance()->getPerformanceRecords(dateRange, filterEmp);
    QVector<PerformanceRecord> filteredRecords;
    for (const auto &r : allRecords) {
        if (filterStatus != "全部状态" && r.status != filterStatus) continue;
        filteredRecords.append(r);
    }

    // 默认按日期降序排列
    std::sort(filteredRecords.begin(), filteredRecords.end(), [](const PerformanceRecord &a, const PerformanceRecord &b){
        return a.serviceDate > b.serviceDate;
    });

    int totalRecords = filteredRecords.size();
    int totalPages = std::max(1, (totalRecords + m_pageSize - 1) / m_pageSize);
    
    // 修正当前页码
    if (m_currentPage > totalPages) m_currentPage = totalPages;
    if (m_currentPage < 1) m_currentPage = 1;

    // 更新分页 UI
    m_pageInfoLabel->setText(QString("第 %1 页 / 共 %2 页").arg(m_currentPage).arg(totalPages));
    m_prevBtn->setEnabled(m_currentPage > 1);
    m_nextBtn->setEnabled(m_currentPage < totalPages);

    int startIdx = (m_currentPage - 1) * m_pageSize;
    int endIdx = std::min(startIdx + m_pageSize, totalRecords);

    for (int i = startIdx; i < endIdx; ++i) {
        const auto &r = filteredRecords[i];
        int row = m_perfTable->rowCount();
        m_perfTable->insertRow(row);
        m_perfTable->setRowHeight(row, 56);

        auto createItem = [](const QString &text) {
            return new QTableWidgetItem(text);
        };

        // 获取员工岗位信息
        QString staffKey = QString("E%1").arg(r.employeeId.toInt(), 3, 10, QChar('0'));
        EmployeeInfo staff = StaffDataManager::instance()->getStaff(staffKey);

        QTableWidgetItem *dateItem = createItem(r.serviceDate);
        dateItem->setData(Qt::UserRole, r.id);
        m_perfTable->setItem(row, 0, dateItem);
        m_perfTable->setItem(row, 1, createItem(r.employeeName));
        m_perfTable->setItem(row, 2, createItem(staff.id)); // 工号
        m_perfTable->setItem(row, 3, createItem(staff.role.isEmpty() ? "普通员工" : staff.role)); // 岗位/角色
        m_perfTable->setItem(row, 4, createItem(r.serviceName));
        m_perfTable->setItem(row, 5, createItem(QString("¥%1").arg(r.orderAmount, 0, 'f', 2)));
        m_perfTable->setItem(row, 6, createItem(QString("¥%1").arg(r.commission, 0, 'f', 2)));

        QLabel *statusLabel = new QLabel(r.status);
        statusLabel->setAlignment(Qt::AlignCenter);
        QString statusColor = (r.status == "已核销") ? "#d1fae5" : "#ffedd5";
        QString textColor = (r.status == "已核销") ? "#065f46" : "#9a3412";
        statusLabel->setFixedSize(60, 20);
        statusLabel->setStyleSheet(QString("background-color: %1; color: %2; border-radius: 10px; font-size: 10px; font-weight: bold; border: none;").arg(statusColor, textColor));
        
        QWidget *statusContainer = new QWidget();
        QHBoxLayout *sl = new QHBoxLayout(statusContainer);
        sl->setContentsMargins(0, 0, 0, 0); sl->setAlignment(Qt::AlignCenter);
        sl->addWidget(statusLabel);
        m_perfTable->setCellWidget(row, 7, statusContainer);
        m_perfTable->setItem(row, 7, new QTableWidgetItem()); 
    }

    if (m_perfTable->rowCount() > 0) {
        m_detailStack->setCurrentIndex(0);
        m_perfTable->selectRow(0);
    } else {
        m_detailStack->setCurrentIndex(1);
    }
}

void PerformanceModule::onPrevPage() {
    if (m_currentPage > 1) {
        m_currentPage--;
        updateTable();
    }
}

void PerformanceModule::onNextPage() {
    m_currentPage++;
    updateTable();
}

void PerformanceModule::onVerifySingle(const QString &recordId)
{
    if (CustomMessageDialog::confirm(this, "核销确认", "确定核销该笔提成并计入员工本月薪资吗？")) {
        SalaryDataManager::instance()->verifyPerformance(recordId);
        CustomMessageDialog::showSuccess(this, "核销成功", "业绩已成功核销。");
    }
}

void PerformanceModule::onFilterChanged()
{
    m_currentPage = 1; 
    QString dateRange = QString("%1,%2").arg(m_startDateEdit->text(), m_endDateEdit->text());
    QString empId = m_employeeCombo->currentData().toString();
    SalaryDataManager::instance()->requestPerformanceRecords(dateRange, empId);
}

void PerformanceModule::setupImagePreview()
{
    QWidget *win = this->window();
    if (!win) return;

    m_imagePreviewOverlay = new QWidget(win);
    m_imagePreviewOverlay->setObjectName("PerfPreviewOverlay");
    m_imagePreviewOverlay->setStyleSheet("#PerfPreviewOverlay { background-color: rgba(0, 0, 0, 215); }");
    m_imagePreviewOverlay->hide();
    m_imagePreviewOverlay->installEventFilter(this);
    
    QVBoxLayout *previewL = new QVBoxLayout(m_imagePreviewOverlay);
    m_previewLabel = new QLabel();
    m_previewLabel->setAlignment(Qt::AlignCenter);
    previewL->addWidget(m_previewLabel);
}

void PerformanceModule::showBigImage(const QString &path, const QString &text)
{
    if (!m_imagePreviewOverlay) setupImagePreview();
    if (!m_imagePreviewOverlay || !m_previewLabel) return;
    
    QPixmap pix;
    if (!path.isEmpty()) {
        pix = ImageUtils::loadPixmap(path);
    } else if (!text.isEmpty()) {
        pix = QPixmap(400, 400);
        pix.fill(Qt::transparent);
        QPainter painter(&pix);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QColor("white"));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(0, 0, 400, 400);
        
        painter.setPen(QColor("#3b82f6"));
        QFont font("Microsoft YaHei", 160, QFont::Bold);
        painter.setFont(font);
        painter.drawText(pix.rect(), Qt::AlignCenter, text);
    }
    
    if (!pix.isNull()) {
        m_previewLabel->setPixmap(pix.scaled(800, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        QWidget *win = this->window();
        if (win) {
            m_imagePreviewOverlay->setGeometry(win->rect());
            m_imagePreviewOverlay->show();
            m_imagePreviewOverlay->raise();
        }
    }
}

void PerformanceModule::hideBigImage()
{
    if (m_imagePreviewOverlay) {
        m_imagePreviewOverlay->hide();
    }
}

bool PerformanceModule::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        // 1. 点击头像放大
        if (watched == m_detailHeaderAvatar) {
            if (m_detailHeaderName->text() == "-") return true;
            showBigImage(m_currentStaffImgPath, m_detailHeaderAvatar->text());
            return true;
        }
        
        // 2. 点击遮罩层关闭
        if (watched == m_imagePreviewOverlay) {
            hideBigImage();
            return true;
        }
    }
    
    return QWidget::eventFilter(watched, event);
}
