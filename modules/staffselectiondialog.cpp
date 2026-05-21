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

// --- 全屏图片查看遮罩 ---
class FullscreenOverlay : public QDialog {
public:
    FullscreenOverlay(const QPixmap &photo, const QString &name, QWidget *parent = nullptr)
        : QDialog(parent), m_photo(photo), m_name(name)
    {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_DeleteOnClose);
        setModal(true);
        setCursor(Qt::PointingHandCursor);
        
        // 全屏覆盖
        QScreen *screen = QApplication::primaryScreen();
        setGeometry(screen->geometry());
    }
    
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        
        // 半透明黑色遮罩
        painter.fillRect(rect(), QColor(0, 0, 0, 200));
        
        // 计算图片区域：留出上下边距，按比例缩放
        int marginV = height() * 0.05; // 上下各5%边距
        int marginH = width() * 0.15;  // 左右各15%边距
        int availW = width() - marginH * 2;
        int availH = height() - marginV * 2 - 60; // 底部留60px给名字
        
        QPixmap scaled = m_photo.scaled(availW, availH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        int x = (width() - scaled.width()) / 2;
        int y = (height() - scaled.height() - 50) / 2; // 稍微上移，给底部名字留空
        
        // 绘制照片（带细圆角和轻微阴影感）
        QPainterPath clipPath;
        clipPath.addRoundedRect(QRectF(x, y, scaled.width(), scaled.height()), 6, 6);
        painter.setClipPath(clipPath);
        painter.drawPixmap(x, y, scaled);
        painter.setClipping(false);
        
        // 底部姓名
        QFont nameFont("Microsoft YaHei", 14, QFont::Bold);
        painter.setFont(nameFont);
        painter.setPen(QColor(255, 255, 255, 220));
        QRect nameRect(0, y + scaled.height() + 12, width(), 40);
        painter.drawText(nameRect, Qt::AlignCenter, m_name);
        
        // 关闭提示
        QFont hintFont("Microsoft YaHei", 10);
        painter.setFont(hintFont);
        painter.setPen(QColor(255, 255, 255, 120));
        QRect hintRect(0, height() - 40, width(), 30);
        painter.drawText(hintRect, Qt::AlignCenter, "点击任意位置关闭");
    }
    
    void mousePressEvent(QMouseEvent *) override {
        accept();
    }
    
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Escape) {
            accept();
        }
        QDialog::keyPressEvent(event);
    }
    
private:
    QPixmap m_photo;
    QString m_name;
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
        avatarLabel->setFixedSize(48, 48);
        avatarLabel->setToolTip("点击查看大图");
        
        QPixmap realAvatar = StaffDataManager::instance()->getStaffPixmap(info.id);
        QPixmap avatarPix;
        if (!realAvatar.isNull()) {
            avatarPix = realAvatar;
        } else {
            avatarPix = QPixmap(info.gender == "女" ? ":/images/female.png" : ":/images/male.png");
        }
        avatarLabel->setPixmap(createCircularAvatar(avatarPix, 48));
        
        // 点击头像放大
        QString staffId = info.id;
        avatarLabel->onClick = [this, staffId]() {
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
    // 先居中裁剪为正方形，避免非正方形图片被压扁
    QPixmap squareSrc;
    if (src.width() != src.height()) {
        int side = qMin(src.width(), src.height());
        int x = (src.width() - side) / 2;
        int y = (src.height() - side) / 2;
        squareSrc = src.copy(x, y, side, side);
    } else {
        squareSrc = src;
    }
    
    QPixmap target(size, size);
    target.fill(Qt::transparent);
    QPainter painter(&target);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    QPainterPath path;
    path.addEllipse(0, 0, size, size);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, size, size, squareSrc.scaled(size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
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
    // 使用模态对话框全屏覆盖，避免与父级模态冲突产生系统提示音
    FullscreenOverlay overlay(avatar, name, this);
    overlay.exec();
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
