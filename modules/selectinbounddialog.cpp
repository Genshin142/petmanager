#include "selectinbounddialog.h"
#include "productdatamanager.h"
#include "custommessagedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>

SelectInboundDialog::SelectInboundDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("选择待上架入库记录");
    resize(900, 500);
    setupUI();
    loadData();

    connect(ProductDataManager::instance(), &ProductDataManager::shelveResult, this, [this](bool success, const QString &msg){
        if (success) {
            CustomMessageDialog::showSuccess(this, "成功", "商品已成功上架");
            accept();
        } else {
            CustomMessageDialog::showWarning(this, "失败", msg);
        }
    });
}

void SelectInboundDialog::setupUI() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    QLabel *tip = new QLabel("以下是系统中记录但尚未上架到档案库的入库单，请选择一条进行上架：");
    tip->setStyleSheet("color: #64748b; font-size: 13px;");
    layout->addWidget(tip);

    m_table = new QTableWidget();
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({"入库单号", "条形码", "商品名称", "规格", "分类", "数量", "进货价"});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    m_table->setStyleSheet("QTableWidget { border: 1px solid #e2e8f0; border-radius: 8px; background: white; } "
                           "QHeaderView::section { background: #f8fafc; padding: 8px; border: none; border-bottom: 1px solid #e2e8f0; font-weight: bold; }");
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(m_table);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    QPushButton *cancelBtn = new QPushButton("取消");
    cancelBtn->setFixedSize(100, 36);
    cancelBtn->setStyleSheet("QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 6px; }");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    
    m_confirmBtn = new QPushButton("确认上架");
    m_confirmBtn->setFixedSize(120, 36);
    m_confirmBtn->setStyleSheet("QPushButton { background: #3b82f6; color: white; border-radius: 6px; font-weight: bold; } "
                                "QPushButton:hover { background: #2563eb; }");
    connect(m_confirmBtn, &QPushButton::clicked, this, &SelectInboundDialog::onConfirm);
    
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(m_confirmBtn);
    layout->addLayout(btnLayout);
}

void SelectInboundDialog::loadData() {
    m_table->setRowCount(0);
    QList<StockInRecord> records = ProductDataManager::instance()->getAllRecords();
    
    for (const auto &rec : records) {
        if (!rec.isShelved && rec.isActive) {
            int row = m_table->rowCount();
            m_table->insertRow(row);
            
            m_table->setItem(row, 0, new QTableWidgetItem(rec.inboundNo));
            m_table->setItem(row, 1, new QTableWidgetItem(rec.barcode));
            m_table->setItem(row, 2, new QTableWidgetItem(rec.productName));
            m_table->setItem(row, 3, new QTableWidgetItem(rec.spec));
            m_table->setItem(row, 4, new QTableWidgetItem(rec.category));
            m_table->setItem(row, 5, new QTableWidgetItem(QString::number(rec.quantity)));
            m_table->setItem(row, 6, new QTableWidgetItem(QString::number(rec.costPrice, 'f', 2)));
            
            // 绑定 ID
            m_table->item(row, 0)->setData(Qt::UserRole, rec.id);
        }
    }
}

void SelectInboundDialog::onConfirm() {
    int row = m_table->currentRow();
    if (row < 0) {
        CustomMessageDialog::showWarning(this, "提示", "请先选择一条入库记录");
        return;
    }
    
    int inboundId = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    ProductDataManager::instance()->shelveProduct(inboundId);
}

void SelectInboundDialog::onRefresh() {
    loadData();
}
