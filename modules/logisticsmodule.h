#ifndef LOGISTICSMODULE_H
#define LOGISTICSMODULE_H

#include <QWidget>
#include "logisticsmanager.h"
#include "logisticsdetaildrawer.h"
#include <QDate>

class QVBoxLayout;
class QFlowLayout;
class QComboBox;
class QScrollArea;
class QLabel;

class LogisticsModule : public QWidget
{
    Q_OBJECT
public:
    explicit LogisticsModule(QWidget *parent = nullptr);

public slots:
    void refreshTasks();
    void onDateChanged(const QDate &date);
    void onTaskSelected(const QString &taskId);
    void showCreateTaskDialog();
    void showHistoryDialog();
    void showBigImage(const QString &path);
    void onPrevDayClicked();
    void onNextDayClicked();
    void onDatePickerClicked();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

private:
    void setupUI();
    void renderTaskCards(const QString &filterStatus = "全部");

    QDate m_currentDate;
    class QPushButton *m_prevDayBtn;
    class QPushButton *m_nextDayBtn;
    class QPushButton *m_datePickerBtn;
    class QPushButton *m_todayBtn;

    LogisticsDetailDrawer *m_detailDrawer;
    QWidget *m_kanbanArea;
    QVBoxLayout *m_todayListLayout;
    QVBoxLayout *m_tomorrowListLayout;
    QLabel *m_todayTitle;
    QLabel *m_tomorrowTitle;
    QString m_selectedTaskId;
    QString m_preselectedTimeSlot;

    QVBoxLayout *m_mainLayout;
    QComboBox *m_filterCombo;
    QScrollArea *m_scrollArea;
    QWidget *m_cardsContainer;

    QLabel *m_statTotalLabel;
    QLabel *m_statPendingLabel;
    QLabel *m_statOngoingLabel;
    QLabel *m_statCompletedLabel;

    void updateStatistics();
    QWidget* createStatCard(const QString &title, const QString &value, const QString &color, QLabel **outValueLabel);
};

#endif // LOGISTICSMODULE_H
