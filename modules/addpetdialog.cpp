#include "addpetdialog.h"
#include "ui_addpetdialog.h"
#include <QGraphicsDropShadowEffect>
#include <QDate>
#include "custommessagedialog.h"

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

    // 阴影效果
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(25);
    shadow->setColor(QColor(0, 0, 0, 50));
    shadow->setOffset(0, 5);
    ui->bgFrame->setGraphicsEffect(shadow);

    // 样式美化
    QString qss =
        "QLabel { color: #606266; font-size: 14px; }"
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
        "QPushButton:hover { opacity: 0.8; }";
    
    this->setStyleSheet(qss);
    ui->saveBtn->setFixedSize(90, 34);
    ui->cancelBtn->setFixedSize(80, 34);

    initBreedData();
    
    ui->speciesCombo->addItems(m_breedData.keys());
    connect(ui->speciesCombo, &QComboBox::currentTextChanged, this, &AddPetDialog::onSpeciesChanged);
    
    // 初始触发一次加载第一个大类的品种
    onSpeciesChanged(ui->speciesCombo->currentText());

    // 初始化年月日
    int currentYear = QDate::currentDate().year();
    for(int y = currentYear - 10; y <= currentYear ; ++y)
        ui->yearCombo->addItem(QString::number(y));
    for(int m = 1; m <= 12; ++m) 
        ui->monthCombo->addItem(QString::number(m).rightJustified(2, '0'));
    for(int d = 1; d <= 31; ++d) 
        ui->dayCombo->addItem(QString::number(d).rightJustified(2, '0'));

    // 设置默认值为今天
    ui->yearCombo->setCurrentText(QString::number(currentYear));
    ui->monthCombo->setCurrentText(QString::number(QDate::currentDate().month()).rightJustified(2, '0'));
    ui->dayCombo->setCurrentText(QString::number(QDate::currentDate().day()).rightJustified(2, '0'));

    connect(ui->saveBtn, &QPushButton::clicked, this, &AddPetDialog::onSaveClicked);
    connect(ui->cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
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

void AddPetDialog::onSpeciesChanged(const QString &species)
{
    ui->breedCombo->clear();
    if (m_breedData.contains(species)) {
        ui->breedCombo->addItems(m_breedData[species]);
    }
}

void AddPetDialog::onSaveClicked()
{
    // 基础必填校验
    if (ui->nameEdit->text().trimmed().isEmpty()) {
        CustomMessageDialog::showWarning(this, "校验失败", "请输入宠物姓名");
        return;
    }

    // 疫苗日期校验
    int year = ui->yearCombo->currentText().toInt();
    int month = ui->monthCombo->currentText().toInt();
    int day = ui->dayCombo->currentText().toInt();
    
    QDate selectedDate(year, month, day);
    QDate today = QDate::currentDate();

    if (!selectedDate.isValid()) {
        CustomMessageDialog::showWarning(this, "日期错误", "所选日期不合法，请检查月份和天数。");
        return;
    }

    if (selectedDate > today) {
        CustomMessageDialog::showWarning(this, "日期超限", "疫苗接种日期不能超过今天，请重新选择。");
        return;
    }

    accept();
}

PetInfo AddPetDialog::getPetInfo() const
{
    PetInfo info;
    info.name = ui->nameEdit->text();
    info.breed = ui->breedCombo->currentText();
    info.history = ui->historyEdit->text();
    info.vaccine = QString("%1-%2-%3")
                    .arg(ui->yearCombo->currentText())
                    .arg(ui->monthCombo->currentText())
                    .arg(ui->dayCombo->currentText());
    // 简单生成一个 ID
    static int idCounter = 1000;
    info.id = QString::number(++idCounter);
    return info;
}
