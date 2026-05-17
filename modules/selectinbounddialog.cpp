#include "selectinbounddialog.h"
#include "productdatamanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidgetItem>
#include <QMessageBox>
#include "custommessagedialog.h"
#include <QPainter>
#include <QStyledItemDelegate>
#include <QEvent>
#include <QPropertyAnimation>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFrame>
#include <QShowEvent>

// 自定义行代理，用于绘制漂亮的圆角选择效果
class InboundRowDelegate : public QStyledItemDelegate {
public:
    private:
    mutable int m_hoveredRow = -1;
public:
    InboundRowDelegate(QObject *parent) : QStyledItemDelegate(parent) {
        QAbstractItemView *view = qobject_cast<QAbstractItemView*>(parent);
        if (view) {
            view->setMouseTracking(true);
            connect(view, &QAbstractItemView::entered, this, [this, view](const QModelIndex &index) {
                m_hoveredRow = index.row();
                view->viewport()->update();
            });
            view->viewport()->installEventFilter(this);
        }
    }
    
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (event->type() == QEvent::Leave) {
            m_hoveredRow = -1;
            QAbstractItemView *view = qobject_cast<QAbstractItemView*>(parent());
            if (view) {
                view->viewport()->update();
            }
        }
        return QStyledItemDelegate::eventFilter(watched, event);
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        if (opt.state & QStyle::State_Selected) {
            // 绘制圆角背景
            QRect rect = opt.rect.adjusted(2, 2, -2, -2);
            painter->setPen(QPen(QColor(59, 130, 246), 2)); // 蓝色边框
            painter->setBrush(QColor(239, 246, 255));      // 浅蓝背景
            painter->drawRoundedRect(rect, 8, 8);
            
            // 绘制文字
            painter->setPen(QColor(30, 58, 138));
            opt.state &= ~QStyle::State_Selected; // 禁用默认选择绘制
        } else if (index.row() == m_hoveredRow) {
            // 悬浮行背景绘制
            QRect rect = opt.rect.adjusted(2, 2, -2, -2);
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(236, 245, 255)); // 浅蓝背景
            painter->drawRoundedRect(rect, 8, 8);
        }

        // 处理文字对齐
        QRect textRect = opt.rect.adjusted(10, 0, -10, 0);
        QString text = opt.text;
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text);

        painter->restore();
    }
};

SelectInboundDialog::SelectInboundDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("选择待上架入库记录");
    resize(1100, 750);
    setStyleSheet("QDialog { background: #f8fafc; }");
    setupUI();
    
    // 1. 绑定数据接收信号
    connect(ProductDataManager::instance(), &ProductDataManager::inboundListReceived, this, [this](const QList<StockInRecord> &list){
        m_currentList = list; // 保存数据
        m_table->setRowCount(0);
        for (const auto &rec : list) {
            int row = m_table->rowCount();
            m_table->insertRow(row);
            
            auto setItem = [&](int col, const QString &text) {
                QTableWidgetItem *item = new QTableWidgetItem(text);
                m_table->setItem(row, col, item);
                return item;
            };

            // 0. 图片预览列
            QLabel *imgLabel = new QLabel();
            imgLabel->setFixedSize(50, 50);
            imgLabel->setCursor(Qt::PointingHandCursor);
            imgLabel->setStyleSheet("border-radius: 8px; background: #f1f5f9;");
            
            QPixmap pix;
            if (!rec.imgData.isEmpty()) {
                if (rec.imgData.startsWith("[")) {
                    // 多图模式：解析 JSON 数组并取第一张
                    QJsonDocument doc = QJsonDocument::fromJson(rec.imgData.toUtf8());
                    if (doc.isArray()) {
                        QJsonArray arr = doc.array();
                        if (!arr.isEmpty()) {
                            pix.loadFromData(QByteArray::fromBase64(arr[0].toString().toUtf8()));
                        }
                    }
                } else {
                    // 单图模式
                    pix.loadFromData(QByteArray::fromBase64(rec.imgData.toUtf8()));
                }
            }
            
            if (pix.isNull()) {
                imgLabel->setText("无图");
                imgLabel->setAlignment(Qt::AlignCenter);
                imgLabel->setStyleSheet("border-radius: 8px; background: #f1f5f9; color: #94a3b8; font-size: 10px;");
            } else {
                imgLabel->setPixmap(pix.scaled(50, 50, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
            }
            
            imgLabel->installEventFilter(this);
            imgLabel->setProperty("fullPix", pix); 
            m_table->setCellWidget(row, 0, imgLabel);

            // 1-6. 核心识别信息
            setItem(1, rec.productName)->setData(Qt::UserRole, rec.id);
            setItem(2, rec.spec.isEmpty() ? "-" : rec.spec);
            setItem(3, rec.category);
            setItem(4, rec.productionDate); 
            setItem(5, QString::number(rec.quantity));
            setItem(6, QString("￥%1").arg(rec.costPrice, 0, 'f', 2));
        }
        
        if (m_table->rowCount() > 0) m_table->selectRow(0);
    });

    // 2. 绑定结果反馈信号
    connect(ProductDataManager::instance(), &ProductDataManager::shelveResult, this, [this](bool success, const QString &msg){
        m_confirmBtn->setEnabled(true);
        m_confirmBtn->setText("确认上架到档案");
        if (success) {
            CustomMessageDialog::showSuccess(this, "成功", "商品已成功上架到档案记录");
            accept();
        } else {
            CustomMessageDialog::showWarning(this, "错误", "上架失败: " + msg);
        }
    });

    // 3. 请求数据
    ProductDataManager::instance()->requestInboundList(true);
}

void SelectInboundDialog::setupUI() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(20);
    layout->setContentsMargins(30, 30, 30, 30);

    // 标题区域
    QLabel *titleLabel = new QLabel("选择待上架入库记录");
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #1e293b;");
    layout->addWidget(titleLabel);

    QLabel *descLabel = new QLabel("以下是系统中记录但尚未上架到档案库的入库单");
    descLabel->setStyleSheet("color: #64748b; font-size: 14px;");
    layout->addWidget(descLabel);

    // 表格卡片容器
    QFrame *tableCard = new QFrame();
    tableCard->setStyleSheet("QFrame { background: white; border: 1px solid #e2e8f0; border-radius: 12px; }");
    QVBoxLayout *tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(1, 1, 1, 1);

    m_table = new QTableWidget();
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({"预览", "商品名称", "规格", "分类", "生产日期", "数量", "进货价"});
    m_table->verticalHeader()->setDefaultSectionSize(60);
    m_table->setItemDelegate(new InboundRowDelegate(m_table));
    
    m_table->setShowGrid(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->setAlternatingRowColors(false);
    m_table->setStyleSheet(
        "QTableWidget { border: none; background: white; gridline-color: transparent; }"
        "QHeaderView::section { background: #f8fafc; padding: 12px; border: none; color: #64748b; font-weight: bold; text-align: left; }"
    );
    
    QHeaderView *header = m_table->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->setColumnWidth(0, 70);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    header->setSectionResizeMode(3, QHeaderView::Stretch);
    header->setSectionResizeMode(4, QHeaderView::Stretch);
    header->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    
    tableLayout->addWidget(m_table);
    layout->addWidget(tableCard);

    // 底部按钮区域
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *cancelBtn = new QPushButton("取消操作");
    cancelBtn->setFixedSize(120, 45);
    cancelBtn->setStyleSheet("QPushButton { background: white; border: 1px solid #e2e8f0; border-radius: 8px; color: #64748b; font-weight: bold; }"
                             "QPushButton:hover { background: #f8fafc; }");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    m_confirmBtn = new QPushButton("确认上架到档案");
    m_confirmBtn->setFixedSize(180, 45);
    m_confirmBtn->setStyleSheet("QPushButton { background: #3b82f6; border-radius: 8px; color: white; font-weight: bold; }"
                                "QPushButton:hover { background: #2563eb; }"
                                "QPushButton:disabled { background: #94a3b8; }");
    connect(m_confirmBtn, &QPushButton::clicked, this, &SelectInboundDialog::onConfirm);
    btnLayout->addWidget(m_confirmBtn);

    layout->addLayout(btnLayout);
}

void SelectInboundDialog::onConfirm() {
    int row = m_table->currentRow();
    if (row < 0) return;
    
    int inboundId = m_table->item(row, 1)->data(Qt::UserRole).toInt();
    
    // 寻找对应记录并发出前端先行信号
    for (const auto &rec : m_currentList) {
        if (rec.id == inboundId) {
            emit shelvedOptimistically(rec);
            break;
        }
    }

    m_confirmBtn->setEnabled(false);
    m_confirmBtn->setText("正在处理...");
    ProductDataManager::instance()->shelveProduct(inboundId);
    
    // 关键修复：信号发出并启动后台任务后，立即关闭当前选择界面
    this->accept();
}

bool SelectInboundDialog::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseButtonRelease) {
        QLabel *label = qobject_cast<QLabel*>(watched);
        if (label && !label->property("fullPix").isNull()) {
            QPixmap pix = label->property("fullPix").value<QPixmap>();
            showBigImage(pix);
            return true;
        }
        
        // 点击遮罩层关闭
        if (m_overlay && watched == m_overlay) {
            hideBigImage();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void SelectInboundDialog::showBigImage(const QPixmap &pix) {
    if (pix.isNull()) return;

    // 获取顶级主窗口用于计算位置和大小
    QWidget *topWindow = this->window();
    while (topWindow->parentWidget()) topWindow = topWindow->parentWidget();

    if (!m_overlay) {
        // 关键改动：将父对象设为当前对话框(this)，确保它永远显示在对话框上方
        QDialog *dlg = new QDialog(this);
        dlg->setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        dlg->setAttribute(Qt::WA_TranslucentBackground);
        
        QVBoxLayout *mainLayout = new QVBoxLayout(dlg);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        
        QFrame *bgFrame = new QFrame();
        bgFrame->setObjectName("bgFrame");
        bgFrame->setStyleSheet("QFrame#bgFrame { background-color: rgba(0, 0, 0, 210); }");
        
        QVBoxLayout *bgLayout = new QVBoxLayout(bgFrame);
        QLabel *label = new QLabel();
        label->setObjectName("bigImageLabel");
        label->setAlignment(Qt::AlignCenter);
        bgLayout->addWidget(label);
        
        mainLayout->addWidget(bgFrame);
        dlg->installEventFilter(this);
        m_overlay = (QLabel*)dlg; 
    }

    QDialog *overlayDlg = qobject_cast<QDialog*>(m_overlay);
    QLabel *imgLabel = overlayDlg->findChild<QLabel*>("bigImageLabel");
    
    // 覆盖整个主窗口区域
    imgLabel->setPixmap(pix.scaled(topWindow->width() * 0.9, topWindow->height() * 0.9, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    overlayDlg->setGeometry(topWindow->geometry());
    
    overlayDlg->show();
    overlayDlg->raise();
    overlayDlg->activateWindow();
}

void SelectInboundDialog::hideBigImage() {
    if (m_overlay) {
        qobject_cast<QDialog*>(m_overlay)->hide();
    }
}

void SelectInboundDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    QWidget *topLevel = parentWidget();
    while (topLevel && topLevel->parentWidget()) topLevel = topLevel->parentWidget();
    if (topLevel) {
        QPoint center = topLevel->mapToGlobal(topLevel->rect().center());
        move(center.x() - width() / 2, center.y() - height() / 2);
    }
}
