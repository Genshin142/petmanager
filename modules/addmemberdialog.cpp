#include "addmemberdialog.h"
#include "ui_addmemberdialog.h"
#include <QMessageBox>
#include "custommessagedialog.h"
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QPushButton>
#include <QIntValidator>
#include <QDate>
#include <QComboBox>
#include <QBoxLayout>
#include "custom_calendar_edit.h"

AddMemberDialog::AddMemberDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddMemberDialog)
{
    ui->setupUi(this);
    
    // 取消系统自带标题栏
    this->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground);

    // 为主布局添加阴影背景框
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(ui->bgFrame);
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 60));
    shadow->setOffset(0, 0);
    ui->bgFrame->setGraphicsEffect(shadow);
 
    // 创建标题栏标签
    QLabel *titleLabel = new QLabel("新增会员档案", ui->bgFrame);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setStyleSheet("font-size: 18px; color: #303133; font-weight: bold; background: transparent; margin-bottom: 10px;");
    
    if (ui->bgFrame->layout()) {
        ui->bgFrame->layout()->setContentsMargins(25, 20, 25, 20);
        QVBoxLayout *vbox = qobject_cast<QVBoxLayout*>(ui->bgFrame->layout());
        if (vbox) {
            vbox->insertWidget(0, titleLabel);
        } else {
            ui->bgFrame->layout()->addWidget(titleLabel);
        }
    }

    // 设置样式 (同步“全部等级”风格)
    this->setStyleSheet(
        "QDialog { background-color: transparent; }"
        "QFrame#bgFrame { background-color: #ffffff; border-radius: 12px; }" // 稍微增加圆角
        "QLabel { font-size: 14px; color: #64748b; font-weight: 500; }"
        
        // 输入框样式
        "QLineEdit { border: 1px solid #e2e8f0; border-radius: 6px; padding: 6px 10px; font-size: 14px; background-color: #f8fafc; color: #0f172a; }"
        "QLineEdit:focus { border: 2px solid #3b82f6; background-color: #ffffff; padding: 5px 9px; }"
        
        // 下拉框样式 (抄“全部等级”)
        "QComboBox { border: 1px solid #e2e8f0; border-radius: 6px; padding: 6px 12px; font-size: 14px; background-color: #f8fafc; color: #0f172a; }"
        "QComboBox:hover { border-color: #cbd5e1; background-color: #f1f5f9; }"
        "QComboBox:focus { border: 2px solid #3b82f6; background-color: #ffffff; padding: 5px 11px; }"
        "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 30px; border-left: none; }"
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 16px; height: 16px; }"
        "QComboBox QAbstractItemView { border: 1px solid #e2e8f0; border-radius: 8px; background-color: white; selection-background-color: #eff6ff; selection-color: #1e40af; outline: none; padding: 4px; }"
        "QComboBox QAbstractItemView::item { height: 32px; padding-left: 10px; border-radius: 4px; }"
        
        // 按钮样式
        "QPushButton { background-color: #3b82f6; color: white; border: none; border-radius: 6px; padding: 10px 24px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: #2563eb; }"
        "QPushButton:pressed { background-color: #1d4ed8; }"
    );

    ui->label_id->setVisible(false);
    ui->idEdit->setVisible(false);

    ui->genderCombo->addItems({"男", "女"});
    
    // --- 生日选择器重构 (CustomCalendarEdit) ---
    m_birthdayEdit = new CustomCalendarEdit(this);
    m_birthdayEdit->setPlaceholderText("请选择会员生日");
    m_birthdayEdit->setText("2000-01-01");
    m_birthdayEdit->setFixedHeight(36); // 稍微增加高度
    m_birthdayEdit->setFixedWidth(200); 
    
    // 注入新控件逻辑
    QBoxLayout *targetLayout = nullptr;
    QList<QBoxLayout*> allLayouts = this->findChildren<QBoxLayout*>();
    for (QBoxLayout *l : allLayouts) {
        if (l->indexOf(ui->yearCombo) != -1) {
            targetLayout = l;
            break;
        }
    }

    if (targetLayout) {
        int index = targetLayout->indexOf(ui->yearCombo);
        targetLayout->insertWidget(index, m_birthdayEdit);
    } else {
        if (ui->yearCombo->parentWidget() && ui->yearCombo->parentWidget()->layout()) {
            QBoxLayout *bl = qobject_cast<QBoxLayout*>(ui->yearCombo->parentWidget()->layout());
            if (bl) {
                int index = bl->indexOf(ui->yearCombo);
                if (index != -1) bl->insertWidget(index, m_birthdayEdit);
                else bl->addWidget(m_birthdayEdit);
            }
        }
    }

    ui->yearCombo->hide();
    ui->monthCombo->hide();
    ui->dayCombo->hide();
    
    QList<QLabel*> labels = ui->bgFrame->findChildren<QLabel*>();
    for (auto label : labels) {
        if (label->text() == "年" || label->text() == "月" || label->text() == "日") {
            label->hide();
        }
    }

    ui->levelCombo->addItem("普通会员");
    ui->levelCombo->addItem("黄金会员");
    ui->levelCombo->addItem("铂金会员");
    ui->levelCombo->addItem("钻石会员");
    ui->levelCombo->setCurrentText("普通会员");

    QPushButton *saveBtn = ui->buttonBox->button(QDialogButtonBox::Save);
    QPushButton *cancelBtn = ui->buttonBox->button(QDialogButtonBox::Cancel);
    
    if (saveBtn) {
        saveBtn->setCursor(Qt::PointingHandCursor);
        saveBtn->setText("保存");
    }
    if (cancelBtn) {
        cancelBtn->setCursor(Qt::PointingHandCursor);
        cancelBtn->setText("取消");
        cancelBtn->setStyleSheet("background-color: #f1f5f9; color: #64748b;");
    }
}

AddMemberDialog::~AddMemberDialog()
{
    delete ui;
}

MemberInfo AddMemberDialog::getMemberInfo() const
{
    MemberInfo info = m_info;
    info.id = ui->idEdit->text();
    info.name = ui->nameEdit->text();
    info.gender = ui->genderCombo->currentText();
    info.phone = ui->phoneEdit->text();
    info.level = ui->levelCombo->currentText();
    info.birthday = m_birthdayEdit->text();
    return info;
}

void AddMemberDialog::setInitialData(const MemberInfo &info)
{
    m_info = info;
    QLabel *titleLabel = findChild<QLabel*>("titleLabel");
    if (titleLabel) {
        titleLabel->setText("修改会员信息");
    }

    ui->label_id->setVisible(true);
    ui->idEdit->setVisible(true);
    ui->idEdit->setReadOnly(true);
    ui->idEdit->setStyleSheet("QLineEdit { background-color: #f1f5f9; color: #94a3b8; border: 1px solid #e2e8f0; }");

    ui->idEdit->setText(info.id);
    ui->nameEdit->setText(info.name);
    ui->phoneEdit->setText(info.phone);
    
    int gIdx = ui->genderCombo->findText(info.gender);
    if (gIdx != -1) ui->genderCombo->setCurrentIndex(gIdx);
    
    m_birthdayEdit->setText(info.birthday);
    
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
