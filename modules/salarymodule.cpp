#include "salarymodule.h"
#include "salarydatamanager.h"
#include "staffdatamanager.h"
#include "common_types.h"
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
#include <QApplication>
#include <QStyledItemDelegate>
#include <QEvent>
#include <QPainter>
#include <QPainterPath>

// --- 复刻：全行圆角选中委托 ---
class SalaryRowDelegate : public QStyledItemDelegate {
public:
    private:
    mutable int m_hoveredRow = -1;
public:
    SalaryRowDelegate(QObject *parent) : QStyledItemDelegate(parent) {
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

SalaryModule::SalaryModule(QWidget *parent) : QWidget(parent)
{
    m_currentMonth = QDate::currentDate().toString("yyyy-MM");
    setupUI();
    refreshData();
    
    if (m_salaryTable->rowCount() > 0) {
        m_salaryTable->selectRow(0);
    }

    // 触发初始月份数据请求
    onMonthChanged(0);

    connect(SalaryDataManager::instance(), &SalaryDataManager::salaryDataChanged, this, &SalaryModule::refreshData);
}

void SalaryModule::setupUI()
{
    setObjectName("SalaryModule");
    setStyleSheet("#SalaryModule { background-color: #f8fafc; font-family: 'Microsoft YaHei'; }");

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

    QLabel *titleLbl = new QLabel("薪资发放管理中心");
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

    QFrame *totalCard = createStatsCard("月度发放总额 (CNY)", "¥ 0.00", "#3b82f6");
    m_totalPayrollVal = static_cast<QLabel*>(totalCard->layout()->itemAt(1)->widget());
    QFrame *paidCard = createStatsCard("已发放人数", "0 人", "#10b981");
    m_paidCountVal = static_cast<QLabel*>(paidCard->layout()->itemAt(1)->widget());
    QFrame *pendingCard = createStatsCard("待审核人数", "0 人", "#f59e0b");
    m_pendingCountVal = static_cast<QLabel*>(pendingCard->layout()->itemAt(1)->widget());

    statsLayout->addWidget(totalCard, 1);
    statsLayout->addWidget(paidCard, 1);
    statsLayout->addWidget(pendingCard, 1);
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

    m_monthCombo = new QComboBox();
    m_monthCombo->setFixedWidth(120);
    for(int i=0; i<6; ++i) {
        m_monthCombo->addItem(QDate::currentDate().addMonths(-i).toString("yyyy-MM"));
    }
    m_monthCombo->setStyleSheet(comboStyle);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("搜索员工姓名...");
    m_searchEdit->setFixedWidth(180);
    m_searchEdit->setStyleSheet(lineEditStyle);


    filterLayout->addWidget(new QLabel("核算月份:"));
    filterLayout->addWidget(m_monthCombo);
    filterLayout->addSpacing(10);
    filterLayout->addWidget(m_searchEdit);
    filterLayout->addStretch();
    leftColumn->addWidget(filterGroup);

    // 表格卡片
    QFrame *tableCard = new QFrame();
    tableCard->setObjectName("tableCard");
    tableCard->setStyleSheet("QFrame#tableCard { background-color: white; border: 1px solid #e2e8f0; border-radius: 12px; }");
    QVBoxLayout *tableCardLayout = new QVBoxLayout(tableCard);
    tableCardLayout->setContentsMargins(0, 0, 0, 0);
    tableCardLayout->setSpacing(0);

    m_salaryTable = new QTableWidget();
    m_salaryTable->setColumnCount(9);
    m_salaryTable->setHorizontalHeaderLabels({"姓名", "工号", "岗位", "基本工资", "提成", "社保代缴", "个税代扣 (3%)", "实发薪资", "状态"});
    m_salaryTable->setItemDelegate(new SalaryRowDelegate(m_salaryTable));
    m_salaryTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_salaryTable->verticalHeader()->setVisible(false);
    m_salaryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_salaryTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_salaryTable->setShowGrid(false);
    m_salaryTable->setFocusPolicy(Qt::NoFocus);
    m_salaryTable->setStyleSheet(R"(
        QTableWidget { border: none; background: white; outline: none; border-radius: 12px; }
        QHeaderView::section { background-color: white; border: none; border-bottom: 1px solid #f1f5f9; padding: 12px; font-weight: bold; color: #64748b; font-size: 13px; }
    )");
    tableCardLayout->addWidget(m_salaryTable, 1);

    // --- 3. 底部页码栏 ---
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

    connect(m_prevBtn, &QPushButton::clicked, this, &SalaryModule::onPrevPage);
    connect(m_nextBtn, &QPushButton::clicked, this, &SalaryModule::onNextPage);

    contentLayout->addLayout(leftColumn, 3);

    // 2.2 右侧列：详情卡片
    QFrame *detailCard = new QFrame();
    detailCard->setObjectName("detailCard");
    detailCard->setFixedWidth(450); 
    detailCard->setStyleSheet("QFrame#detailCard { background-color: white; border: 1px solid #e2e8f0; border-radius: 12px; }");
    QVBoxLayout *detailLayout = new QVBoxLayout(detailCard);
    detailLayout->setContentsMargins(25, 30, 25, 25);
    
    QLabel *detailTitle = new QLabel("薪资明细分析");
    detailTitle->setStyleSheet("font-weight: bold; font-size: 16px; color: #1e293b; margin-bottom: 20px; border: none; background: transparent;");
    detailLayout->addWidget(detailTitle);
    
    // 2.2.1 员工头部
    QWidget *headerWidget = new QWidget();
    headerWidget->setFixedHeight(80);
    QHBoxLayout *headerL = new QHBoxLayout(headerWidget);
    headerL->setContentsMargins(0,0,0,10);
    
    m_detailAvatar = new QLabel();
    m_detailAvatar->setFixedSize(60, 60);
    m_detailAvatar->setStyleSheet("background-color: #f1f5f9; border-radius: 30px; font-weight: 800; font-size: 20px; color: #3b82f6; border: none;");
    m_detailAvatar->setAlignment(Qt::AlignCenter);
    m_detailAvatar->setCursor(Qt::PointingHandCursor);
    
    // 图片放大功能
    m_detailAvatar->installEventFilter(this);
    
    QVBoxLayout *infoL = new QVBoxLayout();
    infoL->setSpacing(4);
    
    QHBoxLayout *nameIdL = new QHBoxLayout();
    nameIdL->setSpacing(10);
    m_detailName = new QLabel("-"); m_detailName->setStyleSheet("font-weight: bold; font-size: 18px; color: #1e293b; border: none;");
    m_detailEmpId = new QLabel("ID: -"); m_detailEmpId->setStyleSheet("color: #94a3b8; font-size: 13px; border: none;");
    nameIdL->addWidget(m_detailName);
    nameIdL->addWidget(m_detailEmpId);
    nameIdL->addStretch();

    m_detailRole = new QLabel("岗位: -"); m_detailRole->setStyleSheet("color: #64748b; font-size: 13px; border: none; background: transparent;");
    
    infoL->addLayout(nameIdL);
    infoL->addWidget(m_detailRole);
    
    headerL->addWidget(m_detailAvatar);
    headerL->addLayout(infoL);
    headerL->addStretch();
    
    detailLayout->addWidget(headerWidget);

    // 分割线
    QFrame *sep = new QFrame(); sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("background: #f1f5f9; min-height: 1px; max-height: 1px; margin: 10px 0; border: none;");
    detailLayout->addWidget(sep);

    auto createDetailItem = [&](const QString &label, QLabel* valLabel, const QString &color = "#1e293b") {
        QWidget *w = new QWidget();
        w->setStyleSheet("background: transparent; border: none;");
        QHBoxLayout *hl = new QHBoxLayout(w);
        hl->setContentsMargins(0,10,0,10);
        QLabel *ll = new QLabel(label); ll->setStyleSheet("color: #64748b; font-size: 14px; border: none;");
        valLabel->setStyleSheet(QString("font-weight: bold; font-size: 14px; color: %1; border: none;").arg(color));
        valLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        hl->addWidget(ll); hl->addStretch(); hl->addWidget(valLabel);
        return w;
    };
    
    detailLayout->addWidget(createDetailItem("底薪", m_detailBaseVal = new QLabel("-")));
    detailLayout->addWidget(createDetailItem("业绩提成", m_detailCommVal = new QLabel("-"), "#10b981"));
    
    // --- 五险一金明细面板 ---
    QFrame *insFrame = new QFrame();
    insFrame->setStyleSheet("background: #f8fafc; border-radius: 8px; margin: 5px 0; border: none;");
    QVBoxLayout *insLayout = new QVBoxLayout(insFrame);
    insLayout->setContentsMargins(12, 8, 12, 8);
    insLayout->setSpacing(6);
    
    auto addSubRow = [&](QVBoxLayout *l, const QString &name, QLabel* &val) {
        QHBoxLayout *row = new QHBoxLayout();
        row->setContentsMargins(0,0,0,0);
        QLabel *n = new QLabel(name);
        n->setStyleSheet("color: #94a3b8; font-size: 12px; border: none; background: transparent;");
        val = new QLabel("--");
        val->setStyleSheet("color: #64748b; font-size: 12px; font-weight: bold; border: none; background: transparent;");
        val->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row->addWidget(n);
        row->addWidget(val, 1);
        l->addLayout(row);
    };
    
    addSubRow(insLayout, "参保基数", m_detailInsBaseVal = new QLabel());
    addSubRow(insLayout, "养老保险 (8%)", m_detailPensionVal = new QLabel());
    addSubRow(insLayout, "医疗保险 (2%)", m_detailMedicalVal = new QLabel());
    addSubRow(insLayout, "失业保险 (0.5%)", m_detailUnemploymentVal = new QLabel());
    addSubRow(insLayout, "住房公积金 (7%)", m_detailHousingFundVal = new QLabel());
    
    detailLayout->addWidget(insFrame);

    detailLayout->addWidget(createDetailItem("社保代缴", m_detailInsuranceVal = new QLabel("-"), "#f43f5e"));
    detailLayout->addWidget(createDetailItem("个税代扣 (3%)", m_detailTaxVal = new QLabel("-"), "#64748b"));
    
    QFrame *sep2 = new QFrame(); sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet("background: #f1f5f9; min-height: 1px; max-height: 1px; margin: 10px 0; border: none;");
    detailLayout->addWidget(sep2);
    
    detailLayout->addWidget(createDetailItem("实发合计", m_detailNetVal = new QLabel("-"), "#1e40af"));
    detailLayout->addStretch();
    
    m_payBtn = new QPushButton("确认发放薪资");
    m_payBtn->setFixedHeight(48);
    m_payBtn->setCursor(Qt::PointingHandCursor);
    m_payBtn->setStyleSheet("QPushButton { background: white; border: 1px solid #3b82f6; color: #3b82f6; border-radius: 8px; font-weight: bold; font-size: 15px; } QPushButton:hover { background: #eff6ff; }");
    detailLayout->addWidget(m_payBtn);

    // 监听表格选择
    connect(m_salaryTable, &QTableWidget::itemSelectionChanged, this, [this](){
        int row = m_salaryTable->currentRow();
        if (row < 0) return;
        QString salaryId = m_salaryTable->item(row, 0)->data(Qt::UserRole).toString();
        m_currentSalaryId = salaryId;
        
        auto salaries = SalaryDataManager::instance()->getSalariesByMonth(m_currentMonth);
        for (const auto &s : salaries) {
            if (s.id == salaryId) {
                m_detailAvatar->setText(s.employeeName.left(1));
                m_detailName->setText(s.employeeName);
                // 格式化 ID 以匹配 StaffDataManager 的格式 (E00X)
                QString staffKey = QString("E%1").arg(s.employeeId.toInt(), 3, 10, QChar('0'));
                EmployeeInfo emp = StaffDataManager::instance()->getStaff(staffKey);
                
                m_detailEmpId->setText("ID: " + emp.id); // 显示工号
                m_currentStaffImgPath = emp.imgPath; // 存储当前路径用于点击放大
                
                // 加载真实头像
                if (!emp.imgPath.isEmpty()) {
                    QPixmap pix = ImageUtils::loadPixmap(emp.imgPath);
                    m_detailAvatar->setPixmap(ImageUtils::getCircularPixmap(pix, 60));
                    m_detailAvatar->setText("");
                } else {
                    m_detailAvatar->setPixmap(QPixmap());
                    m_detailAvatar->setText(s.employeeName.left(1));
                }
                
                m_detailRole->setText(emp.role.isEmpty() ? "普通员工" : emp.role);
                
                m_detailBaseVal->setText(QString("¥%1").arg(s.baseSalary, 0, 'f', 2));
                m_detailCommVal->setText(QString("+¥%1").arg(s.commission, 0, 'f', 2));
                
                // 计算五险一金明细
                double insBase = s.baseSalary; // 默认基数为底薪
                double pension = insBase * 0.08;
                double medical = insBase * 0.02;
                double unemployment = insBase * 0.005;
                double housing = insBase * 0.07;
                double totalIns = pension + medical + unemployment + housing;

                // 计算个税 (起征点5000)
                double taxableIncome = s.baseSalary + s.commission - totalIns - 5000;
                double tax = (taxableIncome > 0) ? taxableIncome * 0.03 : 0;
                
                // 真正的实发薪资
                double realNetPay = s.baseSalary + s.commission - totalIns - tax;

                m_detailInsBaseVal->setText(QString("¥%1").arg(insBase, 0, 'f', 2));
                m_detailPensionVal->setText(QString("¥%1").arg(pension, 0, 'f', 2));
                m_detailMedicalVal->setText(QString("¥%1").arg(medical, 0, 'f', 2));
                m_detailUnemploymentVal->setText(QString("¥%1").arg(unemployment, 0, 'f', 2));
                m_detailHousingFundVal->setText(QString("¥%1").arg(housing, 0, 'f', 2));

                m_detailInsuranceVal->setText(QString("-¥%1").arg(totalIns, 0, 'f', 2));
                m_detailTaxVal->setText(QString("-¥%1").arg(tax, 0, 'f', 2));
                m_detailNetVal->setText(QString("¥%1").arg(realNetPay, 0, 'f', 2));
                
                m_payBtn->setVisible(s.status == "待审核");
                break;
            }
        }
    });

    connect(m_payBtn, &QPushButton::clicked, this, [this](){
        if (m_currentSalaryId.isEmpty()) return;
        if (CustomMessageDialog::confirm(this, "发放确认", "确定完成薪资发放审核吗？")) {
            onPaySalary(m_currentSalaryId);
            m_payBtn->hide(); // 立即隐藏按钮
            refreshData();   // 刷新全局数据与统计
            
            // 刷新详情面板状态（可选：重新根据ID加载最新状态）
            auto s = SalaryDataManager::instance()->getSalary(m_currentSalaryId);
            if (s.id == m_currentSalaryId) {
                 m_payBtn->setVisible(s.status == "待审核");
            }
        }
    });

    contentLayout->addWidget(detailCard, 1);
    rootLayout->addLayout(contentLayout, 1);

    // 事件连接
    connect(m_monthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SalaryModule::onMonthChanged);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &SalaryModule::onSearchEmployee);
}

void SalaryModule::refreshData()
{
    // 该方法由 salaryDataChanged 信号触发
    updateStats();
    updateTable();
}

void SalaryModule::updateStats()
{
    auto stats = SalaryDataManager::instance()->getStats(m_currentMonth);
    m_totalPayrollVal->setText(QString("¥ %1").arg(stats.totalPayroll, 0, 'f', 2));
    m_paidCountVal->setText(QString("%1 人").arg(stats.paidCount));
    m_pendingCountVal->setText(QString("%1 人").arg(stats.pendingCount));
}

void SalaryModule::updateTable()
{
    auto allSalaries = SalaryDataManager::instance()->getSalariesByMonth(m_currentMonth);
    QVector<SalaryInfo> filtered;
    QString filter = m_searchEdit->text().trimmed().toLower();
    for (const auto &s : allSalaries) {
        if (!filter.isEmpty() && !s.employeeName.toLower().contains(filter)) continue;
        filtered.append(s);
    }

    int total = filtered.size();
    int totalPages = std::max(1, (total + m_pageSize - 1) / m_pageSize);
    if (m_currentPage > totalPages) m_currentPage = totalPages;
    if (m_currentPage < 1) m_currentPage = 1;

    m_pageInfoLabel->setText(QString("第 %1 页 / 共 %2 页").arg(m_currentPage).arg(totalPages));
    m_prevBtn->setEnabled(m_currentPage > 1);
    m_nextBtn->setEnabled(m_currentPage < totalPages);

    m_salaryTable->setRowCount(0);
    int start = (m_currentPage - 1) * m_pageSize;
    int end = std::min(start + m_pageSize, total);

    for (int i = start; i < end; ++i) {
        const auto &s = filtered[i];
        int row = m_salaryTable->rowCount();
        m_salaryTable->insertRow(row);
        m_salaryTable->setRowHeight(row, 56);

        auto createItem = [](const QString &text) {
            return new QTableWidgetItem(text);
        };

        QTableWidgetItem *nameItem = createItem(s.employeeName);
        nameItem->setData(Qt::UserRole, s.id);
        m_salaryTable->setItem(row, 0, nameItem);

        QString staffKey = QString("E%1").arg(s.employeeId.toInt(), 3, 10, QChar('0'));
        EmployeeInfo emp = StaffDataManager::instance()->getStaff(staffKey);

        m_salaryTable->setItem(row, 1, createItem(emp.id)); // 工号
        m_salaryTable->setItem(row, 2, createItem(emp.role.isEmpty() ? "普通员工" : emp.role));
        m_salaryTable->setItem(row, 3, createItem(QString("¥%1").arg(s.baseSalary, 0, 'f', 2)));
        m_salaryTable->setItem(row, 4, createItem(QString("¥%1").arg(s.commission, 0, 'f', 2)));
        
        // 计算扣除项与真实的实发薪资
        double insBase = s.baseSalary;
        double totalIns = insBase * (0.08 + 0.02 + 0.005 + 0.07);
        double taxableIncome = s.baseSalary + s.commission - totalIns - 5000;
        double tax = (taxableIncome > 0) ? taxableIncome * 0.03 : 0;
        double realNetPay = s.baseSalary + s.commission - totalIns - tax;

        m_salaryTable->setItem(row, 5, createItem(QString("-¥%1").arg(totalIns, 0, 'f', 2)));
        m_salaryTable->setItem(row, 6, createItem(QString("-¥%1").arg(tax, 0, 'f', 2)));
        m_salaryTable->setItem(row, 7, createItem(QString("¥%1").arg(realNetPay, 0, 'f', 2)));

        QLabel *statusLabel = new QLabel(s.status);
        statusLabel->setAlignment(Qt::AlignCenter);
        QString statusColor = (s.status == "已发放") ? "#d1fae5" : "#ffedd5";
        QString textColor = (s.status == "已发放") ? "#065f46" : "#9a3412";
        statusLabel->setFixedSize(70, 20);
        statusLabel->setStyleSheet(QString("background-color: %1; color: %2; border-radius: 10px; font-size: 10px; font-weight: bold; border: none;").arg(statusColor, textColor));
        
        QWidget *statusContainer = new QWidget();
        QHBoxLayout *sl = new QHBoxLayout(statusContainer);
        sl->setContentsMargins(0, 0, 0, 0); sl->setAlignment(Qt::AlignCenter);
        sl->addWidget(statusLabel);
        m_salaryTable->setCellWidget(row, 8, statusContainer);
        m_salaryTable->setItem(row, 8, new QTableWidgetItem());
    }
}

void SalaryModule::onPrevPage() {
    if (m_currentPage > 1) {
        m_currentPage--;
        updateTable();
    }
}

void SalaryModule::onNextPage() {
    m_currentPage++;
    updateTable();
}

void SalaryModule::onMonthChanged(int index)
{
    m_currentMonth = m_monthCombo->itemText(index);
    SalaryDataManager::instance()->requestSalaries(m_currentMonth);
}

void SalaryModule::onSearchEmployee(const QString &text)
{
    Q_UNUSED(text);
    m_currentPage = 1; // 搜索时重置页码
    updateTable();
}

void SalaryModule::onPaySalary(const QString &salaryId)
{
    SalaryDataManager::instance()->paySalary(salaryId);
}

void SalaryModule::onEditSalary(const QString &salaryId)
{
    Q_UNUSED(salaryId);
}

void SalaryModule::setupImagePreview()
{
    QWidget *win = this->window();
    if (!win) return;

    m_imagePreviewOverlay = new QWidget(win);
    m_imagePreviewOverlay->setObjectName("SalaryPreviewOverlay");
    m_imagePreviewOverlay->setStyleSheet("#SalaryPreviewOverlay { background-color: rgba(0, 0, 0, 215); }");
    m_imagePreviewOverlay->hide();
    m_imagePreviewOverlay->installEventFilter(this);
    
    QVBoxLayout *previewL = new QVBoxLayout(m_imagePreviewOverlay);
    m_previewLabel = new QLabel();
    m_previewLabel->setAlignment(Qt::AlignCenter);
    previewL->addWidget(m_previewLabel);
}

void SalaryModule::showBigImage(const QString &path, const QString &text)
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

void SalaryModule::hideBigImage()
{
    if (m_imagePreviewOverlay) {
        m_imagePreviewOverlay->hide();
    }
}

bool SalaryModule::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        // 1. 点击头像放大
        if (watched == m_detailAvatar) {
            if (m_detailName->text() == "-") return true;
            showBigImage(m_currentStaffImgPath, m_detailAvatar->text());
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
