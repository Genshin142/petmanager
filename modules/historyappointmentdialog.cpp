#include "historyappointmentdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QPushButton>
#include <QDateTime>
#include <QComboBox>

HistoryAppointmentDialog::HistoryAppointmentDialog(const QList<AppointmentInfo> &history, QWidget *parent) 
    : QDialog(parent), m_historyData(history) {
    setupUI();
    for(const auto &info : m_historyData) addRow(info);
}

void HistoryAppointmentDialog::setupUI() {
    setWindowTitle("历史预约记录");
    setFixedSize(980, 600); // 固定窗口大小
    setStyleSheet("QDialog { background-color: white; }");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(25, 25, 25, 25);
    layout->setSpacing(15);

    // 标题和搜索
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("历史预约档案", this);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #303133;");
    
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("搜索姓名/手机号...");
    searchEdit->setFixedWidth(180); 
    searchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 4px; padding: 6px 12px; background: #f5f7fa; color: #606266; }"
        "QLineEdit:focus { border-color: #409eff; background: white; }"
    );

    QLabel *modeLabel = new QLabel("筛选维度:");
    modeLabel->setStyleSheet("color: #909399; font-size: 13px; margin-left: 10px;");
    
    filterModeCombo = new QComboBox();
    filterModeCombo->addItems({"全部记录", "按日筛选", "按月筛选"});
    filterModeCombo->setFixedWidth(130); 
    
    // 按照您的要求，全面美化所有下拉框的选项（增加悬停高亮、左侧蓝色指示条等）
    QString comboStyle = 
        "QComboBox { "
        "   border: 1px solid #dcdfe6; "
        "   border-radius: 4px; "
        "   padding: 6px 12px; "
        "   background: #f5f7fa; "
        "   color: #606266; "
        "   font-size: 13px; "
        "} "
        "QComboBox:hover { border-color: #c0c4cc; } "
        "QComboBox:focus { border-color: #409eff; background: white; } "
        "QComboBox::drop-down { border: none; width: 24px; } "
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 14px; height: 14px; } "
        "QComboBox QAbstractItemView { "
        "   border: 1px solid #ebeef5; "
        "   border-radius: 4px; "
        "   background-color: white; "
        "   outline: none; "
        "   padding: 4px 0px; "
        "} "
        "QComboBox QAbstractItemView::item { "
        "   height: 35px; "
        "   padding-left: 12px; "
        "   color: #606266; "
        "   background-color: white; "
        "} "
        "QComboBox QAbstractItemView::item:selected { "
        "   background-color: #f0f7ff; "
        "   color: #409eff; "
        "   border-left: 3px solid #409eff; " // 对应您截图中左侧的蓝色条
        "}";
        
    filterModeCombo->setStyleSheet(comboStyle);

    // 年月日组合
    yearCombo = new QComboBox();
    monthCombo = new QComboBox();
    dayCombo = new QComboBox();
    
    QDate cur = QDate::currentDate();
    for(int i = -1; i < 2; ++i) yearCombo->addItem(QString::number(cur.year() + i));
    yearCombo->setCurrentText(QString::number(cur.year()));
    
    yLab = new QLabel("年"); mLab = new QLabel("月"); dLab = new QLabel("日");
    QString labStyle = "color: #909399; font-size: 13px;";
    yLab->setStyleSheet(labStyle); mLab->setStyleSheet(labStyle); dLab->setStyleSheet(labStyle);
    
    yearCombo->setFixedWidth(90); monthCombo->setFixedWidth(75); dayCombo->setFixedWidth(75);
    yearCombo->setStyleSheet(comboStyle); monthCombo->setStyleSheet(comboStyle); dayCombo->setStyleSheet(comboStyle);
    
    // 重要：设置占位策略，隐藏时不改变布局
    auto setStable = [&](QWidget* w) {
        QSizePolicy sp = w->sizePolicy();
        sp.setRetainSizeWhenHidden(true);
        w->setSizePolicy(sp);
    };
    setStable(yearCombo); setStable(yLab);
    setStable(monthCombo); setStable(mLab);
    setStable(dayCombo); setStable(dLab);

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch(); // 在标题之后拉伸
    headerLayout->addWidget(searchEdit);
    headerLayout->addSpacing(25); // 显著增加间距，彻底解决重叠
    headerLayout->addWidget(modeLabel);
    headerLayout->addWidget(filterModeCombo);
    headerLayout->addSpacing(20); // 组件间距
    headerLayout->addWidget(yearCombo);
    headerLayout->addWidget(yLab);
    headerLayout->addSpacing(5);
    headerLayout->addWidget(monthCombo);
    headerLayout->addWidget(mLab);
    headerLayout->addSpacing(5);
    headerLayout->addWidget(dayCombo);
    headerLayout->addWidget(dLab);
    layout->addLayout(headerLayout);
    
    // 初始状态隐藏日期
    yearCombo->setVisible(false); yLab->setVisible(false);
    monthCombo->setVisible(false); mLab->setVisible(false);
    dayCombo->setVisible(false); dLab->setVisible(false);

    // 表格
    historyTable = new QTableWidget(0, 7);
    historyTable->setHorizontalHeaderLabels({"会员信息", "预约时间", "服务项目", "工作台", "工作人员", "状态", "记录时间"});
    
    // 调整列宽缩放逻辑
    QHeaderView *header = historyTable->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Interactive); // 允许手动微调
    header->setSectionResizeMode(0, QHeaderView::Stretch);   // 会员信息拉伸
    header->setSectionResizeMode(1, QHeaderView::Stretch);   // 预约时间拉伸
    header->setSectionResizeMode(6, QHeaderView::Stretch);   // 记录时间拉伸
    
    historyTable->setColumnWidth(2, 100); // 服务项目
    historyTable->setColumnWidth(3, 100); // 工作台
    historyTable->setColumnWidth(4, 100); // 工作人员
    historyTable->setColumnWidth(5, 80);  // 状态

    historyTable->setAlternatingRowColors(true);
    historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    historyTable->verticalHeader()->setVisible(false);
    historyTable->verticalHeader()->setDefaultSectionSize(50); // 增加行高以容纳两行文字
    historyTable->setShowGrid(false);
    
    historyTable->setStyleSheet(
        "QTableWidget { border: 1px solid #ebeef5; background-color: white; color: #606266; }"
        "QHeaderView::section { background-color: #f5f7fa; padding: 10px; border: none; font-weight: bold; color: #909399; }"
    );

    layout->addWidget(historyTable);

    QPushButton *closeBtn = new QPushButton("关闭窗口");
    closeBtn->setFixedSize(120, 36);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(
        "QPushButton { background: #f4f4f5; color: #606266; border-radius: 4px; border: none; "
        "              font: 14px 'Microsoft YaHei'; padding: 0px; text-align: center; }"
        "QPushButton:hover { background: #e9e9eb; }"
    );
    
    QHBoxLayout *footerLayout = new QHBoxLayout();
    footerLayout->addStretch();
    footerLayout->addWidget(closeBtn);
    layout->addLayout(footerLayout);

    connect(searchEdit, &QLineEdit::textChanged, this, &HistoryAppointmentDialog::onSearchChanged);
    connect(filterModeCombo, &QComboBox::currentIndexChanged, [&](int index){
        bool showY = (index != 0);
        bool showM = (index != 0);
        bool showD = (index == 1); // 只有“按日”显示日
        
        yearCombo->setVisible(showY); yLab->setVisible(showY);
        monthCombo->setVisible(showM); mLab->setVisible(showM);
        dayCombo->setVisible(showD); dLab->setVisible(showD);
        updateFilter();
    });
    
    auto triggerUpdate = [&](){ updateFilter(); };
    connect(yearCombo, &QComboBox::currentTextChanged, this, &HistoryAppointmentDialog::validateDate);
    connect(monthCombo, &QComboBox::currentTextChanged, this, &HistoryAppointmentDialog::validateDate);
    connect(dayCombo, &QComboBox::currentTextChanged, triggerUpdate);
    
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    validateDate(); // 初始化下拉列表并触发首次过滤，确保在 historyTable 初始化之后
}

void HistoryAppointmentDialog::validateDate() {
    int selY = yearCombo->currentText().toInt();
    if(selY <= 0) return;

    // 1. 更新月份
    QString oldM = monthCombo->currentText();
    monthCombo->blockSignals(true);
    monthCombo->clear();
    for(int m = 1; m <= 12; ++m) monthCombo->addItem(QString::number(m).rightJustified(2, '0'));
    if (monthCombo->findText(oldM) != -1) monthCombo->setCurrentText(oldM);
    else monthCombo->setCurrentIndex(0);
    monthCombo->blockSignals(false);

    // 2. 更新日期
    int selM = monthCombo->currentText().toInt();
    QString oldD = dayCombo->currentText();
    dayCombo->blockSignals(true);
    dayCombo->clear();
    int maxD = QDate(selY, selM, 1).daysInMonth();
    for(int d = 1; d <= maxD; ++d) dayCombo->addItem(QString::number(d).rightJustified(2, '0'));
    if (dayCombo->findText(oldD) != -1) dayCombo->setCurrentText(oldD);
    else dayCombo->setCurrentIndex(0);
    dayCombo->blockSignals(false);
    
    updateFilter();
}

void HistoryAppointmentDialog::addRow(const AppointmentInfo &info) {
    int r = historyTable->rowCount();
    historyTable->insertRow(r);
    
    auto createItem = [&](const QString &text, const QString &color = "#606266") {
        auto *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        item->setForeground(QBrush(QColor(color)));
        return item;
    };

    historyTable->setItem(r, 0, createItem(QString("%1\n%2").arg(info.memberName, info.memberPhone)));
    historyTable->setItem(r, 1, createItem(QString("%1\n%2").arg(info.date, info.hour)));
    historyTable->setItem(r, 2, createItem(info.service));
    historyTable->setItem(r, 3, createItem(info.station));
    historyTable->setItem(r, 4, createItem(info.staff));
    
    // 状态样式
    QString statusText = (info.status == "Completed") ? "已完成" : "已取消";
    QString statusColor = (info.status == "Completed") ? "#67c23a" : "#f56c6c";
    historyTable->setItem(r, 5, createItem(statusText, statusColor));
    
    historyTable->setItem(r, 6, createItem(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"))); // 模拟记录时间
}

void HistoryAppointmentDialog::onSearchChanged(const QString &) {
    updateFilter();
}

void HistoryAppointmentDialog::onDateChanged(const QDate &) {
    updateFilter();
}

void HistoryAppointmentDialog::updateFilter() {
    QString searchText = searchEdit->text().trimmed();
    int mode = filterModeCombo->currentIndex();
    
    // 构建匹配前缀
    QString filterVal;
    if (mode == 1) { // 按日
        filterVal = QString("%1-%2-%3").arg(yearCombo->currentText(), monthCombo->currentText(), dayCombo->currentText());
    } else if (mode == 2) { // 按月
        filterVal = QString("%1-%2").arg(yearCombo->currentText(), monthCombo->currentText());
    }

    for(int i = 0; i < historyTable->rowCount(); ++i) {
        bool nameMatch = historyTable->item(i, 0)->text().contains(searchText, Qt::CaseInsensitive);
        bool dateMatch = true;
        
        if (mode != 0) { 
            dateMatch = historyTable->item(i, 1)->text().startsWith(filterVal);
        }
        
        historyTable->setRowHidden(i, !(nameMatch && dateMatch));
    }
}
