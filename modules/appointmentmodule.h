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

    void addRow(const AppointmentInfo &info); // 内部废弃或仅作单条生成使用
    void updatePagination();
    void updateStats();
    QTableWidget *apptTable;
    class QLineEdit *searchEdit;
    QLabel *todayTotalLabel;
    QLabel *pendingLabel;
    QLabel *finishedLabel;
    
    QList<AppointmentInfo> m_activeData;  // 存储当前未完成预约
    QList<AppointmentInfo> m_historyData; // 存储历史预约记录

    // 分页组件
    int m_currentPage = 1;
    int m_pageSize = 10;
    QLabel *pageLabel;
    class QLineEdit *jumpEdit;
    class QPushButton *prevBtn;
    class QPushButton *nextBtn;
    class QIntValidator *jumpValidator;

private slots:
    void onAddAppointmentClicked();
    void onEditAppointmentClicked();
    void onFinishServiceClicked();
    void onCancelAppointmentClicked();
    void onShowHistoryClicked();
    void onFilter();
    void onPrevPage();
    void onNextPage();
    void onJumpPage();
    void onBatchCancel();

private:
    void setupUI();
};

#endif // APPOINTMENTMODULE_H
