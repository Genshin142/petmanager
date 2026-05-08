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
#include <QPainterPath>

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
    setMinimumSize(560, 780); // 增加尺寸以容纳更多字段

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
    titleLabel->setStyleSheet("font-size: 18px; color: #303133; font-weight: bold;");
    mainLayout->addWidget(titleLabel);

    // 4. 头像置顶区域
    QHBoxLayout *avatarArea = new QHBoxLayout();
    avatarArea->setContentsMargins(0, 10, 0, 10);
    avatarArea->setSpacing(20);

    avatarPreview = new QLabel();
    avatarPreview->setFixedSize(80, 80);
    // 使用 border-radius 实现圆形效果（配合 setScaledContents）
    avatarPreview->setStyleSheet("border: 2px solid #f0f2f5; border-radius: 40px; background: #f5f7fa;");
    avatarPreview->setCursor(Qt::PointingHandCursor);
    avatarPreview->installEventFilter(this);
    
    QVBoxLayout *avatarBtnLayout = new QVBoxLayout();
    avatarBtnLayout->setSpacing(8);
    avatarBtnLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QPushButton *chooseBtn = new QPushButton("更换员工照片");
    chooseBtn->setFixedWidth(120);
    chooseBtn->setCursor(Qt::PointingHandCursor);
    chooseBtn->setStyleSheet(
        "QPushButton { background: #ffffff; color: #606266; border: 1px solid #dcdfe6; border-radius: 4px; padding: 6px 12px; font-size: 13px; } "
        "QPushButton:hover { border-color: #409eff; color: #409eff; background: #f5f9ff; }"
    );

    QLabel *tipLabel = new QLabel("建议尺寸: 200x200px 正方形图片");
    tipLabel->setStyleSheet("color: #909399; font-size: 11px;");

    avatarBtnLayout->addWidget(chooseBtn);
    avatarBtnLayout->addWidget(tipLabel);

    avatarArea->addWidget(avatarPreview);
    avatarArea->addLayout(avatarBtnLayout);
    avatarArea->addStretch();
    mainLayout->addLayout(avatarArea);

    connect(chooseBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "选择员工照片", "E:/QT/work/PetManager/images", "Images (*.png *.jpg *.jpeg)");
        if (!path.isEmpty()) {
            m_selectedImgPath = path;
            // 实时更新并确保圆形显示
            updateAvatarPreview(path);
        }
    });

    // 5. 表单区域 - 使用网格布局
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
    statusCombo->addItems({"在岗", "离岗", "请假"});
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

    // 第七行：学历
    addLabel("最高学历:", 6, 0);
    eduCombo = new QComboBox();
    eduCombo->addItems({"大专", "本科", "硕士", "博士", "其他"});
    formLayout->addWidget(eduCombo, 6, 1, 1, 3);

    // 第八行：基本薪资 / 入职日期
    addLabel("基本底薪:", 7, 0);
    salarySpin = new QSpinBox();
    salarySpin->setRange(0, 99999);
    salarySpin->setSuffix(" 元");
    formLayout->addWidget(salarySpin, 7, 1);

    addLabel("入职日期:", 7, 2);
    joinDateEdit = new CustomCalendarEdit();
    joinDateEdit->setText(QDate::currentDate().toString("yyyy-MM-dd"));
    formLayout->addWidget(joinDateEdit, 7, 3);

    // 第九行：紧急联系人 / 联系电话
    addLabel("紧急联系人:", 8, 0);
    emergencyEdit = new QLineEdit();
    emergencyEdit->setPlaceholderText("姓名");
    formLayout->addWidget(emergencyEdit, 8, 1);

    addLabel("联系电话:", 8, 2);
    emergencyPhoneEdit = new QLineEdit();
    emergencyPhoneEdit->setPlaceholderText("紧急电话");
    formLayout->addWidget(emergencyPhoneEdit, 8, 3);

    // 第十行：家庭住址
    addLabel("家庭住址:", 9, 0);
    addressEdit = new QLineEdit();
    addressEdit->setPlaceholderText("请输入员工当前详细住址");
    formLayout->addWidget(addressEdit, 9, 1, 1, 3);

    // 第十一行：账号与密码 (核心新增)
    addLabel("登录账号:", 10, 0);
    usernameEdit = new QLineEdit();
    usernameEdit->setPlaceholderText("系统自动生成");
    usernameEdit->setReadOnly(true); // 默认设为只读
    usernameEdit->setStyleSheet("QLineEdit { background-color: #f5f7fa; color: #909399; border: 1px solid #dcdfe6; border-radius: 4px; padding: 5px 10px; }");
    formLayout->addWidget(usernameEdit, 10, 1);

    addLabel("登录密码:", 10, 2);
    passwordEdit = new QLineEdit();
    passwordEdit->setPlaceholderText("初始密码");
    passwordEdit->setText("123456"); // 默认密码
    formLayout->addWidget(passwordEdit, 10, 3);

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

void AddEmployeeDialog::setNextAccount(const QString &account)
{
    usernameEdit->setText(account);
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
    
    // HR 新增字段回填
    eduCombo->setCurrentText(info.education.isEmpty() ? "本科" : info.education);
    emergencyEdit->setText(info.emergencyContact);
    emergencyPhoneEdit->setText(info.emergencyPhone);
    addressEdit->setText(info.address);
    if (!info.joinDate.isEmpty()) {
        joinDateEdit->setText(info.joinDate);
    }
    
    // 账号密码回填
    usernameEdit->setText(info.username);
    passwordEdit->setText(info.password);

    // 头像同步
    m_selectedImgPath = info.imgPath;
    if (m_selectedImgPath.isEmpty() || !QFile(m_selectedImgPath).exists()) {
        m_selectedImgPath = info.gender == "女" ? ":/images/female.png" : ":/images/male.png";
    }
    updateAvatarPreview(m_selectedImgPath);
}

void AddEmployeeDialog::updateAvatarPreview(const QString &path)
{
    QPixmap pix(path);
    if (pix.isNull()) return;

    // 创建圆形头像逻辑
    int size = 80;
    QPixmap target(size, size);
    target.fill(Qt::transparent);
    QPainter painter(&target);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    QPainterPath clipPath;
    clipPath.addEllipse(0, 0, size, size);
    painter.setClipPath(clipPath);
    painter.drawPixmap(0, 0, size, size, pix.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    
    avatarPreview->setPixmap(target);
    avatarPreview->setProperty("imgPath", path);
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

    // 提取新增 HR 字段
    info.education = eduCombo->currentText();
    info.department = ""; // 移除部门
    info.emergencyContact = emergencyEdit->text();
    info.emergencyPhone = emergencyPhoneEdit->text();
    info.address = addressEdit->text();
    info.joinDate = joinDateEdit->text();
    info.username = usernameEdit->text();
    info.password = passwordEdit->text();

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
