#include "personalmodule.h"
#include <QStyledItemDelegate>
#include <QPainter>
#include <QPainterPath>
#include <QAbstractItemView>

#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QDateTime>
#include <QMessageBox>
#include "custommessagedialog.h"
#include "passwordchangedialog.h"
#include "backupmanager.h"
#include "backupprogressdialog.h"
#include "systemsettingsdialog.h"
#include "operationlogdialog.h"
#include "scheduledatamanager.h"
#include "staffdatamanager.h"
#include <QThread>
#include <QPainter>
#include <QPainterPath>
#include "../utils/imageutils.h"
#include <QTabWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QNetworkInterface>
#include "productdatamanager.h"
#include "logisticsmanager.h"
#include "petdatamanager.h"


PersonalModule::PersonalModule(UserRole role, const QString &userName, QWidget *parent)
    : QWidget(parent), m_role(role), m_userName(userName) {
    setupUI();

    // 绑定数据管理器的全局变化信号，实现个人中心待办的 100% 实时统计刷新
    connect(PetDataManager::instance(), &PetDataManager::globalDataChanged, this, &PersonalModule::updateTodoCard);
    connect(ProductDataManager::instance(), &ProductDataManager::productDataChanged, this, &PersonalModule::updateTodoCard);
    
    // 初始化时立刻刷新一次数据
    updateTodoCard();
}

void PersonalModule::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(25);

    // 顶部背景装饰区
    QWidget *header = new QWidget();
    header->setFixedHeight(200);
    header->setObjectName("ProfileHeader");
    if (m_role == ADMIN) {
        header->setStyleSheet("QWidget#ProfileHeader { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #3b82f6, stop:1 #2563eb); border-radius: 20px; }");
    } else {
        // 店员使用代表成长与活力的青绿色系
        header->setStyleSheet("QWidget#ProfileHeader { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #10b981, stop:1 #059669); border-radius: 20px; }");
    }
    
    QHBoxLayout *hl = new QHBoxLayout(header);
    hl->setContentsMargins(40, 0, 40, 0);
    hl->setSpacing(25);

    // 头像 - 智能绘制完美的圆角多平台高质量头像图片 (支持当前登录用户的员工头像)
    QLabel *avatar = new QLabel();
    avatar->setFixedSize(100, 100);
    avatar->setCursor(Qt::PointingHandCursor);
    avatar->installEventFilter(this);
    m_avatarLabel = avatar;
    
    // 提取真实的姓名或用户名（去除括号后缀，如 "张震东 (店长)" -> "张震东"）
    QString realName = m_userName;
    int bracketIdx = m_userName.indexOf(" (");
    if (bracketIdx != -1) {
        realName = m_userName.left(bracketIdx).trimmed();
    } else {
        bracketIdx = m_userName.indexOf("(");
        if (bracketIdx != -1) {
            realName = m_userName.left(bracketIdx).trimmed();
        }
    }

    // 1. 查找当前登录用户对应的员工档案以加载其实时头像
    QPixmap originalPixmap;
    auto allStaff = StaffDataManager::instance()->allStaff();
    bool found = false;
    
    // 优先通过真实的姓名或用户名匹配
    for (const auto &s : allStaff) {
        if ((!realName.isEmpty() && (s.name == realName || s.username == realName)) || 
            s.name == m_userName || s.username == m_userName) {
            originalPixmap = StaffDataManager::instance()->getStaffPixmap(s.id);
            found = true;
            break;
        }
    }
    
    // 如果没有匹配到（比如之前登录传的名字为空），则根据角色/职位匹配
    if (!found) {
        QString targetRole = (m_role == ADMIN) ? "店长" : "";
        for (const auto &s : allStaff) {
            if (!targetRole.isEmpty() && s.role == targetRole) {
                originalPixmap = StaffDataManager::instance()->getStaffPixmap(s.id);
                found = true;
                break;
            }
        }
    }

    // 2. 如果未设置自定义头像，则根据角色使用高清扁平插画头像作为默认头像
    if (originalPixmap.isNull()) {
        originalPixmap = QPixmap(m_role == ADMIN ? ":/images/avatar_admin.png" : ":/images/avatar_staff.png");
    }
    m_originalPixmap = originalPixmap;
    
    auto getPremiumCircularAvatar = [](const QPixmap &src, int size, int borderWidth, const QColor &borderColor) -> QPixmap {
        if (src.isNull()) return QPixmap();
        
        QPixmap target(size, size);
        target.fill(Qt::transparent);
        
        QPainter painter(&target);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        
        // 1. 计算图片绘制区域 (缩小以腾出边框空间，防止图片溢出或被遮挡)
        int innerSize = size - borderWidth * 2;
        QPixmap scaled = src.scaled(innerSize, innerSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        
        // 2. 剪裁内部圆形
        QPainterPath path;
        path.addEllipse(borderWidth, borderWidth, innerSize, innerSize);
        painter.save();
        painter.setClipPath(path);
        int x = borderWidth + (innerSize - scaled.width()) / 2;
        int y = borderWidth + (innerSize - scaled.height()) / 2;
        painter.drawPixmap(x, y, scaled);
        painter.restore();
        
        // 3. 绘制完美的圆形外圈 (描边)
        if (borderWidth > 0) {
            QPen pen(borderColor);
            pen.setWidth(borderWidth);
            pen.setCapStyle(Qt::RoundCap);
            pen.setJoinStyle(Qt::RoundJoin);
            painter.setPen(pen);
            double offset = borderWidth / 2.0;
            painter.drawEllipse(QRectF(offset, offset, size - borderWidth, size - borderWidth));
        }
        
        return target;
    };

    if (!originalPixmap.isNull()) {
        QPixmap roundedPixmap = getPremiumCircularAvatar(originalPixmap, 100, 4, Qt::white);
        avatar->setPixmap(roundedPixmap);
    } else {
        // 退回机制：如果图片资源未成功加载，显示字母占位头像
        avatar->setText(realName.isEmpty() ? "P" : realName.left(1).toUpper());
        avatar->setAlignment(Qt::AlignCenter);
        avatar->setStyleSheet("background: rgba(255, 255, 255, 0.2); color: white; font-size: 40px; font-weight: bold; border: 4px solid white; border-radius: 50px;");
    }
    
    hl->addWidget(avatar);

    // 用户信息
    QVBoxLayout *infoVl = new QVBoxLayout();
    infoVl->setAlignment(Qt::AlignCenter);
    QLabel *nameLabel = new QLabel(m_userName);
    nameLabel->setStyleSheet("color: white; font-size: 28px; font-weight: 800; border: none; background: transparent;");
    QLabel *roleLabel = new QLabel(m_role == ADMIN ? "系统超级管理员 (v1.1.8-VARCHAR-Fix)" : "门店营业专家 (v1.1.8-VARCHAR-Fix)");
    roleLabel->setStyleSheet("color: rgba(255, 255, 255, 0.8); font-size: 14px; font-weight: 600; border: none; background: transparent;");
    infoVl->addWidget(nameLabel);
    infoVl->addWidget(roleLabel);
    hl->addLayout(infoVl);
    hl->addStretch();

    // 加入时间
    QLabel *joinDate = new QLabel("版本: v1.1.8-VARCHAR-Fix | 加入于 " + QDate::currentDate().toString("yyyy-MM-dd"));
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

    // 全系统待办 (实时统计待办卡片)
    m_todoCard = static_cast<QFrame*>(createCard("全系统待办", "0 项待办", "暂无待办事项，系统安全运行", "#f59e0b"));
    m_todoCard->setCursor(Qt::PointingHandCursor);
    m_todoCard->installEventFilter(this);
    cardHl->addWidget(m_todoCard);

    // 最近登录 (真实实时记录：记录本次开启客户端的真实时刻与实际本地IP地址)
    static QString s_loginTimeStr = QDateTime::currentDateTime().toString("hh:mm AP");
    static QString s_loginIp;
    if (s_loginIp.isEmpty()) {
        s_loginIp = "127.0.0.1";
        const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
        for (const QHostAddress &address : QNetworkInterface::allAddresses()) {
            if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost) {
                s_loginIp = address.toString();
                break;
            }
        }
    }
    m_loginCard = createCard("最近登录", s_loginTimeStr, "IP: " + s_loginIp, "#3b82f6");
    cardHl->addWidget(m_loginCard);

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

    // --- 本周排班看板 (新增) ---
    QLabel *schedTitle = new QLabel("我的本周排班");
    schedTitle->setStyleSheet("font-size: 18px; font-weight: 800; color: #1e293b; margin-top: 15px; border: none;");
    layout->addWidget(schedTitle);

    QFrame *schedContainer = new QFrame();
    schedContainer->setStyleSheet("background: white; border-radius: 15px; border: 1px solid #e2e8f0;");
    QHBoxLayout *schedLayout = new QHBoxLayout(schedContainer);
    schedLayout->setContentsMargins(15, 15, 15, 15);
    schedLayout->setSpacing(10);

    QStringList weekNames = {"周一", "周二", "周三", "周四", "周五", "周六", "周日"};
    QDate today = QDate::currentDate();
    QDate monday = today.addDays(1 - today.dayOfWeek());

    // 查找当前员工ID
    QString empId = "";
    auto allStaff = StaffDataManager::instance()->allStaff();
    for(const auto& s : allStaff) {
        if(s.name == m_userName) { empId = s.id; break; }
    }

    for (int i = 0; i < 7; ++i) {
        QDate d = monday.addDays(i);
        ScheduleInfo info = ScheduleDataManager::instance()->getSchedule(empId, d);
        
        QWidget *dayBox = new QWidget();
        QVBoxLayout *dv = new QVBoxLayout(dayBox);
        dv->setContentsMargins(0, 0, 0, 0);
        dv->setSpacing(8);

        QLabel *dayName = new QLabel(weekNames[i]);
        dayName->setAlignment(Qt::AlignCenter);
        dayName->setStyleSheet("color: #64748b; font-size: 12px; font-weight: bold; border: none;");
        
        QLabel *shiftTag = new QLabel();
        shiftTag->setFixedSize(75, 45);
        shiftTag->setAlignment(Qt::AlignCenter);
        QString shiftStyle = "border-radius: 8px; font-size: 11px; font-weight: bold; ";
        
        if (info.type == SHIFT_MORNING) {
            shiftTag->setText("☀️ 早班");
            shiftStyle += "background: #fff7ed; color: #f59e0b; border: 1px solid #ffedd5;";
        } else if (info.type == SHIFT_EVENING) {
            shiftTag->setText("🌙 晚班");
            shiftStyle += "background: #f5f3ff; color: #8b5cf6; border: 1px solid #ede9fe;";
        } else {
            shiftTag->setText("🌿 休息");
            shiftStyle += "background: #f8fafc; color: #94a3b8; border: 1px solid #f1f5f9;";
        }
        shiftTag->setStyleSheet(shiftStyle);

        dv->addWidget(dayName);
        dv->addWidget(shiftTag);
        schedLayout->addWidget(dayBox);
    }
    layout->addWidget(schedContainer);

    // 快捷入口
    QLabel *sectionTitle = new QLabel("店员自助服务");
    sectionTitle->setStyleSheet("font-size: 18px; font-weight: 800; color: #1e293b; margin-top: 20px; border: none;");
    layout->addWidget(sectionTitle);

    layout->addWidget(createActionRow("修改密码", "为了您的账号安全，建议每3个月更换一次密码", "立即修改"));
    layout->addWidget(createActionRow("我的排班", "查看下周的轮岗计划及请假申请状态", "查看排班"));
    layout->addWidget(createActionRow("我的提成", "实时查看本月各项服务及销售带来的个人收益", "查看明细"));
    layout->addWidget(createActionRow("我的评价", "查看客户对您最近服务的点评与建议", "查看反馈"));
    layout->addWidget(createActionRow("荣誉勋章", "查看您在店铺获得的年度最佳等荣誉奖励", "我的荣誉"));

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
    v->setObjectName("CardValue");
    v->setStyleSheet(QString("color: %1; font-size: 24px; font-weight: 800; border: none; background: transparent;").arg(color));
    
    QLabel *s = new QLabel(subValue);
    s->setObjectName("CardSubValue");
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

    if (actionName == "数据备份") {
        BackupProgressDialog *dialog = new BackupProgressDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();

        QThread *thread = new QThread();
        BackupManager *worker = new BackupManager();
        worker->moveToThread(thread);

        connect(thread, &QThread::started, worker, &BackupManager::performBackup);
        connect(worker, &BackupManager::progressUpdated, dialog, &BackupProgressDialog::updateProgress);
        connect(worker, &BackupManager::backupCompleted, dialog, &BackupProgressDialog::onBackupComplete);
        connect(worker, &BackupManager::backupFailed, dialog, &BackupProgressDialog::onBackupError);
        
        connect(worker, &BackupManager::finished, thread, &QThread::quit);
        connect(worker, &BackupManager::finished, worker, &QObject::deleteLater);
        connect(thread, &QThread::finished, thread, &QObject::deleteLater);

        thread->start();
        return;
    } else if (actionName == "系统设置") {
        SystemSettingsDialog dialog(this);
        dialog.exec();
        return;
    } else if (actionName == "操作日志") {
        OperationLogDialog dialog(this);
        dialog.exec();
        return;
    }

    // 使用统一的居中弹窗组件
    QString content;
    if (actionName == "我的排班") {
        content = "您本周的排班信息如下：\n\n• 周一至周三: 早班 (09:00 - 18:00)\n• 周四至周五: 晚班 (13:00 - 22:00)\n• 周末: 休息";
    } else if (actionName == "我的提成") {
        content = "本月提成概览：\n\n• 服务提成: ¥3,120 (完成 45 场服务)\n• 商品销售: ¥840 (销售额 ¥4,200)\n• 奖励金: ¥500 (全勤奖励)\n\n合计: ¥4,460\n*数据实时更新，最终以薪资单为准。";
    } else if (actionName == "我的评价") {
        content = "客户评价摘要：\n\n• 综合评分: 4.9 / 5.0\n• 印象标签: '细心负责'、'手法纯熟'\n\n最新留言：\n'张三老师修剪得很细致，我家雪纳瑞非常帅气，下次还会再来！'";
    } else if (actionName == "荣誉勋章") {
        content = "您的荣誉档案库：\n\n2026年4月 最佳销售冠军\n五星级服务口碑奖章\n\n继续保持，下个目标就在眼前！";
    } else {
        content = actionName + " 模块正在加紧研发中...";
    }
    CustomMessageDialog::showSuccess(this, actionName, content);
}

void PersonalModule::showEnlargedAvatar() {
    if (m_originalPixmap.isNull()) return;

    QDialog *dialog = new QDialog(this, Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    dialog->setAttribute(Qt::WA_TranslucentBackground);
    dialog->setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(0, 0, 0, 0);

    // 遮罩背景
    QWidget *mask = new QWidget(dialog);
    mask->setObjectName("mask");
    mask->setStyleSheet("QWidget#mask { background-color: rgba(0, 0, 0, 0.75); }");
    QVBoxLayout *maskLayout = new QVBoxLayout(mask);
    maskLayout->setAlignment(Qt::AlignCenter);

    // 图片框 (大图)
    QLabel *largeLabel = new QLabel();
    
    // 动态计算尺寸，使其恰好占到主窗口尺寸的 80% (等比例缩放)
    QSize windowSize = this->window()->size();
    int maxW = windowSize.width() * 0.8;
    int maxH = windowSize.height() * 0.8;
    
    QPixmap largePix = m_originalPixmap.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    
    largeLabel->setPixmap(largePix);
    largeLabel->setFixedSize(largePix.size());

    // 强烈的阴影效果，体现高级感
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(largeLabel);
    shadow->setBlurRadius(35);
    shadow->setColor(QColor(0, 0, 0, 180));
    shadow->setOffset(0, 12);
    largeLabel->setGraphicsEffect(shadow);

    maskLayout->addWidget(largeLabel);
    layout->addWidget(mask);

    // 保存大图dialog指针，方便eventFilter捕获并关闭
    m_enlargedDialog = dialog;

    // 点击大图或遮罩即可关闭
    mask->installEventFilter(this);
    largeLabel->installEventFilter(this);

    dialog->resize(this->window()->size());
    dialog->show();
}

bool PersonalModule::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_todoCard && event->type() == QEvent::MouseButtonRelease) {
        showTodoDetailsDialog();
        return true;
    }
    if (watched == m_avatarLabel && event->type() == QEvent::MouseButtonRelease) {
        showEnlargedAvatar();
        return true;
    }
    if (m_enlargedDialog && (watched == m_enlargedDialog->findChild<QWidget*>("mask") || watched == m_enlargedDialog->findChild<QLabel*>()) && event->type() == QEvent::MouseButtonRelease) {
        m_enlargedDialog->accept();
        m_enlargedDialog->deleteLater();
        m_enlargedDialog = nullptr;
        return true;
    }
    return QWidget::eventFilter(watched, event);
}


class TodoRowDelegate : public QStyledItemDelegate {
public:
    mutable int m_hoveredRow = -1;
public:
    TodoRowDelegate(QObject *parent) : QStyledItemDelegate(parent) {
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

        // 绘制文本
        painter->setPen(QColor((opt.state & QStyle::State_Selected) ? "#1e40af" : "#303133"));
        QFont font = painter->font();
        font.setWeight(opt.state & QStyle::State_Selected ? QFont::Bold : QFont::Normal);
        font.setPointSize(10);
        painter->setFont(font);
        QRect textRect = opt.rect.adjusted(10, 0, -10, 0);
        painter->drawText(textRect, opt.displayAlignment | Qt::AlignVCenter, opt.text);
        
        painter->restore();
    }
};

TodoDetailsDialog::TodoDetailsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("全系统待办事项详情");
    resize(800, 550);
    setStyleSheet("QDialog { background-color: #f8fafc; }"
                  "QTabWidget::pane { border: 1px solid #e2e8f0; background: white; border-radius: 10px; }"
                  "QTabBar::tab { background: #f1f5f9; color: #64748b; padding: 10px 20px; font-weight: bold; border-top-left-radius: 6px; border-top-right-radius: 6px; }"
                  "QTabBar::tab:selected { background: white; color: #3b82f6; border-bottom: 2px solid #3b82f6; }"
                  "QTableWidget { border: none; background: white; gridline-color: #f1f5f9; }"
                  "QHeaderView::section { background-color: #f8fafc; color: #475569; font-weight: bold; border: none; padding: 8px; }"
                  "QPushButton { background-color: #3b82f6; color: white; font-weight: bold; border-radius: 6px; padding: 6px 12px; border: none; }"
                  "QPushButton:hover { background-color: #2563eb; }");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    QLabel *titleLabel = new QLabel("🔔 全系统实时待办事项列表");
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #0f172a;");
    mainLayout->addWidget(titleLabel);

    QTabWidget *tabs = new QTabWidget(this);
    
    // Tab 1: 库存警报
    QWidget *stockTab = createStockTab();
    tabs->addTab(stockTab, "商品库存警报");

    // Tab 2: 待确认预约
    QWidget *apptTab = createApptTab();
    tabs->addTab(apptTab, "服务预约待确认");

    // Tab 3: 临期车辆调度
    QWidget *logisticsTab = createLogisticsTab();
    tabs->addTab(logisticsTab, "临期车辆调度");

    mainLayout->addWidget(tabs);

    QHBoxLayout *btnHl = new QHBoxLayout();
    btnHl->addStretch();
    QPushButton *closeBtn = new QPushButton("关闭窗口");
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet("QPushButton { background-color: #64748b; color: white; } QPushButton:hover { background-color: #475569; }");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnHl->addWidget(closeBtn);
    mainLayout->addLayout(btnHl);
}

QWidget* TodoDetailsDialog::createStockTab() {
    QWidget *w = new QWidget();
    QVBoxLayout *v = new QVBoxLayout(w);
    v->setContentsMargins(15, 15, 15, 15);
    v->setSpacing(10);

    auto lowStockItems = ProductDataManager::instance()->getLowStockItems();
    if (lowStockItems.isEmpty()) {
        QLabel *empty = new QLabel("✨ 暂无库存警报，所有商品储量充足");
        empty->setAlignment(Qt::AlignCenter);
        empty->setStyleSheet("color: #64748b; font-size: 14px; font-weight: bold; margin: 40px;");
        v->addWidget(empty);
        return w;
    }

    QTableWidget *table = new QTableWidget(this);
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({"条码", "商品名称", "商品分类", "当前库存", "最低安全库存"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setRowCount(lowStockItems.size());
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setShowGrid(false);
    table->setFocusPolicy(Qt::NoFocus);
    table->verticalHeader()->setDefaultSectionSize(45);
    table->setItemDelegate(new TodoRowDelegate(table));
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    table->setShowGrid(false);
    table->setFocusPolicy(Qt::NoFocus);
    table->verticalHeader()->setDefaultSectionSize(45);
    table->setItemDelegate(new TodoRowDelegate(table));
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    table->setShowGrid(false);
    table->setFocusPolicy(Qt::NoFocus);
    table->verticalHeader()->setDefaultSectionSize(45);
    table->setItemDelegate(new TodoRowDelegate(table));
    // Center texts
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);

    for (int i = 0; i < lowStockItems.size(); ++i) {
        const auto &item = lowStockItems[i];
        auto createItem = [&](const QString &t) { auto *it = new QTableWidgetItem(t); it->setTextAlignment(Qt::AlignCenter); return it; };
        table->setItem(i, 0, createItem(item.barcode));
        table->setItem(i, 1, createItem(item.name));
        table->setItem(i, 2, createItem(item.category));
        
        auto *stockItem = createItem(QString::number(item.stock));
        stockItem->setForeground(QBrush(QColor("#ef4444")));
        stockItem->setFont(QFont("Arial", -1, QFont::Bold));
        table->setItem(i, 3, stockItem);
        
        table->setItem(i, 4, createItem(QString::number(item.minStock)));
    }
    v->addWidget(table);

    QPushButton *jumpBtn = new QPushButton("前往 商品库存管理 补货");
    jumpBtn->setCursor(Qt::PointingHandCursor);
    connect(jumpBtn, &QPushButton::clicked, this, [this]() {
        emit requestJumpTo(10); // Index 10 is inboundMod (商品库存管理)
        accept();
    });
    v->addWidget(jumpBtn, 0, Qt::AlignRight);
    return w;
}

QWidget* TodoDetailsDialog::createApptTab() {
    QWidget *w = new QWidget();
    QVBoxLayout *v = new QVBoxLayout(w);
    v->setContentsMargins(15, 15, 15, 15);
    v->setSpacing(10);

    // Count pending appointments
    QList<AppointmentInfo> pendingAppts;
    auto allAppts = PetDataManager::instance()->getAppointments(1, 100000, "", "全部").first;
    for (const auto &appt : allAppts) {
        if (appt.status == "Pending" || appt.status == "待确认") {
            pendingAppts.append(appt);
        }
    }

    if (pendingAppts.isEmpty()) {
        QLabel *empty = new QLabel("✨ 暂无待处理预约，工作台干干净净");
        empty->setAlignment(Qt::AlignCenter);
        empty->setStyleSheet("color: #64748b; font-size: 14px; font-weight: bold; margin: 40px;");
        v->addWidget(empty);
        return w;
    }

    QTableWidget *table = new QTableWidget(this);
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({"预约号", "顾客姓名", "宠物名称", "服务内容", "预约时间"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setRowCount(pendingAppts.size());
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setShowGrid(false);
    table->setFocusPolicy(Qt::NoFocus);
    table->verticalHeader()->setDefaultSectionSize(45);
    table->setItemDelegate(new TodoRowDelegate(table));
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    table->setShowGrid(false);
    table->setFocusPolicy(Qt::NoFocus);
    table->verticalHeader()->setDefaultSectionSize(45);
    table->setItemDelegate(new TodoRowDelegate(table));
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    table->setShowGrid(false);
    table->setFocusPolicy(Qt::NoFocus);
    table->verticalHeader()->setDefaultSectionSize(45);
    table->setItemDelegate(new TodoRowDelegate(table));
    // Center texts
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);

    for (int i = 0; i < pendingAppts.size(); ++i) {
        const auto &appt = pendingAppts[i];
        auto createItem = [&](const QString &t) { auto *it = new QTableWidgetItem(t); it->setTextAlignment(Qt::AlignCenter); return it; };
        table->setItem(i, 0, createItem(appt.id));
        table->setItem(i, 1, createItem(appt.memberName));
        table->setItem(i, 2, createItem(appt.petName));
        table->setItem(i, 3, createItem(appt.service));
        table->setItem(i, 4, createItem(QString("%1 %2").arg(appt.date).arg(appt.hour)));
    }
    v->addWidget(table);

    QPushButton *jumpBtn = new QPushButton("前往 预约服务 确认");
    jumpBtn->setCursor(Qt::PointingHandCursor);
    connect(jumpBtn, &QPushButton::clicked, this, [this]() {
        emit requestJumpTo(5); // Index 5 is apptMod (预约服务)
        accept();
    });
    v->addWidget(jumpBtn, 0, Qt::AlignRight);
    return w;
}

QWidget* TodoDetailsDialog::createLogisticsTab() {
    QWidget *w = new QWidget();
    QVBoxLayout *v = new QVBoxLayout(w);
    v->setContentsMargins(15, 15, 15, 15);
    v->setSpacing(10);

    QList<LogisticsTask> urgentTasks;
    if (LogisticsManager::instance()) {
        QList<LogisticsTask> tasks = LogisticsManager::instance()->getAllTasks();
        QDateTime now = QDateTime::currentDateTime();
        for (const auto &task : tasks) {
            if (task.status != "待处理") continue;
            if (task.appointmentTime.contains(now.date().toString("yyyy-MM-dd"))) {
                QString startTimeStr = task.appointmentTime.mid(11, 5);
                QTime startTime = QTime::fromString(startTimeStr, "HH:mm");
                if (startTime.isValid() && now.time() >= startTime.addSecs(-1800)) {
                    urgentTasks.append(task);
                }
            }
        }
    }

    if (urgentTasks.isEmpty()) {
        QLabel *empty = new QLabel("✨ 暂无紧急出行派送调度，车库运转正常");
        empty->setAlignment(Qt::AlignCenter);
        empty->setStyleSheet("color: #64748b; font-size: 14px; font-weight: bold; margin: 40px;");
        v->addWidget(empty);
        return w;
    }

    QTableWidget *table = new QTableWidget(this);
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({"调度ID", "配送类型", "目的地地址", "计划时间"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setRowCount(urgentTasks.size());
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setShowGrid(false);
    table->setFocusPolicy(Qt::NoFocus);
    table->verticalHeader()->setDefaultSectionSize(45);
    table->setItemDelegate(new TodoRowDelegate(table));
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);

    for (int i = 0; i < urgentTasks.size(); ++i) {
        const auto &task = urgentTasks[i];
        auto createItem = [&](const QString &t) { auto *it = new QTableWidgetItem(t); it->setTextAlignment(Qt::AlignCenter); return it; };
        table->setItem(i, 0, createItem(task.taskId));
        table->setItem(i, 1, createItem(task.type == "接车" ? "🚕 接车任务" : "送回任务"));
        table->setItem(i, 2, createItem(task.address));
        table->setItem(i, 3, createItem(task.appointmentTime));
    }
    v->addWidget(table);

    QPushButton *jumpBtn = new QPushButton("前往 车辆调度 安排");
    jumpBtn->setCursor(Qt::PointingHandCursor);
    connect(jumpBtn, &QPushButton::clicked, this, [this]() {
        emit requestJumpTo(9); // Index 9 is logisticsMod (车辆调度中心)
        accept();
    });
    v->addWidget(jumpBtn, 0, Qt::AlignRight);
    return w;
}

void PersonalModule::updateTodoCard() {
    if (!m_todoCard) return;

    // 1. 获取商品库存警报数量
    int lowStockCount = ProductDataManager::instance()->getLowStockItems().size();
    
    // 2. 获取服务预约待确认数量 (status == "Pending" / "待确认")
    int pendingApptCount = 0;
    auto allAppts = PetDataManager::instance()->getAppointments(1, 100000, "", "全部").first;
    for (const auto &appt : allAppts) {
        if (appt.status == "Pending" || appt.status == "待确认") {
            pendingApptCount++;
        }
    }

    // 3. 获取临期车辆调度数量 ( status == "待处理" 并且半小时内出发 )
    int urgentLogisticsCount = 0;
    if (LogisticsManager::instance()) {
        QList<LogisticsTask> tasks = LogisticsManager::instance()->getAllTasks();
        QDateTime now = QDateTime::currentDateTime();
        for (const auto &task : tasks) {
            if (task.status != "待处理") continue;
            if (task.appointmentTime.contains(now.date().toString("yyyy-MM-dd"))) {
                QString startTimeStr = task.appointmentTime.mid(11, 5);
                QTime startTime = QTime::fromString(startTimeStr, "HH:mm");
                if (startTime.isValid() && now.time() >= startTime.addSecs(-1800)) {
                    urgentLogisticsCount++;
                }
            }
        }
    }

    int totalTodos = lowStockCount + pendingApptCount + urgentLogisticsCount;

    QLabel *v = m_todoCard->findChild<QLabel*>("CardValue");
    QLabel *s = m_todoCard->findChild<QLabel*>("CardSubValue");

    if (v) v->setText(QString("%1 项待办").arg(totalTodos));
    if (s) {
        QStringList parts;
        if (lowStockCount > 0) parts << QString("%1 个库存报警").arg(lowStockCount);
        if (pendingApptCount > 0) parts << QString("%1 个待确认预约").arg(pendingApptCount);
        if (urgentLogisticsCount > 0) parts << QString("%1 个临期物流").arg(urgentLogisticsCount);
        
        if (parts.isEmpty()) {
            s->setText("暂无待办事项，系统安全运行");
        } else {
            s->setText(parts.join(" | "));
        }
    }
}

void PersonalModule::showTodoDetailsDialog() {
    TodoDetailsDialog dlg(this);
    connect(&dlg, &TodoDetailsDialog::requestJumpTo, this, &PersonalModule::requestJumpToModule);
    dlg.exec();
}
