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

RoleModule::RoleModule(QWidget *parent) : QWidget(parent), m_currentPage(1), m_pageSize(30)
{
    // 预加载默认头像并裁剪为圆形
    m_maleAvatar = createCircularAvatar(QPixmap(":/images/male.png"), 36);
    m_femaleAvatar = createCircularAvatar(QPixmap(":/images/female.png"), 36);

    m_drawer = new EmployeeDetailDrawer(this);
    connect(m_drawer, &EmployeeDetailDrawer::closeRequested, this, [=](){ m_drawer->hideDrawer(); });
    connect(m_drawer, &EmployeeDetailDrawer::avatarClicked, this, &RoleModule::showBigImage);

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
    mainLayout->setContentsMargins(25, 25, 25, 25);
    mainLayout->setSpacing(20);

    // 1. 顶部标题栏
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("员工权限与考勤管理", this);
    titleLabel->setStyleSheet("font-size: 22px; color: #303133; font-weight: bold;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    // ═══════════════════════════════════════════
    // 2. 统计卡片行
    // ═══════════════════════════════════════════
    QHBoxLayout *statLayout = new QHBoxLayout();
    statLayout->setSpacing(20);

    auto createStatCard = [&](const QString &icon, const QString &label, QLabel* &outValueLabel, QLabel* &outTrendLabel, const QColor &color, bool showTrend = true) {
        QFrame *card = new QFrame();
        card->setFixedHeight(100);
        card->setStyleSheet("QFrame { background: white; border-radius: 12px; border: 1px solid #f0f2f5; } ");
        
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
        shadow->setBlurRadius(15);
        shadow->setColor(QColor(0, 0, 0, 20));
        shadow->setOffset(0, 4);
        card->setGraphicsEffect(shadow);

        QHBoxLayout *l = new QHBoxLayout(card);
        l->setContentsMargins(20, 15, 20, 15);

        QLabel *iconLabel = new QLabel(icon);
        if (icon.isEmpty()) {
            iconLabel->hide();
        } else {
            iconLabel->setFixedSize(50, 50);
            iconLabel->setAlignment(Qt::AlignCenter);
            iconLabel->setStyleSheet(QString("font-size: 28px; color: %1; background: #f5f7fa; border-radius: 10px; border: none;").arg(color.name()));
        }
        if (!icon.isEmpty()) {
            l->addWidget(iconLabel);
            l->addSpacing(15);
        }

        QVBoxLayout *textLayout = new QVBoxLayout();
        textLayout->setSpacing(2);
        
        QLabel *labelTitle = new QLabel(label);
        labelTitle->setStyleSheet("color: #909399; font-size: 13px; border: none; background: transparent;");
        
        outValueLabel = new QLabel("--");
        outValueLabel->setStyleSheet("font-size: 22px; color: #303133; border: none; background: transparent; font-weight: bold;");
        
        textLayout->addWidget(labelTitle);
        textLayout->addWidget(outValueLabel);

        if (showTrend) {
            QHBoxLayout *trendLayout = new QHBoxLayout();
            outTrendLabel = new QLabel("--");
            outTrendLabel->setStyleSheet("color: #67c23a; font-size: 11px; border: none; background: transparent;");
            trendLayout->addWidget(outTrendLabel);
            trendLayout->addStretch();
            textLayout->addLayout(trendLayout);
        } else {
            outTrendLabel = nullptr;
            textLayout->addStretch();
        }

        l->addLayout(textLayout);
        l->addStretch();

        return card;
    };

    QLabel* dummy = nullptr;
    statLayout->addWidget(createStatCard("", "在职员工", totalEmpLabel, dummy, QColor("#409eff"), false));
    statLayout->addWidget(createStatCard("", "今日实到", todayAttendLabel, attendRateLabel, QColor("#67c23a")));
    mainLayout->addLayout(statLayout);

    // 3. 紧贴表格的操作栏
    QHBoxLayout *operationLayout = new QHBoxLayout();
    operationLayout->setContentsMargins(0, 0, 0, 0);

    // -- 1. 搜索与筛选 (左侧) --
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(" 搜索姓名、工号、电话...");
    searchEdit->setFixedWidth(200);
    searchEdit->setFixedHeight(32);
    searchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 16px; padding: 0 15px; font-size: 13px; background: white; } "
        "QLineEdit:focus { border-color: #409eff; outline: none; }"
    );

    roleFilterCombo = new QComboBox();
    roleFilterCombo->setFixedWidth(110);
    roleFilterCombo->setFixedHeight(32);
    roleFilterCombo->addItems({"全部职位", "店员", "高级美容师", "宠物医生", "实习生", "店长"});
    roleFilterCombo->setStyleSheet(
        "QComboBox { border: 1px solid #dcdfe6; border-radius: 4px; padding: 0 10px; background: white; font-size: 13px; } "
        "QComboBox::drop-down { border: none; width: 24px; } "
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; }"
    );

    statusFilterCombo = new QComboBox();
    statusFilterCombo->setFixedWidth(100);
    statusFilterCombo->setFixedHeight(32);
    statusFilterCombo->addItems({"全部状态", "在岗", "离岗", "请假", "离职"});
    statusFilterCombo->setStyleSheet(
        "QComboBox { border: 1px solid #dcdfe6; border-radius: 4px; padding: 0 10px; background: white; font-size: 13px; } "
        "QComboBox::drop-down { border: none; width: 24px; } "
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; }"
    );

    operationLayout->addWidget(searchEdit);
    operationLayout->addSpacing(8);
    operationLayout->addWidget(roleFilterCombo);
    operationLayout->addSpacing(8);
    operationLayout->addWidget(statusFilterCombo);

    // -- 2. 中间弹簧 --
    operationLayout->addStretch();

    // -- 3. 右侧：批量操作与新增 --
    QPushButton *batchDeleteBtn = new QPushButton("批量删除");
    batchDeleteBtn->setCursor(Qt::PointingHandCursor);
    batchDeleteBtn->setFixedHeight(32);
    batchDeleteBtn->setStyleSheet(
        "QPushButton { background-color: #fef0f0; color: #f56c6c; border: 1px solid #fbc4c4; border-radius: 6px; font-size: 12px; padding: 0 15px; }"
        "QPushButton:hover { background-color: #f56c6c; color: white; }"
    );
    connect(batchDeleteBtn, &QPushButton::clicked, this, &RoleModule::onBatchDelete);

    QPushButton *addBtn = new QPushButton("+ 录入员工");
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setFixedHeight(32);
    addBtn->setStyleSheet("QPushButton { background: #67c23a; color: white; padding: 0 15px; border-radius: 4px; font-size: 13px; } QPushButton:hover { background: #85ce61; }");
    connect(addBtn, &QPushButton::clicked, this, &RoleModule::onAddEmployee);

    operationLayout->addWidget(batchDeleteBtn);
    operationLayout->addSpacing(12);
    operationLayout->addWidget(addBtn);

    mainLayout->addLayout(operationLayout);

    // ═══════════════════════════════════════════
    // 4. 数据表格
    // ═══════════════════════════════════════════
    empTable = new QTableWidget();
    empTable->setColumnCount(7);
    // 核心列: 0=选择, 1=工号, 2=姓名, 3=职位, 4=手机号, 5=状态, 6=操作
    empTable->setHorizontalHeaderLabels({"选择", "工号", "姓名", "职位", "手机号", "状态", "操作"});
    
    empTable->setShowGrid(false);
    empTable->setAlternatingRowColors(false);
    empTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    empTable->setSelectionMode(QAbstractItemView::SingleSelection);
    empTable->setFocusPolicy(Qt::NoFocus);
    empTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    empTable->setWordWrap(true);
    empTable->verticalHeader()->setVisible(false);
    empTable->verticalHeader()->setDefaultSectionSize(60);

    empTable->setStyleSheet(
        "QTableWidget { border: 1px solid #ebeef5; background-color: white; color: black; outline: none; } "
        "QTableWidget::item { border-bottom: 1px solid #f0f2f5; } "
        "QTableWidget::item:selected { background-color: #b3d8ff; } "
        "QHeaderView::section { background-color: #f5f7fa; padding: 10px; border: none; color: #606266; font-size: 13px; font-weight: bold; } "
    );

    QHeaderView *header = empTable->horizontalHeader();
    header->setDefaultAlignment(Qt::AlignCenter);
    header->setSectionResizeMode(QHeaderView::Stretch); // 默认所有列拉伸
    
    // 针对特定列进行固定
    header->setSectionResizeMode(0, QHeaderView::Fixed); empTable->setColumnWidth(0, 48);  // 选择
    header->setSectionResizeMode(1, QHeaderView::Fixed); empTable->setColumnWidth(1, 80);  // 工号
    header->setSectionResizeMode(6, QHeaderView::Fixed); empTable->setColumnWidth(6, 180); // 操作
    
    // 姓名(2)、职位(3)、手机号(4)、状态(5) 保持默认的 Stretch，从而实现四列等宽平均

    connect(empTable, &QTableWidget::currentCellChanged, this, &RoleModule::onCurrentCellChanged);

    mainLayout->addWidget(empTable);
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
    QFrame *statFrame = new QFrame();
    statFrame->setFixedHeight(50);
    statFrame->setStyleSheet("QFrame { background: #f8f9fb; border-top: 1px solid #ebeef5; padding: 0 12px; }");
    QHBoxLayout *footerLayout = new QHBoxLayout(statFrame);

    QLabel *footerInfo = new QLabel("员工档案管理");
    footerInfo->setStyleSheet("color: #909399; font-size: 13px;");
    footerLayout->addWidget(footerInfo);
    footerLayout->addStretch();

    // 分页控件
    jumpEdit = new QLineEdit();
    jumpEdit->setFixedWidth(36);
    jumpEdit->setMaxLength(3);
    jumpEdit->setFixedHeight(24);
    jumpEdit->setAlignment(Qt::AlignCenter);
    jumpEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 4px; padding: 0; font-size: 13px; background: white; margin: 0; } "
        "QLineEdit:focus { border-color: #409eff; outline: none; }"
    );
    jumpValidator = new QIntValidator(1, 1, this);
    jumpEdit->setValidator(jumpValidator);

    QLabel *jumpPrefix = new QLabel("跳转到第");
    jumpPrefix->setStyleSheet("color: #606266; font-size: 13px; margin: 0; padding: 0;");
    QLabel *jumpSuffix = new QLabel("页");
    jumpSuffix->setStyleSheet("color: #606266; font-size: 13px; margin: 0; padding: 0;");

    QPushButton *jumpBtn = new QPushButton("确认");
    jumpBtn->setCursor(Qt::PointingHandCursor);
    jumpBtn->setFixedSize(44, 24);
    jumpBtn->setStyleSheet(
        "QPushButton { border: 1px solid #dcdfe6; border-radius: 4px; background: white; color: #606266; font-size: 12px; padding: 2px 0; text-align: center; }"
        "QPushButton:hover { border-color: #409eff; color: #409eff; }"
    );

    prevBtn = new QPushButton("上一页");
    nextBtn = new QPushButton("下一页");
    pageLabel = new QLabel("第 1 页 / 共 1 页");

    QString pageStyle = "QPushButton { height: 24px; border: 1px solid #dcdfe6; border-radius: 4px; background: white; color: #606266; font-size: 12px; padding: 0 8px; text-align: center; } "
                        "QPushButton:hover { border-color: #409eff; color: #409eff; } "
                        "QPushButton:disabled { background: #f5f7fa; color: #c0c4cc; border-color: #e4e7ed; }";
    prevBtn->setStyleSheet(pageStyle);
    nextBtn->setStyleSheet(pageStyle);
    prevBtn->setCursor(Qt::PointingHandCursor);
    nextBtn->setCursor(Qt::PointingHandCursor);
    pageLabel->setStyleSheet("color: #909399; font-size: 13px; margin: 0; padding: 0 2px;");

    QWidget *jumpGroup = new QWidget();
    jumpGroup->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QHBoxLayout *jumpLayout = new QHBoxLayout(jumpGroup);
    jumpLayout->setContentsMargins(0, 0, 0, 0);
    jumpLayout->setSpacing(2);
    jumpLayout->addWidget(jumpPrefix);
    jumpLayout->addWidget(jumpEdit);
    jumpLayout->addWidget(jumpSuffix);
    jumpLayout->addWidget(jumpBtn);

    QWidget *pageGroup = new QWidget();
    pageGroup->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QHBoxLayout *pageLayout = new QHBoxLayout(pageGroup);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(2);
    pageLayout->addWidget(prevBtn);
    pageLayout->addWidget(pageLabel);
    pageLayout->addWidget(nextBtn);

    footerLayout->addWidget(jumpGroup);
    footerLayout->addSpacing(8);
    footerLayout->addWidget(pageGroup);

    mainLayout->addWidget(statFrame);

    // ═══════════════════════════════════════════
    // 6. 事件绑定
    // ═══════════════════════════════════════════
    connect(searchEdit, &QLineEdit::textChanged, this, &RoleModule::onSearchTextChanged);
    connect(roleFilterCombo, &QComboBox::currentTextChanged, this, [=](){ onFilterChanged(); });
    connect(statusFilterCombo, &QComboBox::currentTextChanged, this, [=](){ onFilterChanged(); });
    connect(prevBtn, &QPushButton::clicked, this, &RoleModule::onPrevPage);
    connect(nextBtn, &QPushButton::clicked, this, &RoleModule::onNextPage);
    connect(jumpBtn, &QPushButton::clicked, this, &RoleModule::onJumpPage);
    connect(jumpEdit, &QLineEdit::returnPressed, this, &RoleModule::onJumpPage);

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
    for (const auto &info : all) {
        addEmployeeRow(info.id, info.name, info.role, info.status, info.gender, info.age, 
                       info.phone, info.email, info.idCard, info.baseSalary, 0, 0, info.imgPath);
    }
}

void RoleModule::updateStats()
{
    int totalEmp = 0;
    int todayAttend = 0;

    for (int i = 0; i < empTable->rowCount(); ++i) {
        // 检查状态 (新索引：第 5 列)
        QWidget* statusWidget = empTable->cellWidget(i, 5);
        if (statusWidget) {
            QLabel* tag = statusWidget->findChild<QLabel*>();
            if (tag) {
                if (tag->text() != "离职") totalEmp++;
                if (tag->text() == "在岗") todayAttend++;
            }
        }
    }

    totalEmpLabel->setText(QString("%1人").arg(totalEmp));
    todayAttendLabel->setText(QString("%1人").arg(todayAttend));
    double rate = (totalEmp > 0) ? (double)todayAttend / totalEmp * 100 : 0;
    attendRateLabel->setText(QString("当日出勤率: %1%").arg(rate, 0, 'f', 1));
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
    // 将完整数据封装并绑定到 UserRole
    EmployeeInfo info;
    info.id = id; info.name = name; info.role = role; info.status = status;
    info.gender = gender; info.age = age; info.phone = phone; info.email = email;
    info.idCard = idCard; info.baseSalary = (int)baseSalary; info.imgPath = imgPath;
    
    // 注入一些默认的 HR 字段（后期可通过编辑对话框修改）
    info.joinDate = "2023-01-01";
    info.emergencyContact = "张三 (紧急)";
    info.emergencyPhone = "13011112222";
    info.department = (role == "店长") ? "总店管理层" : "一线服务部";
    info.education = (age > 30) ? "本科" : "大专";
    info.address = "广东省广州市天河区某某街道 101 号";

    auto setItem = [&](int col, const QString &text, const QColor &color = QColor()) {
        QTableWidgetItem *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        QFont font("Microsoft YaHei", 9);
        item->setFont(font);
        if (color.isValid()) item->setForeground(QBrush(color));
        empTable->setItem(row, col, item);
    };

    // 第0列：复选框
    QWidget *checkContainer = new QWidget();
    QHBoxLayout *checkLayout = new QHBoxLayout(checkContainer);
    checkLayout->setContentsMargins(0, 0, 0, 0);
    checkLayout->setAlignment(Qt::AlignCenter);
    QCheckBox *cb = new QCheckBox();
    cb->setStyleSheet("QCheckBox::indicator { width: 18px; height: 18px; }");
    checkLayout->addWidget(cb);
    empTable->setCellWidget(row, 0, checkContainer);

    // 第1列：工号 (并绑定全量数据)
    QTableWidgetItem *idItem = new QTableWidgetItem(id);
    idItem->setTextAlignment(Qt::AlignCenter);
    idItem->setData(Qt::UserRole, QVariant::fromValue(info)); // 核心：绑定对象
    empTable->setItem(row, 1, idItem);

    // 第2列：圆形头像 + 姓名
    QWidget *nameContainer = new QWidget();
    QHBoxLayout *nameLayout = new QHBoxLayout(nameContainer);
    nameLayout->setContentsMargins(0, 0, 0, 0); 
    nameLayout->setSpacing(12);
    nameLayout->setAlignment(Qt::AlignCenter); 

    QLabel *avatarLabel = new QLabel();
    avatarLabel->setFixedSize(36, 36);
    
    QPixmap targetAvatar;
    QString actualImgPath = imgPath;
    if (actualImgPath.isEmpty() || !QFile(actualImgPath).exists()) {
        targetAvatar = gender == "女" ? m_femaleAvatar : m_maleAvatar;
        actualImgPath = gender == "女" ? ":/images/female.png" : ":/images/male.png";
    } else {
        targetAvatar = createCircularAvatar(QPixmap(actualImgPath), 36);
    }
    
    avatarLabel->setPixmap(targetAvatar);
    avatarLabel->setScaledContents(false);
    avatarLabel->setStyleSheet("border: none; background: transparent;");
    avatarLabel->setCursor(Qt::PointingHandCursor);
    avatarLabel->setProperty("imgPath", actualImgPath);
    avatarLabel->installEventFilter(this);

    QLabel *nameLabel = new QLabel(name);
    nameLabel->setStyleSheet("font-size: 13px; color: #303133; border: none; background: transparent;");

    nameLayout->addWidget(avatarLabel);
    nameLayout->addWidget(nameLabel);
    
    nameContainer->setProperty("row", row);
    nameContainer->installEventFilter(this);
    nameContainer->setCursor(Qt::PointingHandCursor);
    
    empTable->setCellWidget(row, 2, nameContainer);

    // 第3列：职位
    setItem(3, role);

    // 第4列：手机号
    setItem(4, phone);

    // 第8列：状态标签
    QWidget *statusContainer = new QWidget();
    QHBoxLayout *sLayout = new QHBoxLayout(statusContainer);
    sLayout->setContentsMargins(0, 0, 0, 0);
    sLayout->setAlignment(Qt::AlignCenter);
    QLabel *statusTag = new QLabel(status);
    QString tagStyle = "padding: 2px 10px; border-radius: 10px; font-size: 11px; font-weight: 500; ";
    if (status == "在岗") tagStyle += "background-color: #f0f9eb; color: #67c23a; border: 1px solid #e1f3d8;";
    else if (status == "请假") tagStyle += "background-color: #fdf6ec; color: #e6a23c; border: 1px solid #faecd8;";
    else if (status == "离岗") tagStyle += "background-color: #f4f4f5; color: #909399; border: 1px solid #e4e7ed;";
    else if (status == "离职") tagStyle += "background-color: #909399; color: #ffffff; border: 1px solid #909399;"; // 统一使用 10px 圆角和边框
    else tagStyle += "background-color: #fef0f0; color: #f56c6c; border: 1px solid #fde2e2;";
    statusTag->setStyleSheet(tagStyle);
    sLayout->addWidget(statusTag);
    empTable->setCellWidget(row, 5, statusContainer); // 索引改为 5

    // 第6列：操作按钮
    QWidget *btnContainer = new QWidget();
    QHBoxLayout *btnLayout = new QHBoxLayout(btnContainer);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(6);
    btnLayout->setAlignment(Qt::AlignCenter);

    QPushButton *editBtn = new QPushButton("修改");
    editBtn->setCursor(Qt::PointingHandCursor);
    editBtn->setFixedHeight(28);
    editBtn->setFixedWidth(65);
    editBtn->setStyleSheet(
        "QPushButton { background-color: #ecf5ff; color: #409eff; border: 1px solid #b3d8ff; border-radius: 4px; font-size: 12px; padding: 0 10px; }"
        "QPushButton:hover { background-color: #409eff; color: white; }"
    );
    connect(editBtn, &QPushButton::clicked, this, &RoleModule::onEditEmployee);

    QPushButton *delBtn = new QPushButton("删除");
    delBtn->setCursor(Qt::PointingHandCursor);
    delBtn->setFixedHeight(28);
    delBtn->setFixedWidth(65);
    delBtn->setStyleSheet(
        "QPushButton { background-color: #fef0f0; color: #f56c6c; border: 1px solid #fbc4c4; border-radius: 4px; font-size: 12px; padding: 0 10px; }"
        "QPushButton:hover { background-color: #f56c6c; color: white; }"
    );
    connect(delBtn, &QPushButton::clicked, this, &RoleModule::onDeleteEmployee);

    btnLayout->addWidget(editBtn);
    btnLayout->addWidget(delBtn);
    empTable->setCellWidget(row, 6, btnContainer); // 索引改为 6
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
    QString selectedRole = roleFilterCombo->currentText();
    QString selectedStatus = statusFilterCombo->currentText();

    QList<int> visibleRows;

    for (int i = 0; i < empTable->rowCount(); ++i) {
        // 文本搜索匹配（工号、姓名、手机号）
        bool textMatch = searchText.isEmpty();
        if (!textMatch) {
            for (int col : {1, 2, 5}) {
                QString cellText;
                if (col == 2) {
                    // 从 Widget 中提取姓名
                    QWidget *w = empTable->cellWidget(i, 2);
                    if (w) {
                        QLabel *lbl = w->findChildren<QLabel*>().last(); // 姓名标签通常在后面
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
            QTableWidgetItem *item = empTable->item(i, 3);
            if (item) roleMatch = (item->text() == selectedRole);
        }

        // 状态筛选
        bool statusMatch = (selectedStatus == "全部状态");
        if (!statusMatch) {
            QWidget *w = empTable->cellWidget(i, 8);
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

    // 计算分页
    int totalVisible = visibleRows.size();
    int totalPages = qMax(1, (totalVisible + m_pageSize - 1) / m_pageSize);
    if (jumpValidator) jumpValidator->setTop(totalPages);
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
void RoleModule::onJumpPage()
{
    int page = jumpEdit->text().toInt();
    if (page < 1) return;
    m_currentPage = page;
    updatePagination();
    jumpEdit->clear();
    jumpEdit->clearFocus();
}

// ═══════════════════════════════════════════
// 增删改操作
// ═══════════════════════════════════════════

void RoleModule::onAddEmployee()
{
    AddEmployeeDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        EmployeeInfo info = dlg.employeeInfo();
        // 自动生成新工号
        int maxId = 0;
        for (int i = 0; i < empTable->rowCount(); ++i) {
            QTableWidgetItem *item = empTable->item(i, 1);
            if (item && item->text().startsWith("E")) {
                int cur = item->text().mid(1).toInt();
                if (cur > maxId) maxId = cur;
            }
        }
        info.id = QString("E%1").arg(maxId + 1, 3, 10, QChar('0'));
        
        // 同时更新 UI 和数据中心
        StaffDataManager::instance()->addStaff(info);
        addEmployeeRow(info.id, info.name, info.role, info.status, info.gender, info.age, 
                       info.phone, info.email, info.idCard, info.baseSalary, 0, 0, info.imgPath);
        
        updateStats();
        updatePagination();
    }
}

void RoleModule::onEditEmployee()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    for (int i = 0; i < empTable->rowCount(); ++i) {
        QWidget *w = empTable->cellWidget(i, 6); // 操作按钮在第 6 列
        if (w && w->layout() && w->layout()->indexOf(btn) != -1) {
             // 核心：直接从 UserRole 获取完整对象
             EmployeeInfo info = empTable->item(i, 1)->data(Qt::UserRole).value<EmployeeInfo>();
             
             AddEmployeeDialog dlg(this);
             dlg.setEmployeeInfo(info);
             if (dlg.exec() == QDialog::Accepted) {
                  EmployeeInfo newInfo = dlg.employeeInfo();
                  
                  // 同步更新数据中心
                  StaffDataManager::instance()->updateStaff(newInfo);
                  
                  // 原位更新 UI
                  empTable->removeRow(i);
                  empTable->insertRow(i);
                  addEmployeeRowInPlace(i, newInfo);
                  updateStats();
                  updatePagination();
             }
             break;
        }
    }
}

void RoleModule::onDeleteEmployee()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    for (int i = 0; i < empTable->rowCount(); ++i) {
        QWidget *w = empTable->cellWidget(i, 6);
        if (w && w->layout() && w->layout()->indexOf(btn) != -1) {
            EmployeeInfo info = empTable->item(i, 1)->data(Qt::UserRole).value<EmployeeInfo>();
            if (CustomMessageDialog::confirm(this, "删除确认", QString("确定要删除员工 [%1] 的档案吗？").arg(info.name))) {
                StaffDataManager::instance()->removeStaff(info.id);
                empTable->removeRow(i);
                updateStats();
                updatePagination();
            }
            break;
        }
    }
}

void RoleModule::onBatchDelete()
{
    QList<int> checkedRows;
    for (int i = empTable->rowCount() - 1; i >= 0; --i) {
        QWidget *w = empTable->cellWidget(i, 0);
        if (w) {
            QCheckBox *cb = w->findChild<QCheckBox*>();
            if (cb && cb->isChecked()) {
                checkedRows.append(i);
            }
        }
    }
    if (checkedRows.isEmpty()) return;

    if (CustomMessageDialog::confirm(this, "批量删除", QString("确定要删除选中的 %1 名员工吗？").arg(checkedRows.size()))) {
        for (int row : checkedRows) {
            empTable->removeRow(row);
        }
        updateStats();
        updatePagination();
    }
}

void RoleModule::onCurrentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    Q_UNUSED(currentColumn);
    Q_UNUSED(previousRow);
    Q_UNUSED(previousColumn);

    if (currentRow < 0 || currentRow >= empTable->rowCount()) {
        if (m_drawer) m_drawer->hideDrawer();
        return;
    }

    QTableWidgetItem *idItem = empTable->item(currentRow, 1);
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
