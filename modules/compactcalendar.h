#ifndef COMPACTCALENDAR_H
#define COMPACTCALENDAR_H

#include <QCalendarWidget>
#include <QPainter>
#include <QSet>
#include <QDate>

#include <QVariantAnimation>
#include <QToolButton>
#include <QList>

#include <QComboBox>
#include <QHBoxLayout>

class CompactCalendar : public QCalendarWidget
{
    Q_OBJECT
    Q_PROPERTY(double breathingIntensity READ breathingIntensity WRITE setBreathingIntensity)

public:
    explicit CompactCalendar(QWidget *parent = nullptr) : QCalendarWidget(parent), m_intensity(0.0) {
        setNavigationBarVisible(true);
        setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
        setHorizontalHeaderFormat(QCalendarWidget::SingleLetterDayNames);
        
        // --- 1. 深度 UI 美化 (琥珀色系) ---
        setStyleSheet(
            "QCalendarWidget { background-color: white; border: none; border-radius: 12px; } "
            "QCalendarWidget QWidget#qt_calendar_navigationbar { "
            "   background-color: white; border-bottom: 0px solid transparent; min-height: 45px; "
            "} "
            "QCalendarWidget QToolButton { "
            "   color: #2d3436; font-weight: bold; border-radius: 6px; padding: 2px 5px; "
            "   font-size: 14px; font-family: 'PingFang SC', 'Microsoft YaHei'; "
            "} "
            "QCalendarWidget QToolButton:hover { background-color: #fff7ed; color: #ff9f43; } "
            "QCalendarWidget QToolButton::menu-indicator { image: none; width: 0px; } "
            "QCalendarWidget QAbstractItemView { "
            "   background-color: white; selection-background-color: transparent; "
            "   selection-color: transparent; outline: none; border: none; "
            "} "
            "QCalendarWidget QHeaderView::section { "
            "   background-color: transparent; color: #b2bec3; font-weight: bold; "
            "   padding-bottom: 8px; border: none; "
            "} "
            // 下拉列表样式 (工业级加固)
            "QComboBox { border: 1px solid #f0f2f5; border-radius: 6px; padding: 0 10px; min-height: 28px; background: white; font-size: 13px; font-weight: bold; color: #2d3436; text-align: center; } "
            "QComboBox:hover { border-color: #ff9f43; } "
            "QComboBox::drop-down { border: none; width: 18px; } "
            "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 10px; height: 10px; } "
            "QComboBox QAbstractItemView { border: 1px solid #f0f2f5; selection-background-color: #fff7ed; selection-color: #ff9f43; outline: none; } "
        );

        // 查找导航栏及内部组件
        QWidget *navBar = findChild<QWidget*>("qt_calendar_navigationbar");
        if (navBar) {
            // 隐藏原生的年月按钮
            QToolButton *monthBtn = findChild<QToolButton*>("qt_calendar_monthbutton");
            QToolButton *yearBtn = findChild<QToolButton*>("qt_calendar_yearbutton");
            if (monthBtn) monthBtn->hide();
            if (yearBtn) yearBtn->hide();

            // 创建新的下拉框逻辑
            m_monthCombo = new QComboBox(navBar);
            m_yearCombo = new QComboBox(navBar);
            
            for(int i = 1; i <= 12; ++i) m_monthCombo->addItem(QString("%1月").arg(i), i);
            for(int i = 2020; i <= 2035; ++i) m_yearCombo->addItem(QString("%1年").arg(i), i);

            m_monthCombo->setFixedWidth(75);
            m_yearCombo->setFixedWidth(85);

            // 将下拉框放入导航栏并实现紧凑居中 (对照疫苗模版)
            QHBoxLayout *navLayout = qobject_cast<QHBoxLayout*>(navBar->layout());
            if (navLayout) {
                // 清理所有旧的布局内容，重新按模版顺序插入
                // 默认顺序：[Prev] [Stretch] [Year] [Month] [Next] [Stretch]
                navLayout->setContentsMargins(10, 0, 10, 0);
                navLayout->setSpacing(10); // 按钮与下拉框之间的紧凑间距
                
                // 核心：在左右两端各放一个弹簧，把所有组件挤到中间
                navLayout->insertStretch(0, 1);       // 最左侧弹簧
                
                // 找到并移动箭头按钮到中间
                QToolButton *prevBtn = findChild<QToolButton*>("qt_calendar_prevmonth");
                QToolButton *nextBtn = findChild<QToolButton*>("qt_calendar_nextmonth");
                
                // 重新插入顺序：[Stretch] [Prev] [Year] [Month] [Next] [Stretch]
                // 注意：insertWidget 会移动已有的 widget
                navLayout->insertWidget(1, prevBtn);
                navLayout->insertWidget(2, m_yearCombo);
                navLayout->insertWidget(3, m_monthCombo);
                navLayout->insertWidget(4, nextBtn);
                
                navLayout->addStretch(1);             // 最右侧弹簧
            }

            // 同步逻辑：下拉框改变 -> 更新日历
            auto syncToCalendar = [this]() {
                int year = m_yearCombo->currentData().toInt();
                int month = m_monthCombo->currentData().toInt();
                this->setCurrentPage(year, month);
            };
            connect(m_monthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, syncToCalendar);
            connect(m_yearCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, syncToCalendar);

            // 同步逻辑：日历改变 (比如点箭头) -> 更新下拉框
            connect(this, &QCalendarWidget::currentPageChanged, this, [this](int year, int month) {
                m_yearCombo->setCurrentText(QString("%1年").arg(year));
                m_monthCombo->setCurrentIndex(month - 1);
            });

            // 初始化当前页
            m_yearCombo->setCurrentText(QString("%1年").arg(yearShown()));
            m_monthCombo->setCurrentIndex(monthShown() - 1);
        }

        // 查找并转换左右箭头外观
        QList<QToolButton*> buttons = findChildren<QToolButton*>();
        for (auto btn : buttons) {
            if (btn->objectName() == "qt_calendar_prevmonth") {
                btn->setIcon(QIcon());
                btn->setText("<");
                btn->setFixedSize(30, 30);
            } else if (btn->objectName() == "qt_calendar_nextmonth") {
                btn->setIcon(QIcon());
                btn->setText(">");
                btn->setFixedSize(30, 30);
            }
        }
        
        setGridVisible(false);

        // 呼吸动画
        m_breathingAnim = new QVariantAnimation(this);
        m_breathingAnim->setStartValue(0.2);
        m_breathingAnim->setEndValue(0.6);
        m_breathingAnim->setDuration(1500);
        m_breathingAnim->setEasingCurve(QEasingCurve::InOutSine);
        m_breathingAnim->setLoopCount(-1);
        connect(m_breathingAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v){
            setBreathingIntensity(v.toDouble());
        });
        m_breathingAnim->start();
    }

    double breathingIntensity() const { return m_intensity; }
    void setBreathingIntensity(double val) { m_intensity = val; update(); }

    // 三态数据接口: 已完成, 进行中, 异常
    void setStatusData(const QSet<QDate> &done, const QSet<QDate> &doing, const QSet<QDate> &danger) {
        m_doneDates = done;
        m_doingDates = doing;
        m_dangerDates = danger;
        update();
    }

    void setFosterRange(QDate start, QDate end) {
        m_fosterStart = start;
        m_fosterEnd = end;
        if (m_fosterEnd.isNull() || m_fosterEnd < m_fosterStart) {
            m_fosterEnd = QDate::currentDate();
        }
        update();
    }

protected:
    void paintCell(QPainter *painter, const QRect &rect, QDate date) const override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        bool isSelected = (date == selectedDate());
        bool isToday = (date == QDate::currentDate());
        bool isCurrentMonth = (date.month() == monthShown() && date.year() == yearShown());

        // 1. 绘制背景
        if (isSelected) {
            // 选中态：正圆琥珀色背景
            int side = qMin(rect.width(), rect.height()) - 8;
            QRect circleRect(rect.center().x() - side/2, rect.center().y() - side/2, side, side);
            
            // 绘制呼吸扩散圈
            QColor glow = QColor("#ff9f43");
            glow.setAlphaF(m_intensity);
            painter->setBrush(glow);
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(circleRect.adjusted(-3, -3, 3, 3));

            // 主背景
            painter->setBrush(QColor("#ff9f43"));
            painter->drawEllipse(circleRect);
        } else if (isToday) {
            // 今天：空心琥珀色圆圈
            int side = qMin(rect.width(), rect.height()) - 8;
            QRect circleRect(rect.center().x() - side/2, rect.center().y() - side/2, side, side);
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QPen(QColor("#ff9f43"), 1.5, Qt::DashLine));
            painter->drawEllipse(circleRect);
        }

        // 2. 绘制寄养区间 (带呼吸感的轻微背景)
        if (!m_fosterStart.isNull() && date >= m_fosterStart && date <= m_fosterEnd && !isSelected) {
            painter->setBrush(QColor(255, 159, 67, 15)); // 琥珀橙极淡透明背景
            painter->setPen(Qt::NoPen);
            painter->drawRect(rect);
        }

        // 3. 绘制日期数字
        QColor textColor = QColor("#2d3436");
        if (!isCurrentMonth) {
            textColor = QColor("#d1d1d1"); // 非本月淡化
        } else if (isSelected) {
            textColor = Qt::white; // 选中反白
        } else if (isToday) {
            textColor = QColor("#ff9f43"); // 今天强调
        } else if (date.dayOfWeek() == 7) {
            textColor = QColor("#f56c6c"); // 周日：浅红 (照抄样式)
        } else if (date.dayOfWeek() == 6) {
            textColor = QColor("#ff9f43"); // 周六：琥珀橙
        }
        
        painter->setPen(textColor);
        QFont font = painter->font();
        font.setFamily("PingFang SC");
        font.setBold(isSelected || isToday);
        font.setPointSize(isSelected ? 10 : 9);
        painter->setFont(font);
        
        // 确保数字始终在最上层绘制
        painter->drawText(rect.adjusted(0, -2, 0, -2), Qt::AlignCenter, QString::number(date.day()));

        // 4. 绘制三态状态点
        int dotY = rect.bottom() - 8;
        int dotX = rect.center().x();

        if (m_dangerDates.contains(date)) {
            drawStatusDot(painter, dotX, dotY, QColor("#f56c6c")); // 异常
        } else if (m_doingDates.contains(date)) {
            drawStatusDot(painter, dotX, dotY, QColor("#ff9f43")); // 进行中
        } else if (m_doneDates.contains(date)) {
            drawStatusDot(painter, dotX, dotY, QColor("#67c23a")); // 已完成
        }

        painter->restore();
    }

private:
    void drawStatusDot(QPainter *p, int x, int y, const QColor &color) const {
        p->setBrush(color);
        p->setPen(Qt::NoPen);
        p->drawEllipse(x - 2, y - 2, 4, 4);
    }

    QComboBox *m_monthCombo = nullptr;
    QComboBox *m_yearCombo = nullptr;

    QSet<QDate> m_doneDates;
    QSet<QDate> m_doingDates;
    QSet<QDate> m_dangerDates;
    QDate m_fosterStart;
    QDate m_fosterEnd;
    
    double m_intensity;
    QVariantAnimation *m_breathingAnim;
};

#endif // COMPACTCALENDAR_H
