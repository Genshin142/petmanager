#include "staffselectiondialog.h"
#include "staffdatamanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPainterPath>
#include <QFrame>
#include <QMouseEvent>
#include <QApplication>
#include <QScreen>

// --- 可点击头像 Label ---
class ClickableAvatarLabel : public QLabel {
public:
    ClickableAvatarLabel(QWidget *parent = nullptr) : QLabel(parent) {
        setCursor(Qt::PointingHandCursor);
    }
    std::function<void()> onClick;
protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && onClick) {
            onClick();
        }
        QLabel::mousePressEvent(event);
    }
};

StaffSelectionDialog::StaffSelectionDialog(QWidget *parent) : QDialog(parent)
{
    setupUI();
    renderStaffList();
}

QString StaffSelectionDialog::selectedStaff() const
{
    return m_selectedName;
}

void StaffSelectionDialog::setupUI()
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(420, 520);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);

    QFrame *bgFrame = new QFrame();
    bgFrame->setObjectName("bgFrame");
    bgFrame->setStyleSheet(
        "QFrame#bgFrame { background: white; border-radius: 12px; }"
    );
    layout->addWidget(bgFrame);

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 60));
    shadow->setOffset(0, 4);
    bgFrame->setGraphicsEffect(shadow);

    QVBoxLayout *container = new QVBoxLayout(bgFrame);
    container->setContentsMargins(20, 20, 20, 20);
    container->setSpacing(15);

    // 标题
    QLabel *titleLabel = new QLabel("指定服务执行人员");
    titleLabel->setStyleSheet("font-size: 18px; color: #303133; font-weight: bold;");
    container->addWidget(titleLabel);

    // 搜索框
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(" 搜索姓名或职位...");
    m_searchEdit->setFixedHeight(36);
    m_searchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 18px; padding: 0 15px; font-size: 13px; background: #f5f7fa; } "
        "QLineEdit:focus { border-color: #409eff; background: white; }"
    );
    container->addWidget(m_searchEdit);

    // 列表
    m_listWidget = new QListWidget();
    m_listWidget->setSpacing(5);
    m_listWidget->setStyleSheet(
        "QListWidget { border: none; background: transparent; outline: none; } "
        "QListWidget::item { background: #ffffff; border: 1px solid #f0f2f5; border-radius: 8px; padding: 10px; margin-bottom: 5px; } "
        "QListWidget::item:selected { background: #ecf5ff; border-color: #409eff; color: #409eff; } "
        "QListWidget::item:hover { background: #f5f7fa; }"
    );
    container->addWidget(m_listWidget);

    // 按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *cancelBtn = new QPushButton("取消");
    cancelBtn->setFixedSize(100, 42);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setStyleSheet(
        "QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 21px; color: #606266; font-size: 13px; } "
        "QPushButton:hover { background: #f5f7fa; color: #409eff; border-color: #c6e2ff; }"
    );
    
    QPushButton *okBtn = new QPushButton("确认选择");
    okBtn->setFixedSize(120, 42);
    okBtn->setCursor(Qt::PointingHandCursor);
    okBtn->setStyleSheet(
        "QPushButton { background: #409eff; color: white; border: none; border-radius: 21px; font-weight: bold; font-size: 13px; } "
        "QPushButton:hover { background: #66b1ff; }"
    );

    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addSpacing(10);
    btnLayout->addWidget(okBtn);
    container->addLayout(btnLayout);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &StaffSelectionDialog::onSearchChanged);
    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, &StaffSelectionDialog::onItemDoubleClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(okBtn, &QPushButton::clicked, this, &StaffSelectionDialog::onConfirm);
}

void StaffSelectionDialog::renderStaffList(const QString &filter)
{
    m_listWidget->clear();
    auto allStaff = StaffDataManager::instance()->allStaff();
    
    for (const auto &info : allStaff) {
        if (info.status == "离职") continue;
        
        if (!filter.isEmpty() && !info.name.contains(filter) && !info.role.contains(filter) && !info.id.contains(filter)) {
            continue;
        }

        QListWidgetItem *item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(QSize(0, 75));
        
        QWidget *widget = new QWidget();
        QHBoxLayout *layout = new QHBoxLayout(widget);
        layout->setContentsMargins(10, 5, 10, 5);
        layout->setSpacing(15);

        // 头像 - 优先使用真实头像，无头像时按性别显示默认图标
        ClickableAvatarLabel *avatarLabel = new ClickableAvatarLabel();
        avatarLabel->setFixedSize(44, 44);
        avatarLabel->setToolTip("点击查看大图");
        
        QPixmap realAvatar = StaffDataManager::instance()->getStaffPixmap(info.id);
        QPixmap avatarPix;
        if (!realAvatar.isNull()) {
            avatarPix = realAvatar;
        } else {
            avatarPix = QPixmap(info.gender == "女" ? ":/images/female.png" : ":/images/male.png");
        }
        avatarLabel->setPixmap(createCircularAvatar(avatarPix, 44));
        
        // 点击头像放大
        QString staffId = info.id;
        QString staffName = info.name;
        avatarLabel->onClick = [this, staffId, staffName]() {
            onAvatarClicked(staffId);
        };
        
        layout->addWidget(avatarLabel);

        // 信息
        QVBoxLayout *infoLayout = new QVBoxLayout();
        infoLayout->setSpacing(6);
        
        // 姓名 (工号)
        QLabel *nameLabel = new QLabel(QString("%1 (%2)").arg(info.name, info.id));
        nameLabel->setStyleSheet("font-size: 15px; color: #303133; font-weight: bold; background: transparent;");
        
        QLabel *roleLabel = new QLabel(info.role);
        roleLabel->setStyleSheet("font-size: 12px; color: #909399; background: transparent;");
        
        infoLayout->addWidget(nameLabel);
        infoLayout->addWidget(roleLabel);
        layout->addLayout(infoLayout);
        layout->addStretch();

        m_listWidget->setItemWidget(item, widget);
        // UserRole 中存储 "姓名 (ID)" 格式，与入库对话框一致
        item->setData(Qt::UserRole, QString("%1 (%2)").arg(info.name, info.id));
    }
}

QPixmap StaffSelectionDialog::createCircularAvatar(const QPixmap &src, int size)
{
    QPixmap target(size, size);
    target.fill(Qt::transparent);
    QPainter painter(&target);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    QPainterPath path;
    path.addEllipse(0, 0, size, size);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, size, size, src.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    return target;
}

void StaffSelectionDialog::onAvatarClicked(const QString &staffId)
{
    EmployeeInfo emp = StaffDataManager::instance()->getStaff(staffId);
    QPixmap avatar = StaffDataManager::instance()->getStaffPixmap(staffId);
    if (avatar.isNull()) {
        avatar = QPixmap(emp.gender == "女" ? ":/images/female.png" : ":/images/male.png");
    }
    showEnlargedAvatar(avatar, emp.name);
}

void StaffSelectionDialog::showEnlargedAvatar(const QPixmap &avatar, const QString &name)
{
    // 创建全屏半透明遮罩弹窗
    QDialog *overlay = new QDialog(this);
    overlay->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    overlay->setAttribute(Qt::WA_TranslucentBackground);
    overlay->setAttribute(Qt::WA_DeleteOnClose);
    
    // 获取屏幕尺寸，居中显示
    QScreen *screen = QApplication::primaryScreen();
    QRect screenRect = screen->availableGeometry();
    overlay->setFixedSize(screenRect.width(), screenRect.height());
    overlay->move(screenRect.topLeft());
    
    // 半透明黑色遮罩背景
    QWidget *bgWidget = new QWidget(overlay);
    bgWidget->setFixedSize(overlay->size());
    bgWidget->setStyleSheet("background: rgba(0, 0, 0, 160);");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(overlay);
    mainLayout->setAlignment(Qt::AlignCenter);
    
    // 白色内容卡片
    QFrame *card = new QFrame();
    card->setFixedSize(340, 400);
    card->setStyleSheet("QFrame { background: white; border-radius: 16px; }");
    
    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setAlignment(Qt::AlignCenter);
    cardLayout->setSpacing(16);
    cardLayout->setContentsMargins(20, 24, 20, 24);
    
    // 大头像（圆形）
    QLabel *bigAvatar = new QLabel();
    int avatarSize = 220;
    bigAvatar->setFixedSize(avatarSize, avatarSize);
    bigAvatar->setAlignment(Qt::AlignCenter);
    bigAvatar->setPixmap(createCircularAvatar(avatar, avatarSize));
    cardLayout->addWidget(bigAvatar, 0, Qt::AlignCenter);
    
    // 姓名
    QLabel *nameLabel = new QLabel(name);
    nameLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #303133; background: transparent;");
    cardLayout->addWidget(nameLabel);
    
    // 关闭提示
    QLabel *hintLabel = new QLabel("点击任意位置关闭");
    hintLabel->setAlignment(Qt::AlignCenter);
    hintLabel->setStyleSheet("font-size: 12px; color: #909399; background: transparent;");
    cardLayout->addWidget(hintLabel);
    
    mainLayout->addWidget(card);
    
    // 点击任意位置关闭
    overlay->installEventFilter(overlay);
    connect(overlay, &QDialog::finished, overlay, &QDialog::deleteLater);
    
    // 用一个透明按钮覆盖整个遮罩来实现点击关闭
    QPushButton *closeOverlay = new QPushButton(overlay);
    closeOverlay->setFixedSize(overlay->size());
    closeOverlay->setStyleSheet("background: transparent; border: none;");
    closeOverlay->lower(); // 放到最底层
    bgWidget->lower();     // 遮罩在最底层
    connect(closeOverlay, &QPushButton::clicked, overlay, &QDialog::close);
    
    overlay->exec();
}

void StaffSelectionDialog::onSearchChanged(const QString &text)
{
    renderStaffList(text.trimmed());
}

void StaffSelectionDialog::onItemDoubleClicked(QListWidgetItem *item)
{
    m_selectedName = item->data(Qt::UserRole).toString();
    accept();
}

void StaffSelectionDialog::onConfirm()
{
    QListWidgetItem *item = m_listWidget->currentItem();
    if (item) {
        m_selectedName = item->data(Qt::UserRole).toString();
        accept();
    }
}
