#ifndef CUSTOM_CALENDAR_EDIT_H
#define CUSTOM_CALENDAR_EDIT_H

#include <QLineEdit>
#include <QMouseEvent>
#include <QDateTime>
#include <QVBoxLayout>
#include <QToolButton>

class CompactCalendar;

// 1. 背景绘制容器
class BackPaintWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BackPaintWidget(QWidget *parent = nullptr);
protected:
    void paintEvent(QPaintEvent *) override;
    void showEvent(QShowEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
};

// 2. 自定义日历输入框
class CustomCalendarEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit CustomCalendarEdit(QWidget *parent = nullptr);
    ~CustomCalendarEdit();

    void setMinimumDate(const QDate &date);
    void setMaximumDate(const QDate &date);
    QDate date() const;

signals:
    void dateChanged(const QDate &date);

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void initCalendar();
    void popCalendar();

private:
    BackPaintWidget *m_widget;
    CompactCalendar *m_calendar;
    QToolButton *m_arrowBtn;
    QDate m_minDate;
    QDate m_maxDate;
};

#endif // CUSTOM_CALENDAR_EDIT_H
