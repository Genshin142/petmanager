#include "addemployeedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QFrame>
#include <QFileDialog>
#include <QEvent>
#include <QVariant>
#include <QFile>
#include <QPainter>

AddEmployeeDialog::AddEmployeeDialog(QWidget *parent) : QDialog(parent)
{
    setupUI();
}

AddEmployeeDialog::~AddEmployeeDialog()
{
}

void AddEmployeeDialog::setupUI()
{
    // 1. 无边框与透明背景
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumSize(540, 620); // 增加宽度

    // 2. 主背景容器
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(15, 15, 15, 15);
    
    QFrame *bgFrame = new QFrame();
    bgFrame->setObjectName("bgFrame");
    bgFrame->setStyleSheet(
        "QFrame#bgFrame { background-color: #ffffff; border-radius: 12px; } "
    );
    
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 50));
    shadow->setOffset(0, 2);
    bgFrame->setGraphicsEffect(shadow);
    
    rootLayout->addWidget(bgFrame);

    QVBoxLayout *mainLayout = new QVBoxLayout(bgFrame);
    mainLayout->setContentsMargins(30, 25, 30, 25);
    mainLayout->setSpacing(20);

    // 3. 标题
    titleLabel = new QLabel("录入新员工入职档案");
    titleLabel->setStyleSheet("font-size: 18px; color: #303133;");
    mainLayout->addWidget(titleLabel);

    // 4. 表单区域 - 使用网格布局
    QGridLayout *formLayout = new QGridLayout();
    formLayout->setSpacing(15);
    formLayout->setColumnStretch(1, 1);
    formLayout->setColumnStretch(3, 1);

    auto addLabel = [&](const QString &text, int r, int c) {
        QLabel *lbl = new QLabel(text);
        lbl->setStyleSheet("color: #606266; font-size: 13px;");
        formLayout->addWidget(lbl, r, c);
    };

    // 第一行：姓名 (占满)
    addLabel("员工姓名:", 0, 0);
    nameEdit = new QLineEdit();
    nameEdit->setPlaceholderText("请输入姓名");
    formLayout->addWidget(nameEdit, 0, 1, 1, 3);

    // 第二行：岗位 / 性别
    addLabel("所属岗位:", 1, 0);
    roleCombo = new QComboBox();
    roleCombo->addItems({"店员", "高级美容师", "宠物医生", "实习生", "店长"});
    formLayout->addWidget(roleCombo, 1, 1);

    addLabel("员工性别:", 1, 2);
    genderCombo = new QComboBox();
    genderCombo->addItems({"男", "女"});
    formLayout->addWidget(genderCombo, 1, 3);

    // 第三行：年龄 / 状态
    addLabel("员工年龄:", 2, 0);
    ageBox = new QSpinBox();
    ageBox->setRange(16, 70);
    ageBox->setValue(22);
    formLayout->addWidget(ageBox, 2, 1);

    addLabel("当前状态:", 2, 2);
    statusCombo = new QComboBox();
    statusCombo->addItems({"在岗", "离岗", "请假", "离职"});
    formLayout->addWidget(statusCombo, 2, 3);

    // 第四行：手机号 (跨行?) 不，还是正常排列
    addLabel("联系电话:", 3, 0);
    phoneEdit = new QLineEdit();
    phoneEdit->setPlaceholderText("请输入手机号");
    formLayout->addWidget(phoneEdit, 3, 1, 1, 3); // 占 3 列

    // 第五行：电子邮箱
    addLabel("联系邮箱:", 4, 0);
    emailEdit = new QLineEdit();
    emailEdit->setPlaceholderText("example@pet.com");
    formLayout->addWidget(emailEdit, 4, 1, 1, 3);

    // 第六行：身份证号
    addLabel("身份证号:", 5, 0);
    idCardEdit = new QLineEdit();
    idCardEdit->setPlaceholderText("请输入 18 位身份证号");
    formLayout->addWidget(idCardEdit, 5, 1, 1, 3);

    // 第七行：基本薪资
    addLabel("基本底薪:", 6, 0);
    salarySpin = new QSpinBox();
    salarySpin->setRange(0, 99999);
    salarySpin->setSuffix(" 元");
    formLayout->addWidget(salarySpin, 6, 1);

    // 第八行：员工照片
    addLabel("员工照片:", 7, 0);
    QHBoxLayout *photoLayout = new QHBoxLayout();
    photoLayout->setContentsMargins(0, 0, 0, 0);
    photoLayout->setSpacing(10);
    
    avatarPreview = new QLabel();
    avatarPreview->setFixedSize(40, 40);
    avatarPreview->setStyleSheet("border: 1px solid #dcdfe6; border-radius: 4px; background: #f5f7fa;");
    avatarPreview->setPixmap(QPixmap(":/images/male.png").scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    avatarPreview->setProperty("imgPath", ":/images/male.png");
    m_selectedImgPath = ":/images/male.png"; // 初始状态
    avatarPreview->setCursor(Qt::PointingHandCursor);
    avatarPreview->installEventFilter(this);

    QPushButton *chooseBtn = new QPushButton("选择照片...");
    chooseBtn->setFixedWidth(100);
    chooseBtn->setCursor(Qt::PointingHandCursor);
    chooseBtn->setStyleSheet("QPushButton { background: #f5f7fa; color: #606266; border: 1px solid #dcdfe6; border-radius: 4px; padding: 5px; min-height: 25px; } QPushButton:hover { border-color: #409eff; color: #409eff; }");
    
    connect(chooseBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "选择员工照片", "E:/QT/work/PetManager/images", "Images (*.png *.jpg *.jpeg)");
        if (!path.isEmpty()) {
            m_selectedImgPath = path;
            avatarPreview->setPixmap(QPixmap(path).scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            avatarPreview->setProperty("imgPath", path);
        }
    });

    photoLayout->addWidget(avatarPreview);
    photoLayout->addWidget(chooseBtn);
    photoLayout->addStretch();
    formLayout->addLayout(photoLayout, 7, 1, 1, 3);

    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();

    // 5. 底部按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    btnLayout->addStretch();

    QPushButton *cancelBtn = new QPushButton("取消操作");
    cancelBtn->setMinimumSize(120, 36);
    cancelBtn->setStyleSheet(
        "QPushButton { background: #f4f4f5; color: #909399; border-radius: 4px; border: none; font-size: 13px; padding: 0 15px; } "
        "QPushButton:hover { background: #e9e9eb; } "
    );
    connect(cancelBtn, &QPushButton::clicked, this, &AddEmployeeDialog::onCancel);

    QString inputStyle = "QLineEdit { border: 1px solid #dcdfe6; border-radius: 4px; padding: 5px 10px; font-size: 13px; background: white; } QLineEdit:focus { border-color: #409eff; }";
    QString comboStyle = "QComboBox { border: 1px solid #dcdfe6; border-radius: 4px; padding: 5px 10px; font-size: 13px; background: #f5f7fa; color: #606266; } "
                         "QComboBox:focus { border-color: #409eff; background: white; } "
                         "QComboBox::drop-down { border: none; width: 24px; } "
                         "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 14px; }";
    QString btnStyle = "QPushButton { background: #409eff; color: white; border-radius: 4px; border: none; height: 32px; padding: 0 20px; } "
                       "QPushButton:hover { background: #66b1ff; }";
    QString cancelBtnStyle = "QPushButton { background: white; color: #606266; border-radius: 4px; border: 1px solid #dcdfe6; height: 32px; padding: 0 20px; } "
                             "QPushButton:hover { border-color: #409eff; color: #409eff; }";

    nameEdit->setStyleSheet(inputStyle);
    roleCombo->setStyleSheet(comboStyle);

    saveBtn = new QPushButton("保存并入档");
    saveBtn->setMinimumSize(140, 36);
    saveBtn->setStyleSheet(
        "QPushButton { background: #409eff; color: white; border-radius: 4px; border: none; font-size: 13px; padding: 0 15px; } "
        "QPushButton:hover { background: #66b1ff; } "
        "QPushButton:pressed { background: #3a8ee6; } "
    );
    connect(saveBtn, &QPushButton::clicked, this, &AddEmployeeDialog::onSave);

    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(saveBtn);
    mainLayout->addLayout(btnLayout);

    // 6. 全局样式美化 - 模仿会员管理样式
    QString style = 
        "QLineEdit, QComboBox, QSpinBox { "
        "   border: 1px solid #dcdfe6; "
        "   border-radius: 4px; "
        "   padding: 5px 10px; "
        "   background: #f5f7fa; "
        "   font-size: 13px; "
        "   color: #606266; "
        "   min-height: 25px; "
        "} "
        "QLineEdit:focus, QComboBox:focus, QSpinBox:focus { "
        "   border: 1px solid #409eff; "
        "   background: white; "
        "} "
        "QComboBox::drop-down { "
        "   border: none; width: 25px; "
        "} "
        "QComboBox::down-arrow { "
        "   image: url(:/images/chevron-down.svg); "
        "   width: 12px; height: 12px; "
        "} "
        "QSpinBox::up-button, QSpinBox::down-button { border: none; background: transparent; } ";
    setStyleSheet(style);

    // --- 初始化大图预览层 (这里我们延迟创建，或者直接在 showBigImage 中即时创建) ---
    m_imagePreviewOverlay = nullptr; 
}

void AddEmployeeDialog::setEmployeeInfo(const EmployeeInfo &info)
{
    titleLabel->setText("修改员工档案信息");
    m_id = info.id; // 内部保存工号
    nameEdit->setText(info.name);
    roleCombo->setCurrentText(info.role);
    genderCombo->setCurrentText(info.gender);
    ageBox->setValue(info.age);
    phoneEdit->setText(info.phone);
    emailEdit->setText(info.email);
    idCardEdit->setText(info.idCard);
    salarySpin->setValue(info.baseSalary);
    statusCombo->setCurrentText(info.status);
    
    // 头像同步
    m_selectedImgPath = info.imgPath;
    if (m_selectedImgPath.isEmpty() || !QFile(m_selectedImgPath).exists()) {
        m_selectedImgPath = info.gender == "女" ? ":/images/female.png" : ":/images/male.png";
    }
    avatarPreview->setPixmap(QPixmap(m_selectedImgPath).scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    avatarPreview->setProperty("imgPath", m_selectedImgPath);
}

EmployeeInfo AddEmployeeDialog::employeeInfo() const
{
    EmployeeInfo info;
    info.id = m_id; // 返回保存的工号
    info.name = nameEdit->text();
    info.role = roleCombo->currentText();
    info.gender = genderCombo->currentText();
    info.age = ageBox->value();
    info.phone = phoneEdit->text();
    info.email = emailEdit->text();
    info.idCard = idCardEdit->text();
    info.baseSalary = salarySpin->value();
    info.status = statusCombo->currentText();
    info.imgPath = m_selectedImgPath;
    return info;
}

void AddEmployeeDialog::onSave()
{
    // TODO: 简单校验
    if (nameEdit->text().isEmpty()) return;
    accept();
}

void AddEmployeeDialog::onCancel()
{
    reject();
}

void AddEmployeeDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void AddEmployeeDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - dragPosition);
        event->accept();
    }
}

bool AddEmployeeDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        // 1. 点击头像弹出放大
        if (watched == avatarPreview && watched->property("imgPath").isValid()) {
            showBigImage(watched->property("imgPath").toString());
            return true;
        }
        
        // 2. 点击预览层及其内容关闭放大
        if (m_imagePreviewOverlay && (watched == m_imagePreviewOverlay || watched->parent() == m_imagePreviewOverlay)) {
            hideBigImage();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void AddEmployeeDialog::showBigImage(const QString &path)
{
    if (path.isEmpty()) return;
    
    QPixmap pix(path);
    if (pix.isNull()) return;
    
    // 完全照搬宠物逻辑：给图片加白底，且白底大小严格跟随图片
    QPixmap whiteBg(pix.size());
    whiteBg.fill(Qt::white);
    QPainter p(&whiteBg);
    p.drawPixmap(0, 0, pix);
    p.end();

    // 我们需要通过 parentWidget 向上找真正的主窗口用于定位
    QWidget *topWin = this->parentWidget() ? this->parentWidget()->window() : this->window();

    // 关键修正：将预览窗设为 this 的子窗口，这样它才能在模态对话框之上显示并接收点击
    QDialog *preview = new QDialog(this, Qt::FramelessWindowHint);
    // 依然让它对齐主窗口的全屏位置
    preview->setGeometry(topWin->geometry());
    preview->setAttribute(Qt::WA_TranslucentBackground);
    
    QVBoxLayout *layout = new QVBoxLayout(preview);
    layout->setContentsMargins(0, 0, 0, 0);
    
    QFrame *bg = new QFrame();
    bg->setStyleSheet("background-color: rgba(0, 0, 0, 220);");
    layout->addWidget(bg);
    
    QVBoxLayout *bgLayout = new QVBoxLayout(bg);
    bgLayout->setContentsMargins(0, 0, 0, 0);
    bgLayout->setAlignment(Qt::AlignCenter);
    
    QLabel *imgLabel = new QLabel();
    imgLabel->setAlignment(Qt::AlignCenter);
    imgLabel->setStyleSheet("border: none; background: transparent;"); // 纯净样式
    
    int maxWidth = qMin(topWin->width() * 0.8, 600.0);
    int maxHeight = qMin(topWin->height() * 0.8, 600.0);
    imgLabel->setPixmap(whiteBg.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    bgLayout->addWidget(imgLabel, 0, Qt::AlignCenter);
    
    preview->installEventFilter(this);
    connect(preview, &QDialog::finished, preview, &QDialog::deleteLater);
    m_imagePreviewOverlay = preview; 
    preview->show();
    preview->raise();
}

void AddEmployeeDialog::hideBigImage()
{
    if (m_imagePreviewOverlay) {
        QDialog *dlg = qobject_cast<QDialog*>(m_imagePreviewOverlay);
        if (dlg) dlg->close();
        m_imagePreviewOverlay = nullptr;
    }
}
