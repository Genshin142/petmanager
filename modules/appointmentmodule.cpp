#include "appointmentmodule.h"
#include "addappointmentdialog.h"
#include "historyappointmentdialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QHBoxLayout>
#include "custommessagedialog.h"
#include <QGraphicsDropShadowEffect>
#include <QDate>

AppointmentModule::AppointmentModule(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    // 1. 标题栏
    QHBoxLayout *titleLayout = new QHBoxLayout();
    QLabel *title = new QLabel("业务中心：预约服务", this);
    title->setStyleSheet("font-size: 24px; font-weight: bold; color: #303133;");
    
    // Filter/Search bar and buttons
    QHBoxLayout *filterLayout = new QHBoxLayout();
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(" 搜索宠物名称、主号、服务类型...");
    searchEdit->setFixedWidth(280);
    searchEdit->setFixedHeight(32);
    searchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 16px; padding: 0 15px; font-size: 13px; background: white; } "
        "QLineEdit:focus { border-color: #409eff; outline: none; }"
    );
    connect(searchEdit, &QLineEdit::textChanged, this, &AppointmentModule::onFilter);
    filterLayout->addWidget(searchEdit);
    
    titleLayout->addWidget(title);
    titleLayout->addStretch();
    titleLayout->addLayout(filterLayout); // Add the filterLayout to the titleLayout
    titleLayout->addSpacing(20); // Add some spacing between filter and action buttons
    
    QPushButton *historyBtn = new QPushButton("历史预约记录", this);
    historyBtn->setCursor(Qt::PointingHandCursor);
    historyBtn->setStyleSheet(
        "QPushButton { "
        "   background-color: white; color: #606266; border-radius: 6px; "
        "   padding: 10px 18px; font-weight: bold; border: 1px solid #dcdfe6; font-size: 14px; "
        "}"
        "QPushButton:hover { border-color: #409eff; color: #409eff; }"
    );

    QPushButton *addApptBtn = new QPushButton("+ 新增预约", this);
    addApptBtn->setCursor(Qt::PointingHandCursor);
    addApptBtn->setStyleSheet(
        "QPushButton { "
        "   background-color: #409eff; color: white; border-radius: 6px; "
        "   padding: 10px 20px; font-weight: bold; border: none; font-size: 14px; "
        "}"
        "QPushButton:hover { background-color: #66b1ff; }"
    );
    
    titleLayout->addWidget(historyBtn);
    titleLayout->addSpacing(10);
    titleLayout->addWidget(addApptBtn);
    layout->addLayout(titleLayout);

    // 2. 统计卡片 (新版视觉提升)
    QHBoxLayout *dashLayout = new QHBoxLayout();
    dashLayout->setSpacing(20);

    auto createStatCard = [&](const QString &icon, const QString &label, QLabel* &valLabel) {
        QFrame *card = new QFrame();
        card->setFixedHeight(100);
        card->setStyleSheet("QFrame { background: white; border-radius: 12px; border: 1px solid #f0f2f5; }");
        
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
        shadow->setBlurRadius(15);
        shadow->setColor(QColor(0, 0, 0, 25));
        shadow->setOffset(0, 4);
        card->setGraphicsEffect(shadow);

        QHBoxLayout *l = new QHBoxLayout(card);
        l->setContentsMargins(20, 15, 20, 15);

        QLabel *iconLabel = new QLabel(icon);
        iconLabel->setFixedSize(50, 50);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet(QString("font-size: 24px; background: #f5f7fa; border-radius: 10px; border: none;"));
        l->addWidget(iconLabel);
        l->addSpacing(15);

        QVBoxLayout *textLayout = new QVBoxLayout();
        textLayout->setSpacing(2);
        QLabel *labelTitle = new QLabel(label);
        labelTitle->setStyleSheet("color: #909399; font-size: 13px; border: none; background: transparent;");
        valLabel = new QLabel("0");
        valLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: #303133; border: none; background: transparent;");
        
        textLayout->addWidget(labelTitle);
        textLayout->addWidget(valLabel);
        textLayout->addStretch();
        l->addLayout(textLayout);
        l->addStretch();
        return card;
    };

    dashLayout->addWidget(createStatCard("📅", "今日预约总数", todayTotalLabel));
    dashLayout->addWidget(createStatCard("⏳", "待服务预约", pendingLabel));
    dashLayout->addWidget(createStatCard("✅", "已完成工作", finishedLabel));
    layout->addLayout(dashLayout);

    // 2. 预约列表容器
    apptTable = new QTableWidget(0, 6);
    apptTable->setHorizontalHeaderLabels({"会员信息", "预约时间", "服务项目", "工作台", "工作人员", "管理操作"});
    
    apptTable->setShowGrid(false);
    apptTable->setAlternatingRowColors(true);
    apptTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    apptTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    apptTable->verticalHeader()->setVisible(false);
    apptTable->verticalHeader()->setDefaultSectionSize(54); 
    
    apptTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    apptTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch); 
    apptTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); 
    
    apptTable->setColumnWidth(2, 100); 
    apptTable->setColumnWidth(3, 100); 
    apptTable->setColumnWidth(4, 100); 
    apptTable->setColumnWidth(5, 260); // 增加操作列宽度以容纳三个按钮

    apptTable->setStyleSheet(
        "QTableWidget { "
        "   border: 1px solid #ebeef5; "
        "   background-color: white; "
        "   color: black; "
        "   selection-background-color: #e4e7ed; "
        "   selection-color: black; "
        "} "
        "QTableWidget::item { border-bottom: 1px solid #f0f2f5; } "
        "QHeaderView::section { background-color: #f5f7fa; padding: 12px; border: none; border-bottom: 1px solid #ebeef5; color: #606266; font-weight: bold; font-size: 13px; } "
        "QHeaderView::section:vertical { "
        "   background-color: #f5f7fa; " 
        "   color: #909399; "
        "   border: none; "
        "   border-right: 1px solid #ebeef5; "
        "   border-bottom: 1px solid #ebeef5; "
        "   text-align: center; "
        "   font-size: 12px; "
        "} "
    );
    
    apptTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    layout->addWidget(apptTable);

    // 默认示例数据 (注入更多数据)
    addRow({"张三", "13800138000", QDate::currentDate().toString("yyyy-MM-dd"), "10:00", "精修造型", "美容台A", "张三 (高级)", "Pending"});
    addRow({"李四", "13912345678", QDate::currentDate().toString("yyyy-MM-dd"), "14:30", "猫咪洗护", "洗护间B", "王五 (中级)", "Pending"});
    addRow({"王五", "13688889999", QDate::currentDate().toString("yyyy-MM-dd"), "16:00", "药浴Spa", "SPA间A", "李四 (中级)", "Pending"});
    addRow({"赵六", "13500001111", QDate::currentDate().addDays(1).toString("yyyy-MM-dd"), "09:00", "牙齿清洁", "工作台C", "张三 (高级)", "Pending"});

    updateStats();

    connect(addApptBtn, &QPushButton::clicked, this, &AppointmentModule::onAddAppointmentClicked);
    connect(historyBtn, &QPushButton::clicked, this, &AppointmentModule::onShowHistoryClicked);
}

void AppointmentModule::updateStats() {
    int total = 0;
    int pending = 0;
    int finished = m_historyData.size();

    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    for (int i = 0; i < apptTable->rowCount(); ++i) {
        if (apptTable->isRowHidden(i)) continue;
        QString date = apptTable->item(i, 1)->text().split("\n").value(0);
        if (date == today) total++;
        pending++; // 当前表里都是 Pending 状态
    }

    todayTotalLabel->setText(QString("%1 场").arg(total));
    pendingLabel->setText(QString("%1 人").arg(pending));
    finishedLabel->setText(QString("%1 笔").arg(finished));
}

void AppointmentModule::addRow(const AppointmentInfo &info) {
    int r = apptTable->rowCount();
    apptTable->insertRow(r);
    
    auto createItem = [&](const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setFont(QFont("Microsoft YaHei", 9));
        item->setTextAlignment(Qt::AlignCenter);
        return item;
    };

    // 会员信息
    apptTable->setItem(r, 0, createItem(QString("%1\n%2").arg(info.memberName, info.memberPhone)));
    // 预约时间显示包含日期和具体小时
    apptTable->setItem(r, 1, createItem(QString("%1\n%2").arg(info.date, info.hour)));
    apptTable->setItem(r, 2, createItem(info.service));
    apptTable->setItem(r, 3, createItem(info.station));
    apptTable->setItem(r, 4, createItem(info.staff));

    // 操作按钮
    QWidget *actionWidget = new QWidget();
    actionWidget->setStyleSheet("background: transparent;"); 
    QHBoxLayout *actionLayout = new QHBoxLayout(actionWidget);
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(8);
    actionLayout->setAlignment(Qt::AlignCenter);

    auto createBtn = [&](const QString &text, const QString &bgColor, const QString &textColor, const QString &borderColor) {
        QPushButton *btn = new QPushButton(text);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedHeight(30);
        btn->setStyleSheet(QString(
            "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: 4px; font-size: 11px; padding: 0 10px; }"
            "QPushButton:hover { opacity: 0.8; }"
        ).arg(bgColor, textColor, borderColor));
        return btn;
    };

    QPushButton *finishBtn = createBtn("完成服务", "#f0f9eb", "#67c23a", "#c2e7b0");
    QPushButton *editBtn = createBtn("编辑", "#ecf5ff", "#409eff", "#b3d8ff");
    QPushButton *cancelBtn = createBtn("取消", "#fef0f0", "#f56c6c", "#fbc4c4");

    actionLayout->addWidget(finishBtn);
    actionLayout->addWidget(editBtn);
    actionLayout->addWidget(cancelBtn);
    
    connect(finishBtn, &QPushButton::clicked, this, &AppointmentModule::onFinishServiceClicked);
    connect(editBtn, &QPushButton::clicked, this, &AppointmentModule::onEditAppointmentClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &AppointmentModule::onCancelAppointmentClicked);

    apptTable->setCellWidget(r, 5, actionWidget);
}

void AppointmentModule::onAddAppointmentClicked() {
    AddAppointmentDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        AppointmentInfo info = dlg.getAppointmentInfo();
        info.status = "Pending";
        addRow(info);
        updateStats();
    }
}

void AppointmentModule::onEditAppointmentClicked() {
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    int row = -1;
    for (int i = 0; i < apptTable->rowCount(); ++i) {
        QWidget *w = apptTable->cellWidget(i, 5);
        if (w && w->findChildren<QPushButton*>().contains(btn)) {
            row = i;
            break;
        }
    }
    if (row == -1) return;

    AppointmentInfo info;
    QStringList memberParts = apptTable->item(row, 0)->text().split("\n");
    info.memberName = memberParts.value(0);
    info.memberPhone = memberParts.value(1);
    
    QStringList timeParts = apptTable->item(row, 1)->text().split("\n");
    info.date = timeParts.value(0);
    info.hour = timeParts.value(1);
    
    info.service = apptTable->item(row, 2)->text();
    info.station = apptTable->item(row, 3)->text();
    info.staff = apptTable->item(row, 4)->text();

    AddAppointmentDialog dlg(this);
    dlg.setInitialData(info);
    if (dlg.exec() == QDialog::Accepted) {
        AppointmentInfo newInfo = dlg.getAppointmentInfo();
        apptTable->item(row, 0)->setText(QString("%1\n%2").arg(newInfo.memberName, newInfo.memberPhone));
        apptTable->item(row, 1)->setText(QString("%1\n%2").arg(newInfo.date, newInfo.hour));
        apptTable->item(row, 2)->setText(newInfo.service);
        apptTable->item(row, 3)->setText(newInfo.station);
        apptTable->item(row, 4)->setText(newInfo.staff);
    }
}

void AppointmentModule::onFinishServiceClicked() {
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int row = -1;
    for (int i = 0; i < apptTable->rowCount(); ++i) {
        QWidget *w = apptTable->cellWidget(i, 5);
        if (w && w->findChildren<QPushButton*>().contains(btn)) {
            row = i;
            break;
        }
    }
    
    if (row != -1) {
        QString name = apptTable->item(row, 0)->text().split("\n").value(0);
        if (CustomMessageDialog::confirm(this, "服务完成确认", QString("确认会员 [%1] 的服务已顺利完成并转入历史记录吗？").arg(name))) {
            AppointmentInfo info;
            QStringList memberParts = apptTable->item(row, 0)->text().split("\n");
            info.memberName = memberParts.value(0);
            info.memberPhone = memberParts.value(1);
            QStringList timeParts = apptTable->item(row, 1)->text().split("\n");
            info.date = timeParts.value(0);
            info.hour = timeParts.value(1);
            info.service = apptTable->item(row, 2)->text();
            info.station = apptTable->item(row, 3)->text();
            info.staff = apptTable->item(row, 4)->text();
            m_historyData.append(info);
            apptTable->removeRow(row);
            updateStats();
        }
    }
}

void AppointmentModule::onCancelAppointmentClicked() {
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int row = -1;
    for (int i = 0; i < apptTable->rowCount(); ++i) {
        QWidget *w = apptTable->cellWidget(i, 5);
        if (w && w->findChildren<QPushButton*>().contains(btn)) {
            row = i;
            break;
        }
    }
    
    if (row != -1) {
        QString name = apptTable->item(row, 0)->text().split("\n").value(0);
        if (CustomMessageDialog::confirm(this, "取消预约确认", QString("确定取消 [%1] 的预约并将其移至历史记录吗？").arg(name))) {
            AppointmentInfo info;
            QStringList memberParts = apptTable->item(row, 0)->text().split("\n");
            info.memberName = memberParts.value(0);
            info.memberPhone = memberParts.value(1);
            QStringList timeParts = apptTable->item(row, 1)->text().split("\n");
            info.date = timeParts.value(0);
            info.hour = timeParts.value(1);
            info.service = apptTable->item(row, 2)->text();
            info.station = apptTable->item(row, 3)->text();
            info.staff = apptTable->item(row, 4)->text();
            m_historyData.append(info);
            apptTable->removeRow(row);
            updateStats();
        }
    }
}

void AppointmentModule::onFilter() {
    QString kw = searchEdit->text().trimmed().toLower();
    
    for (int i = 0; i < apptTable->rowCount(); ++i) {
        bool visible = true;
        if (!kw.isEmpty()) {
            QString member = apptTable->item(i, 0)->text().toLower();
            QString service = apptTable->item(i, 2)->text().toLower();
            if (!member.contains(kw) && !service.contains(kw)) {
                visible = false;
            }
        }
        apptTable->setRowHidden(i, !visible);
    }
    updateStats();
}

void AppointmentModule::onShowHistoryClicked() {
    auto *dlg = new HistoryAppointmentDialog(m_historyData, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}
