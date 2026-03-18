#include "addemployeedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QFrame>

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
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #303133;");
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
    statusCombo->addItems({"正常", "请假", "离职"});
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
    QString btnStyle = "QPushButton { background: #409eff; color: white; border-radius: 4px; border: none; font-weight: bold; height: 32px; padding: 0 20px; } "
                       "QPushButton:hover { background: #66b1ff; }";
    QString cancelBtnStyle = "QPushButton { background: white; color: #606266; border-radius: 4px; border: 1px solid #dcdfe6; height: 32px; padding: 0 20px; } "
                             "QPushButton:hover { border-color: #409eff; color: #409eff; }";

    nameEdit->setStyleSheet(inputStyle);
    roleCombo->setStyleSheet(comboStyle);
    saveBtn->setMinimumSize(140, 36);
    saveBtn->setStyleSheet(
        "QPushButton { background: #409eff; color: white; border-radius: 4px; border: none; font-size: 13px; font-weight: bold; padding: 0 15px; } "
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
        "QSpinBox::up-button, QSpinBox::down-button { border: none; background: transparent; } ";
    setStyleSheet(style);
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
