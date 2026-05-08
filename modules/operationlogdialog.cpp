#include "operationlogdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QDateTime>
#include <QUuid>
#include <QFrame>
#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QPainterPath>

// 复刻会员模块的圆角选中代理
class LogListDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        // 默认背景（白色）
        painter->fillRect(opt.rect, Qt::white);

        if (opt.state & QStyle::State_Selected) {
            // 选中样式：浅蓝背景 + 蓝边框 + 左右圆角
            QRect rect = opt.rect.adjusted(8, 4, -8, -4);
            int radius = 8;
            QColor borderColor("#3b82f6");
            QColor bgColor("#eff6ff");

            // 绘制圆角矩形背景
            QPainterPath path;
            path.addRoundedRect(rect, radius, radius);
            painter->fillPath(path, bgColor);

            // 绘制边框
            painter->setPen(QPen(borderColor, 1));
            painter->drawPath(path);
        } else if (opt.state & QStyle::State_MouseOver) {
            // 悬停样式：非常浅的灰色背景
            QRect rect = opt.rect.adjusted(8, 4, -8, -4);
            painter->setBrush(QColor("#f8fafc"));
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(rect, 8, 8);
        }

        // 绘制文字
        QString text = index.data().toString();
        QStringList lines = text.split("\n");
        
        // 第一行：时间 (灰色小字)
        painter->setPen(QColor("#64748b"));
        QFont timeFont = opt.font;
        timeFont.setPointSize(9);
        painter->setFont(timeFont);
        painter->drawText(opt.rect.adjusted(24, 10, -24, -30), Qt::AlignLeft | Qt::AlignTop, lines[0]);

        // 第二行：操作内容 (黑/蓝色粗体)
        if (lines.size() > 1) {
            painter->setPen(QColor((opt.state & QStyle::State_Selected) ? "#1e40af" : "#1e293b"));
            QFont contentFont = opt.font;
            contentFont.setWeight(QFont::Bold);
            contentFont.setPointSize(10);
            painter->setFont(contentFont);
            painter->drawText(opt.rect.adjusted(24, 28, -24, -8), Qt::AlignLeft | Qt::AlignVCenter, lines[1]);
        }

        // 绘制底部横线 (分隔线)
        painter->setPen(QPen(QColor("#f1f5f9"), 1));
        painter->drawLine(opt.rect.bottomLeft(), opt.rect.bottomRight());

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        Q_UNUSED(option);
        Q_UNUSED(index);
        return QSize(0, 64); // 增加行高，适应圆角布局
    }
};

OperationLogDialog::OperationLogDialog(QWidget *parent)
    : QDialog(parent), m_currentPage(1), m_itemsPerPage(7), m_totalItems(0), m_dragging(false)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(960, 650);

    m_dataManager = new LogDataManager(this);
    addMockDataIfNeeded();
    setupUi();
    applyStyles();
    loadPage(1);

    if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
        onLogSelected(m_listWidget->item(0));
    }
}

void OperationLogDialog::addMockDataIfNeeded() {
    struct MockEntry {
        QString id;
        QString op;
        QString mod;
        QString act;
        QString field;
        QString oldVal;
        QString newVal;
        int secsAgo;
    };

    QList<MockEntry> mocks = {
        {"m1", "管理员", "订单管理", "更新状态", "订单状态", "待接单", "已完成", 0},
        {"m2", "张三",   "库存管理", "库存盘点", "当前库存量", "50", "48", 3600},
        {"m3", "李四",   "寄养管理", "修改价格", "寄养单价(元/天)", "120.00", "150.00", 7200},
        {"m4", "管理员", "会员管理", "删除会员", "会员状态", "活跃", "已注销", 10800},
        {"m5", "王五",   "库存管理", "商品上架", "上架状态", "未上架", "已上架", 14400},
        {"m6", "张三",   "服务管理", "修改价格", "洗护套餐价格(元)", "88.00", "98.00", 18000},
        {"m7", "李四",   "预约管理", "取消预约", "预约状态", "待确认", "已取消", 21600},
        {"m8", "管理员", "财务管理", "审核工资", "工资状态", "待审核", "已发放", 25200},
        {"m9", "王五",   "寄养管理", "办理入住", "房位状态", "空闲", "已占用", 28800},
        {"m10", "张三",   "会员管理", "会员充值", "卡余额(元)", "500.00", "1500.00", 32400},
        {"m11", "管理员", "库存管理", "库存预警", "预警状态", "正常", "低于安全线", 36000},
        {"m12", "李四",   "订单管理", "订单作废", "订单状态", "已支付", "已作废", 39600},
        {"m13", "王五",   "商品资料", "新增商品", "商品状态", "空", "在售", 43200},
        {"m14", "管理员", "系统配置", "修改费率", "服务提成比例", "20%", "25%", 46800},
        {"m15", "赵六",   "库存管理", "商品入库", "库存数量", "10", "110", 50400}
    };

    qDebug() << "OperationLogDialog: Starting mock data generation...";
    for (const auto &m : mocks) {
        SysOperationLog log;
        log.id = m.id;
        log.timestamp = QDateTime::currentDateTime().addSecs(-m.secsAgo).toString("yyyy-MM-dd HH:mm:ss");
        log.operatorName = m.op;
        log.module = m.mod;
        log.action = m.act;

        QJsonObject diff;
        diff["field"] = m.field;
        diff["old"] = m.oldVal;
        diff["new"] = m.newVal;
        log.details = QJsonDocument(diff).toJson(QJsonDocument::Compact);
        bool ok = m_dataManager->insertMockLog(log);
        if (!ok) qDebug() << "OperationLogDialog: Failed to insert" << m.id;
    }
    
    int finalCount = m_dataManager->getTotalCount();
    qDebug() << "OperationLogDialog: Mock data generation finished. Total count in DB:" << finalCount;
}

void OperationLogDialog::setupUi() {
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    QFrame *bgFrame = new QFrame(this);
    bgFrame->setObjectName("DialogBg");
    outerLayout->addWidget(bgFrame);

    QVBoxLayout *mainLayout = new QVBoxLayout(bgFrame);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ─── Title Bar ────────────────────────────────────────────────
    QWidget *titleBar = new QWidget();
    titleBar->setObjectName("TitleBar");
    titleBar->setFixedHeight(48);
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(20, 0, 12, 0);

    QLabel *titleIcon = new QLabel("📋");
    titleIcon->setStyleSheet("font-size: 18px; border: none; background: transparent;");
    QLabel *titleLabel = new QLabel("日志审计中心");
    titleLabel->setObjectName("TitleLabel");
    titleLayout->addWidget(titleIcon);
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    m_closeBtn = new QPushButton("✕");
    m_closeBtn->setObjectName("CloseBtn");
    m_closeBtn->setFixedSize(32, 32);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    titleLayout->addWidget(m_closeBtn);

    mainLayout->addWidget(titleBar);

    // ─── Filter Bar ──────────────────────────────────────────────
    QWidget *filterBar = new QWidget();
    filterBar->setObjectName("FilterBar");
    filterBar->setFixedHeight(52);
    QHBoxLayout *filterLayout = new QHBoxLayout(filterBar);
    filterLayout->setContentsMargins(20, 0, 20, 0);
    filterLayout->setSpacing(8);

    QLabel *dateLbl = new QLabel("日期：");
    dateLbl->setObjectName("FilterLabel");
    m_startDateEdit = new CustomCalendarEdit();
    m_startDateEdit->setText(QDate::currentDate().addDays(-30).toString("yyyy-MM-dd"));
    m_startDateEdit->setFixedSize(120, 32);

    QLabel *dashLbl = new QLabel("至");
    dashLbl->setObjectName("FilterLabel");
    dashLbl->setFixedWidth(16);
    dashLbl->setAlignment(Qt::AlignCenter);

    m_endDateEdit = new CustomCalendarEdit();
    m_endDateEdit->setText(QDate::currentDate().toString("yyyy-MM-dd"));
    m_endDateEdit->setFixedSize(120, 32);

    QLabel *opLbl = new QLabel("操作人：");
    opLbl->setObjectName("FilterLabel");
    m_operatorEdit = new QComboBox();
    m_operatorEdit->setEditable(true);
    m_operatorEdit->setPlaceholderText("选择或输入...");
    m_operatorEdit->setFixedSize(140, 32);
    
    // 填充已有操作人
    m_operatorEdit->addItem("全部操作人", ""); 
    QStringList operators = m_dataManager->fetchDistinctOperators();
    m_operatorEdit->addItems(operators);
    m_operatorEdit->setCurrentIndex(0);

    QLabel *modLbl = new QLabel("模块：");
    modLbl->setObjectName("FilterLabel");
    m_moduleCombo = new QComboBox();
    m_moduleCombo->setFixedSize(130, 32);
    m_moduleCombo->addItem("全部模块", "");
    QStringList modules = m_dataManager->fetchDistinctModules();
    for (const QString &mod : modules) {
        m_moduleCombo->addItem(mod, mod);
    }

    m_searchBtn = new QPushButton("查询");
    m_searchBtn->setObjectName("PrimaryBtn");
    m_searchBtn->setFixedSize(70, 30);
    m_searchBtn->setCursor(Qt::PointingHandCursor);

    m_resetBtn = new QPushButton("重置");
    m_resetBtn->setObjectName("SecondaryBtn");
    m_resetBtn->setFixedSize(60, 30);
    m_resetBtn->setCursor(Qt::PointingHandCursor);

    filterLayout->addWidget(dateLbl);
    filterLayout->addWidget(m_startDateEdit);
    filterLayout->addWidget(dashLbl);
    filterLayout->addWidget(m_endDateEdit);
    filterLayout->addWidget(opLbl);
    filterLayout->addWidget(m_operatorEdit);
    filterLayout->addWidget(modLbl);
    filterLayout->addWidget(m_moduleCombo);
    filterLayout->addWidget(m_searchBtn);
    filterLayout->addWidget(m_resetBtn);
    filterLayout->addStretch();

    mainLayout->addWidget(filterBar);

    // ─── Master-Detail Split ─────────────────────────────────────
    QHBoxLayout *splitLayout = new QHBoxLayout();
    splitLayout->setContentsMargins(0, 0, 0, 0);
    splitLayout->setSpacing(0);

    // Left Panel: Log List + Pagination
    QWidget *leftPanel = new QWidget();
    leftPanel->setObjectName("LeftPanel");
    leftPanel->setFixedWidth(370);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    m_listWidget = new QListWidget();
    m_listWidget->setObjectName("LogList");
    m_listWidget->setItemDelegate(new LogListDelegate(m_listWidget));
    leftLayout->addWidget(m_listWidget);

    // Pagination bar
    QWidget *pageBar = new QWidget();
    pageBar->setObjectName("PageBar");
    pageBar->setFixedHeight(44);
    QHBoxLayout *pageLayout = new QHBoxLayout(pageBar);
    pageLayout->setContentsMargins(12, 0, 12, 0);
    pageLayout->setSpacing(8);

    m_prevBtn = new QPushButton("‹ 上一页");
    m_prevBtn->setObjectName("PageBtn");
    m_prevBtn->setFixedHeight(28);
    m_prevBtn->setCursor(Qt::PointingHandCursor);

    m_pageLabel = new QLabel("1 / 1");
    m_pageLabel->setObjectName("PageInfo");
    m_pageLabel->setAlignment(Qt::AlignCenter);

    m_nextBtn = new QPushButton("下一页 ›");
    m_nextBtn->setObjectName("PageBtn");
    m_nextBtn->setFixedHeight(28);
    m_nextBtn->setCursor(Qt::PointingHandCursor);

    pageLayout->addWidget(m_prevBtn);
    pageLayout->addStretch();
    pageLayout->addWidget(m_pageLabel);
    pageLayout->addStretch();
    pageLayout->addWidget(m_nextBtn);

    leftLayout->addWidget(pageBar);

    splitLayout->addWidget(leftPanel);

    // Right Panel: Detail Browser
    QWidget *rightPanel = new QWidget();
    rightPanel->setObjectName("RightPanel");
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    m_detailBrowser = new QTextBrowser();
    m_detailBrowser->setObjectName("DetailBrowser");
    m_detailBrowser->setOpenExternalLinks(false);
    rightLayout->addWidget(m_detailBrowser);

    splitLayout->addWidget(rightPanel);

    mainLayout->addLayout(splitLayout);

    // ─── Connections ─────────────────────────────────────────────
    connect(m_searchBtn, &QPushButton::clicked, this, &OperationLogDialog::onSearchClicked);
    connect(m_resetBtn, &QPushButton::clicked, this, &OperationLogDialog::onResetClicked);
    connect(m_prevBtn, &QPushButton::clicked, this, &OperationLogDialog::onPrevClicked);
    connect(m_nextBtn, &QPushButton::clicked, this, &OperationLogDialog::onNextClicked);
    connect(m_listWidget, &QListWidget::itemClicked, this, &OperationLogDialog::onLogSelected);
}

void OperationLogDialog::applyStyles() {
    this->setStyleSheet(R"(
        QFrame#DialogBg {
            background-color: #ffffff;
            border: 1px solid #475569;
            border-radius: 12px;
        }

        /* Title Bar */
        QWidget#TitleBar {
            background-color: #f8fafc;
            border-top-left-radius: 11px;
            border-top-right-radius: 11px;
            border-bottom: 1px solid #e2e8f0;
        }
        QLabel#TitleLabel {
            font-size: 15px;
            font-weight: bold;
            color: #0f172a;
            border: none;
            background: transparent;
        }
        QPushButton#CloseBtn {
            background: transparent;
            border: none;
            border-radius: 16px;
            color: #94a3b8;
            font-size: 16px;
            font-weight: bold;
            text-align: center;
            padding: 0px;
        }
        QPushButton#CloseBtn:hover {
            background-color: #fee2e2;
            color: #ef4444;
        }

        /* Filter Bar */
        QWidget#FilterBar {
            background-color: #f8fafc;
            border-bottom: 1px solid #e2e8f0;
        }
        QLabel#FilterLabel {
            font-size: 12px;
            font-weight: bold;
            color: #475569;
            border: none;
            background: transparent;
        }
        /* DateEdit, LineEdit, ComboBox styling from Inbound module */
        CustomCalendarEdit, QLineEdit, QComboBox {
            border: 1px solid #e2e8f0;
            border-radius: 6px;
            padding: 0 10px;
            font-size: 13px;
            background-color: #f8fafc;
            color: #0f172a;
        }
        CustomCalendarEdit:hover, QLineEdit:hover, QComboBox:hover {
            border-color: #cbd5e1;
            background-color: #f1f5f9;
        }
        CustomCalendarEdit:focus, QLineEdit:focus, QComboBox:focus {
            border: 2px solid #3b82f6;
            background-color: #ffffff;
            padding: 0 9px;
        }
        QComboBox::drop-down {
            border: none;
            width: 24px;
        }
        QComboBox::down-arrow {
            image: none;
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-top: 5px solid #64748b;
            margin-right: 6px;
        }

        /* 复刻会员模块的下拉列表美化样式 */
        QComboBox QAbstractItemView {
            border: 1px solid #e2e8f0;
            background-color: #ffffff;
            selection-background-color: #f1f5f9;
            selection-color: #3b82f6;
            outline: none;
            border-radius: 8px;
            padding: 4px;
        }
        QComboBox QAbstractItemView::item {
            height: 32px;
            padding-left: 10px;
            color: #475569;
            border-radius: 4px;
            margin: 1px 0px;
        }
        QComboBox QAbstractItemView::item:selected {
            background-color: #eff6ff;
            color: #3b82f6;
            border-left: 3px solid #3b82f6;
        }

        /* Primary / Secondary Buttons */
        QPushButton#PrimaryBtn {
            background-color: #3b82f6;
            color: #ffffff;
            font-weight: bold;
            font-size: 12px;
            border-radius: 6px;
            border: none;
            text-align: center;
            padding: 0px;
        }
        QPushButton#PrimaryBtn:hover {
            background-color: #2563eb;
        }
        QPushButton#SecondaryBtn {
            background-color: #f1f5f9;
            color: #475569;
            font-weight: bold;
            font-size: 12px;
            border-radius: 6px;
            border: 1px solid #e2e8f0;
            text-align: center;
            padding: 0px;
        }
        QPushButton#SecondaryBtn:hover {
            background-color: #e2e8f0;
        }

        /* Left Panel */
        QWidget#LeftPanel {
            background-color: #ffffff;
            border-right: 1px solid #e2e8f0;
        }
        QListWidget#LogList {
            background-color: #ffffff;
            border: none;
            outline: none;
            padding: 4px 0px;
        }
        QListWidget#LogList::item {
            border: none;
            background: transparent;
        }
        QListWidget#LogList::item:selected {
            background: transparent;
            border: none;
        }

        /* Pagination Bar */
        QWidget#PageBar {
            background-color: #fafbfc;
            border-top: 1px solid #e2e8f0;
        }
        QPushButton#PageBtn {
            background-color: #f1f5f9;
            border: 1px solid #e2e8f0;
            border-radius: 4px;
            padding: 4px 14px;
            font-size: 12px;
            color: #475569;
            font-weight: bold;
            text-align: center;
        }
        QPushButton#PageBtn:hover {
            background-color: #e2e8f0;
        }
        QPushButton#PageBtn:disabled {
            color: #cbd5e1;
            background-color: #f8fafc;
            border-color: #f1f5f9;
        }
        QLabel#PageInfo {
            font-size: 12px;
            color: #64748b;
            border: none;
            background: transparent;
        }

        /* Right Panel */
        QWidget#RightPanel {
            background-color: #ffffff;
            border-bottom-right-radius: 11px;
        }
        QTextBrowser#DetailBrowser {
            background-color: #ffffff;
            border: none;
            padding: 24px;
            font-size: 13px;
            color: #334155;
        }
    )");
}

void OperationLogDialog::onSearchClicked() {
    loadPage(1);
}

void OperationLogDialog::onResetClicked() {
    m_startDateEdit->setText(QDate::currentDate().addDays(-30).toString("yyyy-MM-dd"));
    m_endDateEdit->setText(QDate::currentDate().toString("yyyy-MM-dd"));
    m_operatorEdit->setEditText("");
    m_moduleCombo->setCurrentIndex(0);
    loadPage(1);
}

void OperationLogDialog::onPrevClicked() {
    if (m_currentPage > 1) {
        loadPage(m_currentPage - 1);
    }
}

void OperationLogDialog::onNextClicked() {
    int totalPages = qMax(1, (m_totalItems + m_itemsPerPage - 1) / m_itemsPerPage);
    if (m_currentPage < totalPages) {
        loadPage(m_currentPage + 1);
    }
}

void OperationLogDialog::loadPage(int page) {
    m_currentPage = page;
    m_listWidget->clear();
    
    QString start = m_startDateEdit->date().toString("yyyy-MM-dd");
    QString end = m_endDateEdit->date().toString("yyyy-MM-dd");
    QString op = m_operatorEdit->currentText();
    if (op == "全部操作人") op = "";
    QString mod = m_moduleCombo->currentData().toString();
    
    m_totalItems = m_dataManager->getTotalCount(start, end, op, mod);
    int totalPages = qMax(1, (m_totalItems + m_itemsPerPage - 1) / m_itemsPerPage);
    
    m_pageLabel->setText(QString("第 %1 / %2 页 · 共 %3 条").arg(m_currentPage).arg(totalPages).arg(m_totalItems));
    m_prevBtn->setEnabled(m_currentPage > 1);
    m_nextBtn->setEnabled(m_currentPage < totalPages);
    
    int offset = (m_currentPage - 1) * m_itemsPerPage;
    QList<SysOperationLog> logs = m_dataManager->fetchLogs(m_itemsPerPage, offset, start, end, op, mod);
    
    for (const auto &log : logs) {
        QListWidgetItem *item = new QListWidgetItem(m_listWidget);
        // 格式: 时间 \n 操作人 · 动作  [模块]
        QString displayText = QString("%1\n%2 · %3  [%4]").arg(log.timestamp, log.operatorName, log.action, log.module);
        item->setText(displayText);

        // 把完整信息存储到 UserRole 系列中
        item->setData(Qt::UserRole, log.details);
        item->setData(Qt::UserRole + 1, log.timestamp);
        item->setData(Qt::UserRole + 2, log.operatorName);
        item->setData(Qt::UserRole + 3, log.module);
        item->setData(Qt::UserRole + 4, log.action);

        item->setSizeHint(QSize(0, 58));
    }

    // 默认选中第一条
    if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
        onLogSelected(m_listWidget->item(0));
    } else {
        m_detailBrowser->setHtml(
            "<div style='text-align:center; padding-top:120px; color:#94a3b8; font-size:14px;'>"
            "暂无符合条件的操作日志</div>"
        );
    }
}

void OperationLogDialog::onLogSelected(QListWidgetItem *item) {
    if (!item) return;

    QString details = item->data(Qt::UserRole).toString();
    QString timestamp = item->data(Qt::UserRole + 1).toString();
    QString operatorName = item->data(Qt::UserRole + 2).toString();
    QString module = item->data(Qt::UserRole + 3).toString();
    QString action = item->data(Qt::UserRole + 4).toString();

    // 构建详情 HTML
    QString html;

    // 操作详情头部
    html += "<div style='margin-bottom: 20px;'>"
            "<h3 style='font-size:16px; color:#0f172a; font-weight:700; margin-bottom:16px;'>操作详情</h3>"
            "<table cellspacing='0' cellpadding='0' style='font-size:13px; border:none;'>"
            "<tr><td style='color:#94a3b8; padding:4px 16px 4px 0; min-width:70px;'>操作时间</td>"
                "<td style='color:#334155; font-weight:500;'>" + timestamp.toHtmlEscaped() + "</td></tr>"
            "<tr><td style='color:#94a3b8; padding:4px 16px 4px 0;'>操作人</td>"
                "<td style='color:#334155; font-weight:500;'>" + operatorName.toHtmlEscaped() + "</td></tr>"
            "<tr><td style='color:#94a3b8; padding:4px 16px 4px 0;'>业务模块</td>"
                "<td style='color:#3b82f6; font-weight:500;'>" + module.toHtmlEscaped() + "</td></tr>"
            "<tr><td style='color:#94a3b8; padding:4px 16px 4px 0;'>操作类型</td>"
                "<td style='color:#334155; font-weight:500;'>" + action.toHtmlEscaped() + "</td></tr>"
            "</table></div>";

    // 分隔线
    html += "<hr style='border:none; border-top:1px solid #f1f5f9; margin:0 0 20px 0;'>";

    // Diff 对比
    html += renderDiffHtml(details);

    m_detailBrowser->setHtml(html);
}

QString OperationLogDialog::renderDiffHtml(const QString &jsonStr) {
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());

    // 尝试解析为数组 (多字段变更)
    QJsonArray diffArray;
    if (doc.isArray()) {
        diffArray = doc.array();
    } else if (doc.isObject()) {
        diffArray.append(doc.object());
    } else {
        return QString("<p style='color:#475569;'>%1</p>").arg(jsonStr.toHtmlEscaped());
    }

    QString html;
    html += "<div>"
            "<h4 style='font-size:14px; color:#0f172a; font-weight:700; margin-bottom:14px; "
            "padding-left:10px; border-left:3px solid #3b82f6;'>"
            "数据变更对比</h4>";

    html += "<table width='100%' cellspacing='0' cellpadding='0' "
            "style='border-collapse:collapse; font-size:13px;'>"
            "<tr style='background-color:#f8fafc;'>"
            "<th style='padding:10px 14px; text-align:left; color:#475569; font-weight:600; font-size:12px; border-bottom:2px solid #e2e8f0; width:30%;'>字段名</th>"
            "<th style='padding:10px 14px; text-align:left; color:#475569; font-weight:600; font-size:12px; border-bottom:2px solid #e2e8f0; width:35%;'>修改前 (Old)</th>"
            "<th style='padding:10px 14px; text-align:left; color:#475569; font-weight:600; font-size:12px; border-bottom:2px solid #e2e8f0; width:35%;'>修改后 (New)</th>"
            "</tr>";

    for (int i = 0; i < diffArray.size(); ++i) {
        QJsonObject obj = diffArray[i].toObject();
        QString field = obj.value("field").toString();
        QString oldVal = obj.value("old").toString();
        QString newVal = obj.value("new").toString();

        html += "<tr>"
                "<td style='padding:10px 14px; border-bottom:1px solid #f1f5f9; font-weight:600; color:#334155;'>"
                    + field.toHtmlEscaped() + "</td>"
                "<td style='padding:10px 14px; border-bottom:1px solid #f1f5f9;'>"
                    "<span style='background-color:#fef2f2; color:#991b1b; text-decoration:line-through; "
                    "padding:2px 8px; border-radius:3px; font-family:Consolas,monospace; font-size:12px;'>"
                    + oldVal.toHtmlEscaped() + "</span></td>"
                "<td style='padding:10px 14px; border-bottom:1px solid #f1f5f9;'>"
                    "<span style='background-color:#f0fdf4; color:#166534; "
                    "padding:2px 8px; border-radius:3px; font-family:Consolas,monospace; font-size:12px;'>"
                    + newVal.toHtmlEscaped() + "</span></td>"
                "</tr>";
    }

    html += "</table></div>";
    return html;
}

// ─── Frameless window dragging ──────────────────────────────────
void OperationLogDialog::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && event->pos().y() < 48) {
        m_dragging = true;
        m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void OperationLogDialog::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragPos);
        event->accept();
    }
}

void OperationLogDialog::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
}
