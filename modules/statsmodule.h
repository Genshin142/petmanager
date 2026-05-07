#ifndef STATSMODULE_H
#define STATSMODULE_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QDateTime>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>

// 前置声明
class QChartView;
class QChart;
class OrderDetailDrawer;

class StatsModule : public QWidget {
    Q_OBJECT
public:
    explicit StatsModule(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onCategoryChanged(int index);
    void onDateRangeChanged();
    void onSearch(const QString &text);

private:
    void setupUI();
    void setupNavigation();
    void setupDashboardCards();
    void setupMainContent();
    
    // 各分类视图初始化
    QWidget* createFinanceView();
    QWidget* createServiceView();
    QWidget* createInventoryView();
    QWidget* createMemberView();

    // 数据刷新
    void refreshData();
    void updateCards();
    void updateCharts();
    void updateTable();

private:
    // 导航组件
    QWidget *m_navBar;
    QList<QPushButton*> m_navButtons;
    QStackedWidget *m_viewStack;

    // KPI 卡片 (仪表盘)
    QList<QLabel*> m_cardValues;
    QList<QLabel*> m_cardTitles;
    QList<QLabel*> m_cardTrends; // 环比趋势指标
    QWidget *m_cardContainer;
    QLabel *m_kpiValue2; // 客单价 / 占比
    QLabel *m_kpiValue3; // 服务单量 / 入住率
    QLabel *m_kpiValue4; // 商品销量 / 预警数

    // 时间筛选
    QLineEdit *m_dateEdit; // 模拟日期区间选择
    QLineEdit *m_searchEdit;
    QComboBox *m_trendRangeCombo; // 趋势时间切换
    QComboBox *m_yearPicker;      // 历史年份选择
    QComboBox *m_monthPicker;     // 历史月份选择
    QLabel *m_customTooltip;      // 自定义悬浮提示框

    // 财务视图组件
    QChartView *m_finTrendChart;
    QChartView *m_finCompChart;
    QChartView *m_finPayChart;

    // 服务分析组件
    QTableWidget *m_staffRankTable;
    QTableWidget *m_serviceRankTable;
    QChartView *m_serviceHeatmapChart;

    // 库存分析组件
    QTableWidget *m_invAlertTable;
    QTableWidget *m_productRankTable;

    OrderDetailDrawer *m_detailDrawer;

    // Image Viewer Overlay
    QWidget *m_backdrop;
    QLabel *m_largePreviewLabel;

    // 状态
    int m_currentCategory; 
    QString m_currentMonth;

    // 内部数据计算
    void updateServiceAnalysis();
    void updateInventoryAnalysis();
    
    // 分页处理
    void updateStaffTable();
    void updateServiceTable();
    void goToStaffPage(int delta);
    void goToServicePage(int delta);

    // 分页状态
    int m_staffPage = 0;
    int m_servicePage = 0;
    bool m_staffSortByRev = true;
    bool m_serviceSortByRev = true;
    int m_staffTimeRange = 1;   // 0:日, 1:月, 2:年
    int m_serviceTimeRange = 1; 
    QLabel *m_staffPageLabel = nullptr;
    QLabel *m_servicePageLabel = nullptr;
    
    // 缓存数据用于分页
    struct StaffRankData { QString name; QString id; QString pos; QString color; int count; double rev; };
    QList<StaffRankData> m_allStaffData;
    struct SvcRankData { QString name; int count; double rev; };
    QList<SvcRankData> m_allServiceData;
};

#endif // STATSMODULE_H
