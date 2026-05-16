#include "addpetdialog.h"
#include "ui_addpetdialog.h"
#include <QGraphicsDropShadowEffect>
#include <QDate>
#include "custommessagedialog.h"
#include <QFileDialog>
#include <QPainter>
#include <QPainterPath>
#include <QTextEdit>
#include <QShowEvent>
#include <QDoubleValidator>

AddPetDialog::AddPetDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddPetDialog)
{
    ui->setupUi(this);
    setupUI();
}

AddPetDialog::~AddPetDialog()
{
    delete ui;
}

void AddPetDialog::setupUI()
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // 1. 初始化成员变量
    genderCombo = new QComboBox();
    genderCombo->addItems({"公", "母"});
    ageYearEdit = new QLineEdit();
    ageYearEdit->setFixedWidth(80);
    ageMonthCombo = new QComboBox();
    for (int i = 1; i <= 12; ++i) ageMonthCombo->addItem(QString::number(i));
    ageMonthCombo->setFixedWidth(80);
    healthCombo = new QComboBox();
    healthCombo->addItems({"健康", "良好", "一般", "亚健康", "疾病中", "康复中"});
    ownerIdEdit = new QLineEdit();
    ownerIdEdit->setReadOnly(true);
    ownerNameEdit = new QLineEdit();
    ownerNameEdit->setReadOnly(true);
    ownerPhoneEdit = new QLineEdit();
    ownerPhoneEdit->setReadOnly(true);
    weightEdit = new QLineEdit();
    weightEdit->setPlaceholderText("请输入宠物体重");
    weightEdit->setValidator(new QDoubleValidator(0.0, 999.99, 2, this));
    statusCombo = new QComboBox();
    statusCombo->addItems({"在家", "已预约", "洗护中", "寄养中", "待接走", "接送中"});
    joinTimeEdit = new QLineEdit();
    joinTimeEdit->setText(QDate::currentDate().toString("yyyy-MM-dd"));

    // 初始化头像 UI
    avatarLabel = new QLabel();
    avatarLabel->setFixedSize(90, 90);
    avatarLabel->setStyleSheet("border-radius: 45px; background: #f5f7fa; border: 1px solid #dcdfe6;");
    avatarLabel->setAlignment(Qt::AlignCenter);
    avatarLabel->setCursor(Qt::PointingHandCursor);
    avatarLabel->installEventFilter(this); // 安装过滤器以响应点击
    
    selectImageBtn = new QPushButton("上传照片");
    selectImageBtn->setFixedWidth(100);
    selectImageBtn->setCursor(Qt::PointingHandCursor);
    selectImageBtn->setStyleSheet(
        "QPushButton { background: white; color: #606266; border: 1px solid #dcdfe6; border-radius: 4px; padding: 5px; font-size: 12px; } "
        "QPushButton:hover { border-color: #409eff; color: #409eff; }"
    );
    connect(selectImageBtn, &QPushButton::clicked, this, &AddPetDialog::onSelectImageClicked);

    // 2. 窗口阴影
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(25);
    shadow->setColor(QColor(0, 0, 0, 50));
    shadow->setOffset(0, 5);
    ui->bgFrame->setGraphicsEffect(shadow);

    // 3. 【恢复】系统统一的高级美化样式 (QSS)
    QString qss =
        "QLabel { color: #606266; font-size: 14px; }"
        "QLineEdit { "
        "   border: 1px solid #dcdfe6; border-radius: 4px; padding: 0 12px; height: 34px; background: white; color: #606266; "
        "} "
        "QLineEdit:focus { border-color: #409eff; } "
        "QLineEdit:read-only { background-color: #f5f7fa; color: #909399; } "
        "QComboBox { "
        "   border: 1px solid #dcdfe6; border-radius: 4px; padding: 0 12px; height: 34px; background: white; color: #606266; "
        "} "
        "QComboBox:hover { border-color: #c0c4cc; } "
        "QComboBox:focus { border-color: #409eff; } "
        "QComboBox::drop-down { border: none; width: 30px; } "
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 14px; height: 14px; } "
        "QComboBox QAbstractItemView { "
        "   border: 1px solid #ebeef5; border-radius: 4px; background-color: white; outline: none; padding: 4px 0px; "
        "} "
        "QComboBox QAbstractItemView::item { "
        "   height: 35px; padding-left: 12px; color: #606266; "
        "} "
        "QComboBox QAbstractItemView::item:selected { "
        "   background-color: #f0f7ff; color: #409eff; border-left: 3px solid #409eff; "
        "} "
        "QPushButton#saveBtn { "
        "   background-color: #409eff; color: white; border-radius: 4px; font: 14px 'Microsoft YaHei'; border: none; padding: 10px 30px; "
        "} "
        "QPushButton#saveBtn:hover { background-color: #66b1ff; } "
        "QPushButton#cancelBtn { "
        "   background-color: #f4f4f5; color: #606266; border-radius: 4px; font: 14px 'Microsoft YaHei'; border: none; padding: 10px 30px; "
        "} "
        "QPushButton#cancelBtn:hover { background-color: #e9e9eb; }";
    
    this->setStyleSheet(qss);
    
    // 取消固定高度，交由系统根据 padding 和 font-size 自然撑开，确保文字不被切割
    ui->saveBtn->setMinimumWidth(100);
    ui->cancelBtn->setMinimumWidth(80);
    
    // 4. 【安全插入】保留原生 UI 结构，直接对原有布局进行优化与行插入
    ui->formLayout->setVerticalSpacing(16);
    ui->formLayout->setContentsMargins(0, 15, 0, 15);
    
    ui->nameEdit->setPlaceholderText("请输入宠物姓名");
    
    // 初始化 QTextEdit 版本的病史和饮食禁忌
    historyTextEdit = new QTextEdit();
    historyTextEdit->setPlaceholderText("请输入详细病史、过敏源或特殊身体状况...");
    historyTextEdit->setFixedHeight(70);
    
    dietaryTextEdit = new QTextEdit();
    dietaryTextEdit->setPlaceholderText("请输入宠物饮食禁忌、偏好或特殊喂食要求...");
    dietaryTextEdit->setFixedHeight(70);

    // 统一 QTextEdit 样式
    QString textEditQss = "QTextEdit { border: 1px solid #dcdfe6; border-radius: 4px; padding: 8px; background: white; color: #606266; font-size: 13px; } "
                          "QTextEdit:focus { border-color: #409eff; }";
    historyTextEdit->setStyleSheet(textEditQss);
    dietaryTextEdit->setStyleSheet(textEditQss);

    // 隐藏并移除原有的 QLineEdit
    ui->formLayout->removeRow(ui->historyEdit);

    initBreedData();
    ui->speciesCombo->addItems(m_breedData.keys());


    // 5. 改用双列网格布局，更紧凑美观
    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->setVerticalSpacing(15);
    gridLayout->setHorizontalSpacing(20);
    gridLayout->setContentsMargins(0, 5, 0, 5);

    QWidget *avatarRow = new QWidget();
    QHBoxLayout *avatarLayout = new QHBoxLayout(avatarRow);
    avatarLayout->setContentsMargins(0, 0, 0, 0);
    avatarLayout->setSpacing(20);
    avatarLayout->addWidget(avatarLabel);
    avatarLayout->addWidget(selectImageBtn);
    avatarLayout->addStretch();
    gridLayout->addWidget(new QLabel("宠物照片:"), 0, 0);
    gridLayout->addWidget(avatarRow, 0, 1, 1, 3);

    gridLayout->addWidget(new QLabel("宠物姓名:"), 1, 0);
    gridLayout->addWidget(ui->nameEdit, 1, 1);
    
    gridLayout->addWidget(new QLabel("性别选择:"), 1, 2);
    gridLayout->addWidget(genderCombo, 1, 3);

    gridLayout->addWidget(new QLabel("宠物种类:"), 2, 0);
    QWidget *speciesWidget = new QWidget();
    QHBoxLayout *speciesLayout = new QHBoxLayout(speciesWidget);
    speciesLayout->setContentsMargins(0,0,0,0);
    speciesLayout->addWidget(ui->speciesCombo);
    speciesLayout->addWidget(ui->breedCombo);
    gridLayout->addWidget(speciesWidget, 2, 1);

    QWidget *ageWidget = new QWidget();
    QHBoxLayout *ageLayout = new QHBoxLayout(ageWidget);
    ageLayout->setContentsMargins(0, 0, 0, 0);
    ageLayout->addWidget(ageYearEdit);
    ageLayout->addWidget(new QLabel("岁"));
    ageLayout->addWidget(ageMonthCombo);
    ageLayout->addWidget(new QLabel("月"));
    ageLayout->addStretch();
    gridLayout->addWidget(new QLabel("宠物年龄:"), 2, 2);
    gridLayout->addWidget(ageWidget, 2, 3);

    gridLayout->addWidget(new QLabel("健康状态:"), 3, 0);
    gridLayout->addWidget(healthCombo, 3, 1);

    QWidget *weightWidget = new QWidget();
    QHBoxLayout *weightLayout = new QHBoxLayout(weightWidget);
    weightLayout->setContentsMargins(0, 0, 0, 0);
    weightLayout->addWidget(weightEdit);
    weightLayout->addWidget(new QLabel("kg"));
    weightLayout->addStretch();
    gridLayout->addWidget(new QLabel("宠物体重:"), 3, 2);
    gridLayout->addWidget(weightWidget, 3, 3);

    gridLayout->addWidget(new QLabel("在店状态:"), 4, 0);
    gridLayout->addWidget(statusCombo, 4, 1);

    gridLayout->addWidget(new QLabel("入店时间:"), 4, 2);
    gridLayout->addWidget(joinTimeEdit, 4, 3);

    gridLayout->addWidget(new QLabel("主人ID:"), 5, 0);
    gridLayout->addWidget(ownerIdEdit, 5, 1);

    gridLayout->addWidget(new QLabel("主人姓名:"), 5, 2);
    gridLayout->addWidget(ownerNameEdit, 5, 3);

    gridLayout->addWidget(new QLabel("联系电话:"), 6, 0);
    gridLayout->addWidget(ownerPhoneEdit, 6, 1, 1, 3);

    gridLayout->addWidget(new QLabel("病史详情:"), 7, 0);
    gridLayout->addWidget(historyTextEdit, 7, 1, 1, 3);

    gridLayout->addWidget(new QLabel("饮食禁忌:"), 8, 0);
    gridLayout->addWidget(dietaryTextEdit, 8, 1, 1, 3);

    ui->label->hide();
    ui->label_2->hide();
    ui->label_3->hide();
    ui->historyEdit->hide();

    for (int i = 0; i < ui->containerLayout->count(); ++i) {
        if (ui->containerLayout->itemAt(i)->layout() == ui->formLayout) {
            ui->containerLayout->takeAt(i);
            break;
        }
    }
    ui->containerLayout->insertLayout(1, gridLayout);
    
    this->setMinimumWidth(650);

    // 6. 信号连接
    connect(ui->speciesCombo, &QComboBox::currentTextChanged, this, &AddPetDialog::onSpeciesChanged);
    onSpeciesChanged(ui->speciesCombo->currentText());
    connect(ui->saveBtn, &QPushButton::clicked, this, &AddPetDialog::onSaveClicked);
    connect(ui->cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    setupBigImageOverlay();
}

void AddPetDialog::setupBigImageOverlay()
{
    m_imagePreviewOverlay = new QWidget(this);
    m_imagePreviewOverlay->setObjectName("PreviewOverlay");
    m_imagePreviewOverlay->setStyleSheet("#PreviewOverlay { background-color: rgba(0, 0, 0, 180); border-radius: 12px; }");
    m_imagePreviewOverlay->hide();
    m_imagePreviewOverlay->installEventFilter(this); // 点击遮罩任意位置关闭
    
    QVBoxLayout *previewL = new QVBoxLayout(m_imagePreviewOverlay);
    m_previewLabel = new QLabel();
    m_previewLabel->setAlignment(Qt::AlignCenter);
    previewL->addWidget(m_previewLabel);
}

bool AddPetDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == avatarLabel && event->type() == QEvent::MouseButtonRelease) {
        showBigImage();
        return true;
    }
    if (obj == m_imagePreviewOverlay && event->type() == QEvent::MouseButtonRelease) {
        hideBigImage();
        return true;
    }
    return QDialog::eventFilter(obj, event);
}

void AddPetDialog::showBigImage()
{
    QPixmap pix(m_avatarPath);
    if (pix.isNull()) pix.load(":/images/load_img.jpg");
    
    // 确保遮罩覆盖整个对话框区域 (考虑到整体边框阴影)
    m_imagePreviewOverlay->setGeometry(ui->bgFrame->geometry());
    
    // 限制预览图最大尺寸为窗口宽度的 90%
    int maxWidth = ui->bgFrame->width() * 0.9;
    m_previewLabel->setPixmap(pix.scaled(maxWidth, maxWidth, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    
    m_imagePreviewOverlay->show();
    m_imagePreviewOverlay->raise(); // 确保遮罩层在最顶层展示
}

void AddPetDialog::hideBigImage()
{
    m_imagePreviewOverlay->hide();
}

void AddPetDialog::initBreedData()
{
    m_breedData["狗"] = {"哈士奇", "柴犬", "金毛", "拉布拉多", "萨摩耶", "边境牧羊犬", "柯基", "贵宾", "德牧", "博美", "其他"};
    m_breedData["猫"] = {"狸花猫", "布偶猫", "英国短毛猫", "美国短毛猫", "暹罗猫", "波斯猫", "缅因猫", "孟加拉豹猫", "斯芬克斯猫", "其他"};
    m_breedData["鸟"] = {"虎皮鹦鹉", "玄凤鹦鹉", "牡丹鹦鹉", "金丝雀", "文鸟", "八哥", "画眉", "其他"};
    m_breedData["鸭"] = {"柯尔鸭", "北京鸭", "樱桃谷鸭", "连城白鸭", "其他"};
    m_breedData["兔"] = {"荷兰垂耳兔", "侏儒兔", "道奇兔", "安哥拉兔", "新西兰兔", "其他"};
    m_breedData["鱼"] = {"金鱼", "锦鲤", "斗鱼", "孔雀鱼", "神仙鱼", "鹦鹉鱼", "龙鱼", "其他"};
    m_breedData["仓鼠"] = {"三线仓鼠", "一线仓鼠", "老公公仓鼠", "金丝熊", "其他"};
    m_breedData["爬行类"] = {"豹纹守宫", "鬃狮蜥", "玉米蛇", "巴西龟", "草龟", "绿鬣蜥", "其他"};
    m_breedData["其他"] = {"蜜袋鼯", "龙猫", "荷兰猪", "其他"};
}

void AddPetDialog::onSelectImageClicked()
{
    QString defaultPath = "E:/QT/work/PetManager/images/pets";
    QString fileName = QFileDialog::getOpenFileName(this, "选择宠物照片", defaultPath, "Images (*.png *.jpg *.jpeg *.bmp)");
    if (fileName.isEmpty()) return;

    m_avatarPath = fileName;
    
    // 渲染圆形预览
    QPixmap pixmap(fileName);
    if (pixmap.isNull()) return;

    QSize size(90, 90);
    QPixmap target(size);
    target.fill(Qt::transparent);
    QPainter painter(&target);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addEllipse(0, 0, size.width(), size.height());
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, size.width(), size.height(), 
                       pixmap.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    avatarLabel->setPixmap(target);
}

void AddPetDialog::onSpeciesChanged(const QString &species)
{
    ui->breedCombo->clear();
    if (m_breedData.contains(species)) {
        ui->breedCombo->addItems(m_breedData[species]);
    }
}

void AddPetDialog::setPetInfo(const PetInfo &info)
{
    m_currentId = info.id;
    // 安全检查：只有当 titleLabel 存在时才设置文本，防止空指针崩溃
    if (ui->titleLabel) {
        ui->titleLabel->setText("修改宠物档案信息");
    }
    
    if (ui->nameEdit) ui->nameEdit->setText(info.name);
    if (ui->speciesCombo) ui->speciesCombo->setCurrentText(info.species);
    if (ui->breedCombo) ui->breedCombo->setCurrentText(info.breed);
    if (historyTextEdit) {
        healthCombo->setCurrentText(info.health);
        historyTextEdit->setText(info.medicalHistory);
    }
    if (dietaryTextEdit) {
        dietaryTextEdit->setText(info.dietary);
    }
    
    if (ownerIdEdit) ownerIdEdit->setText(info.ownerId);
    if (ownerNameEdit) ownerNameEdit->setText(info.ownerName);
    if (ownerPhoneEdit) ownerPhoneEdit->setText(info.ownerPhone);
    if (weightEdit && info.weight > 0) weightEdit->setText(QString::number(info.weight, 'f', 2));
    

    genderCombo->setCurrentText(info.gender);
    
    // 解析年龄字符串，例如 "3岁", "6个月", "1岁零2个月"
    QString ageStr = info.age;
    ageYearEdit->clear();
    
    if (ageStr.contains("岁")) {
        QStringList yearParts = ageStr.split("岁");
        ageYearEdit->setText(yearParts[0].trimmed());
        if (yearParts.size() > 1 && (yearParts[1].contains("个月") || yearParts[1].contains("月"))) {
            QString monthStr = yearParts[1];
            monthStr.replace("零", "").replace("个月", "").replace("月", "");
            ageMonthCombo->setCurrentText(monthStr.trimmed());
        }
    } else if (ageStr.contains("月")) {
        QString monthStr = ageStr;
        monthStr.replace("个月", "").replace("月", "");
        ageMonthCombo->setCurrentText(monthStr.trimmed());
    }

    statusCombo->setCurrentText(info.status);
    joinTimeEdit->setText(info.joinTime.isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : info.joinTime);

    // 处理头像显示
    m_avatarPath = info.avatarPath;
    QPixmap pixmap(m_avatarPath);
    if (pixmap.isNull()) pixmap.load(":/images/load_img.jpg");
    
    QSize size(90, 90);
    QPixmap target(size);
    target.fill(Qt::transparent);
    QPainter p(&target);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addEllipse(0, 0, size.width(), size.height());
    p.setClipPath(path);
    p.drawPixmap(0, 0, size.width(), size.height(), 
                 pixmap.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    avatarLabel->setPixmap(target);
}

void AddPetDialog::onSaveClicked()
{
    // 基础必填校验
    if (ui->nameEdit->text().trimmed().isEmpty()) {
        CustomMessageDialog::showWarning(this, "校验失败", "请输入宠物姓名");
        return;
    }


    accept();
}

PetInfo AddPetDialog::getPetInfo() const
{
    PetInfo info;
    info.name = ui->nameEdit->text();
    info.species = ui->speciesCombo->currentText();
    info.breed = ui->breedCombo->currentText();
    info.health = healthCombo->currentText();
    QString medHist = historyTextEdit->toPlainText().trimmed();
    info.medicalHistory = medHist.isEmpty() ? "无" : medHist;
    QString dietary = dietaryTextEdit->toPlainText().trimmed();
    info.dietary = dietary.isEmpty() ? "常规饮食" : dietary;
    info.vaccine = "已接种"; // 默认占位符，实际由疫苗模块管理
    
    info.gender = genderCombo->currentText();
    QString y = ageYearEdit->text().trimmed();
    QString m = ageMonthCombo->currentText();
    if (!y.isEmpty() && !m.isEmpty()) {
        info.age = QString("%1岁零%2个月").arg(y, m);
    } else if (!y.isEmpty()) {
        info.age = QString("%1岁").arg(y);
    } else if (!m.isEmpty()) {
        info.age = QString("%1个月").arg(m);
    } else {
        info.age = "未知";
    }
    info.status = statusCombo->currentText();
    info.joinTime = joinTimeEdit->text();
    info.ownerId = ownerIdEdit->text();
    info.ownerName = ownerNameEdit->text();
    info.ownerPhone = ownerPhoneEdit->text();
    info.weight = weightEdit->text().toDouble();
    info.avatarPath = m_avatarPath; // 保存照片路径
    
    // 简单生成一个 ID
    if (m_currentId.isEmpty()) {
        static int idCounter = 1010;
        info.id = QString("P%1").arg(++idCounter);
    } else {
        info.id = m_currentId;
    }
    return info;
}

void AddPetDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    adjustSize();
    
    QWidget *topLevel = parentWidget();
    while (topLevel && topLevel->parentWidget()) {
        topLevel = topLevel->parentWidget();
    }
    
    if (topLevel) {
        QPoint center = topLevel->mapToGlobal(topLevel->rect().center());
        move(center.x() - width() / 2, center.y() - height() / 2);
    }
}
