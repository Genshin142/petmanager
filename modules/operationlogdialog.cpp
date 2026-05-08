#include "operationlogdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QDateTime>
#include <QUuid>

OperationLogDialog::OperationLogDialog(QWidget *parent)
    : QDialog(parent), m_currentPage(1), m_itemsPerPage(20), m_totalItems(0)
{
    m_dataManager = new LogDataManager(this);
    
    // Auto-generate some mock data if empty
    addMockDataIfNeeded();
    
    setupUi();
    loadPage(1);
    
    // Pre-select first item
    if(m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
        onLogSelected(m_listWidget->item(0));
    }
}

void OperationLogDialog::addMockDataIfNeeded() {
    if (m_dataManager->getTotalCount() == 0) {
        SysOperationLog log1;
        log1.id = QUuid::createUuid().toString();
        log1.timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        log1.operatorName = "管理员";
        log1.module = "订单管理";
        log1.action = "更新状态";
        
        QJsonObject diff1;
        diff1["field"] = "订单状态";
        diff1["old"] = "待接单";
        diff1["new"] = "已完成";
        QJsonDocument doc1(diff1);
        log1.details = doc1.toJson(QJsonDocument::Compact);
        
        m_dataManager->insertMockLog(log1);
        
        SysOperationLog log2;
        log2.id = QUuid::createUuid().toString();
        log2.timestamp = QDateTime::currentDateTime().addSecs(-3600).toString("yyyy-MM-dd HH:mm:ss");
        log2.operatorName = "张三";
        log2.module = "库存管理";
        log2.action = "库存盘点";
        
        QJsonObject diff2;
        diff2["field"] = "当前库存量";
        diff2["old"] = "50";
        diff2["new"] = "48";
        QJsonDocument doc2(diff2);
        log2.details = doc2.toJson(QJsonDocument::Compact);
        
        m_dataManager->insertMockLog(log2);
    }
}

void OperationLogDialog::setupUi() {
    setWindowTitle("日志审计中心");
    resize(900, 600);
    setStyleSheet("QDialog { background-color: #F8FAFC; }");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Top Filter Bar
    QHBoxLayout *filterLayout = new QHBoxLayout();
    m_startDateEdit = new QDateEdit(QDate::currentDate().addDays(-30));
    m_startDateEdit->setCalendarPopup(true);
    m_endDateEdit = new QDateEdit(QDate::currentDate());
    m_endDateEdit->setCalendarPopup(true);
    m_operatorEdit = new QLineEdit();
    m_operatorEdit->setPlaceholderText("操作人...");
    m_searchBtn = new QPushButton("查询");
    
    filterLayout->addWidget(new QLabel("日期:"));
    filterLayout->addWidget(m_startDateEdit);
    filterLayout->addWidget(new QLabel("-"));
    filterLayout->addWidget(m_endDateEdit);
    filterLayout->addWidget(new QLabel(" 操作人:"));
    filterLayout->addWidget(m_operatorEdit);
    filterLayout->addWidget(m_searchBtn);
    filterLayout->addStretch();
    
    mainLayout->addLayout(filterLayout);
    
    // Master-Detail Split
    QHBoxLayout *splitLayout = new QHBoxLayout();
    
    // Left: List and Pagination
    QVBoxLayout *leftLayout = new QVBoxLayout();
    m_listWidget = new QListWidget();
    m_listWidget->setStyleSheet("QListWidget { background: white; border: 1px solid #e2e8f0; border-radius: 4px; padding: 5px; } "
                                "QListWidget::item { padding: 10px; border-bottom: 1px solid #f1f5f9; } "
                                "QListWidget::item:selected { background: #eff6ff; color: #1E40AF; border-left: 3px solid #3B82F6; }");
    m_listWidget->setFixedWidth(350);
    
    QHBoxLayout *paginationLayout = new QHBoxLayout();
    m_prevBtn = new QPushButton("上一页");
    m_nextBtn = new QPushButton("下一页");
    m_pageLabel = new QLabel("1 / 1");
    
    paginationLayout->addWidget(m_prevBtn);
    paginationLayout->addStretch();
    paginationLayout->addWidget(m_pageLabel);
    paginationLayout->addStretch();
    paginationLayout->addWidget(m_nextBtn);
    
    leftLayout->addWidget(m_listWidget);
    leftLayout->addLayout(paginationLayout);
    
    // Right: Details Browser
    m_detailBrowser = new QTextBrowser();
    m_detailBrowser->setStyleSheet("QTextBrowser { background: white; border: 1px solid #e2e8f0; border-radius: 4px; padding: 20px; }");
    
    splitLayout->addLayout(leftLayout);
    splitLayout->addWidget(m_detailBrowser);
    
    mainLayout->addLayout(splitLayout);
    
    // Connections
    connect(m_searchBtn, &QPushButton::clicked, this, &OperationLogDialog::onSearchClicked);
    connect(m_prevBtn, &QPushButton::clicked, this, &OperationLogDialog::onPrevClicked);
    connect(m_nextBtn, &QPushButton::clicked, this, &OperationLogDialog::onNextClicked);
    connect(m_listWidget, &QListWidget::itemClicked, this, &OperationLogDialog::onLogSelected);
}

void OperationLogDialog::onSearchClicked() {
    loadPage(1);
}

void OperationLogDialog::onPrevClicked() {
    if (m_currentPage > 1) {
        loadPage(m_currentPage - 1);
    }
}

void OperationLogDialog::onNextClicked() {
    int totalPages = (m_totalItems + m_itemsPerPage - 1) / m_itemsPerPage;
    if (m_currentPage < totalPages) {
        loadPage(m_currentPage + 1);
    }
}

void OperationLogDialog::loadPage(int page) {
    m_currentPage = page;
    m_listWidget->clear();
    
    QString start = m_startDateEdit->date().toString("yyyy-MM-dd");
    QString end = m_endDateEdit->date().toString("yyyy-MM-dd");
    QString op = m_operatorEdit->text();
    
    m_totalItems = m_dataManager->getTotalCount(start, end, op);
    int totalPages = qMax(1, (m_totalItems + m_itemsPerPage - 1) / m_itemsPerPage);
    
    m_pageLabel->setText(QString("%1 / %2").arg(m_currentPage).arg(totalPages));
    m_prevBtn->setEnabled(m_currentPage > 1);
    m_nextBtn->setEnabled(m_currentPage < totalPages);
    
    int offset = (m_currentPage - 1) * m_itemsPerPage;
    QList<SysOperationLog> logs = m_dataManager->fetchLogs(m_itemsPerPage, offset, start, end, op);
    
    for (const auto &log : logs) {
        QListWidgetItem *item = new QListWidgetItem(m_listWidget);
        QString displayText = QString("%1\n%2 - %3 [%4]").arg(log.timestamp, log.operatorName, log.module, log.action);
        item->setText(displayText);
        item->setData(Qt::UserRole, log.details);
    }
}

void OperationLogDialog::onLogSelected(QListWidgetItem *item) {
    if (!item) return;
    QString jsonStr = item->data(Qt::UserRole).toString();
    QString html = renderDiffHtml(jsonStr);
    m_detailBrowser->setHtml(html);
}

QString OperationLogDialog::renderDiffHtml(const QString &jsonStr) {
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        return QString("<p style='color:#475569;'>%1</p>").arg(jsonStr.toHtmlEscaped());
    }
    
    QJsonObject obj = doc.object();
    QString field = obj.value("field").toString();
    QString oldVal = obj.value("old").toString();
    QString newVal = obj.value("new").toString();
    
    return QString(
        "<h3>数据变更对比</h3>"
        "<table width='100%' border='1' cellspacing='0' cellpadding='8' style='border-collapse:collapse; border-color:#e2e8f0;'>"
        "<tr style='background-color:#f1f5f9;'><th>字段名</th><th>修改前 (Old)</th><th>修改后 (New)</th></tr>"
        "<tr>"
        "<td>%1</td>"
        "<td style='background-color:#fee2e2; text-decoration:line-through; color:#991b1b;'>%2</td>"
        "<td style='background-color:#dcfce7; color:#166534;'>%3</td>"
        "</tr>"
        "</table>"
    ).arg(field.toHtmlEscaped(), oldVal.toHtmlEscaped(), newVal.toHtmlEscaped());
}
