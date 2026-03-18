#ifndef PRODUCTMODULE_H
#define PRODUCTMODULE_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QDateEdit>
#include <QComboBox>
#include <QTabWidget>
#include <QDateTime>
#include "../common_types.h"

struct StockInRecord {
    QString dateTime;
    QString productName;
    QString barcode;
    int quantity;
    QString supplier;
    QString operatorName;
};

class ProductModule : public QWidget {
    Q_OBJECT
public:
    explicit ProductModule(UserRole role, QWidget *parent = nullptr);

private:
    void setupUI();
    void setupRecordTab(QWidget *tab);
    void addProductRow(const QString &barcode, const QString &name, const QString &spec, 
                      double price, int stock, int minStock);
    void addRecordRow(const StockInRecord &record);
    void updateStats();

private slots:
    void onStockIn();
    void onFilterRecords();
    void onResetRecords();

private:
    QTableWidget *prodTable;
    QLineEdit *searchEdit;

    // 统计指标
    QLabel *totalValueLabel;
    QLabel *lowStockLabel;
    QLabel *varietyLabel;

    // 入库记录页 UI
    QTableWidget *recordTable;
    QLineEdit *recordSearch;
    QComboBox *sYearCombo, *sMonthCombo, *sDayCombo;
    QComboBox *eYearCombo, *eMonthCombo, *eDayCombo;
    QList<StockInRecord> m_records;

    void updateDays(QComboBox *y, QComboBox *m, QComboBox *d);

    UserRole m_role;
};

#endif
