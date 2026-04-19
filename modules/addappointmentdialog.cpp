#include "addappointmentdialog.h"
#include <QDate>
#include <QTime>
#include <QGraphicsDropShadowEffect>
#include <QMessageBox>
#include "custommessagedialog.h"

AddAppointmentDialog::AddAppointmentDialog(QWidget *parent) : QDialog(parent) {
    setupUI();
}

void AddAppointmentDialog::setupUI() {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(480, 680); 

    QWidget *bgWidget = new QWidget(this);
    bgWidget->setObjectName("bgWidget");
    bgWidget->setGeometry(10, 10, 460, 660);
    bgWidget->setStyleSheet(
        "QWidget#bgWidget { background-color: white; border-radius: 12px; }"
        "QLabel { color: #606266; font-size: 14px; margin-top: 10px; }"
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 4px; padding: 8px; background-color: #f5f7fa; font-size: 13px; }"
        "QLineEdit:focus { border: 1px solid #409eff; background-color: #ffffff; }"
        "QComboBox { border: 1px solid #dcdfe6; border-radius: 4px; padding: 6px 12px; background-color: #f5f7fa; font-size: 13px; }"
        "QComboBox:focus { border: 1px solid #409eff; background-color: #ffffff; }"
        "QComboBox::drop-down { border: none; width: 24px; }"
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 14px; height: 14px; }"
        "QComboBox QAbstractItemView { "
        "   border: 1px solid #ebeef5; "
        "   border-radius: 4px; "
        "   background-color: white; "
        "   outline: none; "
        "   padding: 4px 0px; "
        "} "
        "QComboBox QAbstractItemView::item { "
        "   height: 35px; "
        "   padding-left: 12px; "
        "   color: #606266; "
        "   background-color: white; "
        "} "
        "QComboBox QAbstractItemView::item:selected { "
        "   background-color: #f0f7ff; "
        "   color: #409eff; "
        "   border-left: 3px solid #409eff; "
        "}"
        "QPushButton#saveBtn { "
        "   background-color: #409eff; color: white; border: none; border-radius: 4px; "
        "   padding: 0px; text-align: center; font: 14px 'Microsoft YaHei'; "
        "}"
        "QPushButton#cancelBtn { "
        "   background-color: #f4f4f5; color: #606266; border: none; border-radius: 4px; "
        "   padding: 0px; text-align: center; font: 14px 'Microsoft YaHei'; "
        "}"
        "QPushButton:hover { opacity: 0.8; }"
    );

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 60));
    shadow->setOffset(0, 4);
    bgWidget->setGraphicsEffect(shadow);

    QVBoxLayout *mainLayout = new QVBoxLayout(bgWidget);
    mainLayout->setContentsMargins(30, 20, 30, 20);
    mainLayout->setSpacing(5);

    titleLabel = new QLabel("新增预约服务", this);
    titleLabel->setStyleSheet("font-size: 18px; color: #303133; margin-bottom: 10px;");
    mainLayout->addWidget(titleLabel);

    // 会员信息录入
    mainLayout->addWidget(new QLabel("会员姓名:"));
    nameEdit = new QLineEdit();
    nameEdit->setPlaceholderText("请输入会员姓名");
    mainLayout->addWidget(nameEdit);

    mainLayout->addWidget(new QLabel("联系电话:"));
    phoneEdit = new QLineEdit();
    phoneEdit->setPlaceholderText("请输入手机号码");
    mainLayout->addWidget(phoneEdit);

    // 时间选择
    mainLayout->addWidget(new QLabel("预约日期:"));
    QHBoxLayout *dateLayout = new QHBoxLayout();
    yearCombo = new QComboBox();
    QDate cur = QDate::currentDate();
    for(int i = 0; i < 3; ++i) yearCombo->addItem(QString::number(cur.year() + i));
    
    monthCombo = new QComboBox();
    dayCombo = new QComboBox();
    
    dateLayout->addWidget(yearCombo);
    dateLayout->addWidget(new QLabel("年"));
    dateLayout->addWidget(monthCombo);
    dateLayout->addWidget(new QLabel("月"));
    dateLayout->addWidget(dayCombo);
    dateLayout->addWidget(new QLabel("日"));
    
    hourCombo = new QComboBox();
    for(int i = 9; i <= 22; ++i) hourCombo->addItem(QString::number(i).rightJustified(2, '0') + ":00");
    dateLayout->addWidget(hourCombo);
    
    dateLayout->setSpacing(8);
    mainLayout->addLayout(dateLayout);

    // 其他选择
    mainLayout->addWidget(new QLabel("服务项目:"));
    serviceCombo = new QComboBox();
    serviceCombo->addItems({"宠物洗护", "精修造型", "疫苗接种", "驱虫处理", "寄养检查"});
    mainLayout->addWidget(serviceCombo);

    mainLayout->addWidget(new QLabel("工作台/工位:"));
    stationCombo = new QComboBox();
    stationCombo->addItems({"美容台A", "美容台B", "浴室A", "浴室B", "隔离区"});
    mainLayout->addWidget(stationCombo);

    mainLayout->addWidget(new QLabel("工作人员:"));
    staffCombo = new QComboBox();
    staffCombo->addItems({"张三 (高级)", "李四 (中级)", "王五 (学徒)", "赵六 (兽医)"});
    mainLayout->addWidget(staffCombo);

    mainLayout->addStretch();

    QHBoxLayout *btnLayout = new QHBoxLayout();
    saveBtn = new QPushButton("确认预约");
    saveBtn->setObjectName("saveBtn");
    saveBtn->setFixedSize(110, 36);
    cancelBtn = new QPushButton("取消");
    cancelBtn->setObjectName("cancelBtn");
    cancelBtn->setFixedSize(80, 36);
    
    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(saveBtn);
    mainLayout->addLayout(btnLayout);

    // 逻辑连接
    connect(yearCombo, &QComboBox::currentTextChanged, this, &AddAppointmentDialog::validateDate);
    connect(monthCombo, &QComboBox::currentTextChanged, this, &AddAppointmentDialog::validateDate);
    connect(dayCombo, &QComboBox::currentTextChanged, this, &AddAppointmentDialog::validateDate);
    // 初始化月份和天数
    validateDate();

    connect(saveBtn, &QPushButton::clicked, this, &AddAppointmentDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void AddAppointmentDialog::accept() {
    // 1. 会员姓名和手机号校验
    if (nameEdit->text().trimmed().isEmpty() || phoneEdit->text().trimmed().isEmpty()) {
        CustomMessageDialog::showWarning(this, "输入错误", "会员姓名和手机号不能为空！");
        return;
    }

    // 2. 最终日期校验
    QDate selectedDate(yearCombo->currentText().toInt(), monthCombo->currentText().toInt(), dayCombo->currentText().toInt());
    QDate today = QDate::currentDate();
    
    if (selectedDate < today) {
        CustomMessageDialog::showWarning(this, "时间错误", "预约日期不能早于今天！");
        return;
    }

    // 3. 如果是今天，校验小时
    if (selectedDate == today) {
        int selectedHour = hourCombo->currentText().split(":").at(0).toInt();
        if (selectedHour <= QTime::currentTime().hour()) {
            CustomMessageDialog::showWarning(this, "时间错误", "预约的小时必须在当前时间之后！");
            return;
        }
    }

    QDialog::accept();
}

void AddAppointmentDialog::validateDate() {
    QDate cur = QDate::currentDate();
    int selY = yearCombo->currentText().toInt();
    
    // 1. 更新月份列表
    QString oldM = monthCombo->currentText();
    monthCombo->blockSignals(true);
    monthCombo->clear();
    int startM = (selY == cur.year()) ? cur.month() : 1;
    for(int m = startM; m <= 12; ++m) 
        monthCombo->addItem(QString::number(m).rightJustified(2, '0'));
    
    if (monthCombo->findText(oldM) != -1) monthCombo->setCurrentText(oldM);
    else monthCombo->setCurrentIndex(0);
    monthCombo->blockSignals(false);

    // 2. 更新日期列表
    int selM = monthCombo->currentText().toInt();
    QString oldD = dayCombo->currentText();
    dayCombo->blockSignals(true);
    dayCombo->clear();
    
    int startD = (selY == cur.year() && selM == cur.month()) ? cur.day() : 1;
    int maxD = QDate(selY, selM, 1).daysInMonth();
    for(int d = startD; d <= maxD; ++d)
        dayCombo->addItem(QString::number(d).rightJustified(2, '0'));
        
    if (dayCombo->findText(oldD) != -1) dayCombo->setCurrentText(oldD);
    else dayCombo->setCurrentIndex(0);
    dayCombo->blockSignals(false);
}

void AddAppointmentDialog::setInitialData(const AppointmentInfo &info) {
    titleLabel->setText("修改预约信息");
    saveBtn->setText("确认修改");
    
    nameEdit->setText(info.memberName);
    phoneEdit->setText(info.memberPhone);
    
    QStringList dateParts = info.date.split("-");
    if (dateParts.size() == 3) {
        yearCombo->setCurrentText(dateParts[0]);
        validateDate(); // 先刷出对应月份和天数
        monthCombo->setCurrentText(dateParts[1]);
        validateDate();
        dayCombo->setCurrentText(dateParts[2]);
    }
    
    hourCombo->setCurrentText(info.hour);
    serviceCombo->setCurrentText(info.service);
    stationCombo->setCurrentText(info.station);
    staffCombo->setCurrentText(info.staff);
}

AppointmentInfo AddAppointmentDialog::getAppointmentInfo() const {
    AppointmentInfo info;
    info.memberName = nameEdit->text();
    info.memberPhone = phoneEdit->text();
    info.date = QString("%1-%2-%3").arg(yearCombo->currentText()).arg(monthCombo->currentText()).arg(dayCombo->currentText());
    info.hour = hourCombo->currentText();
    info.service = serviceCombo->currentText();
    info.station = stationCombo->currentText();
    info.staff = staffCombo->currentText();
    return info;
}
