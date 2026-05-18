#include "addappointmentdialog.h"
#include "petdatamanager.h"
#include "pricemanager.h"
#include "servicedatamanager.h"
#include "fostermodule.h" // 包含 FullImagePreviewDialog 定义
#include <QDate>
#include <QDateTime>
#include <QCompleter>
#include <QGraphicsDropShadowEffect>
#include <QScrollArea>
#include <QRandomGenerator>
#include <QLineEdit>
#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include <QDir>
#include "custom_calendar_edit.h"
#include <QLayout>
#include <QStyle>

// --- 通用的流式布局类，用于实现标签自动换行 ---
class FlowLayout : public QLayout
{
public:
    explicit FlowLayout(QWidget *parent, int margin = -1, int hSpacing = -1, int vSpacing = -1)
        : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing)
    {
        setContentsMargins(margin, margin, margin, margin);
    }
    explicit FlowLayout(int margin = -1, int hSpacing = -1, int vSpacing = -1)
        : m_hSpace(hSpacing), m_vSpace(vSpacing)
    {
        setContentsMargins(margin, margin, margin, margin);
    }
    ~FlowLayout() {
        QLayoutItem *item;
        while ((item = takeAt(0))) delete item;
    }

    void addItem(QLayoutItem *item) override { m_itemList.append(item); }
    int horizontalSpacing() const { return m_hSpace >= 0 ? m_hSpace : smartSpacing(QStyle::PM_LayoutHorizontalSpacing); }
    int verticalSpacing() const { return m_vSpace >= 0 ? m_vSpace : smartSpacing(QStyle::PM_LayoutVerticalSpacing); }
    Qt::Orientations expandingDirections() const override { return { }; }
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override { return doLayout(QRect(0, 0, width, 0), true); }
    int count() const override { return m_itemList.size(); }
    QLayoutItem *itemAt(int index) const override { return m_itemList.value(index); }
    QSize minimumSize() const override {
        QSize size;
        for (auto item : m_itemList) size = size.expandedTo(item->minimumSize());
        const QMargins margins = contentsMargins();
        size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
        return size;
    }
    void setGeometry(const QRect &rect) override {
        QLayout::setGeometry(rect);
        doLayout(rect, false);
    }
    QSize sizeHint() const override { return minimumSize(); }
    QLayoutItem *takeAt(int index) override {
        if (index >= 0 && index < m_itemList.size()) return m_itemList.takeAt(index);
        return nullptr;
    }

private:
    int doLayout(const QRect &rect, bool testOnly) const {
        int left, top, right, bottom;
        getContentsMargins(&left, &top, &right, &bottom);
        QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
        int x = effectiveRect.x();
        int y = effectiveRect.y();
        int lineHeight = 0;

        for (auto item : m_itemList) {
            QWidget *wid = item->widget();
            int spaceX = horizontalSpacing();
            if (spaceX == -1) spaceX = wid->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
            int spaceY = verticalSpacing();
            if (spaceY == -1) spaceY = wid->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);
            
            int nextX = x + item->sizeHint().width() + spaceX;
            if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
                x = effectiveRect.x();
                y = y + lineHeight + spaceY;
                nextX = x + item->sizeHint().width() + spaceX;
                lineHeight = 0;
            }

            if (!testOnly) item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));
            x = nextX;
            lineHeight = qMax(lineHeight, item->sizeHint().height());
        }
        return y + lineHeight - rect.y() + bottom;
    }
    int smartSpacing(QStyle::PixelMetric pm) const {
        QObject *parent = this->parent();
        if (!parent) return -1;
        else if (parent->isWidgetType()) {
            QWidget *pw = static_cast<QWidget *>(parent);
            return pw->style()->pixelMetric(pm, nullptr, pw);
        } else {
            return static_cast<QLayout *>(parent)->spacing();
        }
    }

    QList<QLayoutItem *> m_itemList;
    int m_hSpace;
    int m_vSpace;
};

AddAppointmentDialog::AddAppointmentDialog(QWidget *parent) : QDialog(parent)
{
    setupUI();
    setWindowTitle("新增预约服务");
    onAddServiceRow();
}

void AddAppointmentDialog::setupUI()
{
    setFixedSize(760, 900); // 大幅增加宽度确保寄养行不挤压，高度 900 容纳更多内容
    setStyleSheet("QDialog { background: #f8faff; }");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 25, 30, 25);
    mainLayout->setSpacing(18);

    // --- 顶部头像区 ---
    QHBoxLayout *avatarLayout = new QHBoxLayout();
    m_avatarLabel = new QLabel();
    m_avatarLabel->setFixedSize(110, 110); // 进一步放大
    m_avatarLabel->setCursor(Qt::PointingHandCursor);
    m_avatarLabel->setStyleSheet(
        "QLabel { "
        "  background: transparent; "
        "  border: none; " // 移除 CSS 边框，改为代码手绘
        "  qproperty-alignment: AlignCenter; "
        "} "
    );
    
    // 给头像加阴影
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 30));
    shadow->setOffset(0, 4);
    m_avatarLabel->setGraphicsEffect(shadow);
    
    // 初始状态画一个圆形的狗狗图标
    int startSize = 110;
    QPixmap startPix(startSize, startSize);
    startPix.fill(Qt::transparent);
    QPainter p(&startPix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QBrush(QColor("#f1f5f9")));
    p.setPen(Qt::NoPen);
    p.drawEllipse(4, 4, startSize - 8, startSize - 8);
    p.setPen(Qt::white);
    QFont f = p.font(); f.setPointSize(40); p.setFont(f);
    p.drawText(startPix.rect(), Qt::AlignCenter, "🐶");
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(Qt::white, 4));
    p.drawEllipse(2, 2, startSize - 4, startSize - 4);
    m_avatarLabel->setPixmap(startPix);
    
    avatarLayout->addStretch();
    avatarLayout->addWidget(m_avatarLabel);
    avatarLayout->addStretch();
    mainLayout->addLayout(avatarLayout);
    mainLayout->addSpacing(5);

    // 安装事件过滤器以处理点击放大
    m_avatarLabel->installEventFilter(this);

    auto addLabel = [&](const QString &text) {
        QLabel *l = new QLabel(text);
        l->setStyleSheet("color: #64748b; font-size: 13px; font-weight: 700; margin-bottom: 2px;");
        return l;
    };

    auto setEditStyle = [&](QWidget *w) {
        w->setFixedHeight(44);
        QString commonStyle = "QWidget { border: 1px solid #e2e8f0; border-radius: 10px; padding: 0 15px; background: #fcfcfd; font-size: 14px; color: #1e293b; } "
                             "QWidget:focus { border-color: #3b82f6; background: white; } ";
        
        if (qobject_cast<QComboBox*>(w)) {
            commonStyle += "QComboBox { padding-right: 35px; } "
                           "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: center right; width: 30px; border: none; background: transparent; } "
                           "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 14px; height: 14px; } "
                           "QComboBox QAbstractItemView { border: 1px solid #e2e8f0; border-radius: 10px; background: white; selection-background-color: #f1f5f9; selection-color: #3b82f6; outline: none; padding: 5px; }";
        }
        w->setStyleSheet(commonStyle);
    };

    // 1. 宠物选择
    mainLayout->addWidget(addLabel("选择宠物"));
    m_petCombo = new QComboBox();
    m_petCombo->setEditable(true);
    setEditStyle(m_petCombo);
    
    auto pets = PetDataManager::instance()->allPets();
    bool hasPets = false;
    for (const auto &pet : pets) {
        if (pet.id.isEmpty() || pet.id == "0") continue; 
        m_petCombo->addItem(QString("%1 (%2) | %3").arg(pet.name, pet.id, pet.breed), pet.id);
        hasPets = true;
    }
    
    if (!hasPets) {
        m_petCombo->addItem("暂无宠物信息", "");
        m_petCombo->setEnabled(false);
        m_petCombo->setStyleSheet(m_petCombo->styleSheet() + "QComboBox { color: #94a3b8; }");
    }
    
    m_petCombo->lineEdit()->installEventFilter(this); 
    mainLayout->addWidget(m_petCombo);

    QFrame *ownerCard = new QFrame();
    ownerCard->setStyleSheet("background: transparent; border: none;");
    QHBoxLayout *ownerLayout = new QHBoxLayout(ownerCard);
    ownerLayout->setContentsMargins(0, 5, 0, 10);
    ownerLayout->setSpacing(15);
    
    auto createInfoCol = [&](const QString &title, QLabel* &valLabel) {
        QVBoxLayout *col = new QVBoxLayout();
        col->setSpacing(6);
        QLabel *tL = new QLabel(title); 
        tL->setStyleSheet("color: #94a3b8; font-size: 11px; font-weight: 800; border: none; text-transform: uppercase; margin-left: 2px;");
        
        valLabel = new QLabel("未选择宠物"); 
        valLabel->setFixedHeight(44);
        valLabel->setFixedWidth(150); // 固定宽度，防止跳动
        valLabel->setStyleSheet(
            "QLabel { "
            "  background: #f8fafc; "
            "  border: 1px solid #e2e8f0; "
            "  border-radius: 10px; "
            "  padding: 0 15px; "
            "  color: #1e293b; "
            "  font-size: 14px; "
            "  font-weight: 600; "
            "} "
        );
        valLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        
        col->addWidget(tL); 
        col->addWidget(valLabel);
        return col;
    };
    ownerLayout->addLayout(createInfoCol("关联主人", m_ownerNameLabel));
    ownerLayout->addLayout(createInfoCol("联系方式", m_ownerPhoneLabel));
    ownerLayout->addStretch(); // 关键：右侧加弹簧，迫使信息向左靠拢
    mainLayout->addWidget(ownerCard);

    connect(m_petCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AddAppointmentDialog::onPetChanged);
    if (hasPets) onPetChanged(0); // 初始化即触发联动

    // 2. 业务选择
    QHBoxLayout *serviceHeader = new QHBoxLayout();
    serviceHeader->addWidget(addLabel("预约业务与项目"));
    serviceHeader->addStretch();
    m_addServiceBtn = new QPushButton("叠加服务");
    m_addServiceBtn->setCursor(Qt::PointingHandCursor);
    m_addServiceBtn->setStyleSheet(
        "QPushButton { "
        "  background: #3b82f6; "
        "  color: white; "
        "  border-radius: 8px; "
        "  padding: 6px 15px; "
        "  font-size: 12px; "
        "  font-weight: 800; "
        "  border: none; "
        "} "
        "QPushButton:hover { background: #2563eb; } "
        "QPushButton:pressed { background: #1d4ed8; }"
    );
    serviceHeader->addWidget(m_addServiceBtn);
    mainLayout->addLayout(serviceHeader);

    QScrollArea *serviceScroll = new QScrollArea();
    serviceScroll->setWidgetResizable(true);
    serviceScroll->setStyleSheet(
        "QScrollArea { background: transparent; border: none; } "
        "QScrollBar:vertical { border: none; background: #f8fafc; width: 6px; margin: 0; border-radius: 3px; } "
        "QScrollBar::handle:vertical { background: #e2e8f0; min-height: 30px; border-radius: 3px; } "
        "QScrollBar::handle:vertical:hover { background: #94a3b8; } "
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; } "
    );
    QWidget *serviceContent = new QWidget();
    serviceContent->setStyleSheet("background: transparent; border: none;");
    m_serviceContainer = new QVBoxLayout(serviceContent);
    m_serviceContainer->setContentsMargins(0, 0, 15, 0); // 增加右侧间距，防止删除 Label 被遮挡
    m_serviceContainer->setSpacing(10);
    m_serviceContainer->addStretch();
    serviceScroll->setWidget(serviceContent);
    serviceScroll->setFixedHeight(480); // 增加高度限制
    serviceScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 强制关闭垂直滚动条
    serviceScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 强制关闭水平滚动条
    mainLayout->addWidget(serviceScroll);

    connect(m_addServiceBtn, &QPushButton::clicked, this, &AddAppointmentDialog::onAddServiceRow);

    // 3. 业务与备注的统一滚动区
    // 顶部标题已在 serviceHeader 中定义，这里不再重复添加
    
    QLabel *notesLabel = addLabel("综合备注说明");
    m_serviceContainer->addWidget(notesLabel);
    
    m_notesEdit = new QTextEdit();
    m_notesEdit->setPlaceholderText("请输入该宠物的整体注意事项...");
    m_notesEdit->setFixedHeight(60);
    m_notesEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_notesEdit->setStyleSheet(
        "QTextEdit { "
        "  border: 1px solid #e2e8f0; "
        "  border-radius: 12px; "
        "  padding: 10px 15px; "
        "  background: white; "
        "  font-size: 14px; "
        "  color: #1e293b; "
        "} "
        "QTextEdit:focus { border-color: #3b82f6; } "
    );
    
    // 自动增长逻辑
    connect(m_notesEdit, &QTextEdit::textChanged, this, [=]() {
        int contentHeight = m_notesEdit->document()->size().height();
        int newHeight = qMax(60, qMin(200, contentHeight + 20)); // 最小60，最大200
        m_notesEdit->setFixedHeight(newHeight);
        m_notesEdit->setVerticalScrollBarPolicy(contentHeight > 180 ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
    });

    m_serviceContainer->addWidget(m_notesEdit);

    m_serviceContainer->addStretch(); // 底部弹簧

    m_errorLabel = new QLabel("");
    m_errorLabel->setStyleSheet("color: #ef4444; font-size: 12px; font-weight: bold;");
    mainLayout->addWidget(m_errorLabel);

    mainLayout->addStretch();

    QHBoxLayout *btnLayout = new QHBoxLayout();
    cancelBtn = new QPushButton("取消");
    saveBtn = new QPushButton("确认预约");
    cancelBtn->setFixedSize(110, 48);
    cancelBtn->setStyleSheet("QPushButton { background: #f1f5f9; color: #64748b; border-radius: 12px; font-weight: 700; border: none; } QPushButton:hover { background: #e2e8f0; }");
    saveBtn->setFixedSize(200, 48);
    saveBtn->setStyleSheet("QPushButton { background: #3b82f6; color: white; border-radius: 12px; font-weight: 800; border: none; font-size: 15px; } QPushButton:hover { background: #2563eb; }");
    
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, this, &AddAppointmentDialog::accept);
    
    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(saveBtn);
    mainLayout->addLayout(btnLayout);
}

void AddAppointmentDialog::onPetChanged(int index)
{
    QString petId = m_petCombo->itemData(index).toString();
    if (petId.isEmpty()) {
        m_ownerNameLabel->setText("未选择宠物");
        m_ownerPhoneLabel->setText("未选择宠物");
        m_currentAvatarPath = "";
        
        int size = 110;
        QPixmap emptyPix(size, size);
        emptyPix.fill(Qt::transparent);
        QPainter p(&emptyPix);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(QBrush(QColor("#f1f5f9")));
        p.setPen(Qt::NoPen);
        p.drawEllipse(4, 4, size - 8, size - 8);
        p.setPen(Qt::white);
        QFont f = p.font(); f.setPointSize(40); p.setFont(f);
        p.drawText(emptyPix.rect(), Qt::AlignCenter, "🐶");
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(Qt::white, 4));
        p.drawEllipse(2, 2, size - 4, size - 4);
        m_avatarLabel->setPixmap(emptyPix);
        return;
    }
    PetInfo pet = PetDataManager::instance()->getPet(petId);
    m_ownerNameLabel->setText(QString("%1 (%2)").arg(pet.ownerName, pet.ownerId));
    m_ownerPhoneLabel->setText(pet.ownerPhone);
    
    // 更新头像
    m_currentAvatarPath = pet.avatarPath;
    m_avatarLabel->setMask(QRegion()); // 彻底弃用蒙版模式
    
    int size = 110;
    QPixmap target(size, size);
    target.fill(Qt::transparent);
    QPainter painter(&target);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (!m_currentAvatarPath.isEmpty()) {
        QPixmap pix(m_currentAvatarPath);
        if (!pix.isNull()) {
            // 1. 绘制圆形裁剪的图片
            QPainterPath path;
            path.addEllipse(4, 4, size - 8, size - 8); // 留出 4px 给边框
            painter.save();
            painter.setClipPath(path);
            QPixmap scaled = pix.scaled(size - 8, size - 8, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            painter.drawPixmap(4 + (size - 8 - scaled.width()) / 2, 4 + (size - 8 - scaled.height()) / 2, scaled);
            painter.restore();
            
            // 2. 绘制白色描边
            painter.setPen(QPen(Qt::white, 4));
            painter.drawEllipse(2, 2, size - 4, size - 4);
            m_avatarLabel->setPixmap(target);
        }
    } else {
        // 绘制带名字首字母的圆形占位图
        painter.setBrush(QBrush(QColor("#3b82f6")));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(4, 4, size - 8, size - 8);
        
        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setBold(true);
        font.setPointSize(32);
        painter.setFont(font);
        painter.drawText(target.rect(), Qt::AlignCenter, pet.name.left(1).toUpper());
        
        // 描边
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(Qt::white, 4));
        painter.drawEllipse(2, 2, size - 4, size - 4);
        m_avatarLabel->setPixmap(target);
    }

    // 联动更新所有已存在服务行的价格（根据新宠物的体型）
    for (auto &row : m_serviceRows) {
        QString currentType = row.serviceCombo->currentText();
        double finalPrice = PriceManager::instance()->calculateFinalAmount(currentType, pet.breed);
        row.amountEdit->setText(QString::number(finalPrice, 'f', 2));
    }
}

void AddAppointmentDialog::accept()
{
    if (m_petCombo->currentData().toString().isEmpty()) {
        m_errorLabel->setText("⚠ 请先选择宠物");
        return;
    }

    QString petId = m_petCombo->currentData().toString();
    QString petName = m_petCombo->currentText().split(" (").first();
    auto existingAppts = PetDataManager::instance()->getAppointments(1, 2000, "").first;

    for (int i = 0; i < m_serviceRows.size(); ++i) {
        const auto &row = m_serviceRows[i];
        QString date = qobject_cast<QLineEdit*>(row.dateEdit)->text();
        QString hour = row.hourCombo->currentText();
        QString service = row.serviceCombo->currentText();
        
        // 1. 检查与数据库中已有预约的冲突
        for (const auto &existing : existingAppts) {
            if (existing.petId == petId && existing.date == date && existing.hour == hour && existing.status != "Cancelled") {
                m_errorLabel->setText(QString("⚠ 冲突：宠物【%1】在 %2 %3 已有预约服务。").arg(petName, date, hour));
                return;
            }
        }

        // 2. 检查本次新增列表内部的冲突 (防止在同一个对话框里加了两个相同时间的)
        for (int j = 0; j < i; ++j) {
            QString prevDate = qobject_cast<QLineEdit*>(m_serviceRows[j].dateEdit)->text();
            QString prevHour = m_serviceRows[j].hourCombo->currentText();
            if (prevDate == date && prevHour == hour) {
                m_errorLabel->setText(QString("⚠ 警告：您在本次提交中为相同时段添加了多项服务。"));
                return;
            }
        }

        // 3. 基础校验：洗护/美容等必须选子项
        bool hasSelected = false;
        auto tags = row.tagsContainer->findChildren<QPushButton*>();
        for (auto btn : tags) {
            if (btn->isChecked()) {
                hasSelected = true;
                break;
            }
        }
        
        QString typeKey = row.serviceCombo->itemData(row.serviceCombo->currentIndex()).toString().split("|").first();
        if ((typeKey == "Grooming" || typeKey == "Beauty" || typeKey == "Medical") && !hasSelected) {
            m_errorLabel->setText(QString("⚠ 请选择【%1】的具体服务项目").arg(service));
            return;
        }
    } // 关闭 for 循环
    
    QDialog::accept();
} // 关闭 accept 函数



// 实现点击放大逻辑
bool AddAppointmentDialog::eventFilter(QObject *obj, QEvent *event)
{
    // 点击搜索框任意位置打开下拉列表
    if (obj == m_petCombo->lineEdit() && event->type() == QEvent::MouseButtonPress) {
        m_petCombo->showPopup();
        return false; 
    }

    if (obj == m_avatarLabel && event->type() == QEvent::MouseButtonRelease) {
        // --- 100% 复制自 ProductModule 的商品图片放大逻辑 ---
        QWidget *mainWin = nullptr;
        for (QWidget *w : QApplication::topLevelWidgets()) {
            if (w->objectName() == "MainWindow" || w->inherits("QMainWindow")) {
                mainWin = w;
                break;
            }
        }
        if (!mainWin) mainWin = this->window();

        QWidget *currentWin = this;
        QDialog *preview = new QDialog(currentWin, Qt::FramelessWindowHint);
        
        // 铺满主窗口
        preview->setGeometry(mainWin->geometry());
        preview->setAttribute(Qt::WA_TranslucentBackground);
        
        QVBoxLayout *layout = new QVBoxLayout(preview);
        layout->setContentsMargins(0, 0, 0, 0);
        
        // 全屏背景遮罩 - 调深阴影 (215)
        QFrame *bg = new QFrame();
        bg->setStyleSheet("background-color: rgba(0, 0, 0, 215);");
        layout->addWidget(bg);
        
        QVBoxLayout *bgLayout = new QVBoxLayout(bg);
        bgLayout->setContentsMargins(0, 0, 0, 0);
        bgLayout->setAlignment(Qt::AlignCenter);
        
        QLabel *imgLabel = new QLabel();
        int maxDim = qMin(mainWin->width(), mainWin->height()) * 0.8;
        
        if (!m_currentAvatarPath.isEmpty()) {
            QPixmap pix(m_currentAvatarPath);
            if (!pix.isNull()) {
                imgLabel->setPixmap(pix.scaled(maxDim, maxDim, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        } else {
            QString txt = m_avatarLabel->text();
            if (txt.isEmpty()) txt = "🐶";
            
            // 对于 Emoji，同样复刻商品图的白色大矩形质感
            QPixmap pix(maxDim, maxDim);
            pix.fill(Qt::white);
            QPainter p(&pix);
            p.setRenderHint(QPainter::Antialiasing);
            p.setRenderHint(QPainter::TextAntialiasing);
            QFont f = p.font();
            f.setPixelSize(maxDim * 0.6);
            p.setFont(f);
            p.drawText(pix.rect(), Qt::AlignCenter, txt);
            p.end();
            imgLabel->setPixmap(pix);
        }
        
        imgLabel->setStyleSheet("border: none; background: transparent; padding: 0;");
        imgLabel->setAlignment(Qt::AlignCenter);
        bgLayout->addWidget(imgLabel);
        
        // --- 100% 复制自 ProductModule：点击背景任意位置关闭 ---
        bg->installEventFilter(this);
        bg->setProperty("isPreviewBg", true);
        bg->setProperty("previewDlg", QVariant::fromValue((void*)preview));
        bg->setCursor(Qt::PointingHandCursor);
        
        connect(preview, &QDialog::finished, preview, &QDialog::deleteLater);
        preview->show();
        return true;
    }

    // --- 处理业务行删除按钮 (Label 模拟点击) ---
    if (obj->property("isDelBtn").toBool()) {
        if (event->type() == QEvent::MouseButtonPress) {
            void* ptr = qvariant_cast<void*>(obj->property("rowPtr"));
            if (ptr) {
                QWidget *rowWidget = static_cast<QWidget*>(ptr);
                rowWidget->deleteLater();
                for (int i=0; i < m_serviceRows.count(); ++i) {
                    if (m_serviceRows[i].container == rowWidget) {
                        m_serviceRows.removeAt(i);
                        break;
                    }
                }
                return true;
            }
        }
    }

    // --- 100% 复制自 ProductModule：点击预览背景关闭 ---
    if (obj->property("isPreviewBg").toBool()) {
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) {
            void* ptr = qvariant_cast<void*>(obj->property("previewDlg"));
            if (ptr) {
                QDialog *dlg = static_cast<QDialog*>(ptr);
                dlg->close();
                return true;
            }
        }
    }

    return QDialog::eventFilter(obj, event);
}

void AddAppointmentDialog::onAddServiceRow()
{
    QFrame *rowWidget = new QFrame();
    rowWidget->setObjectName("rowWidget");
    rowWidget->setAttribute(Qt::WA_StyledBackground);
    rowWidget->setStyleSheet("#rowWidget { background: transparent; border: none; }");
    
    // --- 核心重构：垂直容器，包含“主选择行”和“附加原子服务行” ---
    QVBoxLayout *itemLayout = new QVBoxLayout(rowWidget);
    itemLayout->setContentsMargins(0, 5, 0, 10);
    itemLayout->setSpacing(8);

    // --- 第一行：主选择行 ---
    QHBoxLayout *mainRow = new QHBoxLayout();
    mainRow->setContentsMargins(0, 0, 0, 0);
    mainRow->setSpacing(8);

    // 1. 业务选择
    QComboBox *combo = new QComboBox();
    combo->setFixedHeight(44);
    combo->setStyleSheet(
        "QComboBox { border: 1px solid #e2e8f0; border-radius: 10px; padding: 0 10px; background: #fcfcfd; font-size: 13px; color: #1e293b; padding-right: 25px; } "
        "QComboBox:focus { border-color: #3b82f6; background: white; } "
        "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: center right; width: 20px; border: none; background: transparent; } "
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; } "
        "QComboBox QAbstractItemView { border: 1px solid #e2e8f0; border-radius: 10px; background: white; selection-background-color: #f1f5f9; selection-color: #3b82f6; outline: none; padding: 5px; }"
    );
    combo->addItem("洗护", "Grooming|洗护");
    combo->addItem("美容", "Beauty|美容");
    combo->addItem("保健", "Medical|保健");
    combo->addItem("寄养", "Boarding|寄养");
    combo->addItem("接送", "Transport|接送");


    // 2. 日期选择
    CustomCalendarEdit *dateEdit = new CustomCalendarEdit();
    dateEdit->setFixedHeight(44);
    dateEdit->setFixedWidth(125); // 从 115 增加到 125
    dateEdit->setMinimumDate(QDate::currentDate());
    QDate defaultDate = QDate::currentDate();
    if (QTime::currentTime().hour() >= 20) defaultDate = defaultDate.addDays(1);
    dateEdit->setText(defaultDate.toString("yyyy-MM-dd"));
    dateEdit->setStyleSheet("QLineEdit { border: 1px solid #e2e8f0; border-radius: 10px; padding: 0 8px; background: #fcfcfd; font-size: 12px; color: #1e293b; }");

    // 3. 时间段选择
    QComboBox *hourC = new QComboBox();
    hourC->setFixedHeight(44);
    hourC->setFixedWidth(155); // 从 145 增加到 155
    hourC->setStyleSheet(
        "QComboBox { border: 1px solid #e2e8f0; border-radius: 10px; padding: 0 5px 0 8px; background: #fcfcfd; font-size: 13px; color: #1e293b; padding-right: 25px; } "
        "QComboBox:focus { border-color: #3b82f6; background: white; } "
        "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: center right; width: 20px; border: none; background: transparent; } "
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; } "
        "QComboBox QAbstractItemView { background: white; border: 1px solid #e2e8f0; outline: none; selection-background-color: #f1f5f9; selection-color: #3b82f6; }"
    );

    // 4. 金额输入 (新)
    QLineEdit *amtEdit = new QLineEdit();
    amtEdit->setFixedHeight(44);
    amtEdit->setFixedWidth(100);
    amtEdit->setPlaceholderText("金额");
    amtEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #e2e8f0; border-radius: 10px; padding: 0 8px; background: #fcfcfd; font-size: 13px; font-weight: bold; color: #f59e0b; } "
        "QLineEdit:focus { border-color: #3b82f6; background: white; }"
    );

    // 5. 删除按钮
    QLabel *delLabel = new QLabel("×");
    delLabel->setFixedSize(30, 30);
    delLabel->setCursor(Qt::PointingHandCursor);
    delLabel->setAlignment(Qt::AlignCenter);
    delLabel->setStyleSheet("QLabel { color: #94a3b8; font-size: 20px; font-weight: bold; background: transparent; } QLabel:hover { color: #ef4444; }");
    delLabel->setProperty("isDelBtn", true);
    delLabel->installEventFilter(this);

    // 寄养模式专用控件（默认隐藏）
    QLabel *boardingDateHint = new QLabel("入店:");
    boardingDateHint->setStyleSheet("color: #64748b; font-size: 12px; font-weight: bold;");
    boardingDateHint->setVisible(false);

    QLabel *boardingToLabel = new QLabel("→  离店:");
    boardingToLabel->setStyleSheet("color: #64748b; font-size: 12px; font-weight: bold;");
    boardingToLabel->setVisible(false);

    CustomCalendarEdit *boardingEndDateEdit = new CustomCalendarEdit();
    boardingEndDateEdit->setFixedHeight(44);
    boardingEndDateEdit->setFixedWidth(125); // 同样增加到 125
    boardingEndDateEdit->setMinimumDate(QDate::currentDate().addDays(1));
    boardingEndDateEdit->setText(QDate::currentDate().addDays(3).toString("yyyy-MM-dd"));
    boardingEndDateEdit->setStyleSheet("QLineEdit { border: 1px solid #e2e8f0; border-radius: 10px; padding: 0 8px; background: #fcfcfd; font-size: 12px; color: #1e293b; }");
    boardingEndDateEdit->setVisible(false);

    QLabel *boardingDaysLabel = new QLabel("共 3 天");
    boardingDaysLabel->setStyleSheet("color: #3b82f6; font-size: 13px; font-weight: bold;");
    boardingDaysLabel->setVisible(false);

    QLabel *roomHint = new QLabel("分配房间:");
    roomHint->setStyleSheet("color: #64748b; font-size: 12px; font-weight: bold;");
    roomHint->setVisible(false);

    QComboBox *roomCombo = new QComboBox();
    roomCombo->setFixedHeight(44);
    roomCombo->setFixedWidth(110);
    roomCombo->setStyleSheet(
        "QComboBox { border: 1px solid #e2e8f0; border-radius: 10px; padding: 0 8px; background: #fcfcfd; font-size: 12px; color: #1e293b; padding-right: 20px; } "
        "QComboBox:focus { border-color: #3b82f6; background: white; } "
        "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: center right; width: 18px; border: none; background: transparent; } "
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 10px; height: 10px; } "
        "QComboBox QAbstractItemView { border: 1px solid #e2e8f0; background: white; selection-background-color: #f1f5f9; selection-color: #3b82f6; outline: none; }"
    );
    roomCombo->setVisible(false);

    // --- 第一行：主选择行 (仅业务选择和删除) ---
    mainRow->addWidget(combo, 1);
    mainRow->addWidget(amtEdit);
    mainRow->addStretch();
    mainRow->addWidget(delLabel);
    itemLayout->addLayout(mainRow);

    // --- 第二行：动态原子服务区 ---
    QFrame *tagContainer = new QFrame();
    tagContainer->setObjectName("tagContainer");
    tagContainer->setAttribute(Qt::WA_StyledBackground);
    tagContainer->setStyleSheet("#tagContainer { background: transparent; border: none; }");
    QHBoxLayout *tagLayout = new QHBoxLayout(tagContainer);
    tagLayout->setContentsMargins(5, 0, 0, 0);
    tagLayout->setSpacing(6);
    QLabel *tagHint = new QLabel("附加项目:");
    tagHint->setStyleSheet("color: #94a3b8; font-size: 11px; font-weight: bold; background: transparent;");
    tagLayout->addWidget(tagHint);
    
    QFrame *tagsWrapper = new QFrame();
    tagsWrapper->setObjectName("tagsWrapper");
    tagsWrapper->setAttribute(Qt::WA_StyledBackground);
    tagsWrapper->setStyleSheet("#tagsWrapper { background: transparent; border: none; }");
    FlowLayout *tagsRow = new FlowLayout(tagsWrapper, 0, 8, 8);
    tagsRow->setContentsMargins(0, 0, 0, 0);
    tagsRow->setSpacing(6);
    tagLayout->addWidget(tagsWrapper, 1);
    
    itemLayout->addWidget(tagContainer);

    // --- 第三行：日期/时间选择区 (新) ---
    QHBoxLayout *dateRow = new QHBoxLayout();
    dateRow->setContentsMargins(5, 0, 0, 5);
    dateRow->setSpacing(8);

    QLabel *boardingStatusLabel = new QLabel("");
    boardingStatusLabel->setStyleSheet("font-size: 11px; font-weight: bold; padding: 2px 8px; border-radius: 4px;");
    boardingStatusLabel->setVisible(false);

    // 常规日期/时间标签（非寄养时可根据需要加标签，目前直接放控件）
    dateRow->addWidget(boardingDateHint);
    dateRow->addWidget(dateEdit);
    dateRow->addWidget(hourC);
    dateRow->addWidget(boardingToLabel);
    dateRow->addWidget(boardingEndDateEdit);
    dateRow->addWidget(boardingDaysLabel);
    dateRow->addWidget(roomHint);
    dateRow->addWidget(roomCombo);
    dateRow->addWidget(boardingStatusLabel);
    dateRow->addStretch();

    itemLayout->addLayout(dateRow);

    // --- 第四行：动态返程参数区 ---
    QWidget *returnParamWidget = new QWidget();
    returnParamWidget->setStyleSheet("background: transparent; border: none;");
    returnParamWidget->setVisible(false); // 默认隐藏
    QHBoxLayout *returnLayout = new QHBoxLayout(returnParamWidget);
    returnLayout->setContentsMargins(0, 0, 0, 5);
    returnLayout->setSpacing(8);

    QLabel *returnHint = new QLabel("预约返程时间:");
    returnHint->setStyleSheet("color: #64748b; font-size: 12px; font-weight: bold;");
    returnLayout->addWidget(returnHint);

    CustomCalendarEdit *retDateEdit = new CustomCalendarEdit();
    retDateEdit->setFixedHeight(44);
    retDateEdit->setFixedWidth(115);
    retDateEdit->setMinimumDate(QDate::currentDate());
    retDateEdit->setText(dateEdit->text());
    retDateEdit->setStyleSheet("QLineEdit { border: 1px solid #e2e8f0; border-radius: 10px; padding: 0 8px; background: #fcfcfd; font-size: 12px; color: #1e293b; }");

    QComboBox *retHourC = new QComboBox();
    retHourC->setFixedHeight(44);
    retHourC->setFixedWidth(145);
    retHourC->setStyleSheet(
        "QComboBox { border: 1px solid #e2e8f0; border-radius: 10px; padding: 0 5px 0 8px; background: #fcfcfd; font-size: 13px; color: #1e293b; padding-right: 25px; } "
        "QComboBox:focus { border-color: #3b82f6; background: white; } "
        "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: center right; width: 20px; border: none; background: transparent; } "
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; } "
        "QComboBox QAbstractItemView { border: 1px solid #e2e8f0; border-radius: 10px; background: white; selection-background-color: #f1f5f9; selection-color: #3b82f6; outline: none; padding: 5px; }"
    );
    QStringList returnSlots = {"09:00 - 11:00", "11:00 - 14:00", "14:00 - 16:00", "16:00 - 18:00", "18:00 - 21:00"};
    for (const auto &s : returnSlots) retHourC->addItem(s, s);
    retHourC->setCurrentIndex(3);

    returnLayout->addWidget(retDateEdit);
    returnLayout->addWidget(retHourC);
    returnLayout->addStretch();
    
    // --- 第五行：地址输入区 (新) ---
    QFrame *addressContainer = new QFrame();
    addressContainer->setObjectName("addressContainer");
    addressContainer->setVisible(false);
    QHBoxLayout *addressLayout = new QHBoxLayout(addressContainer);
    addressLayout->setContentsMargins(5, 0, 0, 5);
    addressLayout->setSpacing(8);

    QLabel *addressIcon = new QLabel("📍");
    addressIcon->setStyleSheet("font-size: 14px;");
    
    QLineEdit *addressEdit = new QLineEdit();
    addressEdit->setPlaceholderText("请输入详细接送地址...");
    addressEdit->setFixedHeight(40);
    addressEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #e2e8f0; border-radius: 8px; padding: 0 10px; background: #f8fafc; font-size: 12px; color: #1e293b; } "
        "QLineEdit:focus { border-color: #3b82f6; background: white; }"
    );
    
    addressLayout->addWidget(addressIcon);
    addressLayout->addWidget(addressEdit, 1);
    itemLayout->addWidget(addressContainer);

    // (寄养控件已内联到主行)

    auto updateFinalPrice = [=]() {
        double totalAmount = 0.0;
        auto allServices = ServiceDataManager::instance()->allServices();
        
        auto tagBtns = tagsWrapper->findChildren<QPushButton*>();
        for (auto btn : tagBtns) {
            if (btn->isChecked()) {
                QString name = btn->text();
                for (const auto &info : allServices) {
                    if (info.name == name) {
                        totalAmount += info.price;
                        break;
                    }
                }
            }
        }

        // 寄养业务逻辑：按天计费
        if (combo->currentText() == "寄养") {
            QDate startDate = QDate::fromString(dateEdit->text(), "yyyy-MM-dd");
            QDate endDate = QDate::fromString(boardingEndDateEdit->text(), "yyyy-MM-dd");
            int days = qMax(1, startDate.daysTo(endDate));
            totalAmount *= days;
        }

        amtEdit->setText(QString::number(totalAmount, 'f', 2));
    };

    // 寄养天数自动计算与房间库存检查
    auto updateBoardingDays = [=]() {
        QDate startDate = QDate::fromString(dateEdit->text(), "yyyy-MM-dd");
        QDate endDate = QDate::fromString(boardingEndDateEdit->text(), "yyyy-MM-dd");
        if (startDate.isValid() && endDate.isValid()) {
            int days = startDate.daysTo(endDate);
            if (days < 1) {
                boardingEndDateEdit->setText(startDate.addDays(1).toString("yyyy-MM-dd"));
                days = 1;
            }
            boardingDaysLabel->setText(QString("共 %1 天").arg(days));
            
            // 重要：同步更新价格
            updateFinalPrice();

            // 库存检查
            int maxOccupation = PetDataManager::instance()->getBoardingOccupation(startDate, endDate);
            int totalRooms = 10; 
            
            if (maxOccupation >= totalRooms) {
                boardingStatusLabel->setText("⚠ 房间已满");
                boardingStatusLabel->setStyleSheet("background: #fef2f2; color: #dc2626; border: 1px solid #fecaca; padding: 2px 8px; border-radius: 4px; font-size: 11px;");
            } else if (maxOccupation >= totalRooms * 0.8) {
                boardingStatusLabel->setText(QString("房源紧张 (余%1)").arg(totalRooms - maxOccupation));
                boardingStatusLabel->setStyleSheet("background: #fffbeb; color: #d97706; border: 1px solid #fde68a; padding: 2px 8px; border-radius: 4px; font-size: 11px;");
            } else {
                boardingStatusLabel->setText(QString("房源充沛 (余%1)").arg(totalRooms - maxOccupation));
                boardingStatusLabel->setStyleSheet("background: #f0fdf4; color: #16a34a; border: 1px solid #bbf7d0; padding: 2px 8px; border-radius: 4px; font-size: 11px;");
            }
        }
    };
    
    // 新增：更新可用房号列表
    auto updateAvailableRooms = [=]() {
        QDate startDate = QDate::fromString(dateEdit->text(), "yyyy-MM-dd");
        QDate endDate = QDate::fromString(boardingEndDateEdit->text(), "yyyy-MM-dd");
        if (startDate.isValid() && endDate.isValid()) {
            QString currentRoom = roomCombo->currentText();
            roomCombo->clear();
            
            // 确定房型过滤条件
            QString typeFilter = "";
            auto tags = tagsWrapper->findChildren<QPushButton*>();
            for (auto btn : tags) {
                if (btn->isChecked()) {
                    QString name = btn->text();
                    if (name.contains("豪华")) typeFilter = "豪华房";
                    else if (name.contains("多宠")) typeFilter = "多宠房";
                    else if (name.contains("普通")) typeFilter = "标准房";
                    break;
                }
            }

            QList<int> available = PetDataManager::instance()->getAvailableRooms(startDate, endDate, typeFilter);
            if (available.isEmpty()) {
                roomCombo->addItem(typeFilter.isEmpty() ? "无空房" : QString("无%1空闲").arg(typeFilter), "");
            } else {
                for (int rid : available) {
                    BoardingRoom r = PetDataManager::instance()->getRoom(rid);
                    QString display = r.roomNo.isEmpty() ? QString::number(rid) : r.roomNo;
                    roomCombo->addItem(QString("%1号房").arg(display), QString::number(rid));
                }
            }
            
            int idx = roomCombo->findText(currentRoom);
            if (idx != -1) roomCombo->setCurrentIndex(idx);
        }
    };
    
    connect(dateEdit, &QLineEdit::textChanged, this, updateBoardingDays);
    connect(boardingEndDateEdit, &QLineEdit::textChanged, this, updateBoardingDays);
    connect(dateEdit, &QLineEdit::textChanged, this, updateAvailableRooms);
    connect(boardingEndDateEdit, &QLineEdit::textChanged, this, updateAvailableRooms);

    auto updateAtomicTags = [=](const QString &mainType) {
        QLayoutItem *child;
        while ((child = tagsRow->takeAt(0)) != nullptr) {
            if (child->widget()) {
                child->widget()->hide();
                delete child->widget(); // 立即删除，确保同步
            }
            delete child;
        }

        // 动态加载：从数据管理器获取该分类下的所有服务
        QString cat = mainType.split("|").last();
        QList<ServiceInfo> allServices = ServiceDataManager::instance()->allServices();
        
        for (const auto &info : allServices) {
            if (!info.isActive) continue; // 过滤已下架的服务项目
            if (info.category == cat) {
                QPushButton *tagBtn = new QPushButton(info.name);
                tagBtn->setCheckable(true);
                tagBtn->setCursor(Qt::PointingHandCursor);
                tagBtn->setStyleSheet(
                    "QPushButton { background: #f1f5f9; border: 1px solid #e2e8f0; border-radius: 6px; padding: 4px 10px; color: #64748b; font-size: 11px; font-weight: 600; } "
                    "QPushButton:hover { background: #e2e8f0; } "
                    "QPushButton:checked { background: #eff6ff; border-color: #3b82f6; color: #3b82f6; }"
                );
                
                // 互斥与价格联动逻辑
                connect(tagBtn, &QPushButton::clicked, this, [=](bool checked) {
                    if (checked) {
                        QString name = tagBtn->text();
                        QString cat = combo->currentText();
                        
                        // 1. 寄养分类：所有项目绝对互斥
                        if (cat == "寄养") {
                            auto allBtns = tagsWrapper->findChildren<QPushButton*>();
                            for (auto other : allBtns) {
                                if (other != tagBtn) other->setChecked(false);
                            }
                        } 
                        // 2. 接送分类：互斥逻辑
                        else if (cat == "接送") {
                            QStringList transExcl = {"单程接宠入店", "单程送宠回家", "往返接送服务"};
                            if (transExcl.contains(name)) {
                                auto otherBtns = tagsWrapper->findChildren<QPushButton*>();
                                for (auto other : otherBtns) {
                                    if (other != tagBtn && transExcl.contains(other->text())) {
                                        other->setChecked(false);
                                    }
                                }
                            }
                        }
                        // 3. 洗护/美容/保健：部分核心项目互斥
                        else {
                            QMap<QString, QStringList> exclusionGroups;
                            exclusionGroups["洗护"] << "基础三项护理" << "全身深度洗护" << "专业除臭洗护";
                            exclusionGroups["美容"] << "全身推子造型" << "全手剪精修造型";

                            if (exclusionGroups.contains(cat)) {
                                const QStringList &exclList = exclusionGroups[cat];
                                if (exclList.contains(name)) {
                                    auto otherBtns = tagsWrapper->findChildren<QPushButton*>();
                                    for (auto other : otherBtns) {
                                        if (other != tagBtn && exclList.contains(other->text())) {
                                            other->setChecked(false);
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // 联动控制：如果任意接/送相关的标签被选中，显示地址栏
                    bool needsAddress = false;
                    auto allBtns = tagsWrapper->findChildren<QPushButton*>();
                    for (auto btn : allBtns) {
                        if (btn->isChecked() && (btn->text().contains("接") || btn->text().contains("送"))) {
                            needsAddress = true;
                            break;
                        }
                    }
                    addressContainer->setVisible(needsAddress);

                    if (combo->currentText() == "寄养") {
                        updateAvailableRooms();
                    }
                    updateFinalPrice();
                });
                
                // 特殊逻辑：如果是“送回入户”，联动显示返程时间选择器
                if (info.name == "送回入户") {
                    connect(tagBtn, &QPushButton::toggled, returnParamWidget, &QWidget::setVisible);
                }
                
                tagsRow->addWidget(tagBtn);
            }
        }
    };


    auto updateSlots = [=]() {
        QString current = hourC->currentData().toString();
        hourC->clear();
        QStringList timeSlots = {"09:00 - 11:00", "11:00 - 14:00", "14:00 - 16:00", "16:00 - 18:00", "18:00 - 21:00"};
        QTime now = QTime::currentTime();
        bool isToday = (QDate::fromString(dateEdit->text(), "yyyy-MM-dd") == QDate::currentDate());

        for (const QString &slot : timeSlots) {
            if (isToday) {
                QTime startTime = QTime::fromString(slot.left(5), "HH:mm");
                if (now > startTime) continue; 
            }
            hourC->addItem(slot, slot);
        }
        
        if (hourC->count() == 0 && isToday) {
            dateEdit->setText(QDate::currentDate().addDays(1).toString("yyyy-MM-dd"));
            return;
        }

        // 尝试恢复之前选中的值，或者默认选中第一个
        int idx = hourC->findData(current);
        if (idx != -1) {
            hourC->setCurrentIndex(idx);
        } else if (hourC->count() > 0) {
            hourC->setCurrentIndex(0);
        }
    };

    connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index){
        QString typeKey = combo->itemData(index).toString().split("|").first();
        updateAtomicTags(combo->itemData(index).toString());
        updateFinalPrice(); // 更新价格
        
        // 寄养模式切换：隐藏时间槽，显示日期范围
        bool isBoarding = (typeKey == "Boarding");
        hourC->setVisible(!isBoarding);
        boardingDateHint->setVisible(isBoarding);
        boardingToLabel->setVisible(isBoarding);
        boardingEndDateEdit->setVisible(isBoarding);
        boardingDaysLabel->setVisible(isBoarding);
        roomHint->setVisible(isBoarding);
        roomCombo->setVisible(isBoarding);
        boardingStatusLabel->setVisible(isBoarding);
        
        if (isBoarding) {
            updateBoardingDays(); 
            updateAvailableRooms();
        } else {
            updateSlots(); // 切换回常规模式时，必须刷新时间槽
        }
    });
    
    // 初始触发一次
    if (combo->currentIndex() >= 0) {
        updateAtomicTags(combo->currentData().toString());
    }

    updateSlots();
    connect(dateEdit, &QLineEdit::textChanged, updateSlots);

    int insertIndex = m_serviceRows.count();
    m_serviceContainer->insertWidget(insertIndex, rowWidget);

    ServiceRowItems items;
    items.serviceCombo = combo;
    items.dateEdit = dateEdit;
    items.hourCombo = hourC;
    items.tagsContainer = tagsWrapper;
    items.returnParamWidget = returnParamWidget;
    items.returnDateEdit = retDateEdit;
    items.returnHourCombo = retHourC;
    items.boardingEndDateEdit = boardingEndDateEdit;
    items.boardingDaysLabel = boardingDaysLabel;
    items.boardingStatusLabel = boardingStatusLabel;
    items.roomHint = roomHint;
    items.roomCombo = roomCombo;
    items.amountEdit = amtEdit;
    items.addressEdit = addressEdit;
    items.container = rowWidget;
    delLabel->setProperty("rowPtr", QVariant::fromValue((void*)rowWidget));
    m_serviceRows.append(items);

    // 初始计算一次价格
    updateFinalPrice();
}

QList<AppointmentInfo> AddAppointmentDialog::getAppointmentInfos() const
{
    QList<AppointmentInfo> list;
    QString petId = m_petCombo->currentData().toString();
    if (petId.isEmpty()) return list;

    PetInfo pet = PetDataManager::instance()->getPet(petId);
    QString notes = m_notesEdit->toPlainText();
    
    // 生成这批预约的统一 Group ID
    QString currentGroupId = "GRP" + QString::number(QDateTime::currentMSecsSinceEpoch());

    for (const auto &row : m_serviceRows) {
        AppointmentInfo info;
        info.id = "AP" + QString::number(QDateTime::currentMSecsSinceEpoch()).right(6) + QString::number(QRandomGenerator::global()->bounded(100));
        info.groupId = currentGroupId;
        info.petId = petId;
        info.petName = pet.name;
        info.petAvatar = pet.avatarPath;
        info.breed = pet.breed;
        info.memberName = pet.ownerName;
        info.memberPhone = pet.ownerPhone;
        
        // 统一名称：使用下拉框显示的文字（如“专业美容”）作为业务名
        QString categoryName = row.serviceCombo->currentText();
        QStringList selectedTags;
        bool hasReturnTrip = false;
        auto tags = row.tagsContainer->findChildren<QPushButton*>();
        for (auto btn : tags) {
            if (btn->isChecked()) {
                selectedTags.append(btn->text());
                if (btn->text() == "送回入户") hasReturnTrip = true;
            }
        }
        
        info.service = categoryName;
        if (!selectedTags.isEmpty()) {
            info.service += QString(" (%1)").arg(selectedTags.join(", "));
        }
        
        info.type = row.serviceCombo->currentData().toString().split("|").first();
        info.date = static_cast<CustomCalendarEdit*>(row.dateEdit)->text();
        info.hour = row.hourCombo->currentData().toString();
        info.notes = notes;
        info.status = "Pending";
        if (info.type == "Transport") {
            info.address = row.addressEdit->text();
            if (info.address.isEmpty()) info.address = pet.address; // 兜底使用宠物登记地址
        }
        info.amount = row.amountEdit->text().toDouble();
        
        // 寄养特殊处理：使用日期范围代替时间槽
        if (info.type == "Boarding" && row.boardingEndDateEdit) {
            info.boardingEndDate = static_cast<CustomCalendarEdit*>(row.boardingEndDateEdit)->text();
            QDate startD = QDate::fromString(info.date, "yyyy-MM-dd");
            QDate endD = QDate::fromString(info.boardingEndDate, "yyyy-MM-dd");
            info.duration = startD.daysTo(endD);
            info.hour = ""; // 寄养不需要时间槽
            if (row.roomCombo) info.roomNo = row.roomCombo->currentData().toString();
        }
        
        list.append(info);
        
        // 如果勾选了“送回入户”，自动生成第二条预约记录（返程）
        if (hasReturnTrip) {
            AppointmentInfo retInfo = info;
            retInfo.id = "AP" + QString::number(QDateTime::currentMSecsSinceEpoch() + 1).right(6) + QString::number(QRandomGenerator::global()->bounded(100));
            retInfo.service = "送宠回程 (预约返程)";
            retInfo.type = "Transport";
            retInfo.date = static_cast<CustomCalendarEdit*>(row.returnDateEdit)->text();
            retInfo.hour = row.returnHourCombo->currentData().toString();
            retInfo.notes = "系统自动生成返程单: " + notes;
            list.append(retInfo);
        }
    }
    return list;
}

void AddAppointmentDialog::setInitialData(const AppointmentInfo &info) {
    if (info.id.isEmpty() || m_serviceRows.isEmpty()) return;

    // 禁用添加业务按钮（修改场景默认单条修改）
    m_addServiceBtn->setVisible(false);

    // 设置备注 (显式设置并确保非空)
    m_notesEdit->setPlainText(info.notes);
    
    // 设置宠物 (不再禁用，保持与新增界面视觉一致)
    int petIdx = m_petCombo->findData(info.petId);
    if (petIdx >= 0) m_petCombo->setCurrentIndex(petIdx);

    // 填充第一条业务线
    auto &row = m_serviceRows.first();
    
    // 定位业务类型
    for (int i = 0; i < row.serviceCombo->count(); ++i) {
        if (row.serviceCombo->itemData(i).toString().startsWith(info.type + "|")) {
            row.serviceCombo->setCurrentIndex(i);
            break;
        }
    }

    // 设置日期
    static_cast<CustomCalendarEdit*>(row.dateEdit)->setText(info.date);
    
    // 设置时间槽
    // 需要先触发 updateSlots 让组合框填充当前日期的可用时间段
    row.hourCombo->clear();
    QStringList timeSlots = {"09:00 - 11:00", "11:00 - 14:00", "14:00 - 16:00", "16:00 - 18:00", "18:00 - 21:00"};
    for (const auto &s : timeSlots) {
        row.hourCombo->addItem(s, s);
    }
    int hourIdx = row.hourCombo->findData(info.hour);
    if (hourIdx >= 0) row.hourCombo->setCurrentIndex(hourIdx);

    // 设置选中的原子服务标签
    // 此时 updateAtomicTags 已经因为 currentIndexChanged 同步执行完毕
    QList<QPushButton*> allTags = row.tagsContainer->findChildren<QPushButton*>();
    QString serviceStr = info.service;
    for (auto btn : allTags) {
        if (serviceStr.contains(btn->text(), Qt::CaseInsensitive)) {
            btn->setChecked(true);
        }
    }
}
