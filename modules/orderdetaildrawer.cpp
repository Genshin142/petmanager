#include "orderdetaildrawer.h"
#include "petdatamanager.h"
#include "productdatamanager.h"
#include "custommessagedialog.h"
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QApplication>
#include <QLineEdit>
#include <QDateTime>
#include <QMouseEvent>
#include <QPainterPath>

OrderDetailDrawer::OrderDetailDrawer(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground); // Allow QSS to style the background
    setupUI();
}

void OrderDetailDrawer::setupUI()
{
    setFixedWidth(470); // 略微增加总宽度以容纳外边距
    // 父容器保持透明
    setStyleSheet("background: transparent; border: none;");

    // 移除阴影效果，改为使用边框
    this->setGraphicsEffect(nullptr);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    // 增加四周外边距，确保右侧和上下的圆角/边框不会被父容器裁剪
    mainLayout->setContentsMargins(10, 20, 20, 20); 
    mainLayout->setSpacing(0);

    m_stack = new QStackedWidget();
    m_stack->setStyleSheet("background: transparent; border: none;");

    // --- 1. Detail Widget ---
    m_detailWidget = new QWidget();
    m_detailWidget->setObjectName("detailCard");
    // 使用 #detailCard 定向设置样式，避免边框被子控件继承
    m_detailWidget->setStyleSheet(
        "QWidget#detailCard { "
        "   background: white; "
        "   border-radius: 24px; "
        "   border: none; "
        "}"
    );
    QVBoxLayout *detailLayout = new QVBoxLayout(m_detailWidget);
    detailLayout->setContentsMargins(0, 0, 0, 0);
    detailLayout->setSpacing(0);

    // --- Header (Pet/Customer Profile) ---
    QWidget *header = new QWidget();
    header->setMinimumHeight(140);
    header->setStyleSheet("background: transparent; border: none;");
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(15, 20, 100, 15); // Large right margin for button area
    headerLayout->setSpacing(12);

    m_petAvatar = new QLabel();
    m_petAvatar->setFixedSize(70, 70); // Slightly smaller to gain width
    m_petAvatar->setCursor(Qt::PointingHandCursor);
    m_petAvatar->setStyleSheet("background: transparent; border: none;"); // Styling via painter
    m_petAvatar->setAlignment(Qt::AlignCenter);
    m_petAvatar->installEventFilter(this);
    headerLayout->addWidget(m_petAvatar);

    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(8); // More breathing room
    infoLayout->setAlignment(Qt::AlignVCenter);
    
    m_petInfoLabel = new QLabel("加载中...");
    m_petInfoLabel->setStyleSheet("font-size: 20px; font-weight: 800; color: #0f172a; border: none; font-family: 'Microsoft YaHei';");
    
    m_memberNameLabel = new QLabel("会员: --");
    m_memberNameLabel->setStyleSheet("font-size: 15px; color: #1e293b; font-weight: 600; border: none; font-family: 'Microsoft YaHei';");
    
    infoLayout->addWidget(m_petInfoLabel);
    infoLayout->addWidget(m_memberNameLabel);
    headerLayout->addLayout(infoLayout, 1);

    m_cancelOrderBtn = new QPushButton("作废订单");
    m_cancelOrderBtn->setCursor(Qt::PointingHandCursor);
    m_cancelOrderBtn->setMinimumWidth(80);
    m_cancelOrderBtn->setStyleSheet(
        "QPushButton { "
        "  color: #f43f5e; "
        "  border: 1px solid #fecaca; "
        "  background: #fff1f2; "
        "  border-radius: 6px; "
        "  font-size: 11px; "
        "  font-weight: 700; "
        "  padding: 5px 10px; "
        "  font-family: 'Microsoft YaHei'; "
        "} "
        "QPushButton:hover { "
        "  background: #ffe4e6; "
        "  border-color: #f43f5e; "
        "} "
        "QPushButton:pressed { "
        "  background: #fecaca; "
        "}"
    );
    connect(m_cancelOrderBtn, &QPushButton::clicked, this, [this](){
        if (CustomMessageDialog::confirm(this, "重要提示", "确认作废此订单吗？\n作废后关联业务单将恢复为待处理状态。")) {
            PetDataManager::instance()->cancelOrder(m_order.id, "收银员手动取消");
            emit orderCancelled(m_order.id, "手动作废");
        }
    });
    
    // Position button absolutely with FIXED size to prevent it from ballooning
    m_cancelOrderBtn->setParent(header);
    m_cancelOrderBtn->setFixedSize(75, 26);
    m_cancelOrderBtn->move(355, 20); // Relative to 450 width
    
    detailLayout->addWidget(header);

    // --- Bill List ---
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet(
        "QScrollArea { border: none; background: transparent; } "
        "QScrollBar:vertical { border: none; background: #f8fafc; width: 6px; margin: 0; } "
        "QScrollBar::handle:vertical { background: #e2e8f0; border-radius: 3px; min-height: 20px; } "
        "QScrollBar::handle:vertical:hover { background: #cbd5e1; } "
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; } "
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
    );
    m_itemsContainer = new QWidget();
    m_itemsContainer->setStyleSheet("background: transparent;");
    m_itemsLayout = new QVBoxLayout(m_itemsContainer);
    m_itemsLayout->setContentsMargins(25, 20, 25, 20);
    m_itemsLayout->setSpacing(0);
    scroll->setWidget(m_itemsContainer);
    detailLayout->addWidget(scroll, 1);

    // --- Footer ---
    QWidget *footer = new QWidget();
    footer->setStyleSheet("background: transparent; border-top: 1px solid #f1f5f9;");
    QVBoxLayout *footerLayout = new QVBoxLayout(footer);
    footerLayout->setContentsMargins(30, 25, 30, 30);
    footerLayout->setSpacing(20);

    QHBoxLayout *totalRow = new QHBoxLayout();
    QLabel *totalTitle = new QLabel("应付总额");
    totalTitle->setStyleSheet("font-size: 14px; font-weight: 800; color: #475569; border: none; font-family: 'Microsoft YaHei';");
    m_finalAmountLabel = new QLabel("¥ 0.00");
    m_finalAmountLabel->setStyleSheet("font-size: 34px; font-weight: 800; color: #3b82f6; border: none; font-family: 'Microsoft YaHei';");
    totalRow->addWidget(totalTitle);
    totalRow->addStretch();
    totalRow->addWidget(m_finalAmountLabel);
    footerLayout->addLayout(totalRow);

    QGridLayout *payGrid = new QGridLayout();
    payGrid->setSpacing(12);
    QStringList methods = {"银行卡", "现金", "支付宝", "微信支付", "会员卡余额", "其他"};
    for (int i = 0; i < methods.size(); ++i) {
        QPushButton *btn = new QPushButton(methods[i]);
        btn->setCheckable(true);
        btn->setFixedHeight(46);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { background: white; border: 1px solid #e2e8f0; border-radius: 8px; color: #475569; font-weight: 700; font-size: 13px; font-family: 'Microsoft YaHei'; } "
            "QPushButton:hover { background: #f8fafc; border-color: #3b82f6; } "
            "QPushButton:checked { background: #eff6ff; border-color: #3b82f6; color: #3b82f6; }"
        );
        payGrid->addWidget(btn, i / 2, i % 2);
        m_payMethodButtons.append(btn);
        connect(btn, &QPushButton::clicked, this, [this, btn](){
            for (auto b : m_payMethodButtons) if (b != btn) b->setChecked(false);
        });
    }
    footerLayout->addLayout(payGrid);

    m_confirmBtn = new QPushButton("确认结算");
    m_confirmBtn->setFixedHeight(54);
    m_confirmBtn->setCursor(Qt::PointingHandCursor);
    m_confirmBtn->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; border-radius: 10px; font-size: 16px; font-weight: 800; border: none; font-family: 'Microsoft YaHei'; } "
        "QPushButton:hover { background: #2563eb; } "
        "QPushButton:pressed { background: #1d4ed8; }"
        "QPushButton:disabled { background: #e2e8f0; color: #94a3b8; }"
    );
    connect(m_confirmBtn, &QPushButton::clicked, this, [this](){
        QString method;
        for (auto b : m_payMethodButtons) if (b->isChecked()) method = b->text();
        if (method.isEmpty()) {
            CustomMessageDialog::showWarning(this, "收银操作", "请先选择支付方式！");
            return;
        }
        m_order.status = "Paid";
        m_order.payMethod = method;
        m_order.finalAmount = m_order.totalAmount;
        PetDataManager::instance()->updateOrder(m_order);
        emit settlementConfirmed(m_order);
    });
    footerLayout->addWidget(m_confirmBtn);
    detailLayout->addWidget(footer);

    // --- 2. Empty Widget ---
    m_emptyWidget = new QWidget();
    m_emptyWidget->setObjectName("emptyCard");
    m_emptyWidget->setStyleSheet(
        "QWidget#emptyCard { "
        "   background: white; "
        "   border-radius: 24px; "
        "   border: none; "
        "}"
    );
    QVBoxLayout *emptyLayout = new QVBoxLayout(m_emptyWidget);
    emptyLayout->setAlignment(Qt::AlignCenter);
    emptyLayout->setSpacing(20);
    QLabel *emptyIcon = new QLabel("📄");
    emptyIcon->setStyleSheet("font-size: 64px; color: #e4e7ed;");
    QLabel *emptyText = new QLabel("暂无选中订单\n请在左侧列表选择订单进行结算");
    emptyText->setAlignment(Qt::AlignCenter);
    emptyText->setStyleSheet("color: #c0c4cc; font-size: 14px; line-height: 1.6; font-family: 'Microsoft YaHei';");
    emptyLayout->addWidget(emptyIcon);
    emptyLayout->addWidget(emptyText);

    m_stack->addWidget(m_detailWidget);
    m_stack->addWidget(m_emptyWidget);
    mainLayout->addWidget(m_stack);
    showEmptyState();
}

void OrderDetailDrawer::setOrder(const OrderInfo &order)
{
    m_order = order;
    m_stack->setCurrentWidget(m_detailWidget);
    updateUI();
}

void OrderDetailDrawer::showEmptyState()
{
    m_stack->setCurrentWidget(m_emptyWidget);
}

void OrderDetailDrawer::updateUI()
{
    bool isProduct = (m_order.sourceModule == "Product");
    int avatarSize = m_petAvatar->width();
    QPixmap target(avatarSize, avatarSize);
    target.fill(Qt::transparent);
    QPainter painter(&target);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    // 主标题：会员信息
    QString memberTitle = QString("%1 %2").arg(m_order.memberName).arg(m_order.memberId.isEmpty() ? "临时客" : m_order.memberId);
    m_petInfoLabel->setText(memberTitle);

    if (isProduct) {
        // 商品订单：极致简化，隐藏副标题，仅保留主标题（会员名）
        m_memberNameLabel->setVisible(false);
        m_avatarPathForPreview = "C:\\Users\\任坤\\.gemini\\antigravity\\brain\\bb7984ab-9f62-43f3-aed8-a8540dde0c4b\\generic_product_placeholder_1777719930887.png";
        
        QPixmap pix(m_avatarPathForPreview);
        if (!pix.isNull()) {
            QPainterPath path;
            path.addEllipse(2, 2, avatarSize - 4, avatarSize - 4);
            painter.setClipPath(path);
            painter.drawPixmap(2, 2, avatarSize - 4, avatarSize - 4, pix);
            
            painter.setClipping(false);
            painter.setPen(QPen(QColor("#e2e8f0"), 2));
            painter.drawEllipse(1, 1, avatarSize - 2, avatarSize - 2);
        } else {
            painter.setBrush(QBrush(QColor("#fff7ed")));
            painter.setPen(QPen(QColor("#ffedd5"), 1));
            painter.drawEllipse(2, 2, avatarSize - 4, avatarSize - 4);
            painter.setPen(Qt::black);
            QFont f = painter.font(); f.setPixelSize(36); painter.setFont(f);
            painter.drawText(target.rect(), Qt::AlignCenter, "📦");
        }
    } else {
        // 宠物服务：恢复并显示宠物信息
        m_memberNameLabel->setVisible(true);
        PetInfo pet = PetDataManager::instance()->getPet(m_order.petId);
        m_memberNameLabel->setText(QString("宠物: %1 %2 | %3")
            .arg(m_order.petName)
            .arg(m_order.petId)
            .arg(pet.breed.isEmpty() ? "未知品种" : pet.breed));
        m_memberNameLabel->setStyleSheet("font-size: 15px; color: #1e293b; font-weight: 600; border: none; font-family: 'Microsoft YaHei';");
        
        m_avatarPathForPreview = pet.avatarPath;
        if (!pet.avatarPath.isEmpty()) {
            QPixmap pix(pet.avatarPath);
            if (!pix.isNull()) {
                QPainterPath path;
                path.addEllipse(2, 2, avatarSize - 4, avatarSize - 4);
                painter.setClipPath(path);
                QPixmap scaled = pix.scaled(avatarSize - 4, avatarSize - 4, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                painter.drawPixmap(2 + (avatarSize - 4 - scaled.width())/2, 2 + (avatarSize - 4 - scaled.height())/2, scaled);
                
                painter.setClipping(false);
                painter.setPen(QPen(QColor("#e2e8f0"), 2));
                painter.drawEllipse(1, 1, avatarSize - 2, avatarSize - 2);
            }
        } else {
            painter.setBrush(QBrush(Qt::white));
            painter.setPen(QPen(QColor("#e2e8f0"), 1));
            painter.drawEllipse(2, 2, avatarSize - 4, avatarSize - 4);
            
            painter.setPen(Qt::black);
            QFont f = painter.font(); f.setPixelSize(32); painter.setFont(f);
            painter.drawText(target.rect(), Qt::AlignCenter, "🐾");
        }
    }
    m_petAvatar->setPixmap(target);
    
    // Clear items
    QLayoutItem *child;
    while ((child = m_itemsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) delete child->widget();
        delete child;
    }

    // Add items helper (增加图片参数)
    auto addItem = [&](const QString &name, const QString &subtitle, double price, const QString &extra = "", const QString &iconPath = "") {
        QWidget *itemW = new QWidget();
        itemW->setStyleSheet("background: white;");
        QHBoxLayout *rowLayout = new QHBoxLayout(itemW);
        rowLayout->setContentsMargins(0, 15, 0, 15);
        rowLayout->setSpacing(15);

        // 如果是商品，显示缩略图
        if (isProduct) {
            QLabel *img = new QLabel();
            img->setFixedSize(48, 48);
            img->setStyleSheet("background: #f8fafc; border-radius: 6px; border: 1px solid #f1f5f9;");
            img->setAlignment(Qt::AlignCenter);
            if (!iconPath.isEmpty()) {
                QPixmap p(iconPath);
                if (!p.isNull()) img->setPixmap(p.scaled(44, 44, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            } else {
                img->setText("📦");
            }
            img->setCursor(Qt::PointingHandCursor);
            img->installEventFilter(this);
            img->setProperty("isProductIcon", true);
            img->setProperty("iconPath", iconPath);
            rowLayout->addWidget(img);
        }

        QVBoxLayout *textLayout = new QVBoxLayout();
        textLayout->setSpacing(4);

        QHBoxLayout *topRow = new QHBoxLayout();
        QLabel *nameLbl = new QLabel(name);
        nameLbl->setStyleSheet("font-size: 15px; font-weight: 700; color: #1e293b; font-family: 'Microsoft YaHei';");
        QLabel *priceLbl = new QLabel(QString("¥ %1").arg(price, 0, 'f', 2));
        priceLbl->setStyleSheet("font-size: 18px; font-weight: 800; color: #0f172a; font-family: 'Microsoft YaHei';");
        topRow->addWidget(nameLbl);
        topRow->addStretch();
        topRow->addWidget(priceLbl);

        QLabel *subLbl = new QLabel(subtitle);
        subLbl->setStyleSheet("font-size: 12px; color: #64748b; font-weight: 500; font-family: 'Microsoft YaHei';");

        textLayout->addLayout(topRow);
        textLayout->addWidget(subLbl);
        
        if (!extra.isEmpty()) {
            QLabel *exLbl = new QLabel(extra);
            exLbl->setStyleSheet("font-size: 11px; color: #94a3b8; font-family: 'Consolas', 'Microsoft YaHei';");
            textLayout->addWidget(exLbl);
        }

        rowLayout->addLayout(textLayout, 1);

        QFrame *line = new QFrame();
        line->setFixedHeight(1);
        line->setStyleSheet("background-color: transparent; border: none; border-top: 1px dashed #e2e8f0;");
        m_itemsLayout->addWidget(itemW);
        m_itemsLayout->addWidget(line);
    };

    if (isProduct) {
        // 智能合并与计价逻辑
        QStringList rawItems = m_order.itemDetails.split("+", Qt::SkipEmptyParts);
        QMap<QString, int> itemGroups;
        for (const QString &raw : rawItems) {
            QString name = raw.trimmed();
            itemGroups[name]++;
        }

        // 获取商品价格与图片映射 (用于精准计价和显示图标)
        QMap<QString, double> priceMap;
        QMap<QString, QString> imageMap;
        auto allProducts = ProductDataManager::instance()->allProducts();
        for (const auto &p : allProducts) {
            priceMap[p.name] = p.price;
            if (!p.images.isEmpty()) imageMap[p.name] = p.images[0];
        }

        // 罗列合并后的项目
        for (auto it = itemGroups.begin(); it != itemGroups.end(); ++it) {
            QString name = it.key();
            int count = it.value();
            double unitPrice = priceMap.value(name, 0.0);
            QString imgPath = imageMap.value(name, ""); // 核心修复：获取商品实拍图路径
            
            // 如果价格库里没找到，且该订单只有这一种商品，则用订单总额平摊
            if (unitPrice <= 0 && itemGroups.size() == 1) {
                unitPrice = m_order.totalAmount / count;
            }

            double lineTotal = unitPrice * count;
            QString displayName = (count > 1) ? QString("%1 × %2").arg(name).arg(count) : name;
            
            // 显示逻辑：主标题显示“品名 × 数量”，价格显示“折后总价”，并传入实拍图路径
            addItem(displayName, "", lineTotal, "", imgPath);
        }
    } else {
        addItem(m_order.itemDetails, "业务项目结算", m_order.totalAmount);
    }
    
    m_itemsLayout->addStretch();
    m_finalAmountLabel->setText(QString("¥ %1").arg(m_order.totalAmount, 0, 'f', 2));
    
    // 支付状态处理
    bool isUnpaid = (m_order.status == "Unpaid");
    m_confirmBtn->setEnabled(isUnpaid);
    m_cancelOrderBtn->setVisible(isUnpaid);

    // 处理支付方式按钮的状态
    for (auto b : m_payMethodButtons) {
        b->setChecked(false);
        // 如果已支付，选中记录中的支付方式，并禁用按钮防止修改
        if (!isUnpaid && b->text() == m_order.payMethod) {
            b->setChecked(true);
        }
        b->setEnabled(isUnpaid); // 只有未支付时才能选择
    }
    
    if (!isUnpaid) {
        m_confirmBtn->setText("已结算 (" + m_order.payMethod + ")");
        m_confirmBtn->setStyleSheet("QPushButton { background: #dcfce7; color: #166534; border-radius: 10px; font-size: 16px; font-weight: 800; border: none; font-family: 'Microsoft YaHei'; }");
    } else {
        m_confirmBtn->setText("确认结算");
        m_confirmBtn->setStyleSheet(
            "QPushButton { background: #3b82f6; color: white; border-radius: 10px; font-size: 16px; font-weight: 800; border: none; font-family: 'Microsoft YaHei'; } "
            "QPushButton:hover { background: #2563eb; }"
        );
    }
}

bool OrderDetailDrawer::eventFilter(QObject *obj, QEvent *event)
{
    // 处理头像或商品图标点击放大
    bool isAvatar = (obj == m_petAvatar);
    bool isProdIcon = obj->property("isProductIcon").toBool();

    if ((isAvatar || isProdIcon) && event->type() == QEvent::MouseButtonRelease) {
        QString pathToShow = isAvatar ? m_avatarPathForPreview : obj->property("iconPath").toString();
        // 如果路径为空（例如默认包裹图标），则不放大或显示占位
        if (pathToShow.isEmpty() && isProdIcon) pathToShow = m_avatarPathForPreview; 

        QWidget *mainWin = nullptr;
        for (QWidget *w : QApplication::topLevelWidgets()) {
            if (w->objectName() == "MainWindow" || w->inherits("QMainWindow")) {
                mainWin = w;
                break;
            }
        }
        if (!mainWin) mainWin = this->window();

        QDialog *preview = new QDialog(this, Qt::FramelessWindowHint);
        preview->setGeometry(mainWin->geometry());
        preview->setAttribute(Qt::WA_TranslucentBackground);
        
        QVBoxLayout *layout = new QVBoxLayout(preview);
        layout->setContentsMargins(0, 0, 0, 0);
        
        QFrame *bg = new QFrame();
        bg->setStyleSheet("background-color: rgba(0, 0, 0, 215);");
        layout->addWidget(bg);
        
        QVBoxLayout *bgLayout = new QVBoxLayout(bg);
        bgLayout->setAlignment(Qt::AlignCenter);
        
        QLabel *imgLabel = new QLabel();
        int maxDim = qMin(mainWin->width(), mainWin->height()) * 0.8;
        
        if (!pathToShow.isEmpty()) {
            QPixmap pix(pathToShow);
            if (!pix.isNull()) {
                imgLabel->setPixmap(pix.scaled(maxDim, maxDim, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        } else {
            // Emoji or Placeholder
            QPixmap pix(maxDim, maxDim);
            pix.fill(Qt::white);
            QPainter p(&pix);
            p.setRenderHint(QPainter::Antialiasing);
            QFont f = p.font();
            f.setPixelSize(maxDim * 0.5);
            p.setFont(f);
            p.drawText(pix.rect(), Qt::AlignCenter, m_petAvatar->text());
            p.end();
            imgLabel->setPixmap(pix);
        }
        
        bgLayout->addWidget(imgLabel);
        
        bg->setCursor(Qt::PointingHandCursor);
        bg->installEventFilter(this);
        bg->setProperty("isPreviewBg", true);
        bg->setProperty("previewDlg", QVariant::fromValue((void*)preview));
        
        connect(preview, &QDialog::finished, preview, &QDialog::deleteLater);
        preview->show();
        return true;
    }
    
    if (obj->property("isPreviewBg").toBool()) {
        if (event->type() == QEvent::MouseButtonPress) {
            void* ptr = qvariant_cast<void*>(obj->property("previewDlg"));
            if (ptr) {
                static_cast<QDialog*>(ptr)->close();
                return true;
            }
        }
    }
    
    return QWidget::eventFilter(obj, event);
}

void OrderDetailDrawer::paintEvent(QPaintEvent *event) { QWidget::paintEvent(event); }
