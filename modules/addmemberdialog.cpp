#include "addmemberdialog.h"
#include "ui_addmemberdialog.h"
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QPushButton>
#include <QIntValidator>

AddMemberDialog::AddMemberDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddMemberDialog)
{
    ui->setupUi(this);
    
    // 取消系统自带标题栏
    this->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground);

    // 为主布局添加阴影背景框 (复用类似 loginWindow 的风格)
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(ui->bgFrame);
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 60));
    shadow->setOffset(0, 0);
    ui->bgFrame->setGraphicsEffect(shadow);

    // 设置现代化的 UI 样式
    this->setStyleSheet(
        "QDialog {"
        "   background-color: transparent;" // Make dialog completely transparent
        "}"
        "QFrame#bgFrame {"
        "   background-color: #ffffff;"
        "   border-radius: 10px;"
        "}"
        "QLabel {"
        "   font-size: 14px;"
        "   color: #606266;"
        "}"
        "QLineEdit, QComboBox {"
        "   border: 1px solid #dcdfe6;"
        "   border-radius: 4px;"
        "   padding: 5px 10px;"
        "   padding-right: 30px;" // 为右侧箭头留出空间
        "   font-size: 14px;"
        "   background-color: #f5f7fa;"
        "   min-height: 25px;"
        "   color: #606266;"
        "}"
        "QLineEdit:focus, QComboBox:focus {"
        "   border: 1px solid #409eff;"
        "   background-color: #ffffff;"
        "}"
        "QComboBox::drop-down {"
        "   subcontrol-origin: padding;"
        "   subcontrol-position: top right;"
        "   width: 30px;"
        "   border: none;"
        "}"
        "QComboBox::down-arrow {"
        "   image: url(:/images/chevron-down.svg);"
        "   width: 16px;"
        "   height: 16px;"
        "   margin-right: 10px;"
        "}"
        "QComboBox::down-arrow:on {" // 当下拉列表打开时
        "   top: 1px;"
        "   left: 1px;"
        "}"
        "QComboBox QAbstractItemView {"
        "   border: 1px solid #dcdfe6;"
        "   padding: 5px;"
        "   background-color: #ffffff;"
        "   selection-background-color: #409eff;"
        "   selection-color: #ffffff;"
        "   outline: none;"
        "}"
        "QPushButton {"
        "   background-color: #409eff;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 4px;"
        "   padding: 8px 20px;"
        "   font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #66b1ff;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #3a8ee6;"
        "}"
    );

    // 初始化会员等级下拉列表
    ui->levelCombo->addItem("普通会员");
    ui->levelCombo->addItem("白银会员");
    ui->levelCombo->addItem("黄金会员");
    ui->levelCombo->addItem("铂金会员");
    ui->levelCombo->addItem("钻石会员");

    // 修改现有的 buttonBox 样式
    QPushButton *saveBtn = ui->buttonBox->button(QDialogButtonBox::Save);
    QPushButton *cancelBtn = ui->buttonBox->button(QDialogButtonBox::Cancel);
    
    // 给积分输入框增加数字限制
    ui->pointsEdit->setValidator(new QIntValidator(0, 99999, this));
    
    if (saveBtn) {
        saveBtn->setCursor(Qt::PointingHandCursor);
        saveBtn->setText("保存");
    }
    if (cancelBtn) {
        cancelBtn->setCursor(Qt::PointingHandCursor);
        cancelBtn->setText("取消");
        cancelBtn->setStyleSheet(
            "background-color: #f4f4f5;"
            "color: #909399;"
        );
    }
}

AddMemberDialog::~AddMemberDialog()
{
    delete ui;
}

MemberInfo AddMemberDialog::getMemberInfo() const
{
    MemberInfo info;
    info.name = ui->nameEdit->text();
    info.phone = ui->phoneEdit->text();
    info.level = ui->levelCombo->currentText();
    info.points = ui->pointsEdit->text().toInt();
    return info;
}

void AddMemberDialog::setInitialData(const MemberInfo &info)
{
    // 修改标题
    QLabel *titleLabel = findChild<QLabel*>("titleLabel");
    if (titleLabel) {
        titleLabel->setText("修改会员信息");
    }

    // 填充数据
    ui->nameEdit->setText(info.name);
    ui->phoneEdit->setText(info.phone);
    ui->pointsEdit->setText(QString::number(info.points));
    
    // 选中对应的等级
    int index = ui->levelCombo->findText(info.level);
    if (index != -1) {
        ui->levelCombo->setCurrentIndex(index);
    }
    
    // 编辑模式下手机号通常不允许修改，或者作为主键
    // ui->phoneEdit->setEnabled(false); 
}

void AddMemberDialog::on_buttonBox_accepted()
{
    if (ui->nameEdit->text().trimmed().isEmpty() || ui->phoneEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "输入错误", "会员姓名和手机号不能为空！");
        // 不调用 accept() 让弹窗保持开启
        return;
    }
    accept();
}
