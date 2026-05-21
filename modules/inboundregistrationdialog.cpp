#include "inboundregistrationdialog.h"
#include "productdatamanager.h"
#include "staffdatamanager.h"
#include <QDateTime>
#include <QWheelEvent>
#include "custommessagedialog.h"
#include <QFileDialog>
#include <QIntValidator>
#include <QDoubleValidator>
#include "custom_calendar_edit.h"
#include "../utils/imageutils.h"

InboundRegistrationDialog::InboundRegistrationDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    setWindowTitle("商品入库登记工作台");
    setMinimumWidth(900);
}

void InboundRegistrationDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(20);
    setStyleSheet("QDialog { background-color: #f8fafc; }");

    // Section 1: Scan & Preview
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setSpacing(25);

    // Left: Scan Area
    QFrame *scanFrame = new QFrame();
    scanFrame->setObjectName("scanFrame");
    scanFrame->setStyleSheet("QFrame#scanFrame { background: white; border-radius: 15px; border: 1px solid #e2e8f0; }");
    QVBoxLayout *scanLayout = new QVBoxLayout(scanFrame);
    scanLayout->setContentsMargins(25, 20, 25, 20);

    QLabel *scanLabel = new QLabel("条形码扫描");
    scanLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #64748b;");
    
    m_barcodeEdit = new QLineEdit();
    m_barcodeEdit->setPlaceholderText("等待扫码...");
    m_barcodeEdit->setFixedHeight(45);
    m_barcodeEdit->setStyleSheet("QLineEdit { background: #f1f5f9; border: none; border-radius: 8px; padding: 0 15px; font-size: 16px; font-weight: 600; color: #1e293b; }");
    
    scanLayout->addWidget(scanLabel);
    scanLayout->addWidget(m_barcodeEdit);
    topLayout->addWidget(scanFrame, 3);

    // Right: Preview Area
    m_previewCard = new QFrame();
    m_previewCard->setStyleSheet("background: white; border-radius: 15px; border: none;");
    QHBoxLayout *previewLayout = new QHBoxLayout(m_previewCard);
    previewLayout->setContentsMargins(20, 15, 20, 15);
    previewLayout->setSpacing(20);

    // Image Stack
    QWidget *imgWrapper = new QWidget();
    QVBoxLayout *imgStack = new QVBoxLayout(imgWrapper);
    imgStack->setContentsMargins(0, 0, 0, 0);
    imgStack->setSpacing(8);

    m_previewImg = new QLabel();
    m_previewImg->setFixedSize(100, 100);
    m_previewImg->setAlignment(Qt::AlignCenter);
    m_previewImg->setStyleSheet("background: #f8fafc; border: 2px dashed #e2e8f0; border-radius: 12px; color: #94a3b8; font-size: 20px;");
    m_previewImg->setText("+");
    m_previewImg->installEventFilter(this);

    m_dotsContainer = new QWidget();
    m_dotsLayout = new QHBoxLayout(m_dotsContainer);
    m_dotsLayout->setContentsMargins(0, 0, 0, 0);
    m_dotsLayout->setSpacing(4);
    m_dotsLayout->setAlignment(Qt::AlignCenter);

    imgStack->addWidget(m_previewImg);
    imgStack->addWidget(m_dotsContainer);

    QVBoxLayout *previewInfo = new QVBoxLayout();
    m_previewName = new QLabel("商品预览...");
    m_previewName->setStyleSheet("font-size: 16px; font-weight: 800; color: #0f172a; border: none;");
    m_previewSpec = new QLabel("滚动滑轮切换图片");
    m_previewSpec->setStyleSheet("color: #64748b; font-size: 12px; border: none;");
    
    previewInfo->addWidget(m_previewName);
    previewInfo->addWidget(m_previewSpec);
    previewInfo->addStretch();

    previewLayout->addWidget(imgWrapper);
    previewLayout->addLayout(previewInfo);
    topLayout->addWidget(m_previewCard, 2);

    mainLayout->addLayout(topLayout);

    // Section 2: Form
    QFrame *formFrame = new QFrame();
    formFrame->setStyleSheet("background: white; border-radius: 15px; border: 1px solid #e2e8f0;");
    QGridLayout *formLayout = new QGridLayout(formFrame);
    formLayout->setContentsMargins(25, 25, 25, 25);
    formLayout->setSpacing(15);

    auto createField = [&](const QString &label, QWidget *edit, int row, int col) {
        QVBoxLayout *l = new QVBoxLayout();
        l->setSpacing(6);
        QLabel *lbl = new QLabel(label);
        lbl->setStyleSheet("color: #64748b; font-size: 12px; font-weight: bold; border: none;");
        edit->setFixedHeight(36);
        
        // 采用会员界面的专业样式
        QString baseStyle = 
            "QWidget { border: 1px solid #e2e8f0; border-radius: 6px; padding: 0 10px; font-size: 13px; background-color: #f8fafc; color: #0f172a; } "
            "QWidget:hover { border-color: #cbd5e1; background-color: #f1f5f9; } "
            "QWidget:focus { border: 2px solid #3b82f6; background-color: #ffffff; padding: 0 9px; } ";
        
        if (qobject_cast<QComboBox*>(edit)) {
            QString comboStyle = baseStyle +
                "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 30px; border-left: none; } "
                "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 14px; height: 14px; } "
                "QComboBox QAbstractItemView { border: 1px solid #e2e8f0; border-radius: 8px; background-color: white; selection-background-color: #eff6ff; selection-color: #1e40af; outline: none; padding: 4px; } "
                "QComboBox QAbstractItemView::item { height: 30px; padding-left: 10px; border-radius: 4px; } ";
            edit->setStyleSheet(comboStyle);
        } else {
            edit->setStyleSheet(baseStyle);
        }
        
        l->addWidget(lbl);
        l->addWidget(edit);
        formLayout->addLayout(l, row, col);
    };

    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("请输入商品全称");
    m_categoryCombo = new QComboBox();
    m_categoryCombo->addItems({
        "猫/主食", "猫/零食", "猫/玩具", "猫/洗护", 
        "狗/主食", "狗/零食", "狗/玩具", "狗/洗护",
        "小宠/零食", "其他/日用"
    });
    m_categoryCombo->setEditable(true); // 允许手动输入特殊分类
    m_specEdit = new QLineEdit();
    m_originEdit = new QLineEdit();
    m_qtyEdit = new QLineEdit("1");
    m_qtyEdit->setValidator(new QIntValidator(1, 999999, this)); // 必须大于0
    
    m_costEdit = new QLineEdit("0.00");
    QDoubleValidator *costValidator = new QDoubleValidator(0, 999999, 2, this);
    costValidator->setNotation(QDoubleValidator::StandardNotation);
    m_costEdit->setValidator(costValidator);
    
    m_supplierEdit = new QLineEdit();
    m_supplierPhoneEdit = new QLineEdit();
    
    m_priceEdit = new QLineEdit("0.00"); // 零售价
    QDoubleValidator *priceValidator = new QDoubleValidator(0, 999999, 2, this);
    priceValidator->setNotation(QDoubleValidator::StandardNotation);
    m_priceEdit->setValidator(priceValidator);
    
    m_operatorCombo = new QComboBox(); // 动态加载所有在职员工
    {
        QList<EmployeeInfo> allStaff = StaffDataManager::instance()->allStaff();
        for (const EmployeeInfo &emp : allStaff) {
            if (emp.status == "离职") continue; // 跳过已离职员工
            QString displayText = QString("%1 (%2)").arg(emp.name, emp.id);
            m_operatorCombo->addItem(displayText, emp.id); // UserRole 存储员工ID
        }
        if (m_operatorCombo->count() == 0) {
            m_operatorCombo->addItem("暂无在职员工");
        }
    }
    
    m_dateEdit = new CustomCalendarEdit();
    m_dateEdit->setText(QDate::currentDate().toString("yyyy-MM-dd"));
    m_dateEdit->setMaximumDate(QDate::currentDate()); // 生产日期不能是未来
    
    m_shelfLifeEdit = new QLineEdit("365");
    m_shelfLifeEdit->setValidator(new QIntValidator(0, 36500, this)); // 保质期天数非负
    
    m_minStockEdit = new QLineEdit("10"); // 默认预警值为 10
    m_minStockEdit->setValidator(new QIntValidator(0, 999999, this));

    createField("商品名称", m_nameEdit, 0, 0);
    createField("所属分类", m_categoryCombo, 0, 1);
    createField("产地/品牌", m_originEdit, 0, 2); // 挪到第一行
    
    createField("规格单位", m_specEdit, 1, 0);
    createField("入库数量", m_qtyEdit, 1, 1);
    createField("经办人", m_operatorCombo, 1, 2);
    
    createField("进货成本", m_costEdit, 2, 0);
    createField("零售价格", m_priceEdit, 2, 1); // 新增价格
    createField("库存预警值", m_minStockEdit, 2, 2); // 占用供应商位置，供应商往下移
    
    createField("生产日期", m_dateEdit, 3, 0);
    createField("保质期(天)", m_shelfLifeEdit, 3, 1);
    createField("供应商", m_supplierEdit, 3, 2);
    
    createField("供应商电话", m_supplierPhoneEdit, 4, 0);

    mainLayout->addWidget(formFrame);

    // Confirm Button
    m_confirmBtn = new QPushButton("确认入库并同步资料");
    m_confirmBtn->setFixedHeight(48);
    m_confirmBtn->setStyleSheet("QPushButton { background: white; border: 1.5px solid #3b82f6; color: #3b82f6; font-size: 15px; font-weight: 800; border-radius: 8px; text-align: center; padding: 12px 0; } "
                               "QPushButton:hover { background: #eff6ff; border-color: #2563eb; color: #2563eb; } "
                               "QPushButton:pressed { background: #dbeafe; } ");
    mainLayout->addWidget(m_confirmBtn);

    connect(m_barcodeEdit, &QLineEdit::returnPressed, this, &InboundRegistrationDialog::onBarcodeEntered);
    connect(m_confirmBtn, &QPushButton::clicked, this, &InboundRegistrationDialog::onConfirmInbound);
}

void InboundRegistrationDialog::onBarcodeEntered()
{
    QString barcode = m_barcodeEdit->text().trimmed();
    if (barcode.isEmpty()) return;
    updatePreviewCard(barcode);
}

void InboundRegistrationDialog::updatePreviewCard(const QString &barcode)
{
    ProductInfo info = ProductDataManager::instance()->getProduct(barcode);
    if (info.barcode.isEmpty()) {
        m_previewName->setText("新商品录入");
        m_nameEdit->clear();
        m_specEdit->clear();
        m_originEdit->clear();
        m_costEdit->setText("0.00");
        m_priceEdit->setText("0.00");
        m_supplierEdit->clear();
        m_supplierPhoneEdit->clear();
        m_shelfLifeEdit->setText("365");
        m_minStockEdit->setText("10");
        m_imagePaths.clear();
        switchImage(false);
    } else {
        m_previewName->setText(info.name);
        m_nameEdit->setText(info.name);
        m_categoryCombo->setCurrentText(info.category);
        m_specEdit->setText(info.spec);
        
        // 品牌/产地结合显示
        QString originBrandCombined = info.origin;
        if (!info.brand.isEmpty()) {
            if (!originBrandCombined.isEmpty()) {
                originBrandCombined = info.brand + "/" + originBrandCombined;
            } else {
                originBrandCombined = info.brand;
            }
        }
        m_originEdit->setText(originBrandCombined);
        
        m_costEdit->setText(QString::number(info.costPrice, 'f', 2));
        m_priceEdit->setText(QString::number(info.price, 'f', 2));
        m_supplierEdit->setText(info.supplier);
        m_supplierPhoneEdit->setText(info.supplierPhone);
        m_shelfLifeEdit->setText(QString::number(info.shelfLifeDays > 0 ? info.shelfLifeDays : 365));
        m_minStockEdit->setText(QString::number(info.minStock));
        
        // 数量和生产日期不回填，保持默认或手动输入
        m_qtyEdit->setText("1");
        m_dateEdit->setText(QDate::currentDate().toString("yyyy-MM-dd"));
        
        m_imagePaths = info.images;
        m_currentImgIndex = 0;
        switchImage(false);
    }
}

void InboundRegistrationDialog::setEditMode(const StockInRecord &rec, const ProductInfo &pInfo)
{
    m_isEditMode = true;
    m_currentRecord = rec;
    
    setWindowTitle("修改入库资料");
    m_confirmBtn->setText("保存资料修改");
    
    // 1. 先加载上方卡片预览和图片
    updatePreviewCard(rec.barcode);
    
    // 2. 然后用具体记录和档案的最新值覆盖表单字段，确保回填数据绝对正确
    m_barcodeEdit->setText(rec.barcode);
    m_barcodeEdit->setEnabled(false); // 条码不可改
    
    m_nameEdit->setText(rec.productName);
    m_categoryCombo->setCurrentText(pInfo.category.isEmpty() ? rec.category : pInfo.category);
    m_specEdit->setText(rec.spec);
    
    // 产地/品牌结合回填
    QString originBrandCombined = pInfo.origin;
    if (!pInfo.brand.isEmpty()) {
        if (!originBrandCombined.isEmpty()) {
            originBrandCombined = pInfo.brand + "/" + originBrandCombined;
        } else {
            originBrandCombined = pInfo.brand;
        }
    }
    if (originBrandCombined.isEmpty()) {
        originBrandCombined = rec.origin;
    }
    m_originEdit->setText(originBrandCombined);
    
    m_qtyEdit->setText(QString::number(rec.quantity));
    m_costEdit->setText(QString::number(rec.costPrice, 'f', 2));
    
    // 售价回填：如果编辑记录有售价则用记录的，否则用商品库的
    double salePrice = rec.salePrice > 0 ? rec.salePrice : pInfo.price;
    m_priceEdit->setText(QString::number(salePrice, 'f', 2));
    
    m_supplierEdit->setText(rec.supplier);
    m_supplierPhoneEdit->setText(rec.supplierPhone);
    m_dateEdit->setText(rec.productionDate);
    
    // 保质期回填：如果 rec 里的保质期有效则用，否则用商品档案的，兜底 365
    int shelfLife = rec.shelfLifeDays > 0 ? rec.shelfLifeDays : pInfo.shelfLifeDays;
    if (shelfLife <= 0) shelfLife = 365;
    m_shelfLifeEdit->setText(QString::number(shelfLife));
    
    m_minStockEdit->setText(QString::number(pInfo.minStock > 0 ? pInfo.minStock : 10));
    // 回选经办人：优先按姓名前缀匹配，兼容旧数据格式
    for (int i = 0; i < m_operatorCombo->count(); ++i) {
        if (m_operatorCombo->itemText(i).startsWith(rec.operatorName)) {
            m_operatorCombo->setCurrentIndex(i);
            break;
        }
    }
    
    // 图片集回填
    m_imagePaths = rec.imgPaths.isEmpty() ? pInfo.images : rec.imgPaths;
    m_currentImgIndex = 0;
    switchImage(false);
}

void InboundRegistrationDialog::onConfirmInbound()
{
    QString barcode = m_barcodeEdit->text().trimmed();
    if (barcode.isEmpty()) {
        CustomMessageDialog::showWarning(this, "提示", "请先扫描商品条形码");
        return;
    }

    StockInRecord rec;
    if (m_isEditMode) {
        rec.id = m_currentRecord.id;
        rec.dateTime = m_currentRecord.dateTime; // 保持原有入库时间不改变，保证查询唯一性
        rec.isShelved = m_currentRecord.isShelved; // 关键修复：保留原有上架状态，防止被覆写为未上架！
    } else {
        rec.id = 0;
        rec.dateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        rec.isShelved = false; // 新记录默认未上架
    }
    rec.barcode = barcode;
    rec.productName = m_nameEdit->text();
    rec.spec = m_specEdit->text().trimmed(); // 必须回填，防止 spec 被清空！
    rec.quantity = m_qtyEdit->text().toInt();
    rec.costPrice = m_costEdit->text().toDouble();
    rec.salePrice = m_priceEdit->text().toDouble(); // 必须回填，防止售价无法保存！
    rec.supplier = m_supplierEdit->text();
    rec.supplierPhone = m_supplierPhoneEdit->text(); // 保存联系方式
    rec.productionDate = m_dateEdit->text();
    rec.operatorName = m_operatorCombo->currentText(); // 保存格式：姓名 (ID)
    rec.imgPaths = m_imagePaths;
    if (!m_imagePaths.isEmpty()) rec.imgData = m_imagePaths.first(); // 设置主图
    rec.shelfLifeDays = m_shelfLifeEdit->text().toInt();
    rec.category = m_categoryCombo->currentText(); // 修复：必须给 rec.category 赋值，防止入库记录表里的商品分类被写成 NULL！

    // --- 同步更新商品档案库 ---
    ProductInfo info = ProductDataManager::instance()->getProduct(barcode);
    info.barcode = barcode;
    info.name = rec.productName;
    info.category = m_categoryCombo->currentText();
    info.spec = rec.spec;
    
    // 解析 产地/品牌 为单独的 brand 和 origin 存储
    QString brandOrOrigin = m_originEdit->text().trimmed();
    if (brandOrOrigin.contains("/")) {
        QStringList parts = brandOrOrigin.split("/", Qt::SkipEmptyParts);
        info.brand = parts.first().trimmed();
        info.origin = parts.last().trimmed();
    } else {
        info.brand = brandOrOrigin;
        info.origin = "";
    }
    rec.origin = brandOrOrigin; // 入库记录中也同步记录该组合值
    
    info.costPrice = rec.costPrice;
    info.price = m_priceEdit->text().toDouble();
    info.shelfLifeDays = rec.shelfLifeDays;
    info.minStock = m_minStockEdit->text().toInt();
    info.supplier = rec.supplier;
    info.supplierPhone = rec.supplierPhone;
    info.images = rec.imgPaths;
    info.imgData = rec.imgData;
    info.isActive = true;
    
    if (ProductDataManager::instance()->getProduct(barcode).barcode.isEmpty()) {
        ProductDataManager::instance()->addProduct(info);
    } else {
        ProductDataManager::instance()->updateProduct(info);
    }

    if (m_isEditMode) {
        ProductDataManager::instance()->updateRecord(m_currentRecord.dateTime, m_currentRecord.barcode, rec);
        emit recordUpdated();
    } else {
        ProductDataManager::instance()->addRecord(rec);
        emit recordAdded();
    }
    accept();
}

void InboundRegistrationDialog::onAddImage()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "选择商品图片", "", "Images (*.png *.jpg *.jpeg)");
    if (!files.isEmpty()) {
        m_imagePaths.append(files);
        m_currentImgIndex = m_imagePaths.size() - files.size();
        updateDots();
        switchImage(false);
    }
}

void InboundRegistrationDialog::switchImage(bool next)
{
    if (m_imagePaths.isEmpty()) return;
    if (next) m_currentImgIndex = (m_currentImgIndex + 1) % m_imagePaths.size();
    
    QPixmap pix = ImageUtils::loadPixmap(m_imagePaths[m_currentImgIndex]);
    m_previewImg->setPixmap(pix.scaled(m_previewImg->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    m_previewImg->setStyleSheet("background: white; border: 1px solid #e2e8f0; border-radius: 12px;");
    updateDots();
}

void InboundRegistrationDialog::updateDots()
{
    QLayoutItem *child;
    while ((child = m_dotsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }
    if (m_imagePaths.size() <= 1) { m_dotsContainer->hide(); return; }
    m_dotsContainer->show();
    for (int i = 0; i < m_imagePaths.size(); ++i) {
        QFrame *dot = new QFrame();
        dot->setFixedSize(8, 8);
        dot->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        dot->setStyleSheet(i == m_currentImgIndex ? "background: #3b82f6; border-radius: 4px;" : "background: #e2e8f0; border-radius: 4px;");
        m_dotsLayout->addWidget(dot, 0, Qt::AlignCenter);
    }
}

bool InboundRegistrationDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_previewImg) {
        if (event->type() == QEvent::Wheel) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
            switchImage(wheelEvent->angleDelta().y() < 0);
            return true;
        }
        if (event->type() == QEvent::MouseButtonPress) {
            onAddImage();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}
