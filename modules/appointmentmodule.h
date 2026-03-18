#ifndef APPOINTMENTMODULE_H
#define APPOINTMENTMODULE_H

#include <QWidget>
#include "addappointmentdialog.h"

class QTableWidget;
class QLineEdit;
class QLabel;

class AppointmentModule : public QWidget {
    Q_OBJECT
public:
    explicit AppointmentModule(QWidget *parent = nullptr);

private slots:
    void onAddAppointmentClicked();
    void onEditAppointmentClicked();
    void onFinishServiceClicked();
    void onCancelAppointmentClicked();
    void onShowHistoryClicked();
    void onFilter(); // 新增：过滤搜索

private:
    void addRow(const AppointmentInfo &info);
    void updateStats();
    QTableWidget *apptTable;
    class QLineEdit *searchEdit;
    QLabel *todayTotalLabel;
    QLabel *pendingLabel;
    QLabel *finishedLabel;
    QList<AppointmentInfo> m_historyData; // 存储历史预约记录
};

#endif // APPOINTMENTMODULE_H
