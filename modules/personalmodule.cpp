#include "personalmodule.h"
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QDateTime>
#include <QMessageBox>
#include "passwordchangedialog.h"

PersonalModule::PersonalModule(UserRole role, const QString &userName, QWidget *parent)
    : QWidget(parent), m_role(role), m_userName(userName) {
    setupUI();
}

void PersonalModule::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(25);

    // 顶部背景装饰区
    QWidget *header = new QWidget();
    header->setFixedHeight(200);
    header->setObjectName("ProfileHeader");
    header->setStyleSheet("QWidget#ProfileHeader { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #3b82f6, stop:1 #2563eb); border-radius: 20px; }");
    
    QHBoxLayout *hl = new QHBoxLayout(header);
    hl->setContentsMargins(40, 0, 40, 0);
    hl->setSpacing(25);

    // 头像
    QLabel *avatar = new QLabel(m_userName.left(1).toUpper());
    avatar->setFixedSize(100, 100);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setStyleSheet("background: rgba(255, 255, 255, 0.2); border: 4px solid white; border-radius: 50px; color: white; font-size: 40px; font-weight: bold;");
    hl->addWidget(avatar);

    // 用户信息
    QVBoxLayout *infoVl = new QVBoxLayout();
    infoVl->setAlignment(Qt::AlignCenter);
    QLabel *nameLabel = new QLabel(m_userName);
    nameLabel->setStyleSheet("color: white; font-size: 28px; font-weight: 800; border: none; background: transparent;");
    QLabel *roleLabel = new QLabel(m_role == ADMIN ? "系统超级管理员" : "门店营业专家");
    roleLabel->setStyleSheet("color: rgba(255, 255, 255, 0.8); font-size: 14px; font-weight: 600; border: none; background: transparent;");
    infoVl->addWidget(nameLabel);
    infoVl->addWidget(roleLabel);
    hl->addLayout(infoVl);
    hl->addStretch();

    // 加入时间
    QLabel *joinDate = new QLabel("加入于 " + QDate::currentDate().toString("yyyy-MM-dd"));
    joinDate->setStyleSheet("color: rgba(255, 255, 255, 0.6); font-size: 13px; border: none; background: transparent;");
    hl->addWidget(joinDate, 0, Qt::AlignBottom | Qt::AlignRight);
    hl->setContentsMargins(40, 40, 40, 20);

    mainLayout->addWidget(header);

    // 滚动区域
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("background: transparent;");
    
    if (m_role == ADMIN) {
        scroll->setWidget(createAdminView());
    } else {
        scroll->setWidget(createStaffView());
    }
    
    mainLayout->addWidget(scroll);
}

QWidget* PersonalModule::createAdminView() {
    QWidget *view = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(view);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(20);

    // 账户与系统安全看板
    QHBoxLayout *cardHl = new QHBoxLayout();
    cardHl->addWidget(createCard("安全健康值", "98 / 100", "密码强度：极高", "#10b981"));
    cardHl->addWidget(createCard("全系统待办", "5 项待办", "包含 2 个库存预警", "#f59e0b"));
    cardHl->addWidget(createCard("最近登录", "09:30 AM", "IP: 192.168.1.105", "#3b82f6"));
    layout->addLayout(cardHl);

    // 设置列表
    QLabel *sectionTitle = new QLabel("管理中心");
    sectionTitle->setStyleSheet("font-size: 18px; font-weight: 800; color: #1e293b; margin-top: 20px; border: none;");
    layout->addWidget(sectionTitle);

    layout->addWidget(createActionRow("安全设置", "修改管理员登录密码及多重验证设置", "修改密码"));
    layout->addWidget(createActionRow("数据备份", "手动触发全量数据库备份并导出离线镜像", "立即备份"));
    layout->addWidget(createActionRow("系统设置", "配置门店基础信息、打印模版及积分规则", "进入设置"));
    layout->addWidget(createActionRow("操作日志", "查看所有店员的操作流水，追踪异常操作", "查看日志"));

    layout->addStretch();
    return view;
}

QWidget* PersonalModule::createStaffView() {
    QWidget *view = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(view);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(20);

    // 我的绩效
    QHBoxLayout *cardHl = new QHBoxLayout();
    cardHl->addWidget(createCard("我的本月营收", "¥12,450", "完成度 85%", "#3b82f6"));
    cardHl->addWidget(createCard("累计成单量", "156", "好评率 98.5%", "#10b981"));
    cardHl->addWidget(createCard("服务星级", "⭐⭐⭐⭐⭐", "全店排名 2nd", "#f59e0b"));
    layout->addLayout(cardHl);

    // 快捷入口
    QLabel *sectionTitle = new QLabel("店员自助服务");
    sectionTitle->setStyleSheet("font-size: 18px; font-weight: 800; color: #1e293b; margin-top: 20px; border: none;");
    layout->addWidget(sectionTitle);

    layout->addWidget(createActionRow("修改密码", "为了您的账号安全，建议每3个月更换一次密码", "立即修改"));
    layout->addWidget(createActionRow("我的排班", "查看下周的轮岗计划及请假申请状态", "查看排班"));
    layout->addWidget(createActionRow("荣誉勋章", "查看您在店铺获得的年度最佳等荣誉奖励", "查看奖牌"));

    layout->addStretch();
    return view;
}

QWidget* PersonalModule::createCard(const QString &title, const QString &value, const QString &subValue, const QString &color) {
    QFrame *card = new QFrame();
    card->setObjectName("KpiCard");
    card->setFixedHeight(140);
    card->setStyleSheet(QString("QFrame#KpiCard { background: white; border-radius: 15px; border: 1px solid #e2e8f0; }"));
    
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(15);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 30));
    card->setGraphicsEffect(shadow);

    QVBoxLayout *vl = new QVBoxLayout(card);
    vl->setContentsMargins(20, 20, 20, 20);
    
    QLabel *t = new QLabel(title);
    t->setStyleSheet("color: #64748b; font-size: 13px; font-weight: bold; border: none; background: transparent;");
    
    QLabel *v = new QLabel(value);
    v->setStyleSheet(QString("color: %1; font-size: 24px; font-weight: 800; border: none; background: transparent;").arg(color));
    
    QLabel *s = new QLabel(subValue);
    s->setStyleSheet("color: #94a3b8; font-size: 11px; border: none; background: transparent;");

    vl->addWidget(t);
    vl->addWidget(v);
    vl->addWidget(s);
    
    return card;
}

QWidget* PersonalModule::createActionRow(const QString &label, const QString &desc, const QString &btnText) {
    QWidget *row = new QWidget();
    row->setObjectName("ActionRow");
    row->setFixedHeight(90); // 略微增加行高
    row->setStyleSheet("QWidget#ActionRow { background: white; border-radius: 12px; border: 1px solid #f1f5f9; } QWidget#ActionRow:hover { background: #f8fafc; border: 1px solid #e2e8f0; }");
    
    QHBoxLayout *hl = new QHBoxLayout(row);
    hl->setContentsMargins(30, 0, 30, 0); // 增加左右内边距
    hl->setSpacing(15);

    QVBoxLayout *vl = new QVBoxLayout();
    vl->setAlignment(Qt::AlignCenter);
    QLabel *l = new QLabel(label);
    l->setStyleSheet("font-size: 16px; font-weight: bold; color: #1e293b; border: none; background: transparent;");
    QLabel *d = new QLabel(desc);
    d->setStyleSheet("font-size: 13px; color: #64748b; border: none; background: transparent;");
    vl->addWidget(l);
    vl->addWidget(d);
    hl->addLayout(vl);
    hl->addStretch();

    QPushButton *btn = new QPushButton(btnText);
    btn->setFixedSize(120, 44); // 略微增加宽度
    btn->setCursor(Qt::PointingHandCursor);
    // 强制覆盖全局 QMainWindow 的 QPushButton 样式（之前全局设置了 text-align: left 和 padding）
    btn->setStyleSheet("QPushButton { background: #f1f5f9; color: #334155; font-weight: 800; font-size: 14px; border-radius: 8px; border: none; text-align: center; padding: 0px; } "
                       "QPushButton:hover { background: #3b82f6; color: white; }");
    hl->addWidget(btn);

    connect(btn, &QPushButton::clicked, this, [this, label]() {
        onActionClicked(label);
    });

    return row;
}

void PersonalModule::onActionClicked(const QString &actionName) {
    if (actionName == "安全设置" || actionName == "修改密码") {
        PasswordChangeDialog dialog(this);
        dialog.exec();
        return;
    }

    // 模拟功能实现
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(actionName);
    msgBox.setStyleSheet("QMessageBox { background-color: #ffffff; }"
                         "QLabel { color: #334155; font-size: 14px; min-width: 250px; }");

    if (actionName == "数据备份") {
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("正在执行全量数据库冷备份...\n\n进度：[||||||||||||||||||||] 100%\n备份文件已加密存储至本地 D:/PetManager_Backup/ 目录下。");
    } else if (actionName == "系统设置") {
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("即将进入系统高级参数配置页。\n\n警告：修改底层参数可能影响门店营业逻辑，请谨慎操作！");
    } else if (actionName == "操作日志") {
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("日志审计中心已连接。\n\n已成功拉取近 30 天内共 1,284 条员工行为操作流水，即将打开详情列表...");
    } else if (actionName == "我的排班") {
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("您本周的排班信息如下：\n\n• 周一至周三: 早班 (09:00 - 18:00)\n• 周四至周五: 晚班 (13:00 - 22:00)\n• 周末: 休息");
    } else if (actionName == "荣誉勋章") {
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("您的荣誉档案库：\n\n🏅 2026年4月 最佳销售冠军\n⭐ 五星级服务口碑奖章\n\n继续保持，下个目标就在眼前！");
    } else {
        msgBox.setText(actionName + " 模块正在加紧研发中...");
    }
    
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.button(QMessageBox::Ok)->setStyleSheet("QPushButton { background-color: #3b82f6; color: white; font-weight: bold; padding: 6px 20px; border-radius: 4px; border: none; }"
                                                  "QPushButton:hover { background-color: #2563eb; }");
    msgBox.exec();
}
