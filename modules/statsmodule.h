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
#include <QJsonObject>
#include <QJsonArray>

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
    void showEvent(QShowEvent *event) override;

private slots:
    void onCategoryChanged(int index);
    void onDateRangeChanged();
    void onSearch(const QString &text);
    
    // 网络回调
    void onDashboardStatsReceived(const QJsonObject &data);
    void onRevenueTrendReceived(const QJsonArray &data);


private:
    void setupUI();
    void setupNavigation();
    void setupDashboardCards();
    void setupMainContent();
    void ensureChartsCreated();

    bool m_chartsCreated = false;
    QJsonObject m_cachedDashboardStats;
    QJsonArray m_cachedRevenueTrend;
    
    // 各分类视图初始化
    QWidget* createFinanceView();
    QWidget* createServiceView();
    QWidget* createInventoryView();
    QWidget* createServiceRankView();
    QWidget* createMemberView();

    // 数据刷新
    void refreshData();
    void updateCards();
    void updateCharts();
    void updateTable();
    void updateTopCharts();

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
    QWidget *m_topPieContainer = nullptr; // 控制顶部扇形图显示/隐藏
    QWidget *m_finPiesContainer = nullptr;
    QWidget *m_prodPieContainer = nullptr;
    QWidget *m_svcPieContainer = nullptr;
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
    QChartView *m_finTrendChart = nullptr;
    QChartView *m_finCompChart = nullptr;
    QChartView *m_finPayChart = nullptr;
    QWidget *m_finTrendChartContainer = nullptr;
    QWidget *m_finCompChartContainer = nullptr;
    QWidget *m_finPayChartContainer = nullptr;
    QStackedWidget *m_financeMainStack = nullptr;
    QTableWidget *m_dailyRevenueTable = nullptr;
    QLabel *m_dailyRevPageLabel = nullptr;

    // 服务分析组件
    QTableWidget *m_staffRankTable;
    QTableWidget *m_serviceRankTable; // 用于店员界面的服务明细，或新视图的服务单项
    QTableWidget *m_serviceCategoryRankTable = nullptr; // 服务类目排行
    QChartView *m_serviceHeatmapChart = nullptr;
    QChartView *m_productCategoryChart = nullptr; // 商品销售占比图
    QWidget *m_serviceHeatmapChartContainer = nullptr;
    QWidget *m_productCategoryChartContainer = nullptr;

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
    void updateProductAnalysis();
    void updateServiceRankAnalysis();
    
    void updateDailyRevTable();
    void goToDailyRevPage(int delta);
    
    // 分页处理
    void updateStaffTable();
    void updateServiceTable();
    void updateProductTable();
    void updateCategoryTable();
    void updateServiceItemTable();
    void updateServiceCategoryTable();

    void goToStaffPage(int delta);
    void goToServicePage(int delta);
    void goToProductPage(int delta);
    void goToCategoryPage(int delta);
    void goToServiceItemPage(int delta);
    void goToServiceCategoryPage(int delta);

    // 分页状态
    int m_staffPage = 0;
    int m_servicePage = 0;
    int m_productPage = 0;
    int m_categoryPage = 0;
    int m_serviceItemPage = 0;
    int m_serviceCategoryPage = 0;

    int m_dailyRevPage = 0;

    bool m_staffSortByRev = true;
    bool m_serviceSortByRev = true;
    bool m_productSortByRev = true;
    bool m_categorySortByRev = true;
    bool m_serviceItemSortByRev = true;
    bool m_serviceCategorySortByRev = true;

    int m_staffTimeRange = 1;   // 0:日, 1:月, 2:年
    int m_serviceTimeRange = 1; 
    int m_productTimeRange = 1; 
    int m_serviceRankTimeRange = 1;

    QLabel *m_staffPageLabel = nullptr;
    QLabel *m_servicePageLabel = nullptr;
    QLabel *m_productPageLabel = nullptr;
    QLabel *m_categoryPageLabel = nullptr;
    QLabel *m_serviceItemPageLabel = nullptr;
    QLabel *m_serviceCategoryPageLabel = nullptr;

    QTableWidget *m_categoryRankTable = nullptr;
    
    // 缓存数据用于分页
    struct DailyRevData { QString date; double total; double svc; double prod; double foster; int count; double avg; };
    QList<DailyRevData> m_allDailyRevData;

    struct StaffRankData { QString name; QString id; QString pos; QString color; int count; double rev; };
    QList<StaffRankData> m_allStaffData;
    struct SvcRankData { QString name; int count; double rev; };
    QList<SvcRankData> m_allServiceData;
    QList<SvcRankData> m_allServiceCategoryData;
    struct ProductRankData { QString name; int count; double rev; QString imgData; };
    QList<ProductRankData> m_allProductData;
    QList<ProductRankData> m_allCategoryData;
};

#endif // STATSMODULE_H
