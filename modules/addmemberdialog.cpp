#include "addmemberdialog.h"
#include "ui_addmemberdialog.h"
#include <QMessageBox>
#include "custommessagedialog.h"
#include <QGraphicsDropShadowEffect>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QPushButton>
#include <QIntValidator>
#include <QDate>
#include <QComboBox>

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
        "   padding: 5px 4px;"
        "   padding-right: 18px;" // 极简内边距，确保小控件内容可见
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
        "   width: 15px;"
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

    // 新增模式下隐藏会员ID行（ID为自增）
    ui->label_id->setVisible(false);
    ui->idEdit->setVisible(false);

    // 初始化性别下拉列表
    ui->genderCombo->addItems({"男", "女"});
    
    // 初始化生日选择器 (三组合框模式)
    int currentYear = QDate::currentDate().year();
    for (int y = currentYear; y >= currentYear - 80; --y) {
        ui->yearCombo->addItem(QString::number(y));
    }
    for (int m = 1; m <= 12; ++m) {
        ui->monthCombo->addItem(QString::number(m).rightJustified(2, '0'));
    }

    auto updateDays = [this]() {
        int year = ui->yearCombo->currentText().toInt();
        int month = ui->monthCombo->currentText().toInt();
        int days = QDate(year, month, 1).daysInMonth();
        
        QString currentDay = ui->dayCombo->currentText();
        ui->dayCombo->clear();
        for (int d = 1; d <= days; ++d) {
            ui->dayCombo->addItem(QString::number(d).rightJustified(2, '0'));
        }
        int idx = ui->dayCombo->findText(currentDay);
        if (idx != -1) ui->dayCombo->setCurrentIndex(idx);
    };

    connect(ui->yearCombo, &QComboBox::currentTextChanged, this, updateDays);
    connect(ui->monthCombo, &QComboBox::currentTextChanged, this, updateDays);
    
    // 初始触发一次生成日
    updateDays();
    
    // 设置宽度样式
    ui->yearCombo->setFixedWidth(95);
    ui->monthCombo->setFixedWidth(75);
    ui->dayCombo->setFixedWidth(75);
    
    // 默认值
    ui->yearCombo->setCurrentText("2000");
    ui->monthCombo->setCurrentText("01");
    ui->dayCombo->setCurrentText("01");

    // 初始化会员等级下拉列表
    ui->levelCombo->addItem("普通会员");
    ui->levelCombo->addItem("黄金会员");
    ui->levelCombo->addItem("铂金会员");
    ui->levelCombo->addItem("钻石会员");
    ui->levelCombo->setCurrentText("普通会员");

    // 修改现有的 buttonBox 样式
    QPushButton *saveBtn = ui->buttonBox->button(QDialogButtonBox::Save);
    QPushButton *cancelBtn = ui->buttonBox->button(QDialogButtonBox::Cancel);
    
    // 移除已删除字段的校验器 (balanceEdit, consumeAmtEdit, pointsEdit)
    
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
    info.id = ui->idEdit->text();
    info.name = ui->nameEdit->text();
    info.gender = ui->genderCombo->currentText();
    info.phone = ui->phoneEdit->text();
    info.level = ui->levelCombo->currentText();
    
    // 从三组合框拼接生日字符串
    info.birthday = QString("%1-%2-%3")
            .arg(ui->yearCombo->currentText())
            .arg(ui->monthCombo->currentText())
            .arg(ui->dayCombo->currentText());
    
    // 财务字段初始化为0（对于新会员）或保持原值
    info.balance = 0.0;
    info.consume_amt = 0.0;
    info.points = 0;
    return info;
}

void AddMemberDialog::setInitialData(const MemberInfo &info)
{
    // 修改标题
    QLabel *titleLabel = findChild<QLabel*>("titleLabel");
    if (titleLabel) {
        titleLabel->setText("修改会员信息");
    }

    // 编辑模式下显示ID（只读）
    ui->label_id->setVisible(true);
    ui->idEdit->setVisible(true);
    ui->idEdit->setReadOnly(true);
    ui->idEdit->setStyleSheet(ui->idEdit->styleSheet() + "QLineEdit { background-color: #ebeef5; color: #909399; }");

    // 填充数据
    ui->idEdit->setText(info.id);
    ui->nameEdit->setText(info.name);
    ui->phoneEdit->setText(info.phone);
    
    // 性别定位
    int gIdx = ui->genderCombo->findText(info.gender);
    if (gIdx != -1) ui->genderCombo->setCurrentIndex(gIdx);
    
    // 生日定位 (拆分字符串到三组合框)
    QStringList dateParts = info.birthday.split("-");
    if (dateParts.size() == 3) {
        ui->yearCombo->setCurrentText(dateParts[0]);
        ui->monthCombo->setCurrentText(dateParts[1]);
        ui->dayCombo->setCurrentText(dateParts[2]);
    }
    
    // 选中对应的等级
    int index = ui->levelCombo->findText(info.level);
    if (index != -1) {
        ui->levelCombo->setCurrentIndex(index);
    }
}

void AddMemberDialog::accept()
{
    if (ui->nameEdit->text().trimmed().isEmpty() || ui->phoneEdit->text().trimmed().isEmpty()) {
        CustomMessageDialog::showWarning(this, "输入错误", "会员姓名和手机号不能为空！");
        return;
    }
    QDialog::accept();
}
