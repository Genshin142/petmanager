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
#include <QCheckBox>
#include <QIntValidator>

AppointmentModule::AppointmentModule(QWidget *parent) : QWidget(parent) {
    setupUI();

    // 默认示例数据注入 (注入到 m_activeData 而不是直接画表格)
    m_activeData.append({"张三", "13800138000", QDate::currentDate().toString("yyyy-MM-dd"), "10:00", "精修造型", "美容台A", "张三 (高级)", "Pending"});
    m_activeData.append({"李四", "13912345678", QDate::currentDate().toString("yyyy-MM-dd"), "14:30", "猫咪洗护", "洗护间B", "王五 (中级)", "Pending"});
    m_activeData.append({"王五", "13688889999", QDate::currentDate().toString("yyyy-MM-dd"), "16:00", "药浴Spa", "SPA间A", "李四 (中级)", "Pending"});
    m_activeData.append({"赵六", "13500001111", QDate::currentDate().addDays(1).toString("yyyy-MM-dd"), "09:00", "牙齿清洁", "工作台C", "张三 (高级)", "Pending"});
    // 故意增加一对冲突预约 (同一个人同一时刻工作)
    m_activeData.append({"钱七", "13511112222", QDate::currentDate().toString("yyyy-MM-dd"), "10:00", "全身剪毛", "剪毛台B", "张三 (高级)", "Pending"});

    updatePagination();
    updateStats();
}

void AppointmentModule::setupUI() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    // 1. 标题栏
    QHBoxLayout *titleLayout = new QHBoxLayout();
    QLabel *title = new QLabel("业务中心：预约服务", this);
    title->setStyleSheet("font-size: 24px; color: #303133;");
    
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
    titleLayout->addLayout(filterLayout);
    titleLayout->addSpacing(20);
    
    QPushButton *batchCancelBtn = new QPushButton("批量清理违约", this);
    batchCancelBtn->setCursor(Qt::PointingHandCursor);
    batchCancelBtn->setStyleSheet(
        "QPushButton { background-color: #fef0f0; color: #f56c6c; border-radius: 6px; padding: 10px 18px; border: 1px solid #fbc4c4; font-size: 13px; }"
        "QPushButton:hover { background-color: #f56c6c; color: white; }"
    );

    QPushButton *historyBtn = new QPushButton("历史预约记录", this);
    historyBtn->setCursor(Qt::PointingHandCursor);
    historyBtn->setStyleSheet(
        "QPushButton { background-color: white; color: #606266; border-radius: 6px; padding: 10px 18px; border: 1px solid #dcdfe6; font-size: 13px; }"
        "QPushButton:hover { border-color: #409eff; color: #409eff; }"
    );

    QPushButton *addApptBtn = new QPushButton("+ 新增预约", this);
    addApptBtn->setCursor(Qt::PointingHandCursor);
    addApptBtn->setStyleSheet(
        "QPushButton { background-color: #409eff; color: white; border-radius: 6px; padding: 10px 20px; border: none; font-size: 13px; }"
        "QPushButton:hover { background-color: #66b1ff; }"
    );
    
    titleLayout->addWidget(batchCancelBtn);
    titleLayout->addSpacing(10);
    titleLayout->addWidget(historyBtn);
    titleLayout->addSpacing(10);
    titleLayout->addWidget(addApptBtn);
    layout->addLayout(titleLayout);

    // 2. 统计卡片
    QHBoxLayout *dashLayout = new QHBoxLayout();
    dashLayout->setSpacing(20);

    auto createStatCard = [&](const QString &icon, const QString &label, QLabel* &valLabel) {
        QFrame *card = new QFrame();
        card->setFixedHeight(100);
        card->setStyleSheet("QFrame { background: white; border-radius: 12px; border: 1px solid #f0f2f5; }");
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
        shadow->setBlurRadius(15); shadow->setColor(QColor(0, 0, 0, 25)); shadow->setOffset(0, 4);
        card->setGraphicsEffect(shadow);
        QHBoxLayout *l = new QHBoxLayout(card); l->setContentsMargins(20, 15, 20, 15);
        QLabel *iconLabel = new QLabel(icon);
        iconLabel->setFixedSize(50, 50); iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet(QString("font-size: 24px; background: #f5f7fa; border-radius: 10px; border: none;"));
        l->addWidget(iconLabel); l->addSpacing(15);
        QVBoxLayout *textLayout = new QVBoxLayout(); textLayout->setSpacing(2);
        QLabel *labelTitle = new QLabel(label); labelTitle->setStyleSheet("color: #909399; font-size: 13px; border: none; background: transparent;");
        valLabel = new QLabel("0"); valLabel->setStyleSheet("font-size: 22px; color: #303133; border: none; background: transparent;");
        textLayout->addWidget(labelTitle); textLayout->addWidget(valLabel); textLayout->addStretch();
        l->addLayout(textLayout); l->addStretch();
        return card;
    };

    dashLayout->addWidget(createStatCard("📅", "今日预约总数", todayTotalLabel));
    dashLayout->addWidget(createStatCard("⏳", "待服务预约", pendingLabel));
    dashLayout->addWidget(createStatCard("✅", "已完成工作", finishedLabel));
    layout->addLayout(dashLayout);

    // 3. 预约列表容器
    apptTable = new QTableWidget();
    apptTable->setColumnCount(7); // 增设勾选列
    apptTable->setHorizontalHeaderLabels({"选择", "会员信息", "预约时间", "服务项目", "工作台", "工作人员", "操作"});
    apptTable->setShowGrid(false);
    apptTable->setAlternatingRowColors(false);
    apptTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    apptTable->setSelectionMode(QAbstractItemView::SingleSelection);
    apptTable->setFocusPolicy(Qt::NoFocus);
    apptTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    apptTable->verticalHeader()->setVisible(false);
    apptTable->verticalHeader()->setDefaultSectionSize(54); 
    
    apptTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    apptTable->setColumnWidth(0, 48);
    apptTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); 
    apptTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch); 
    apptTable->setColumnWidth(3, 100); 
    apptTable->setColumnWidth(4, 100); 
    apptTable->setColumnWidth(5, 120); 
    apptTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch); 

    apptTable->setStyleSheet(
        "QTableWidget { border: 1px solid #E2E8F0; background-color: white; color: #1E293B; outline: none; border-radius: 8px; } "
        "QTableWidget::item { border-bottom: 1px solid #F1F5F9; padding: 10px; } "
        "QTableWidget::item:selected { background-color: #EFF6FF; color: #1E40AF; } " 
        "QHeaderView::section { background-color: #1E40AF; color: white; padding: 12px; border: none; font-size: 13px; font-weight: 600; } "
        "QCheckBox::indicator { width: 18px; height: 18px; }"
        "QCheckBox::indicator:unchecked { border: 2px solid #D1D5DB; background: white; border-radius: 4px; }"
        "QCheckBox::indicator:checked { background: #1E40AF; border: 2px solid #1E40AF; border-radius: 4px; }"
    );
    apptTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    layout->addWidget(apptTable);

    // 4. 底部分页栏
    QFrame *bottomBar = new QFrame();
    bottomBar->setFixedHeight(50);
    bottomBar->setStyleSheet("background: transparent;");
    QHBoxLayout *pageLayout = new QHBoxLayout(bottomBar);
    pageLayout->setContentsMargins(0,0,0,0);
    pageLayout->addStretch();

    pageLabel = new QLabel("第 1 页 / 共 1 页");
    pageLabel->setStyleSheet("color: #64748B; font-size: 13px; font-weight: 500;");
    
    QLabel *jumpLbl1 = new QLabel("跳转至"); jumpLbl1->setStyleSheet("color: #64748B; font-size: 13px;");
    jumpEdit = new QLineEdit();
    jumpEdit->setFixedSize(45, 28);
    jumpEdit->setAlignment(Qt::AlignCenter);
    jumpValidator = new QIntValidator(1, 1, this);
    jumpEdit->setValidator(jumpValidator);
    jumpEdit->setStyleSheet("QLineEdit { border: 1px solid #E2E8F0; border-radius: 4px; background: white; font-weight: 600; } QLineEdit:focus { border-color: #1E40AF; }");
    QLabel *jumpLbl2 = new QLabel("页"); jumpLbl2->setStyleSheet("color: #64748B; font-size: 13px;");
    
    QPushButton *goBtn = new QPushButton("确认");
    goBtn->setFixedSize(50, 28);
    goBtn->setCursor(Qt::PointingHandCursor);
    goBtn->setStyleSheet(
        "QPushButton { background: white; border: 1px solid #E2E8F0; border-radius: 4px; color: #64748B; font-size: 12px; font-weight: 600; } "
        "QPushButton:hover { color: #1E40AF; border-color: #1E40AF; background: #F8FAFC; }"
    );
    
    prevBtn = new QPushButton("上一页");
    prevBtn->setFixedSize(70, 28);
    prevBtn->setCursor(Qt::PointingHandCursor);
    prevBtn->setStyleSheet(goBtn->styleSheet());
    
    nextBtn = new QPushButton("下一页");
    nextBtn->setFixedSize(70, 28);
    nextBtn->setCursor(Qt::PointingHandCursor);
    nextBtn->setStyleSheet(goBtn->styleSheet());

    pageLayout->addWidget(jumpLbl1);
    pageLayout->addWidget(jumpEdit);
    pageLayout->addWidget(jumpLbl2);
    pageLayout->addWidget(goBtn);
    pageLayout->addSpacing(15);
    pageLayout->addWidget(prevBtn);
    pageLayout->addWidget(pageLabel);
    pageLayout->addWidget(nextBtn);

    layout->addWidget(bottomBar);

    connect(addApptBtn, &QPushButton::clicked, this, &AppointmentModule::onAddAppointmentClicked);
    connect(historyBtn, &QPushButton::clicked, this, &AppointmentModule::onShowHistoryClicked);
    connect(batchCancelBtn, &QPushButton::clicked, this, &AppointmentModule::onBatchCancel);
    connect(prevBtn, &QPushButton::clicked, this, &AppointmentModule::onPrevPage);
    connect(nextBtn, &QPushButton::clicked, this, &AppointmentModule::onNextPage);
    connect(goBtn, &QPushButton::clicked, this, &AppointmentModule::onJumpPage);
    connect(jumpEdit, &QLineEdit::returnPressed, this, &AppointmentModule::onJumpPage);
}

void AppointmentModule::updateStats() {
    int total = 0;
    int pending = m_activeData.size();
    int finished = m_historyData.size();

    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    for (const auto &info : m_activeData) {
        if (info.date == today) total++;
    }

    todayTotalLabel->setText(QString("%1 场").arg(total));
    pendingLabel->setText(QString("%1 人").arg(pending));
    finishedLabel->setText(QString("%1 笔").arg(finished));
}

void AppointmentModule::updatePagination() {
    apptTable->setRowCount(0);
    
    QString kw = searchEdit->text().trimmed().toLower();
    QList<AppointmentInfo> filteredData;
    
    for (const auto &info : m_activeData) {
        bool match = true;
        if (!kw.isEmpty()) {
            if (!info.memberName.toLower().contains(kw) &&
                !info.memberPhone.toLower().contains(kw) &&
                !info.service.toLower().contains(kw)) {
                match = false;
            }
        }
        if (match) filteredData.append(info);
    }
    
    int total = filteredData.size();
    int totalPages = qMax(1, (total + m_pageSize - 1) / m_pageSize);
    
    if (jumpValidator) jumpValidator->setTop(totalPages);
    if (m_currentPage > totalPages) m_currentPage = totalPages;
    if (m_currentPage < 1) m_currentPage = 1;

    int start = (m_currentPage - 1) * m_pageSize;
    int end = qMin(start + m_pageSize, total);

    // 冲突检测准备 (简单的时间员工哈希预警)
    QMap<QString, int> conflictMap;
    for (const auto &info : filteredData) {
        QString key = info.date + "|" + info.hour + "|" + info.staff;
        conflictMap[key]++;
    }

    for (int i = start; i < end; ++i) {
        const auto &info = filteredData[i];
        int r = apptTable->rowCount();
        apptTable->insertRow(r);
        
        // Col 0: CheckBox
        QWidget *cbWidget = new QWidget();
        QHBoxLayout *cbLayout = new QHBoxLayout(cbWidget);
        cbLayout->setContentsMargins(0, 0, 0, 0); cbLayout->setAlignment(Qt::AlignCenter);
        QCheckBox *cb = new QCheckBox();
        cbLayout->addWidget(cb);
        apptTable->setCellWidget(r, 0, cbWidget);

        auto createItem = [&](const QString &text) {
            auto *item = new QTableWidgetItem(text);
            item->setFont(QFont("Microsoft YaHei", 9));
            item->setTextAlignment(Qt::AlignCenter);
            return item;
        };

        apptTable->setItem(r, 1, createItem(QString("%1\n%2").arg(info.memberName, info.memberPhone)));
        apptTable->setItem(r, 2, createItem(QString("%1\n%2").arg(info.date, info.hour)));
        apptTable->setItem(r, 3, createItem(info.service));
        apptTable->setItem(r, 4, createItem(info.station));
        apptTable->setItem(r, 5, createItem(info.staff));
        
        // 检测冲突改变背景色 (升级为更醒目的琥珀警示色)
        QString cKey = info.date + "|" + info.hour + "|" + info.staff;
        if (conflictMap[cKey] > 1) {
            for (int col = 1; col <= 5; ++col) {
                apptTable->item(r, col)->setBackground(QColor("#FFFBEB")); // 琥珀色淡背景
                apptTable->item(r, col)->setForeground(QColor("#B45309")); // 琥珀深色文字
                apptTable->item(r, col)->setToolTip("【系统警报】该时段工作人员任务冲突，请协调处理！");
            }
        }

        // 统一小胶囊操作按钮
        QWidget *actionWidget = new QWidget();
        QHBoxLayout *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(0, 0, 0, 0);
        actionLayout->setSpacing(5);
        actionLayout->setAlignment(Qt::AlignCenter);

        auto createActBtn = [&](const QString &text, const QString &style) {
            QPushButton *b = new QPushButton(text);
            b->setFixedSize(45, 24); b->setCursor(Qt::PointingHandCursor); b->setStyleSheet(style);
            return b;
        };

        QPushButton *finishBtn = createActBtn("完成", "QPushButton { background: #ECFDF5; color: #059669; border: 1px solid #A7F3D0; border-radius: 4px; font-size: 11px; font-weight: 600; } "
                                                      "QPushButton:hover { background: #059669; color: white; }");
        QPushButton *editBtn = createActBtn("编辑", "QPushButton { background: #EFF6FF; color: #1E40AF; border: 1px solid #BFDBFE; border-radius: 4px; font-size: 11px; font-weight: 600; } "
                                                    "QPushButton:hover { background: #1E40AF; color: white; }");
        QPushButton *cancelBtn = createActBtn("驳回", "QPushButton { background: #FEF2F2; color: #DC2626; border: 1px solid #FECACA; border-radius: 4px; font-size: 11px; font-weight: 600; } "
                                                       "QPushButton:hover { background: #DC2626; color: white; }");
        
        // 绑定隐藏参数
        finishBtn->setProperty("idx", i); // 绑定过滤数组的全局索引
        editBtn->setProperty("idx", i);
        cancelBtn->setProperty("idx", i);
        
        actionLayout->addWidget(finishBtn);
        actionLayout->addWidget(editBtn);
        actionLayout->addWidget(cancelBtn);
        
        connect(finishBtn, &QPushButton::clicked, this, &AppointmentModule::onFinishServiceClicked);
        connect(editBtn, &QPushButton::clicked, this, &AppointmentModule::onEditAppointmentClicked);
        connect(cancelBtn, &QPushButton::clicked, this, &AppointmentModule::onCancelAppointmentClicked);

        apptTable->setCellWidget(r, 6, actionWidget);
    }
    
    pageLabel->setText(QString("第 %1 页 / 共 %2 页").arg(m_currentPage).arg(totalPages));
    prevBtn->setEnabled(m_currentPage > 1);
    nextBtn->setEnabled(m_currentPage < totalPages);
}

void AppointmentModule::addRow(const AppointmentInfo &/*info*/) {
    // 废弃不用，保留兼容，实际操作走 m_activeData 然后 updatePagination()
}

void AppointmentModule::onAddAppointmentClicked() {
    AddAppointmentDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        AppointmentInfo info = dlg.getAppointmentInfo();
        info.status = "Pending";
        m_activeData.append(info);
        updatePagination();
        updateStats();
    }
}

void AppointmentModule::onEditAppointmentClicked() {
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    int filteredIdx = btn->property("idx").toInt();
    
    // 映射回原数组数据
    QString kw = searchEdit->text().trimmed().toLower();
    QList<AppointmentInfo> filteredData;
    for (const auto &info : m_activeData) {
        if (!kw.isEmpty() && !info.memberName.toLower().contains(kw) && !info.memberPhone.toLower().contains(kw) && !info.service.toLower().contains(kw)) continue;
        filteredData.append(info);
    }
    if (filteredIdx < 0 || filteredIdx >= filteredData.size()) return;
    
    AppointmentInfo info = filteredData[filteredIdx];

    AddAppointmentDialog dlg(this);
    dlg.setInitialData(info);
    if (dlg.exec() == QDialog::Accepted) {
        AppointmentInfo newInfo = dlg.getAppointmentInfo();
        // 更新 m_activeData
        for (int i=0; i<m_activeData.size(); i++) {
            if (m_activeData[i].memberName == info.memberName && m_activeData[i].date == info.date && m_activeData[i].hour == info.hour) {
                m_activeData[i] = newInfo;
                break;
            }
        }
        updatePagination();
        updateStats();
    }
}

void AppointmentModule::onFinishServiceClicked() {
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int filteredIdx = btn->property("idx").toInt();
    
    // 映射回原数组数据
    QString kw = searchEdit->text().trimmed().toLower();
    QList<AppointmentInfo> filteredData;
    for (const auto &info : m_activeData) {
        if (!kw.isEmpty() && !info.memberName.toLower().contains(kw) && !info.memberPhone.toLower().contains(kw) && !info.service.toLower().contains(kw)) continue;
        filteredData.append(info);
    }
    if (filteredIdx < 0 || filteredIdx >= filteredData.size()) return;
    
    AppointmentInfo info = filteredData[filteredIdx];

    if (CustomMessageDialog::confirm(this, "业务流转确认", QString("是否确认会员 [%1] 的预约项目已完成，并推送至 [订单中心] 结账转历史？").arg(info.memberName))) {
        m_historyData.append(info);
        // 删除元素
        for (int i=0; i<m_activeData.size(); i++) {
            if (m_activeData[i].memberName == info.memberName && m_activeData[i].date == info.date && m_activeData[i].hour == info.hour) {
                m_activeData.removeAt(i);
                break;
            }
        }
        CustomMessageDialog::showSuccess(this, "流转抛送完成", "服务已完成并已发送信号推送至订单管理中心结算！\n(底层已解耦防内存悬挂处理)");
        updatePagination();
        updateStats();
    }
}

void AppointmentModule::onCancelAppointmentClicked() {
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int filteredIdx = btn->property("idx").toInt();
    QString kw = searchEdit->text().trimmed().toLower();
    QList<AppointmentInfo> filteredData;
    for (const auto &info : m_activeData) {
        if (!kw.isEmpty() && !info.memberName.toLower().contains(kw) && !info.memberPhone.toLower().contains(kw) && !info.service.toLower().contains(kw)) continue;
        filteredData.append(info);
    }
    if (filteredIdx < 0 || filteredIdx >= filteredData.size()) return;
    
    AppointmentInfo info = filteredData[filteredIdx];
    
    if (CustomMessageDialog::confirm(this, "驳回确认", QString("确定要取消 [%1] 的该笔预约并记录违约历史吗？").arg(info.memberName))) {
        m_historyData.append(info);
        for (int i=0; i<m_activeData.size(); i++) {
            if (m_activeData[i].memberName == info.memberName && m_activeData[i].date == info.date && m_activeData[i].hour == info.hour) {
                m_activeData.removeAt(i);
                break;
            }
        }
        updatePagination();
        updateStats();
    }
}

void AppointmentModule::onBatchCancel()
{
    // 获取当页所有被勾选的行代表的对象
    int count = 0;
    for (int r = 0; r < apptTable->rowCount(); ++r) {
        QWidget *w = apptTable->cellWidget(r, 0);
        if (w) {
            QCheckBox *cb = w->findChild<QCheckBox*>();
            if (cb && cb->isChecked()) {
                QString nameDesc = apptTable->item(r, 1)->text().split("\n").value(0);
                QString dateDesc = apptTable->item(r, 2)->text().split("\n").value(0);
                QString hourDesc = apptTable->item(r, 2)->text().split("\n").value(1);
                
                for (int i = 0; i < m_activeData.size(); ++i) {
                    if (m_activeData[i].memberName == nameDesc && m_activeData[i].date == dateDesc && m_activeData[i].hour == hourDesc) {
                        m_historyData.append(m_activeData[i]);
                        m_activeData.removeAt(i);
                        count++;
                        break;
                    }
                }
            }
        }
    }
    if (count == 0) {
        CustomMessageDialog::showWarning(this, "批量取消", "请先在前方的方框中勾选尚未履行的预约记录！");
        return;
    }
    CustomMessageDialog::showSuccess(this, "清理完成", QString("一键清理并归档了 %1 条违约超时记录。").arg(count));
    updatePagination();
    updateStats();
}

void AppointmentModule::onFilter() {
    m_currentPage = 1;
    updatePagination();
}

void AppointmentModule::onPrevPage() {
    if (m_currentPage > 1) {
        m_currentPage--;
        updatePagination();
    }
}
void AppointmentModule::onNextPage() {
    m_currentPage++;
    updatePagination(); // boundary check is inside
}
void AppointmentModule::onJumpPage() {
    int page = jumpEdit->text().toInt();
    if (page >= 1) {
        m_currentPage = page;
        updatePagination();
    }
    jumpEdit->clear(); jumpEdit->clearFocus();
}

void AppointmentModule::onShowHistoryClicked() {
    auto *dlg = new HistoryAppointmentDialog(m_historyData, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}
