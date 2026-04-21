#ifndef PETMODULE_H
#define PETMODULE_H

#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QIntValidator>
#include <QCheckBox>
#include <QDialog>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QPainter>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include "custommessagedialog.h"
#include <QTimer>
#include <QDateEdit>
#include "custom_calendar_edit.h"
#include "petrecorddrawer.h"
#include <QMap>
#include <QMouseEvent>
#include <QStyledItemDelegate>
#include <QHelpEvent>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QGraphicsDropShadowEffect>

// --- 高级悬浮气泡提示控件 ---
class FloatingTooltip : public QWidget {
    Q_OBJECT
public:
    explicit FloatingTooltip(QWidget *parent = nullptr) : QWidget(nullptr) {
        setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
        // 【关键修复1】：让鼠标事件穿透气泡，避免和底层表格抢夺焦点导致疯狂闪烁
        setAttribute(Qt::WA_TransparentForMouseEvents); 
        (void)parent;
        
        m_bg = new QFrame(this);
        m_bg->setObjectName("TooltipBG");
        m_bg->setStyleSheet("#TooltipBG { background: rgba(48, 49, 51, 210); border-radius: 8px; border: 1px solid rgba(255, 255, 255, 30); }");
        
        QVBoxLayout *layout = new QVBoxLayout(this);
        m_label = new QLabel();
        m_label->setWordWrap(true);
        m_label->setStyleSheet("color: #ffffff; font-size: 13px; font-family: 'Microsoft YaHei'; line-height: 1.4;");
        layout->addWidget(m_label);
        layout->setContentsMargins(15, 12, 15, 12);
        
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(15);
        shadow->setColor(QColor(0, 0, 0, 100));
        shadow->setOffset(0, 4);
        m_bg->setGraphicsEffect(shadow);
    }

    void showText(const QPoint &pos, const QString &text) {
        if (text.isEmpty()) return;
        m_label->setText(text);
        
        // 动态调整宽度，最大 320
        int idealWidth = m_label->fontMetrics().horizontalAdvance(text) + 40;
        m_label->setFixedWidth(qMin(320, idealWidth));
        adjustSize();
        
        QPoint targetPos = pos + QPoint(15, 15);
        QRect screen = QGuiApplication::primaryScreen()->geometry();
        if (targetPos.x() + width() > screen.right()) targetPos.setX(pos.x() - width() - 5);
        if (targetPos.y() + height() > screen.bottom()) targetPos.setY(pos.y() - height() - 5);
        
        move(targetPos);
        if (!isVisible()) show();
        raise();
    }

protected:
    void resizeEvent(QResizeEvent *event) override {
        m_bg->setGeometry(rect());
        QWidget::resizeEvent(event);
    }

private:
    QLabel *m_label;
    QFrame *m_bg;
};

// --- 自定义单元格代理：只负责绘制省略号，剥离重复的事件触发 ---
class CustomTooltipDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit CustomTooltipDelegate(QObject *parent = nullptr) 
        : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        QString text = opt.text.trimmed();
        
        // 【核心修复】：如果是标准的、无特殊含义的短文本，直接原样绘制，绝对不加省略号！
        if (text == "无" || text == "常规饮食" || text == "暂无病史") {
            QStyledItemDelegate::paint(painter, opt, index);
            return;
        }

        // 把扣减的宽度从 15 缩小到 8，防止对刚好能显示下的中等文本产生误伤
        int availableWidth = opt.rect.width() - 8; 

        // 只有确实超出可用宽度的长文本，才执行截断并追加 ...
        if (opt.fontMetrics.horizontalAdvance(text) > availableWidth) {
            opt.text = opt.fontMetrics.elidedText(opt.text, Qt::ElideRight, availableWidth);
        }

        QStyledItemDelegate::paint(painter, opt, index);
    }
};

class PetModule : public QWidget
{
    Q_OBJECT
public:
    explicit PetModule(QWidget *parent = nullptr);
    void addPetRow(const PetInfo &info); // 使用结构体简化
    void addPet(const PetInfo &pet);
    void filterByMemberAndHighlightPet(const QString &memberName, const QString &petName);

private:
    void setupUI();
    void updateStats();
    void updatePagination();
    void updateRowStatus(int row);
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *event) override;
private slots:
    void onSearch(const QString &keyword);
    void onEditPet();
    void onDeletePet();
    void onPrevPage();
    void onNextPage();
    void onJumpPage();
    void onBatchDelete();
    
    // UI 增强相关槽函数
    void onCurrentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
    void onLogAdded(const QString &petId, const PetActivityLog &log);
    void onQuickAction(); // 处理投喂/洗护按钮

private:
    QTableWidget *petTable;
    QLineEdit *searchEdit;

    // 统计卡片价值标签
    QLabel *totalPetsLabel;
    QLabel *boardingPetsLabel;
    QLabel *groomingPetsLabel;

    // 分页辅助
    QPushButton *prevBtn;
    QPushButton *nextBtn;
    QLabel *pageLabel;
    int m_currentPage;
    int m_pageSize;
    QLineEdit *jumpEdit;
    QIntValidator *jumpValidator;
    QPushButton *jumpBtn;

    // UI 增强
    PetRecordDrawer *m_drawer;
    
    // 数据同步增强：不再在内部存储全量数据，而是通过 DataManager 获取
    void refreshTable();
    void syncRowData(int row, const PetInfo &info);

    // 大图预览交互
    QWidget *m_imagePreviewOverlay;
    QLabel *m_previewLabel;
    class FloatingTooltip *m_floatingTooltip;
    QModelIndex m_lastHoveredIndex;
    void showBigImage(const QString &path);
    void hideBigImage();
};

// 详情弹窗：疫苗接种档案
class VaccineDetailDialog : public QDialog {
    Q_OBJECT
public:
    explicit VaccineDetailDialog(const QString &petName, const QList<VaccineRecord> &records, QWidget *parent = nullptr) 
        : QDialog(parent) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        setAttribute(Qt::WA_TranslucentBackground);
        setFixedSize(540, 520); // 增加宽高，防止内容挤压

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(10, 10, 10, 10);

        // 容器背景
        QFrame *container = new QFrame();
        container->setStyleSheet("QFrame { background: white; border-radius: 15px; }");
        QVBoxLayout *layout = new QVBoxLayout(container);
        layout->setContentsMargins(25, 25, 25, 25);
        layout->setSpacing(0); // 内部使用 addSpacing 精确控制

        // 顶部标题栏
        QHBoxLayout *header = new QHBoxLayout();
        QLabel *icon = new QLabel("💉");
        icon->setStyleSheet("font-size: 24px;");
        QLabel *title = new QLabel(QString("%1 的疫苗接种档案").arg(petName));
        title->setStyleSheet("font-size: 18px; color: #303133; font-weight: bold;");
        
        // 新增：标题栏操作按钮
        QPushButton *addBtn = new QPushButton();
        addBtn->setText("+ 新增接种记录");
        addBtn->setFixedSize(130, 30);
        addBtn->setCursor(Qt::PointingHandCursor);
        addBtn->setStyleSheet(
            "QPushButton { background-color: #409eff; color: white; border: none; border-radius: 4px; "
            "font-size: 12px; font-weight: bold; text-align: center; padding: 0 5px; } "
            "QPushButton:hover { background-color: #66b1ff; }"
        );
        
        header->addWidget(icon);
        header->addWidget(title);
        header->addSpacing(15);
        header->addWidget(addBtn);
        header->addStretch();
        
        QPushButton *closeBtn = new QPushButton("×");
        closeBtn->setFixedSize(30, 30);
        closeBtn->setCursor(Qt::PointingHandCursor);
        closeBtn->setStyleSheet("QPushButton { border: none; font-size: 24px; color: #909399; } QPushButton:hover { color: #f56c6c; }");
        connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
        header->addWidget(closeBtn);
        layout->addLayout(header);
        layout->addSpacing(20);

        // 1. 顶部状态显示 (琥珀色动态条)
        QLabel *statusLabel = new QLabel(QString("🛡️ 已按计划完成 %1 项接种").arg(records.size()));
        statusLabel->setFixedHeight(36);
        statusLabel->setStyleSheet("background: #f0f9eb; color: #67c23a; padding: 0 15px; border-radius: 6px; font-size: 13px; font-weight: bold;");
        layout->addWidget(statusLabel);
        layout->addSpacing(15);

        // 2. 数据表格
        QTableWidget *table = new QTableWidget();
        table->setColumnCount(4);
        table->setHorizontalHeaderLabels({"疫苗种类", "接种日期", "有效期至", "操作"});
        table->setShowGrid(false); // 隐藏原生网格
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed); // 种类列
        table->setColumnWidth(0, 160);
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed); // 拓宽日期列
        table->setColumnWidth(1, 140);
        table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
        table->setColumnWidth(3, 80);
        table->verticalHeader()->setVisible(false);
        table->verticalHeader()->setDefaultSectionSize(40);
        table->setMinimumHeight(220);
        
        // 统一样式定义：纯白背景 + 拓宽 Padding
        QString commonStyle = 
            "QComboBox, CustomCalendarEdit { "
            "   border: 1px solid #dcdfe6; "
            "   border-radius: 4px; "
            "   padding: 2px 12px; "
            "   background: white; "
            "   color: #606266; "
            "   font-size: 13px; "
            "} "
            "QComboBox:hover, CustomCalendarEdit:hover { border-color: #c0c4cc; } "
            "QComboBox:focus, CustomCalendarEdit:focus { border-color: #409eff; outline: none; } ";
            
        QString arrowStyle = 
            "QComboBox::drop-down, QDateEdit::drop-down { border: none; background: transparent; width: 24px; } "
            "QComboBox::down-arrow, QDateEdit::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; } "
            "QComboBox QAbstractItemView { border: 1px solid #ebeef5; border-radius: 4px; background-color: white; outline: none; padding: 4px 0px; } "
            "QComboBox QAbstractItemView::item { height: 35px; padding-left: 12px; color: #606266; background-color: white; } "
            "QComboBox QAbstractItemView::item:selected { background-color: #f0f7ff; color: #409eff; } ";
        
        table->setStyleSheet(
            "QTableWidget { border: 1px solid #ebeef5; background: white; border-radius: 4px; } "
            "QHeaderView::section { background: #f5f7fa; padding: 8px; border: none; font-weight: bold; color: #606266; } "
            "QTableWidget::item { border-bottom: 1px solid #f5f7fa; color: #606266; }"
        );
        
        // 极简风格删除按钮：平时低调，悬浮显色
        auto createDelBtn = [=]() {
            QPushButton *btn = new QPushButton("删除记录");
            btn->setMinimumWidth(80);
            btn->setFixedHeight(32);
            btn->setStyleSheet(
                "QPushButton { background: transparent; color: #f56c6c; border: 1px solid #fbc4c4; border-radius: 4px; font-size: 12px; padding: 0 8px; } "
                "QPushButton:hover { background: #f56c6c; color: white; border: none; }"
            );
            return btn;
        };

        table->setRowCount(records.size());
        for (int i = 0; i < records.size(); ++i) {
            // 1. 种类
            QComboBox *cb = new QComboBox();
            cb->setEditable(true);
            cb->addItems({"猫三联 (第一针)", "猫三联 (第二针)", "猫三联 (第三针)", "妙三多", "狂犬疫苗", "犬八联", "犬四联"});
            cb->setCurrentText(records[i].type);
            cb->setStyleSheet(commonStyle + arrowStyle);
            table->setCellWidget(i, 0, cb);
            
            // 2. 接种日期 (升级为 CustomCalendarEdit)
            CustomCalendarEdit *de = new CustomCalendarEdit();
            de->setText(records[i].date);
            de->setStyleSheet(commonStyle + arrowStyle);
            table->setCellWidget(i, 1, de);
            
            // 3. 有效期至 (自动推算, 只读显示)
            QLabel *expLabel = new QLabel(records[i].expiry);
            expLabel->setAlignment(Qt::AlignCenter);
            expLabel->setStyleSheet(commonStyle + "background: #fdfdfd; color: #909399;");
            table->setCellWidget(i, 2, expLabel);
            
            // 联动逻辑：日期变动 -> 自动推算有效期
            connect(de, &CustomCalendarEdit::dateChanged, this, [=](const QDate &date) {
                expLabel->setText(date.addYears(1).toString("yyyy-MM-dd"));
            });
            
            // 4. 操作
            QWidget *w = new QWidget();
            QHBoxLayout *l = new QHBoxLayout(w);
            l->setContentsMargins(0,0,0,0); l->setAlignment(Qt::AlignCenter);
            
            QPushButton *delBtn = createDelBtn();
            connect(delBtn, &QPushButton::clicked, this, [=]() {
                if (CustomMessageDialog::confirm(this, "确认删除", "确定要永久删除这条接种记录吗？")) {
                    int currentRow = table->indexAt(w->pos()).row();
                    if (currentRow != -1) {
                        table->removeRow(currentRow);
                        statusLabel->setText(QString("🛡️ 当前库中记录：%1 项").arg(table->rowCount()));
                        statusLabel->setStyleSheet("background: #f0f9eb; color: #67c23a; padding: 0 15px; border-radius: 6px; font-size: 13px; font-weight: bold;");
                    }
                }
            });
            l->addWidget(delBtn);
            table->setCellWidget(i, 3, w);
        }
        layout->addWidget(table);
        layout->addSpacing(25);

        // 新增按钮逻辑：带状态反馈和二次确认
        connect(addBtn, &QPushButton::clicked, this, [=]() {
            int row = 0; 
            table->insertRow(row);
            table->setRowHeight(row, 44);
            
            statusLabel->setText(QString("✏️ 正在录入第 %1 项健康记录...").arg(table->rowCount()));
            statusLabel->setStyleSheet("background: #ecf5ff; color: #409eff; padding: 0 15px; border-radius: 6px; font-size: 13px; font-weight: bold;");

            QComboBox *typeCombo = new QComboBox();
            typeCombo->setEditable(true);
            typeCombo->addItems({"猫三联 (第一针)", "猫三联 (第二针)", "猫三联 (第三针)", "妙三多", "狂犬疫苗", "犬八联", "犬四联"});
            typeCombo->setStyleSheet(commonStyle + arrowStyle);
            table->setCellWidget(row, 0, typeCombo);
            
            CustomCalendarEdit *dateEdit = new CustomCalendarEdit();
            dateEdit->setText(QDate::currentDate().toString("yyyy-MM-dd"));
            dateEdit->setStyleSheet(commonStyle + arrowStyle);
            table->setCellWidget(row, 1, dateEdit);
            
            QLabel *autoExpLabel = new QLabel(QDate::currentDate().addYears(1).toString("yyyy-MM-dd"));
            autoExpLabel->setAlignment(Qt::AlignCenter);
            autoExpLabel->setStyleSheet(commonStyle + "background: #fdfdfd; color: #909399;");
            table->setCellWidget(row, 2, autoExpLabel);
            
            connect(dateEdit, &CustomCalendarEdit::dateChanged, this, [=](const QDate &date) {
                autoExpLabel->setText(date.addYears(1).toString("yyyy-MM-dd"));
            });

            QPushButton *cancelRowBtn = new QPushButton("移除记录");
            cancelRowBtn->setMinimumWidth(80);
            cancelRowBtn->setFixedHeight(32);
            cancelRowBtn->setStyleSheet(
                "QPushButton { background: transparent; color: #f56c6c; border: 1px solid #fbc4c4; border-radius: 4px; font-size: 12px; padding: 0 8px; } "
                "QPushButton:hover { background: #f56c6c; color: white; border: none; }"
            );
            
            QWidget *container = new QWidget();
            QHBoxLayout *lay = new QHBoxLayout(container);
            lay->setContentsMargins(0,0,0,0); lay->setAlignment(Qt::AlignCenter);
            lay->addWidget(cancelRowBtn);
            table->setCellWidget(row, 3, container);

            connect(cancelRowBtn, &QPushButton::clicked, this, [=]() { 
                if (CustomMessageDialog::confirm(this, "确认移除", "确定要移除这条正在录入的记录吗？")) {
                    int r = table->indexAt(container->pos()).row();
                    if (r != -1) {
                        table->removeRow(r); 
                        statusLabel->setText(QString("🛡️ 当前库中记录：%1 项").arg(table->rowCount()));
                        statusLabel->setStyleSheet("background: #f0f9eb; color: #67c23a; padding: 0 15px; border-radius: 6px; font-size: 13px; font-weight: bold;");
                    }
                }
            });
            
            QTimer::singleShot(50, typeCombo, [typeCombo]() { typeCombo->showPopup(); });
        });

        // 底部：取消与保存 (终极视觉加固版)
        QHBoxLayout *bottomLayout = new QHBoxLayout();
        bottomLayout->setSpacing(15);
        bottomLayout->addStretch();

        QPushButton *cancelBtn = new QPushButton("取消");
        cancelBtn->setFixedSize(110, 44);
        cancelBtn->setCursor(Qt::PointingHandCursor);
        cancelBtn->setStyleSheet(
            "QPushButton { "
            "   background: white; color: #606266; border: 1px solid #dcdfe6; border-radius: 8px; "
            "   font-size: 15px; font-weight: bold; text-align: center; padding: 0px; min-height: 44px; "
            "} "
            "QPushButton:hover { background: #f5f7fa; color: #409eff; border-color: #409eff; }"
        );
        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

        QPushButton *confirmBtn = new QPushButton("保存");
        confirmBtn->setFixedSize(110, 44);
        confirmBtn->setCursor(Qt::PointingHandCursor);
        confirmBtn->setStyleSheet(
            "QPushButton { "
            "   background: #409eff; color: white; border-radius: 8px; "
            "   font-size: 15px; font-weight: bold; text-align: center; padding: 0px; min-height: 44px; outline: none; "
            "} "
            "QPushButton:hover { background: #66b1ff; }"
        );
        connect(confirmBtn, &QPushButton::clicked, this, &QDialog::accept);

        bottomLayout->addWidget(cancelBtn);
        bottomLayout->addWidget(confirmBtn);
        layout->addLayout(bottomLayout);

        mainLayout->addWidget(container);
    }
};

#endif // PETMODULE_H
