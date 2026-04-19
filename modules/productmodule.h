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
#include <QPushButton>
#include <QCheckBox>
#include "../common_types.h"

struct StockInRecord {
    QString dateTime;
    QString productName;
    QString barcode;
    int quantity;
    QString supplier;
    QString operatorName;
    QString imgPath; // 新增图片路径
};

class ProductModule : public QWidget {
    Q_OBJECT
public:
    explicit ProductModule(UserRole role, QWidget *parent = nullptr);
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupUI();
    void setupRecordTab(QWidget *tab);
    void addProductRow(const QString &barcode, const QString &name, const QString &spec, 
                      double price, int stock, int minStock, const QString &imgPath = "E:/QT/work/PetManager/images/stores/default.png");
    void addRecordRow(const StockInRecord &record);
    void updateStats();

private slots:
    void onStockIn();
    void onFilterRecords();
    void onResetRecords();
    
    // 分页与搜索
    void onPrevPage();
    void onNextPage();
    void onJumpPage();
    void onSearchChanged(const QString &text);
    void onPreviewImage();
    void onEditProduct();
    void onDeleteProduct();
    void onBatchDelete();

    // 记录分页 slots
    void onRecPrevPage();
    void onRecNextPage();
    void onRecJumpPage();

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
    void updatePagination();
    void updateRecordPagination();

    // 分页 UI
    QPushButton *prevBtn;
    QPushButton *nextBtn;
    QLabel *pageLabel;
    int m_currentPage;
    int m_pageSize;
    QLineEdit *jumpEdit;
    class QIntValidator *jumpValidator;
    QPushButton *jumpBtn;

    // 记录分页 UI
    QPushButton *recPrevBtn;
    QPushButton *recNextBtn;
    QLabel *recPageLabel;
    int m_recCurrentPage;
    int m_recPageSize;
    QLineEdit *recJumpEdit;
    class QIntValidator *recJumpValidator;
    QPushButton *recJumpBtn;

    UserRole m_role;
};

#endif
