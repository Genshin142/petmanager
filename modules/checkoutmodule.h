#ifndef CHECKOUTMODULE_H
#define CHECKOUTMODULE_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QDateEdit>
#include "custom_calendar_edit.h"
#include "../common_types.h"

class OrderDetailDrawer;
class QPushButton;
#include <QTimer>

class CheckoutModule : public QWidget {
    Q_OBJECT
public:
    explicit CheckoutModule(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupUI();
    void updatePagination();
    void updateStats();
    void refreshView();

    QList<OrderInfo> m_displayData;
    int m_currentPage = 1;
    int m_pageSize = 12;

    // UI Components
    QTableWidget *orderTable;
    QLineEdit *m_searchEdit;
    
    // Cockpit Stats
    QLabel *m_statTodayRevenue;
    QLabel *m_statPeriodRevenue;
    QLabel *m_statPendingCount;
    QLabel *m_statAvgTicket;
    CustomCalendarEdit *m_startDateEdit;
    CustomCalendarEdit *m_endDateEdit;
    QWidget *moduleFilterContainer;
    QString m_currentModuleFilter;

    void setDateRange(const QDate &start, const QDate &end);
    
    QLabel *pageLabel;
    QLineEdit *jumpEdit;
    QPushButton *prevBtn;
    QPushButton *nextBtn;

    OrderDetailDrawer *m_detailDrawer;

private slots:
    void onFilter();
    void onPrevPage();
    void onNextPage();
    void onJumpPage();
    void onOrderClicked(int row);

private:
    QTimer *m_refreshTimer;
};

#endif // CHECKOUTMODULE_H
