#include "systemsettingsdialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QTextEdit>
#include <QPushButton>
#include <QPrinterInfo>
#include <QMessageBox>

SystemSettingsDialog::SystemSettingsDialog(QWidget *parent) : QDialog(parent) {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(900, 650);
    setupUI();
    applyStyles();
}

void SystemSettingsDialog::setupUI() {
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    QFrame *bgFrame = new QFrame(this);
    bgFrame->setObjectName("DialogBg");
    outerLayout->addWidget(bgFrame);

    QHBoxLayout *mainLayout = new QHBoxLayout(bgFrame);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Sidebar
    m_sidebar = new QListWidget(this);
    m_sidebar->setFixedWidth(220);
    m_sidebar->addItem("🏢  门店基础信息");
    m_sidebar->addItem("🖨️  打印排版设置");
    m_sidebar->addItem("🎁  积分与风控规则");

    // Content Area
    QWidget *contentContainer = new QWidget(this);
    contentContainer->setObjectName("ContentContainer");
    QVBoxLayout *contentLayout = new QVBoxLayout(contentContainer);
    contentLayout->setContentsMargins(40, 40, 40, 30);
    contentLayout->setSpacing(20);

    QLabel *titleLabel = new QLabel("系统高级设置", this);
    titleLabel->setObjectName("MainTitle");
    contentLayout->addWidget(titleLabel);

    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->addWidget(createStoreInfoPage());
    m_stackedWidget->addWidget(createPrintConfigPage());
    m_stackedWidget->addWidget(createPointsRulePage());

    contentLayout->addWidget(m_stackedWidget);

    // Bottom Action Bar
    QHBoxLayout *actionLayout = new QHBoxLayout();
    actionLayout->addStretch();
    
    m_cancelBtn = new QPushButton("取消");
    m_cancelBtn->setObjectName("SecondaryBtn");
    m_cancelBtn->setFixedSize(100, 44);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    m_saveBtn = new QPushButton("保存配置");
    m_saveBtn->setObjectName("PrimaryBtn");
    m_saveBtn->setFixedSize(120, 44);
    connect(m_saveBtn, &QPushButton::clicked, this, &SystemSettingsDialog::onSaveClicked);

    actionLayout->addWidget(m_cancelBtn);
    actionLayout->addWidget(m_saveBtn);
    contentLayout->addLayout(actionLayout);

    mainLayout->addWidget(m_sidebar);
    mainLayout->addWidget(contentContainer);

    // Connect Sidebar to StackedWidget
    connect(m_sidebar, &QListWidget::currentRowChanged, m_stackedWidget, &QStackedWidget::setCurrentIndex);
    m_sidebar->setCurrentRow(0);
}

QWidget* SystemSettingsDialog::createStoreInfoPage() {
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignTop);

    QFormLayout *form = new QFormLayout();
    form->setSpacing(20);
    
    QLineEdit *nameEdit = new QLineEdit("PetManager 旗舰店");
    QLineEdit *phoneEdit = new QLineEdit("400-123-4567");
    QLineEdit *addrEdit = new QLineEdit("高新区科创大道 88 号");
    QLineEdit *timeEdit = new QLineEdit("09:00 - 22:00");
    
    // Set heights for modern look
    nameEdit->setFixedHeight(36);
    phoneEdit->setFixedHeight(36);
    addrEdit->setFixedHeight(36);
    timeEdit->setFixedHeight(36);

    form->addRow("门店名称：", nameEdit);
    form->addRow("联系电话：", phoneEdit);
    form->addRow("详细地址：", addrEdit);
    form->addRow("营业时间：", timeEdit);
    
    layout->addLayout(form);

    // Logo Upload section
    QLabel *logoTitle = new QLabel("\n门店 Logo 设置：");
    logoTitle->setStyleSheet("font-weight: bold; color: #475569; margin-top: 10px; border: none; background: transparent;");
    layout->addWidget(logoTitle);
    
    QHBoxLayout *logoLayout = new QHBoxLayout();
    QLabel *logoPreview = new QLabel("暂无 Logo");
    logoPreview->setFixedSize(120, 120);
    logoPreview->setAlignment(Qt::AlignCenter);
    logoPreview->setStyleSheet("background-color: #f1f5f9; border: 2px dashed #cbd5e1; border-radius: 8px; color: #94a3b8;");
    
    QPushButton *uploadBtn = new QPushButton("上传/更换 Logo");
    uploadBtn->setFixedSize(140, 40);
    uploadBtn->setStyleSheet("QPushButton { background-color: #ffffff; border: 1px solid #cbd5e1; border-radius: 6px; color: #334155; padding: 8px 0; text-align: center; } QPushButton:hover { background-color: #f8fafc; }");
    
    logoLayout->addWidget(logoPreview);
    logoLayout->addWidget(uploadBtn);
    logoLayout->addStretch();
    layout->addLayout(logoLayout);

    return page;
}

QWidget* SystemSettingsDialog::createPrintConfigPage() {
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignTop);

    QFormLayout *form = new QFormLayout();
    form->setSpacing(20);

    QComboBox *printerCombo = new QComboBox();
    printerCombo->setFixedHeight(36);
    
    // Load available printers
    QList<QPrinterInfo> printers = QPrinterInfo::availablePrinters();
    for (const QPrinterInfo &info : printers) {
        printerCombo->addItem(info.printerName());
    }
    if (printerCombo->count() == 0) printerCombo->addItem("未检测到系统打印机");

    QComboBox *paperCombo = new QComboBox();
    paperCombo->setFixedHeight(36);
    paperCombo->addItems({"80mm 热敏纸", "58mm 热敏纸"});

    QCheckBox *printLogoCb = new QCheckBox(" 在小票顶部打印门店 Logo");
    printLogoCb->setChecked(true);
    QCheckBox *multiCopyCb = new QCheckBox(" 结账时默认打印出留底联");
    
    QLineEdit *headerEdit = new QLineEdit("欢迎光临，祝您生活愉快！");
    headerEdit->setFixedHeight(36);
    
    QTextEdit *footerEdit = new QTextEdit("凭此小票在 7 日内可享受洗护 8 折优惠。\n最终解释权归门店所有。");
    footerEdit->setFixedHeight(80);

    form->addRow("默认打印机：", printerCombo);
    form->addRow("纸张规格：", paperCombo);
    form->addRow("", printLogoCb);
    form->addRow("", multiCopyCb);
    form->addRow("页眉问候语：", headerEdit);
    form->addRow("底部说明：", footerEdit);

    layout->addLayout(form);
    return page;
}

QWidget* SystemSettingsDialog::createPointsRulePage() {
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignTop);

    QCheckBox *enablePointsCb = new QCheckBox("启用会员积分系统 (全局开关)");
    enablePointsCb->setStyleSheet("font-weight: bold; color: #1e293b; font-size: 15px; margin-bottom: 20px; border: none; background: transparent;");
    layout->addWidget(enablePointsCb);
    enablePointsCb->setChecked(true);

    QFormLayout *form = new QFormLayout();
    form->setSpacing(20);

    // 获取比例
    QHBoxLayout *earnLayout = new QHBoxLayout();
    QLabel *earnLabel1 = new QLabel("消费");
    earnLabel1->setStyleSheet("border: none; background: transparent;");
    QLineEdit *earnEdit = new QLineEdit("10");
    earnEdit->setFixedSize(80, 36);
    earnEdit->setAlignment(Qt::AlignCenter);
    QLabel *earnLabel2 = new QLabel("元 积 1 分");
    earnLabel2->setStyleSheet("border: none; background: transparent;");
    earnLayout->addWidget(earnLabel1);
    earnLayout->addWidget(earnEdit);
    earnLayout->addWidget(earnLabel2);
    earnLayout->addStretch();
    form->addRow("获取规则：", earnLayout);

    // 抵扣比例
    QHBoxLayout *spendLayout = new QHBoxLayout();
    QLineEdit *spendEdit = new QLineEdit("100");
    spendEdit->setFixedSize(80, 36);
    spendEdit->setAlignment(Qt::AlignCenter);
    QLabel *spendLabel2 = new QLabel("积分 抵扣 1 元");
    spendLabel2->setStyleSheet("border: none; background: transparent;");
    spendLayout->addWidget(spendEdit);
    spendLayout->addWidget(spendLabel2);
    spendLayout->addStretch();
    form->addRow("抵扣规则：", spendLayout);

    // 风控设置
    QHBoxLayout *riskLayout = new QHBoxLayout();
    QLabel *riskLabel1 = new QLabel("单次最高抵扣订单金额的");
    riskLabel1->setStyleSheet("border: none; background: transparent;");
    QLineEdit *riskEdit = new QLineEdit("30");
    riskEdit->setFixedSize(60, 36);
    riskEdit->setAlignment(Qt::AlignCenter);
    QLabel *riskLabel2 = new QLabel("%");
    riskLabel2->setStyleSheet("border: none; background: transparent;");
    riskLayout->addWidget(riskLabel1);
    riskLayout->addWidget(riskEdit);
    riskLayout->addWidget(riskLabel2);
    riskLayout->addStretch();
    form->addRow("风控限制：", riskLayout);

    layout->addLayout(form);
    return page;
}

void SystemSettingsDialog::applyStyles() {
    this->setStyleSheet(R"(
        QFrame#DialogBg {
            background-color: #ffffff;
            border: 1px solid #475569;
            border-radius: 12px;
        }
        QWidget#ContentContainer {
            background-color: #ffffff;
            border-top-right-radius: 12px;
            border-bottom-right-radius: 12px;
        }
        QLabel#MainTitle {
            font-size: 20px;
            font-weight: bold;
            color: #0f172a;
            border: none;
            background: transparent;
        }
        QListWidget {
            background-color: #f8fafc;
            border: none;
            border-right: 1px solid #e2e8f0;
            border-top-left-radius: 11px;
            border-bottom-left-radius: 11px;
            outline: none;
            padding-top: 20px;
        }
        QListWidget::item {
            height: 50px;
            padding-left: 20px;
            color: #64748b;
            font-size: 14px;
            font-weight: bold;
        }
        QListWidget::item:selected {
            background-color: #ffffff;
            color: #3b82f6;
            border-left: 4px solid #3b82f6;
        }
        QListWidget::item:hover:!selected {
            background-color: #f1f5f9;
        }
        QLineEdit, QTextEdit, QComboBox {
            border: 1px solid #cbd5e1;
            border-radius: 6px;
            padding: 4px 10px;
            background-color: #ffffff;
            color: #334155;
            font-size: 13px;
        }
        QLineEdit:focus, QTextEdit:focus, QComboBox:focus {
            border: 1px solid #3b82f6;
        }
        QLabel {
            color: #475569;
            font-size: 14px;
            border: none;
            background: transparent;
        }
        QCheckBox {
            color: #475569;
            font-size: 14px;
            border: none;
            background: transparent;
        }
        QPushButton#PrimaryBtn {
            background-color: #3b82f6;
            color: #ffffff;
            font-weight: bold;
            font-size: 14px;
            border-radius: 6px;
            border: none;
            padding: 10px 0;
            text-align: center;
        }
        QPushButton#PrimaryBtn:hover {
            background-color: #2563eb;
        }
        QPushButton#SecondaryBtn {
            background-color: #ffffff;
            color: #334155;
            font-weight: bold;
            font-size: 14px;
            border-radius: 6px;
            border: 1px solid #cbd5e1;
            padding: 10px 0;
            text-align: center;
        }
        QPushButton#SecondaryBtn:hover {
            background-color: #f8fafc;
        }
    )");
}

void SystemSettingsDialog::onSaveClicked() {
    QMessageBox::information(this, "保存成功", "系统高级参数配置已保存。\n\n打印机与网络参数可能需要在重新启动客户端后生效。");
    accept();
}
