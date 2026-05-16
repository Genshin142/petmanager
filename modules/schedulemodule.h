#ifndef SCHEDULEMODULE_H
#define SCHEDULEMODULE_H

#include <QWidget>
#include <QTableWidget>
#include <QDate>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include "common_types.h"

class ScheduleModule : public QWidget
{
    Q_OBJECT
public:
    explicit ScheduleModule(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onPrevWeek();
    void onNextWeek();
    void onToday();
    void onDateChanged(const QDate &date);
    void onCellClicked(int row, int col);
    void onExport();
    void onApplyTemplate();

private:
    void setupUI();
    void updateTable();
    void updateHeaderDates();
    
    QTableWidget *m_table;
    QLabel *m_dateRangeLabel;
    QDate m_currentMonday;
    class CustomCalendarEdit *m_jumpCalendar;

    // 大图预览交互 (复刻 RoleModule)
    QWidget *m_imagePreviewOverlay;
    QLabel *m_previewLabel;
    void showBigImage(const QString &path);
    void hideBigImage();
    QPixmap createCircularAvatar(const QPixmap &src, int size);

    // 样式辅助
    QWidget* createShiftWidget(const ScheduleInfo &info);
    void updateShiftTag(QLabel *tag, const ScheduleInfo &info);
};

#endif // SCHEDULEMODULE_H
