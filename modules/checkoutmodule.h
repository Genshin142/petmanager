#ifndef CHECKOUTMODULE_H
#define CHECKOUTMODULE_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QDate>

class CheckoutModule : public QWidget {
    Q_OBJECT
public:
    explicit CheckoutModule(QWidget *parent = nullptr);

private:
    void setupUI();
    void updatePagination();
    void updateTotal();

    struct OrderRecord {
        QString date;
        QString orderId;
        QString memberId;
        QString memberName;
        QString item;
        double unitPrice;
        int qty;
        double discount;
        double total;
    };
    QList<OrderRecord> m_orderData;

    // 分页组件
    int m_currentPage = 1;
    int m_pageSize = 10;
    QLabel *pageLabel;
    class QLineEdit *jumpEdit;
    class QPushButton *prevBtn;
    class QPushButton *nextBtn;
    class QIntValidator *jumpValidator;

private slots:
    void onFilter();
    void updateDayCombo(QComboBox* y, QComboBox* m, QComboBox* d);
    void initDateGroup(QComboBox* y, QComboBox* m, QComboBox* d, const QDate &initDate);
    
    void onPrevPage();
    void onNextPage();
    void onJumpPage();
    void onBatchDelete();
    void onDeleteSingle();

private:
    QTableWidget *orderTable;
    QLineEdit *memberSearchEdit;
    QLabel *totalLabel;
    QLabel *orderCountLabel;
    QLabel *totalAmtLabel;
    QLabel *avgAmtLabel;
    QComboBox *startYearCombo, *startMonthCombo, *startDayCombo;
    QComboBox *endYearCombo, *endMonthCombo, *endDayCombo;
};

#endif // CHECKOUTMODULE_H
