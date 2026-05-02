#include "custom_calendar_edit.h"
#include <QApplication>
#include <QScreen>
#include <QGuiApplication>
#include <QPainter>
#include <QCalendarWidget>
#include "compactcalendar.h"
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

// BackPaintWidget 实现
BackPaintWidget::BackPaintWidget(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    // 使用 Qt::Popup：它是专为下拉菜单设计的，自动处理点击外部消失，且不显示独立任务栏图标
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
}

void BackPaintWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    
    // 绘制阴影和背景
    p.setBrush(QColor(0, 0, 0, 40));
    p.drawRoundedRect(rect(), 10, 10);
    
    p.setBrush(Qt::white);
    p.drawRoundedRect(rect().adjusted(2, 2, -2, -2), 8, 8);
}

void BackPaintWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    setFocus();
}

void BackPaintWidget::focusOutEvent(QFocusEvent *event) {
    // Qt::Popup 模式下，这个事件通常意味着用户点击了外部
    QWidget::focusOutEvent(event);
}

// CustomCalendarEdit 实现
CustomCalendarEdit::CustomCalendarEdit(QWidget *parent) 
    : QLineEdit(parent), m_widget(nullptr), m_calendar(nullptr) {
    setReadOnly(true);
    setCursor(Qt::PointingHandCursor);
    setContextMenuPolicy(Qt::NoContextMenu);
    
    // 创建内置箭头
    m_arrowBtn = new QToolButton(this);
    m_arrowBtn->setIcon(QIcon(":/images/chevron-down.svg"));
    m_arrowBtn->setIconSize(QSize(12, 12));
    m_arrowBtn->setCursor(Qt::PointingHandCursor);
    m_arrowBtn->setStyleSheet("QToolButton { border: none; background: transparent; padding: 0; }");
    
    connect(m_arrowBtn, &QToolButton::clicked, this, &CustomCalendarEdit::popCalendar);
}

CustomCalendarEdit::~CustomCalendarEdit() {
    if (m_widget) m_widget->deleteLater();
}

void CustomCalendarEdit::resizeEvent(QResizeEvent *event) {
    QLineEdit::resizeEvent(event);
    // 宽度设为 24px，与 QComboBox 一致
    m_arrowBtn->setGeometry(width() - 24, 0, 24, height());
}

void CustomCalendarEdit::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        // 延迟 10ms 弹出，确保焦点稳定
        QTimer::singleShot(10, this, &CustomCalendarEdit::popCalendar);
    }
    QLineEdit::mouseReleaseEvent(event);
}

void CustomCalendarEdit::initCalendar() {
    // 传入 window() 确保它不是一个独立的顶级窗口
    m_widget = new BackPaintWidget(this->window());
    m_widget->setFixedSize(340, 360);

    QVBoxLayout *layout = new QVBoxLayout(m_widget);
    layout->setContentsMargins(8, 8, 8, 8);

    m_calendar = new CompactCalendar(m_widget);
    // 确保日历能够获取焦点并响应点击
    m_calendar->setFocusPolicy(Qt::StrongFocus);
    
    if (m_minDate.isValid()) m_calendar->setMinimumDate(m_minDate);
    if (m_maxDate.isValid()) m_calendar->setMaximumDate(m_maxDate);
    
    layout->addWidget(m_calendar);

    connect(m_calendar, &QCalendarWidget::clicked, this, [=](const QDate &date) {
        setText(date.toString("yyyy-MM-dd"));
        m_widget->hide();
        emit dateChanged(date);
    });
}

void CustomCalendarEdit::popCalendar() {
    if (!m_widget) initCalendar();

    QPoint globalPos = mapToGlobal(QPoint(0, height() + 2));
    
    // 检查屏幕范围，防止出屏
    QScreen *screen = QGuiApplication::screenAt(globalPos);
    QRect screenRect = screen ? screen->availableGeometry() : QRect(0,0,1920,1080);
    
    // 垂直方向检查
    if (globalPos.y() + m_widget->height() > screenRect.bottom()) {
        globalPos.setY(mapToGlobal(QPoint(0,0)).y() - m_widget->height() - 2);
    }
    
    // 水平方向检查 (解决右侧日期日历显示不全问题)
    if (globalPos.x() + m_widget->width() > screenRect.right()) {
        globalPos.setX(mapToGlobal(QPoint(width(), 0)).x() - m_widget->width());
    }
    
    m_widget->move(globalPos);
    
    // 每次打开强制跳转到今天所在的月份 (用户要求)
    if (m_calendar) {
        m_calendar->setCurrentPage(QDate::currentDate().year(), QDate::currentDate().month());
    }
    
    m_widget->show();
    m_widget->setFocus();
}

void CustomCalendarEdit::setMinimumDate(const QDate &date) {
    m_minDate = date;
    if (m_calendar) m_calendar->setMinimumDate(date);
}

void CustomCalendarEdit::setMaximumDate(const QDate &date) {
    m_maxDate = date;
    if (m_calendar) m_calendar->setMaximumDate(date);
}

QDate CustomCalendarEdit::date() const {
    return QDate::fromString(text(), "yyyy-MM-dd");
}
