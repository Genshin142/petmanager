#include "rolemodule.h"
#include "addemployeedialog.h"
#include "custommessagedialog.h"
#include "staffdatamanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QtGlobal>
#include <QPushButton>
#include <QHeaderView>
#include <QGraphicsDropShadowEffect>
#include <QCheckBox>
#include <QPainter>
#include <QPainterPath>
#include <QMenu>
#include <QDialog>
#include <QIcon>
#include <QEvent>
#include <QFile>
#include <QTimer>
#include <QStyledItemDelegate>

// --- 复刻：全行圆角边框选中委托 ---
class RoleRowDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->fillRect(opt.rect, Qt::white);

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

RoleModule::RoleModule(QWidget *parent) : QWidget(parent), m_currentRoleFilter("全部职位"), m_currentStatusFilter("全部状态"), m_currentPage(1), m_pageSize(20)
{
    // 预加载默认头像并裁剪为圆形
    m_maleAvatar = createCircularAvatar(QPixmap(":/images/male.png"), 36);
    m_femaleAvatar = createCircularAvatar(QPixmap(":/images/female.png"), 36);

    m_drawer = new EmployeeDetailDrawer(this);
    connect(m_drawer, &EmployeeDetailDrawer::closeRequested, this, [=](){ m_drawer->hideDrawer(); });
    connect(m_drawer, &EmployeeDetailDrawer::avatarClicked, this, &RoleModule::showBigImage);
    connect(m_drawer, &EmployeeDetailDrawer::editRequested, this, &RoleModule::onEditEmployeeFromDrawer);

    connect(StaffDataManager::instance(), &StaffDataManager::staffDataChanged, this, [=](){
        empTable->setRowCount(0);
        addSampleData();
        updateStats();
        updatePagination();
    });

    setupUI();
    
    // 默认选中第一行
    QTimer::singleShot(100, this, [=](){
        if (empTable->rowCount() > 0) {
            empTable->setCurrentCell(0, 0);
        }
    });
}

bool RoleModule::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseButtonRelease) {
        // 1. 如果点击的是头像，直接放大
        if (watched->property("imgPath").isValid()) {
            showBigImage(watched->property("imgPath").toString());
            return true;
        }
        
        // 2. 如果点击的是头像周边的信息区域，手动选中该行
        if (watched->property("row").isValid()) {
            int row = watched->property("row").toInt();
            empTable->selectRow(row);
            return true;
        }
    }
    
    // 3. 处理遮罩层点击（关闭预览）
    if (watched == m_imagePreviewOverlay && event->type() == QEvent::MouseButtonRelease) {
        hideBigImage();
        return true;
    }
    
    return QWidget::eventFilter(watched, event);
}

void RoleModule::showBigImage(const QString &path)
{
    if (path.isEmpty()) return;
    
    QPixmap pix(path);
    if (pix.isNull()) return;
    
    // 确保遮罩覆盖整个模块区域
    m_imagePreviewOverlay->setGeometry(rect());
    
    // 统一标准：取窗口最小维度的 80%
    int maxDim = qMin(this->window()->width(), this->window()->height()) * 0.8;
    m_previewLabel->setPixmap(pix.scaled(maxDim, maxDim, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    
    m_imagePreviewOverlay->show();
    m_imagePreviewOverlay->raise();
}

void RoleModule::hideBigImage()
{
    m_imagePreviewOverlay->hide();
}

QPixmap RoleModule::createCircularAvatar(const QPixmap &src, int size)
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

void RoleModule::setupUI()
{
    QHBoxLayout *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    QWidget *masterWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(masterWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // --- 顶部统计与标题容器 (复刻会员模块风格) ---
    QFrame *topContainer = new QFrame();
    topContainer->setObjectName("TopStatisticsContainer");
    topContainer->setFixedHeight(160); // 限制高度，防止过度拉伸
    topContainer->setStyleSheet("#TopStatisticsContainer { background: white; border: 1px solid #ebeef5; border-radius: 12px; }");
    QVBoxLayout *topLayout = new QVBoxLayout(topContainer);
    topLayout->setContentsMargins(25, 15, 25, 15);
    topLayout->setSpacing(12);

    // 1. 顶部标题
    QLabel *titleLabel = new QLabel("员工权限与考勤管理", this);
    titleLabel->setStyleSheet("font-size: 20px; color: #303133; font-weight: bold; border: none; background: transparent;");
    topLayout->addWidget(titleLabel);

    // ═══════════════════════════════════════════
    // 2. 统计卡片行
    // ═══════════════════════════════════════════
    QHBoxLayout *statLayout = new QHBoxLayout();
    statLayout->setSpacing(15);
    topLayout->addLayout(statLayout);

    mainLayout->addWidget(topContainer);

    auto createStatCard = [&](const QString &icon, const QString &label, QLabel* &outValueLabel, const QColor &color) {
        QFrame *card = new QFrame();
        card->setFixedHeight(80);
        card->setStyleSheet("QFrame { background: #f8fafc; border-radius: 8px; border: 1px solid #f1f5f9; } ");
        
        QHBoxLayout *l = new QHBoxLayout(card);
        l->setContentsMargins(20, 10, 20, 10);

        QLabel *iconLabel = new QLabel(icon);
        if (icon.isEmpty()) {
            iconLabel->hide();
        } else {
            iconLabel->setFixedSize(40, 40);
            iconLabel->setAlignment(Qt::AlignCenter);
            iconLabel->setStyleSheet(QString("font-size: 20px; color: %1; background: white; border-radius: 8px; border: 1px solid #f1f5f9;").arg(color.name()));
        }
        if (!icon.isEmpty()) {
            l->addWidget(iconLabel);
            l->addSpacing(12);
        }

        QVBoxLayout *textLayout = new QVBoxLayout();
        textLayout->setSpacing(2);
        
        QLabel *labelTitle = new QLabel(label);
        labelTitle->setStyleSheet("color: #94a3b8; font-size: 12px; border: none; background: transparent;");
        
        outValueLabel = new QLabel("--");
        outValueLabel->setStyleSheet("font-size: 20px; color: #1e293b; border: none; background: transparent; font-weight: bold;");
        
        textLayout->addWidget(labelTitle);
        textLayout->addWidget(outValueLabel);
        textLayout->addStretch();

        l->addLayout(textLayout);
        l->addStretch();

        return card;
    };

    statLayout->addWidget(createStatCard("", "在职员工", totalEmpLabel, QColor("#3b82f6")));
    statLayout->addWidget(createStatCard("", "今日实到", todayAttendLabel, QColor("#67c23a")));
    statLayout->addWidget(createStatCard("", "请假人数", onLeaveLabel, QColor("#e6a23c")));
    statLayout->addWidget(createStatCard("", "当日出勤率", attendRateLabel, QColor("#3b82f6")));

    // 3. 紧贴表格的操作栏 (增加卡片容器包裹)
    QFrame *operationCard = new QFrame();
    operationCard->setObjectName("OperationCard");
    operationCard->setStyleSheet("#OperationCard { background: white; border: 1px solid #ebeef5; border-radius: 12px; }");
    
    QHBoxLayout *operationLayout = new QHBoxLayout(operationCard);
    operationLayout->setContentsMargins(25, 12, 25, 12);
    operationLayout->setSpacing(0);

    // -- 1. 搜索与筛选 (左侧) --
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(" 搜索姓名、工号、电话...");
    searchEdit->setFixedWidth(200);
    searchEdit->setFixedHeight(36);
    searchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 15px; font-size: 13px; background: white; } "
        "QLineEdit:focus { border-color: #409eff; outline: none; }"
    );

    roleFilterCombo = new QComboBox();
    roleFilterCombo->setFixedWidth(110);
    roleFilterCombo->setFixedHeight(36);
    roleFilterCombo->addItems({"全部职位", "店员", "高级美容师", "宠物医生", "实习生", "店长"});
    roleFilterCombo->setStyleSheet(
        "QComboBox { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 10px; background: white; font-size: 13px; } "
        "QComboBox:hover { border-color: #409eff; } "
        "QComboBox::drop-down { border: none; width: 24px; } "
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; } "
        "QComboBox QAbstractItemView { border: 1px solid #e2e8f0; border-radius: 8px; background: white; selection-background-color: #f1f5f9; selection-color: #3b82f6; outline: none; padding: 5px; }"
    );

    statusFilterContainer = new QWidget();
    QHBoxLayout *statusLayout = new QHBoxLayout(statusFilterContainer);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setSpacing(8);

    QStringList statuses = {"全部状态", "在岗", "离岗", "请假", "离职"};
    for (const QString &st : statuses) {
        QPushButton *btn = new QPushButton(st);
        btn->setCheckable(true);
        btn->setAutoExclusive(true);
        btn->setFixedHeight(36);
        btn->setCursor(Qt::PointingHandCursor);
        if (st == m_currentStatusFilter) btn->setChecked(true);

        btn->setStyleSheet(
            "QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 16px; padding: 0 15px; color: #606266; font-size: 13px; } "
            "QPushButton:hover { background: #ecf5ff; border-color: #409eff; color: #409eff; } "
            "QPushButton:checked { background: #409eff; border-color: #409eff; color: white; font-weight: bold; } "
        );

        connect(btn, &QPushButton::clicked, this, [=](){
            m_currentStatusFilter = st;
            onFilterChanged();
        });
        statusLayout->addWidget(btn);
    }

    operationLayout->addWidget(searchEdit);
    operationLayout->addSpacing(8);
    operationLayout->addWidget(roleFilterCombo);
    operationLayout->addSpacing(8);
    operationLayout->addWidget(statusFilterContainer);

    // -- 2. 中间弹簧 --
    operationLayout->addStretch();

    QPushButton *addBtn = new QPushButton("录入员工");
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setFixedHeight(36);
    addBtn->setStyleSheet(
        "QPushButton { background: white; border: 1px solid #3b82f6; color: #3b82f6; padding: 0 15px; border-radius: 6px; font-size: 13px; font-weight: bold; } "
        "QPushButton:hover { background: #eff6ff; }"
    );
    connect(addBtn, &QPushButton::clicked, this, &RoleModule::onAddEmployee);

    operationLayout->addWidget(addBtn);

    mainLayout->addWidget(operationCard);

    // 4. 表格卡片容器 (12px 圆角)
    QFrame *tableCard = new QFrame();
    tableCard->setStyleSheet("QFrame { background: white; border-radius: 12px; border: 1px solid #ebeef5; }");
    QVBoxLayout *tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(0, 5, 0, 0);

    empTable = new QTableWidget();
    empTable->setColumnCount(7);
    empTable->setHorizontalHeaderLabels({"工号", "员工信息", "职位", "状态", "联系电话", "入职日期", "操作"});
    empTable->setItemDelegate(new RoleRowDelegate(empTable));
    
    empTable->setShowGrid(false);
    empTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    empTable->setSelectionMode(QAbstractItemView::SingleSelection);
    empTable->setFocusPolicy(Qt::NoFocus);
    empTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    empTable->setWordWrap(true);
    empTable->verticalHeader()->setVisible(false);
    empTable->verticalHeader()->setDefaultSectionSize(60);

    empTable->setStyleSheet(
        "QTableWidget { border: none; background: white; outline: none; border-radius: 6px; } "
        
        "QHeaderView { border: none; background: transparent; border-radius: 12px 12px 0 0; }"
    );
    
    tableLayout->addWidget(empTable);
    mainLayout->addWidget(tableCard);

    QHeaderView *header = empTable->horizontalHeader();
    header->setDefaultAlignment(Qt::AlignCenter);
    header->setSectionResizeMode(QHeaderView::Stretch); // 默认所有列拉伸
    
    // 针对特定列进行固定
    header->setSectionResizeMode(0, QHeaderView::Fixed); empTable->setColumnWidth(0, 80);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(6, QHeaderView::Fixed); empTable->setColumnWidth(6, 180);
    
    connect(empTable, &QTableWidget::currentCellChanged, this, &RoleModule::onCurrentCellChanged);

    // --- 初始化全屏大图预览层 ---
    m_imagePreviewOverlay = new QWidget(this);
    m_imagePreviewOverlay->setObjectName("EmployeePreviewOverlay");
    m_imagePreviewOverlay->setStyleSheet("#EmployeePreviewOverlay { background-color: rgba(0, 0, 0, 215); }");
    m_imagePreviewOverlay->hide();
    m_imagePreviewOverlay->installEventFilter(this);

    QVBoxLayout *previewL = new QVBoxLayout(m_imagePreviewOverlay);
    m_previewLabel = new QLabel();
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet("border: none; background: transparent;"); 
    previewL->addWidget(m_previewLabel, 0, Qt::AlignCenter);

    // ═══════════════════════════════════════════
    // 5. 底部统计+分页栏
    // ═══════════════════════════════════════════
    // 5. 底部统计+分页栏 (模仿会员中心)
    QFrame *statFrame = new QFrame();
    statFrame->setFixedHeight(50);
    // 背景改为白色，移除顶部边框，设置底部圆角
    statFrame->setStyleSheet("QFrame { background: white; border: none; padding: 0 12px; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; }");
    QHBoxLayout *footerLayout = new QHBoxLayout(statFrame);
    
    // 关键：将分页栏加入表格卡片的布局中，而不是 mainLayout
    tableLayout->addWidget(statFrame);

    prevBtn = new QPushButton("上一页");
    nextBtn = new QPushButton("下一页");
    pageLabel = new QLabel("第 1 页 / 共 1 页");

    QString pageStyle = "QPushButton { height: 28px; border: 1px solid #e2e8f0; border-radius: 6px; background: white; color: #64748b; font-size: 12px; padding: 0 12px; text-align: center; font-weight: bold; } "
                        "QPushButton:hover { border-color: #3b82f6; color: #3b82f6; background: #eff6ff; } "
                        "QPushButton:disabled { background: white; color: #cbd5e1; border-color: #f1f5f9; }";
    prevBtn->setStyleSheet(pageStyle);
    nextBtn->setStyleSheet(pageStyle);
    prevBtn->setCursor(Qt::PointingHandCursor);
    nextBtn->setCursor(Qt::PointingHandCursor);
    
    pageLabel->setStyleSheet("color: #64748b; font-size: 13px; font-weight: bold; margin: 0 10px;");

    footerLayout->addStretch();
    footerLayout->addWidget(prevBtn);
    footerLayout->addSpacing(20);
    footerLayout->addWidget(pageLabel);
    footerLayout->addSpacing(20);
    footerLayout->addWidget(nextBtn);

    // ═══════════════════════════════════════════
    // 6. 事件绑定
    // ═══════════════════════════════════════════
    connect(searchEdit, &QLineEdit::textChanged, this, &RoleModule::onSearchTextChanged);
    connect(roleFilterCombo, &QComboBox::currentTextChanged, this, [=](const QString &text){ 
        m_currentRoleFilter = text;
        onFilterChanged(); 
    });
    connect(prevBtn, &QPushButton::clicked, this, &RoleModule::onPrevPage);
    connect(nextBtn, &QPushButton::clicked, this, &RoleModule::onNextPage);

    empTable->setContextMenuPolicy(Qt::NoContextMenu);

    // 7. 注入演示数据
    addSampleData();
    updateStats();
    updatePagination();

    rootLayout->addWidget(masterWidget, 1);
    rootLayout->addWidget(m_drawer);
}

void RoleModule::addSampleData()
{
    // 从统一数据管理器加载初始数据，确保全程序一致
    auto all = StaffDataManager::instance()->allStaff();
    
    // 排序逻辑：离职状态排在最后，其余按工号排序
    // 排序逻辑：离职员工排在最后，其余按工号排序
    std::sort(all.begin(), all.end(), [](const EmployeeInfo &a, const EmployeeInfo &b){
        bool resA = (a.status == "离职");
        bool resB = (b.status == "离职");
        if (resA != resB) return !resA;
        return a.id < b.id;
    });

    for (const auto &info : all) {
        addEmployeeRow(info.id, info.name, info.role, info.status, info.gender, info.age, 
                       info.phone, info.email, info.idCard, info.baseSalary, 0, 0, info.imgPath);
    }
}

void RoleModule::updateStats()
{
    int totalEmp = 0;
    int todayAttend = 0;
    int onLeave = 0;

    for (int i = 0; i < empTable->rowCount(); ++i) {
        if (empTable->isRowHidden(i)) continue;
        // 检查状态 (新索引：第 3 列)
        QWidget* statusWidget = empTable->cellWidget(i, 3);
        if (statusWidget) {
            QLabel* tag = statusWidget->findChild<QLabel*>();
            if (tag) {
                QString status = tag->text();
                if (status != "离职") totalEmp++;
                if (status == "在岗") todayAttend++;
                if (status == "请假") onLeave++;
            }
        }
    }

    totalEmpLabel->setText(QString("%1人").arg(totalEmp));
    todayAttendLabel->setText(QString("%1人").arg(todayAttend));
    onLeaveLabel->setText(QString("%1人").arg(onLeave));
    
    double rate = (totalEmp > 0) ? (double)todayAttend / totalEmp * 100 : 0;
    attendRateLabel->setText(QString("%1%").arg(rate, 0, 'f', 1));
}

void RoleModule::addEmployeeRow(const QString &id, const QString &name, const QString &role, const QString &status, 
                                const QString &gender, int age, const QString &phone, const QString &email, const QString &idCard,
                                double baseSalary, double performance, double commission, const QString &imgPath)
{
    int row = empTable->rowCount();
    empTable->insertRow(row);
    setEmployeeRowData(row, id, name, role, status, gender, age, phone, email, idCard, baseSalary, performance, commission, imgPath);
}

void RoleModule::addEmployeeRowInPlace(int row, const EmployeeInfo &info)
{
    setEmployeeRowData(row, info.id, info.name, info.role, info.status, info.gender, info.age, 
                       info.phone, info.email, info.idCard, info.baseSalary, 0, 0, info.imgPath);
}

void RoleModule::setEmployeeRowData(int row, const QString &id, const QString &name, const QString &role, const QString &status, 
                                    const QString &gender, int age, const QString &phone, const QString &email, const QString &idCard,
                                    double baseSalary, double /*performance*/, double /*commission*/, const QString &imgPath)
{
    // 核心：从数据中心获取完整对象，确保包含账号密码等所有字段
    EmployeeInfo info = StaffDataManager::instance()->getStaff(id);
    if (info.id.isEmpty()) {
        // 如果数据中心找不到（比如刚添加还没同步），则手动构建基础信息
        info.id = id; info.name = name; info.role = role; info.status = status;
        info.gender = gender; info.age = age; info.phone = phone; info.email = email;
        info.idCard = idCard; info.baseSalary = (int)baseSalary; info.imgPath = imgPath;
        info.joinDate = "2023-01-01";
    }

    auto setItem = [&](int col, const QString &text, const QColor &color = QColor()) {
        QTableWidgetItem *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        if (color.isValid()) item->setForeground(QBrush(color));
        empTable->setItem(row, col, item);
    };

    // 0. 工号
    QTableWidgetItem *idItem = new QTableWidgetItem(id);
    idItem->setTextAlignment(Qt::AlignCenter);
    idItem->setData(Qt::UserRole, QVariant::fromValue(info));
    empTable->setItem(row, 0, idItem);

    // 1. 员工信息 (头像 + 姓名)
    QWidget *infoWidget = new QWidget();
    QHBoxLayout *infoLayout = new QHBoxLayout(infoWidget);
    infoLayout->setContentsMargins(12, 5, 12, 5); infoLayout->setSpacing(12);
    infoLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QLabel *avatarLabel = new QLabel();
    avatarLabel->setFixedSize(40, 40);
    avatarLabel->setStyleSheet("border: none; background: transparent;"); // 确保没有外框
    QPixmap targetAvatar = imgPath.isEmpty() ? (gender == "女" ? m_femaleAvatar : m_maleAvatar) : createCircularAvatar(QPixmap(imgPath), 40);
    avatarLabel->setPixmap(targetAvatar);
    avatarLabel->setProperty("imgPath", imgPath.isEmpty() ? (gender == "女" ? ":/images/female.png" : ":/images/male.png") : imgPath);
    avatarLabel->installEventFilter(this);

    QLabel *nameLabel = new QLabel(name);
    nameLabel->setStyleSheet("font-weight: bold; color: #303133; font-size: 14px; border: none; background: transparent;");
    
    infoLayout->addWidget(avatarLabel);
    infoLayout->addWidget(nameLabel);
    infoLayout->addStretch();
    infoWidget->setProperty("row", row);
    infoWidget->installEventFilter(this);
    empTable->setCellWidget(row, 1, infoWidget);

    // 2. 职位
    setItem(2, role);

    // 3. 状态
    QWidget *statusContainer = new QWidget();
    QHBoxLayout *sLayout = new QHBoxLayout(statusContainer);
    sLayout->setContentsMargins(0, 0, 0, 0); sLayout->setAlignment(Qt::AlignCenter);
    QLabel *statusTag = new QLabel(status);
    QString tagStyle = "padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold; ";
    if (status == "在岗") tagStyle += "background-color: #dcfce7; color: #166534;";
    else if (status == "请假") tagStyle += "background-color: #ffedd5; color: #9a3412;";
    else if (status == "离岗") tagStyle += "background-color: #f1f5f9; color: #64748b;";
    else tagStyle += "background-color: #fee2e2; color: #991b1b;";
    statusTag->setStyleSheet(tagStyle);
    sLayout->addWidget(statusTag);
    empTable->setCellWidget(row, 3, statusContainer);

    // 4. 联系电话
    setItem(4, phone);

    // 5. 入职日期
    setItem(5, info.joinDate);

    // 6. 操作
    QWidget *btnContainer = new QWidget();
    btnContainer->setStyleSheet("background: transparent; border: none;"); 
    QHBoxLayout *btnLayout = new QHBoxLayout(btnContainer);
    btnLayout->setContentsMargins(10, 0, 10, 0); btnLayout->setSpacing(8); btnLayout->setAlignment(Qt::AlignCenter);

    if (status == "离职") {
        // --- 离职状态下的操作按钮 ---
        QPushButton *restoreBtn = new QPushButton("恢复");
        restoreBtn->setCursor(Qt::PointingHandCursor);
        restoreBtn->setFixedSize(70, 28);
        restoreBtn->setStyleSheet(
            "QPushButton { background-color: #eff6ff; color: #3b82f6; border: 1px solid #dbeafe; border-radius: 4px; font-size: 12px; padding: 0; text-align: center; } "
            "QPushButton:hover { background-color: #3b82f6; color: white; }"
        );
        btnLayout->addWidget(restoreBtn);
        connect(restoreBtn, &QPushButton::clicked, this, [=](){
            if (CustomMessageDialog::confirm(this, "恢复确认", QString("确定恢复员工 [%1] 的在岗档案吗？").arg(name))) {
                StaffDataManager::instance()->restoreStaff(id);
                refreshTablePreservingSelection(id);
                updateStats();
                CustomMessageDialog::showSuccess(this, "恢复成功", QString("员工 %1 已重新入职").arg(name));
            }
        });

        // 彻底删除
        QPushButton *hardDelBtn = new QPushButton("彻底删除");
        hardDelBtn->setCursor(Qt::PointingHandCursor);
        hardDelBtn->setFixedSize(70, 28);
        hardDelBtn->setStyleSheet(
            "QPushButton { background-color: #fef2f2; color: #dc2626; border: 1px solid #fee2e2; border-radius: 4px; font-size: 12px; padding: 0; text-align: center; } "
            "QPushButton:hover { background-color: #dc2626; color: white; }"
        );
        btnLayout->addWidget(hardDelBtn);
        connect(hardDelBtn, &QPushButton::clicked, this, [=](){
            if (CustomMessageDialog::confirm(this, "彻底删除警示", QString("确定要永久抹除员工 [%1] 的所有档案吗？\n此操作不可撤销。").arg(name))) {
                StaffDataManager::instance()->hardDeleteStaff(id);
                m_drawer->hideDrawer();
                
                // 刷新数据
                empTable->setRowCount(0);
                addSampleData();
                updateStats();
                updatePagination();
                CustomMessageDialog::showSuccess(this, "清理成功", QString("员工 %1 的档案已彻底移除").arg(name));
            }
        });
    } else {
        // --- 正常状态下的操作按钮 ---
        
        // 重置密码按钮
        QPushButton *resetBtn = new QPushButton("重置密码");
        resetBtn->setCursor(Qt::PointingHandCursor);
        resetBtn->setFixedSize(90, 28);
        resetBtn->setStyleSheet(
            "QPushButton { background-color: #eff6ff; color: #3b82f6; border: 1px solid #dbeafe; border-radius: 4px; font-size: 12px; padding: 0; text-align: center; } "
            "QPushButton:hover { background-color: #3b82f6; color: white; }"
        );
        btnLayout->addWidget(resetBtn);
        connect(resetBtn, &QPushButton::clicked, this, [=](){
            onResetPassword(id); // 执行一键重置逻辑
        });

        QPushButton *delBtn = new QPushButton("删除");
        delBtn->setCursor(Qt::PointingHandCursor);
        delBtn->setFixedSize(70, 28);
        delBtn->setStyleSheet(
            "QPushButton { background-color: #fef0f0; color: #f56c6c; border: 1px solid #fbc4c4; border-radius: 4px; font-size: 12px; padding: 0; text-align: center; } "
            "QPushButton:hover { background-color: #f56c6c; color: white; }"
        );
        btnLayout->addWidget(delBtn);
        connect(delBtn, &QPushButton::clicked, this, [=](){
            if (CustomMessageDialog::confirm(this, "删除确认", QString("确定将员工 [%1] 标记为离职吗？\n离职后档案将保留在系统中。").arg(name))) {
                StaffDataManager::instance()->removeStaff(id);
                refreshTablePreservingSelection(id);
                updateStats();
                CustomMessageDialog::showSuccess(this, "操作成功", QString("员工 %1 已被标记为离职").arg(name));
            }
        });
    }

    empTable->setCellWidget(row, 6, btnContainer);
    empTable->setItem(row, 6, new QTableWidgetItem("")); 
}

// ═══════════════════════════════════════════
// 筛选与搜索
// ═══════════════════════════════════════════

void RoleModule::onFilterChanged()
{
    m_currentPage = 1;
    updatePagination();
}

void RoleModule::onSearchTextChanged(const QString &text)
{
    Q_UNUSED(text);
    m_currentPage = 1;
    updatePagination();
}

void RoleModule::updatePagination()
{
    QString searchText = searchEdit->text().trimmed().toLower();
    QString selectedRole = m_currentRoleFilter;
    QString selectedStatus = m_currentStatusFilter;

    QList<int> visibleRows;

    for (int i = 0; i < empTable->rowCount(); ++i) {
        // 文本搜索匹配（工号、姓名、手机号）
        bool textMatch = searchText.isEmpty();
        if (!textMatch) {
            for (int col : {0, 1, 4}) {
                QString cellText;
                if (col == 1) {
                    // 从 Widget 中提取姓名
                    QWidget *w = empTable->cellWidget(i, 1);
                    if (w) {
                        QLabel *lbl = w->findChildren<QLabel*>().last(); 
                        if (lbl) cellText = lbl->text();
                    }
                } else {
                    QTableWidgetItem *item = empTable->item(i, col);
                    if (item) cellText = item->text();
                }

                if (cellText.toLower().contains(searchText)) {
                    textMatch = true;
                    break;
                }
            }
        }

        // 职位筛选
        bool roleMatch = (selectedRole == "全部职位");
        if (!roleMatch) {
            QTableWidgetItem *item = empTable->item(i, 2);
            if (item) roleMatch = (item->text() == selectedRole);
        }

        // 状态筛选 (索引 3)
        bool statusMatch = (selectedStatus == "全部状态");
        if (!statusMatch) {
            QWidget *w = empTable->cellWidget(i, 3);
            if (w) {
                QLabel *lbl = w->findChild<QLabel*>();
                if (lbl) statusMatch = (lbl->text() == selectedStatus);
            }
        }

        if (textMatch && roleMatch && statusMatch) {
            visibleRows.append(i);
        }
        empTable->setRowHidden(i, true); // 先全部隐藏
    }

    // 2. 计算分页 (物理行顺序已在 addSampleData 中处理)

    // 计算分页
    int totalVisible = visibleRows.size();
    int totalPages = qMax(1, (totalVisible + m_pageSize - 1) / m_pageSize);
    if (m_currentPage > totalPages) m_currentPage = totalPages;
    if (m_currentPage < 1) m_currentPage = 1;

    int start = (m_currentPage - 1) * m_pageSize;
    int end = qMin(start + m_pageSize, totalVisible);

    for (int i = start; i < end; ++i) {
        empTable->setRowHidden(visibleRows[i], false);
    }

    pageLabel->setText(QString("第 %1 页 / 共 %2 页").arg(m_currentPage).arg(totalPages));
    prevBtn->setEnabled(m_currentPage > 1);
    nextBtn->setEnabled(m_currentPage < totalPages);

    // 自动选中第一行
    if (totalVisible > 0) {
        empTable->selectRow(visibleRows[start]);
    } else {
        empTable->clearSelection();
    }

    if (totalVisible == 0) {
        prevBtn->setEnabled(false);
        nextBtn->setEnabled(false);
        pageLabel->setText("第 0 页 / 共 0 页");
    }

    updateStats();
}

void RoleModule::onPrevPage()
{
    if (m_currentPage > 1) { m_currentPage--; updatePagination(); }
}
void RoleModule::onNextPage()
{
    m_currentPage++; updatePagination();
}

// ═══════════════════════════════════════════
// 增删改操作
// ═══════════════════════════════════════════

void RoleModule::onAddEmployee()
{
    AddEmployeeDialog dlg(this);
    
    // 自动计算下一个登录账号 (staff + maxId+1)
    auto allStaff = StaffDataManager::instance()->allStaff();
    int maxId = 0;
    for (const auto &s : allStaff) {
        if (s.id.startsWith("E")) {
            int cur = s.id.mid(1).toInt();
            if (cur > maxId) maxId = cur;
        }
    }
    QString nextAccount = QString("staff%1").arg(maxId + 1, 2, 10, QChar('0'));
    dlg.setNextAccount(nextAccount);

    if (dlg.exec() == QDialog::Accepted) {
        EmployeeInfo info = dlg.employeeInfo();
        // 自动分配工号
        info.id = QString("E%1").arg(maxId + 1, 3, 10, QChar('0'));
        
        // 同时更新 UI 和数据中心
        StaffDataManager::instance()->addStaff(info);
        
        // 刷新列表（使用统一的数据加载逻辑）
        empTable->setRowCount(0);
        addSampleData();
        
        updateStats();
        updatePagination();
    }
}

void RoleModule::onResetPassword(const QString &id)
{
    EmployeeInfo info = StaffDataManager::instance()->getStaff(id);
    if (info.id.isEmpty()) return;

    // 一键重置逻辑
    info.password = "123456";
    StaffDataManager::instance()->updateStaff(info);
    
    CustomMessageDialog::showSuccess(this, "密码重置成功", 
        QString("已将员工 [%1] 的登录密码重置为：123456").arg(info.name));
}

void RoleModule::onEditEmployee()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    for (int i = 0; i < empTable->rowCount(); ++i) {
        QWidget *w = empTable->cellWidget(i, 6); // 操作按钮在第 6 列
        if (w && w->layout() && w->layout()->indexOf(btn) != -1) {
             // 核心：直接从 UserRole 获取完整对象
             EmployeeInfo info = empTable->item(i, 0)->data(Qt::UserRole).value<EmployeeInfo>();
             
             AddEmployeeDialog dlg(this);
             dlg.setEmployeeInfo(info);
             if (dlg.exec() == QDialog::Accepted) {
                  EmployeeInfo newInfo = dlg.employeeInfo();
                  
                  // 同步更新数据中心
                  StaffDataManager::instance()->updateStaff(newInfo);
                  
                  // 原位刷新，避免详情页被收回
                  addEmployeeRowInPlace(i, newInfo);
                  empTable->selectRow(i);
                  updateStats();
                  updatePagination();
             }
             break;
        }
    }
}


void RoleModule::refreshTablePreservingSelection(const QString &targetId)
{
    m_isRefreshing = true;
    
    // 保存当前分页信息
    int savedPage = m_currentPage;
    
    empTable->setRowCount(0);
    addSampleData(); // 这里已经包含了排序逻辑
    updateStats();
    updatePagination();
    
    // 寻找并选中目标 ID
    int targetIdx = -1;
    for (int i = 0; i < empTable->rowCount(); ++i) {
        if (empTable->item(i, 0) && empTable->item(i, 0)->text() == targetId) {
            targetIdx = i;
            break;
        }
    }
    
    if (targetIdx != -1) {
        // 计算该行所在的页码 (基于当前的筛选条件)
        QList<int> visibleRows;
        QString searchText = searchEdit->text();
        QString selectedRole = roleFilterCombo->currentText();
        QString selectedStatus = m_currentStatusFilter;
        
        for (int i = 0; i < empTable->rowCount(); ++i) {
            bool match = true;
            if (!searchText.isEmpty() && !empTable->item(i, 1)->text().contains(searchText, Qt::CaseInsensitive) && !empTable->item(i, 0)->text().contains(searchText, Qt::CaseInsensitive)) match = false;
            if (selectedRole != "全部职位" && empTable->item(i, 2)->text() != selectedRole) match = false;
            if (selectedStatus != "全部状态") {
                QWidget *wa = empTable->cellWidget(i, 3);
                if (wa) { QLabel *l = wa->findChild<QLabel*>(); if(l && l->text() != selectedStatus) match = false; }
            }
            if (match) visibleRows.append(i);
        }
        
        int k = visibleRows.indexOf(targetIdx);
        if (k != -1) {
            m_currentPage = (k / m_pageSize) + 1;
            updatePagination();
            empTable->selectRow(targetIdx);
            onCurrentCellChanged(targetIdx, 0, -1, -1);
        }
    } else {
        m_currentPage = savedPage;
        updatePagination();
    }
    
    m_isRefreshing = false;
}

void RoleModule::onCurrentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    if (m_isRefreshing) return;
    Q_UNUSED(currentColumn);
    Q_UNUSED(previousRow);
    Q_UNUSED(previousColumn);

    if (currentRow < 0 || currentRow >= empTable->rowCount()) {
        if (m_drawer) m_drawer->hideDrawer();
        return;
    }

    QTableWidgetItem *idItem = empTable->item(currentRow, 0);
    if (!idItem) return; 

    // 直接提取绑定的全量对象
    EmployeeInfo info = idItem->data(Qt::UserRole).value<EmployeeInfo>();
    if (info.id.isEmpty()) return;

    if (m_drawer) {
        m_drawer->setEmployee(info);
        if (m_drawer->width() < 100) {
            m_drawer->showDrawer();
        }
    }
}

void RoleModule::onEditEmployeeFromDrawer(const EmployeeInfo &info)
{
    // 1. 在表格中寻找对应行
    int row = -1;
    for (int i = 0; i < empTable->rowCount(); ++i) {
        if (empTable->item(i, 0)->text() == info.id) {
            row = i;
            break;
        }
    }
    
    if (row == -1) return;

    // 2. 弹出编辑对话框
    AddEmployeeDialog dlg(this);
    dlg.setEmployeeInfo(info);
    if (dlg.exec() == QDialog::Accepted) {
        EmployeeInfo newInfo = dlg.employeeInfo();
        
        // 3. 更新表格 UI
        addEmployeeRowInPlace(row, newInfo);
        
        // 4. 同步更新详情页 Drawer
        m_drawer->setEmployee(newInfo);
        
        updateStats();
    }
}
