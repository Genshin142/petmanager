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
    void addOrderRow(const QDate &date, const QString &orderId, const QString &memberId, const QString &memberName,
                     const QString &item, double unitPrice, int qty, double discount);
    void updateTotal();

private slots:
    void onFilter();
    void updateDayCombo(QComboBox* y, QComboBox* m, QComboBox* d);
    void initDateGroup(QComboBox* y, QComboBox* m, QComboBox* d, const QDate &initDate);

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
