#ifndef STATSMODULE_H
#define STATSMODULE_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QDateTime>

struct SalesRecord {
    QDate date;
    QString name;
    int count;
    double amount;
    bool isProduct; // true for product, false for service
};

class QChartView;
class QChart;

class StatsModule : public QWidget {
    Q_OBJECT
public:
    explicit StatsModule(QWidget *parent = nullptr);

private:
    void setupUI();
    void showServiceRank();
    void showProductRank();
    void updateDashboard();
    void setupCharts(); // 新增：初始化图表
    void updateCharts(); // 新增：刷新图表数据
    
private slots:
    void onFilter();
    void onExport(); // 新增：导出报表
    void updateDayCombo(QComboBox* y, QComboBox* m, QComboBox* d);
    
private:
    QTableWidget *rankTable;
    QPushButton *serviceBtn;
    QPushButton *productBtn;
    QLineEdit *searchEdit;
    QComboBox *startYearCombo, *startMonthCombo, *startDayCombo;
    QComboBox *endYearCombo, *endMonthCombo, *endDayCombo;

    // 仪表盘
    QLabel *todayRevenueLabel;
    QLabel *avgOrderLabel;
    QLabel *serviceCountLabel;
    QLabel *productCountLabel;
    
    // 图表组件
    QChartView *pieChartView;
    QChartView *barChartView;
    
    bool isServiceMode;
    QList<SalesRecord> m_sales;
};

#endif
