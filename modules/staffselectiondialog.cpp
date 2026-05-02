#include "staffselectiondialog.h"
#include "staffdatamanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPainterPath>
#include <QFrame>

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
    setFixedSize(400, 500);

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
    cancelBtn->setFixedSize(100, 42); // 增大按钮 (90x36 -> 100x42)
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setStyleSheet(
        "QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 21px; color: #606266; font-size: 13px; } "
        "QPushButton:hover { background: #f5f7fa; color: #409eff; border-color: #c6e2ff; }"
    );
    
    QPushButton *okBtn = new QPushButton("确认选择");
    okBtn->setFixedSize(120, 42); // 增大按钮 (110x36 -> 120x42)
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
        
        if (!filter.isEmpty() && !info.name.contains(filter) && !info.role.contains(filter)) {
            continue;
        }

        QListWidgetItem *item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(QSize(0, 75)); // 增加高度 (60->75) 解决截断问题
        
        QWidget *widget = new QWidget();
        QHBoxLayout *layout = new QHBoxLayout(widget);
        layout->setContentsMargins(10, 5, 10, 5);
        layout->setSpacing(15);

        // 头像
        QLabel *avatarLabel = new QLabel();
        avatarLabel->setFixedSize(40, 40);
        QPixmap pix(info.gender == "女" ? ":/images/female.png" : ":/images/male.png");
        avatarLabel->setPixmap(createCircularAvatar(pix, 40));
        layout->addWidget(avatarLabel);

    // 信息
    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(6); // 增加姓名和职位的间距
    
    QLabel *nameLabel = new QLabel(info.name);
    nameLabel->setStyleSheet("font-size: 15px; color: #303133; font-weight: bold; background: transparent;");
    
    QLabel *roleLabel = new QLabel(info.role);
    roleLabel->setStyleSheet("font-size: 12px; color: #909399; background: transparent;");
    
    infoLayout->addWidget(nameLabel);
    infoLayout->addWidget(roleLabel);
    layout->addLayout(infoLayout);
    layout->addStretch();

        m_listWidget->setItemWidget(item, widget);
        item->setData(Qt::UserRole, info.name);
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
