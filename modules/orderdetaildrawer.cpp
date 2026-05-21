#include "orderdetaildrawer.h"
#include "petdatamanager.h"
#include "productdatamanager.h"
#include "memberdatamanager.h"
#include "addmemberdialog.h"
#include "custommessagedialog.h"
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QApplication>
#include <QMenu>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QAction>
#include <QMouseEvent>
#include <QPainterPath>
#include <QJsonDocument>
#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>

OrderDetailDrawer::OrderDetailDrawer(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground); // Allow QSS to style the background
    setupUI();
    
    // 监听数据变化，当商品信息同步完成后自动刷新图片和价格
    connect(ProductDataManager::instance(), &ProductDataManager::productDataChanged, this, [this](){
        if (!m_order.id.isEmpty() && (m_order.sourceModule == "Product" || m_order.sourceModule == "RetailPOS")) {
            updateUI();
        }
    });
}

void OrderDetailDrawer::setupUI()
{
    setFixedWidth(450); // 增加宽度防止溢出
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
    header->setFixedHeight(120); // 增加高度确保文字完整显示
    header->setStyleSheet("background: transparent; border: none;");
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(0);

    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(8);
    infoLayout->setContentsMargins(30, 20, 20, 20); // 增加上下边距
    
    m_memberNameLabel = new QLabel("收银订单详情");
    m_memberNameLabel->setStyleSheet("font-size: 14px; color: #64748b; font-weight: 800; border: none; font-family: 'Microsoft YaHei'; text-transform: uppercase; letter-spacing: 1px;");
    m_memberNameLabel->setAlignment(Qt::AlignLeft);
    
    m_petInfoLabel = new QLabel("加载中...");
    m_petInfoLabel->setStyleSheet("font-size: 26px; font-weight: 800; color: #1e293b; border: none; font-family: 'Microsoft YaHei';");
    m_petInfoLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    infoLayout->addWidget(m_memberNameLabel);
    infoLayout->addSpacing(5);
    infoLayout->addWidget(m_petInfoLabel);
    infoLayout->setContentsMargins(30, 15, 20, 0); // 增加左边距，使其与列表项对齐
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
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 彻底禁止横向滚动条
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
    footerLayout->setContentsMargins(20, 15, 20, 20); // 减小页脚边距
    footerLayout->setSpacing(10); // 减小间距

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
    QStringList methods = {"银行卡", "现金", "支付宝", "微信支付", "会员卡余额"};
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
    m_confirmBtn->setFixedHeight(44); // 适度降低按钮高度
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

        // --- 会员卡余额校验逻辑 ---
        if (method == "会员卡余额") {
            MemberInfo member = MemberDataManager::instance()->getMember(m_order.memberId);
            if (member.id.isEmpty() || member.id == "Temporary") {
                CustomMessageDialog::showWarning(this, "支付失败", "该订单为散客/临时客订单，无法使用会员卡支付！");
                return;
            }
            if (member.balance < m_order.totalAmount) {
                CustomMessageDialog::showWarning(this, "余额不足", 
                    QString("当前会员卡余额仅为 ¥ %1\n本次订单金额为 ¥ %2\n请充值后再进行支付。")
                    .arg(member.balance, 0, 'f', 2).arg(m_order.totalAmount, 0, 'f', 2));
                return;
            }
            
            // 余额充足，执行预扣减（实际更新由 PetDataManager::updateOrder 同步触发或在此手动触发）
            // 注意：PetDataManager::instance()->updateOrder 主要是更新订单状态
            // 我们需要手动更新会员余额，或者确保后端/DataManager 有联动逻辑
            member.balance -= m_order.totalAmount;
            member.consume_amt += m_order.totalAmount;
            MemberDataManager::instance()->updateMember(member);
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
    qDebug() << "[DRAWER] setOrder started for ID:" << order.id;
    m_order = order;
    if (m_stack) {
        m_stack->setCurrentWidget(m_detailWidget);
    }
    updateUI();
    qDebug() << "[DRAWER] setOrder finished.";
}

void OrderDetailDrawer::showEmptyState()
{
    if (m_stack) {
        m_stack->setCurrentWidget(m_emptyWidget);
    }
}

void OrderDetailDrawer::updateUI()
{
    if (m_order.id.isEmpty()) return;

    // 1. 设置头部信息：居中显示会员姓名和ID，顶部固定标题
    QString displayName = m_order.memberName.isEmpty() ? "临时客" : m_order.memberName;
    QString memberTitle = displayName;
    if (!m_order.memberId.isEmpty() && m_order.memberId != "Temporary") {
        memberTitle += " " + m_order.memberId;
    }
    m_petInfoLabel->setText(memberTitle);
    m_memberNameLabel->setText("收银订单详情");

    // 2. 清理旧项目
    QLayoutItem *child;
    while ((child = m_itemsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->hide();
            child->widget()->deleteLater();
        }
        delete child;
    }

    // 3. 定义统一的 addItem 助手
    auto addItem = [&](const QJsonObject &itemObj, bool isPetService) {
        QWidget *itemW = new QWidget();
        itemW->setObjectName("OrderItem");
        itemW->setStyleSheet("QWidget#OrderItem { background: white; border-bottom: 1px solid #f1f5f9; }");
        QHBoxLayout *rowLayout = new QHBoxLayout(itemW);
        rowLayout->setContentsMargins(10, 15, 10, 15);
        rowLayout->setSpacing(15);

        // 左侧：头像/图片 (48x48)
        QLabel *iconLabel = new QLabel();
        iconLabel->setFixedSize(48, 48);
        iconLabel->setAlignment(Qt::AlignCenter);
        
        QString iconPath;
        if (isPetService) {
            QString petId = itemObj["petId"].toString();
            iconPath = itemObj["petPhoto"].toString();
            
            QPixmap pix = PetDataManager::instance()->getPetPixmap(petId);
            iconLabel->setStyleSheet("background: #f8fafc; border-radius: 24px; border: 1px solid #e2e8f0;");
            
            if (!pix.isNull()) {
                QPixmap target(48, 48);
                target.fill(Qt::transparent);
                QPainter painter(&target);
                painter.setRenderHint(QPainter::Antialiasing);
                QPainterPath path;
                path.addEllipse(0, 0, 48, 48);
                painter.setClipPath(path);
                painter.drawPixmap(0, 0, 48, 48, pix.scaled(48, 48, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
                iconLabel->setPixmap(target);
            } else {
                iconLabel->setText("🐾");
                iconLabel->setStyleSheet("font-size: 20px; background: #f8fafc; border-radius: 24px; border: 1px solid #e2e8f0;");
            }
        } else {
            iconPath = itemObj["photo"].toString();
            if (iconPath.isEmpty()) {
                ProductInfo p = ProductDataManager::instance()->getProduct(itemObj["barcode"].toString());
                if (!p.images.isEmpty()) iconPath = p.images[0];
            }
            iconLabel->setStyleSheet("background: #f8fafc; border-radius: 8px; border: 1px solid #f1f5f9;");
            
            QPixmap pix = ProductDataManager::instance()->getProductPixmap(itemObj["barcode"].toString());
            if (!pix.isNull()) {
                iconLabel->setPixmap(pix.scaled(44, 44, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            } else {
                iconLabel->setText("📦");
            }
        }
        
        iconLabel->setCursor(Qt::PointingHandCursor);
        iconLabel->installEventFilter(this);
        iconLabel->setProperty("isOrderIcon", true); // 用于 eventFilter 识别
        iconLabel->setProperty("iconPath", iconPath);
        rowLayout->addWidget(iconLabel);

        // 右侧：信息展示
        QVBoxLayout *textLayout = new QVBoxLayout();
        textLayout->setSpacing(4);

        if (isPetService) {
            // 第一行：宠物名称
            QString petName = itemObj["petName"].toString();
            QString petId = itemObj["petId"].toString();
            QString breed = itemObj["petBreed"].toString();
            QString room = itemObj["roomName"].toString();
            
            QLabel *petNameLbl = new QLabel(petName);
            petNameLbl->setStyleSheet("font-size: 16px; font-weight: 800; color: #1e293b; font-family: 'Microsoft YaHei';");
            
            // 第二行：ID | 品种 | 房间
            QString metaStr = QString("ID: %1 | 品种: %2").arg(petId).arg(breed.isEmpty() ? "未知" : breed);
            if (!room.isEmpty()) metaStr += " | 房间: " + room;
            
            QLabel *petMetaLbl = new QLabel(metaStr);
            petMetaLbl->setStyleSheet("font-size: 12px; color: #64748b; font-family: 'Microsoft YaHei';");
            
            QHBoxLayout *topRow = new QHBoxLayout();
            topRow->addWidget(petNameLbl);
            topRow->addStretch();
            
            // 金额显示在右侧
            double price = itemObj["price"].toDouble();
            QLabel *priceLbl = new QLabel(QString("¥ %1").arg(price, 0, 'f', 2));
            priceLbl->setStyleSheet("font-size: 18px; font-weight: 800; color: #0f172a; font-family: 'Microsoft YaHei';");
            topRow->addWidget(priceLbl);
            
            textLayout->addLayout(topRow);
            textLayout->addWidget(petMetaLbl);

            // 第三行：服务名称 + 时长
            QString svcName = itemObj["name"].toString();
            int duration = itemObj.contains("duration") ? itemObj["duration"].toInt() : 0;
            if (duration > 0 && m_order.sourceModule == "Boarding") {
                svcName += QString(" (%1 天)").arg(duration);
            }
            
            QLabel *svcNameLbl = new QLabel(svcName);
            svcNameLbl->setStyleSheet("font-size: 13px; color: #475569; font-weight: 600; font-family: 'Microsoft YaHei';");
            textLayout->addWidget(svcNameLbl);

            // 显示服务人员信息
            QString staffName = itemObj["staff"].toString();
            if (!staffName.isEmpty() && staffName != "待分配") {
                QLabel *staffLbl = new QLabel("服务人员: " + staffName);
                staffLbl->setStyleSheet("font-size: 12px; color: #64748b; font-family: 'Microsoft YaHei'; font-weight: bold;");
                textLayout->addWidget(staffLbl);
            }
        } else {
            // 商品展示
            double price = itemObj["price"].toDouble();
            int count = itemObj["count"].toInt();
            QString nameStr = itemObj["name"].toString();
            if (count > 1) nameStr += QString(" ×%1").arg(count);
            
            QLabel *nameLbl = new QLabel(nameStr);
            nameLbl->setWordWrap(true); // 允许长名称换行
            nameLbl->setStyleSheet("font-size: 15px; font-weight: 700; color: #1e293b; font-family: 'Microsoft YaHei';");
            
            QLabel *priceLbl = new QLabel(QString("¥ %1").arg(price * count, 0, 'f', 2));
            priceLbl->setFixedWidth(100); // 固定宽度确保金额不被切掉
            priceLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            priceLbl->setStyleSheet("font-size: 18px; font-weight: 800; color: #0f172a; font-family: 'Microsoft YaHei';");
            
            QHBoxLayout *topRow = new QHBoxLayout();
            topRow->addWidget(nameLbl);
            topRow->addStretch();
            topRow->addWidget(priceLbl);
            textLayout->addLayout(topRow);

            QString qtyStr = (count > 1) ? QString("数量: %1").arg(count) : "单品零售";
            QLabel *subLbl = new QLabel(qtyStr);
            subLbl->setStyleSheet("font-size: 12px; color: #64748b; font-weight: 500; font-family: 'Microsoft YaHei';");
            textLayout->addWidget(subLbl);

            QString barcode = itemObj["barcode"].toString();
            if (!barcode.isEmpty()) {
                QLabel *bcLbl = new QLabel("条形码: " + barcode);
                bcLbl->setStyleSheet("font-size: 11px; color: #94a3b8; font-family: 'Consolas';");
                textLayout->addWidget(bcLbl);
            }
        }

        rowLayout->addLayout(textLayout, 1);
        m_itemsLayout->addWidget(itemW);
    };

    // 4. 解析 JSON 并填充
    QString detailsRaw = m_order.itemDetails;
    QJsonDocument doc = QJsonDocument::fromJson(detailsRaw.toUtf8());
    bool isPetService = (m_order.sourceModule == "Boarding" || m_order.sourceModule == "Appointment" || m_order.sourceModule == "Direct");
    
    if (!doc.isNull() && doc.isArray()) {
        QJsonArray arr = doc.array();
        
        for (int i = 0; i < arr.size(); ++i) {
            addItem(arr[i].toObject(), isPetService);
        }
    } else {
        // 兜底处理旧格式 (例如 "项目A + 项目A + 项目B")
        QStringList rawItems = detailsRaw.split("+", Qt::SkipEmptyParts);
        QMap<QString, int> counts;
        for (const QString &s : rawItems) counts[s.trimmed()]++;
        
        for (auto it = counts.begin(); it != counts.end(); ++it) {
            QJsonObject dummy;
            dummy["name"] = it.key();
            dummy["count"] = it.value();
            
            // 尝试找一下对应的价格
            double unitPrice = m_order.totalAmount / qMax(1, rawItems.size());
            dummy["price"] = unitPrice;
            
            // 注入兜底元数据，用于老订单显示
            if (isPetService && !m_order.petId.isEmpty()) {
                dummy["petId"] = m_order.petId;
                dummy["petName"] = m_order.petName;
                PetInfo pi = PetDataManager::instance()->getPet(m_order.petId);
                if (!pi.id.isEmpty()) {
                    dummy["petBreed"] = pi.breed;
                    dummy["petPhoto"] = pi.avatarPath;
                }
            }
            
            addItem(dummy, isPetService);
        }
    }

    m_itemsLayout->addStretch();
    m_finalAmountLabel->setText(QString("¥ %1").arg(m_order.totalAmount, 0, 'f', 2));

    // 5. 支付状态处理
    bool isUnpaid = (m_order.status == "Unpaid");
    m_confirmBtn->setEnabled(isUnpaid);
    m_cancelOrderBtn->setVisible(isUnpaid);

    for (auto b : m_payMethodButtons) {
        b->setChecked(false);
        if (!isUnpaid && b->text() == m_order.payMethod) b->setChecked(true);
        b->setEnabled(isUnpaid);
    }
    
    if (!isUnpaid) {
        m_confirmBtn->setText("已结算 (" + m_order.payMethod + ")");
        m_confirmBtn->setStyleSheet("QPushButton { background: #dcfce7; color: #166534; border-radius: 10px; font-size: 16px; font-weight: 800; border: none; font-family: 'Microsoft YaHei'; }");
    } else {
        m_confirmBtn->setText("确认结算");
        m_confirmBtn->setStyleSheet("QPushButton { background: #3b82f6; color: white; border-radius: 10px; font-size: 16px; font-weight: 800; border: none; font-family: 'Microsoft YaHei'; } QPushButton:hover { background: #2563eb; }");
    }
}

bool OrderDetailDrawer::eventFilter(QObject *obj, QEvent *event)
{
    // 处理列表图标点击放大
    bool isIcon = obj->property("isOrderIcon").toBool();

    if (isIcon && event->type() == QEvent::MouseButtonRelease) {
        QString pathToShow = obj->property("iconPath").toString();
        if (pathToShow.isEmpty()) return true;

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
        
        QPixmap pix;
        if (pathToShow.startsWith("/9j/") || pathToShow.length() > 512) {
            QByteArray data = QByteArray::fromBase64(pathToShow.toLatin1());
            pix.loadFromData(data);
        } else {
            pix.load(pathToShow);
        }

        if (!pix.isNull()) {
            imgLabel->setPixmap(pix.scaled(maxDim, maxDim, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            return true;
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
