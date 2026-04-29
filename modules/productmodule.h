#ifndef PRODUCTMODULE_H
#define PRODUCTMODULE_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QDateEdit>
#include <QComboBox>
#include <QScrollArea>
#include <QGridLayout>
#include <QTabWidget>
#include <QDateTime>
#include <QPushButton>
#include <QCheckBox>
#include "../common_types.h"



class ProductModule : public QWidget {
    Q_OBJECT
public:
    explicit ProductModule(UserRole role, QWidget *parent = nullptr);
    int getLowStockCount() const;
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupUI();
    void showProductEditDialog(const ProductInfo &info, bool isNew = false);
    void setupDetailDrawer();
    void updateDetailDrawer(const ProductInfo &info);
    void updateNutritionTable(const QVariantMap &nutrition);
    
    void addProductRow(const ProductInfo &info);
    void updateStats();

private slots:
    void onListing();
    
    // 分页与搜索
    void onPrevPage();
    void onNextPage();
    void onJumpPage();
    void onSearchChanged(const QString &text);
    void onPreviewImage();
    void onEditProduct();
    void onDeleteProduct();
    void onBatchDelete();
    void onShowBatchDetails(int row, int col);

private:
    QTabWidget *m_mainTabs;
    QTableWidget *prodTable;
    QLineEdit *searchEdit;
    class QButtonGroup *m_categoryGroup;

    // 统计指标
    QLabel *totalValueLabel;
    QLabel *lowStockLabel;
    QLabel *varietyLabel;

    void updatePagination();
    
    // 分页 UI
    QPushButton *prevBtn;
    QPushButton *nextBtn;
    QLabel *pageLabel;
    int m_currentPage;
    int m_pageSize;
    QLineEdit *jumpEdit;
    class QIntValidator *jumpValidator;
    QPushButton *jumpBtn;

    UserRole m_role;
    QWidget *m_detailDrawer;
    QWidget *m_backdrop;        // 半透明遮罩层
    QScrollArea *m_detailScroll;
    QLabel *m_mainPreview;
    QWidget *m_thumbContainer;
    QLabel *m_lblDetailName, *m_lblDetailBarcode, *m_lblDetailSupplier, *m_lblDetailSupplierContact;
    QLabel *m_lblDetailBrand, *m_lblDetailOrigin, *m_lblDetailSpec;
    QLabel *m_lblDetailIngredients, *m_lblDetailStorage;
    QPushButton *m_btnModifyInfo;
    QPushButton *m_btnExpandIngredients;
    bool m_isIngredientsExpanded = false;
    
    QLabel *m_lblDrawerHeaderTitle;
    
    // 财务与库存标签
    QLabel *m_lblSalePrice, *m_lblCostPrice, *m_lblGrossMargin;
    QLabel *m_lblCurrentStock, *m_lblMinStock;
    
    // 适用性与销售话术
    QLabel *m_lblSuitablePets, *m_lblPairingSuggestion;
    QWidget *m_tagContainer;
    
    // 质控标签
    QLabel *m_lblHealthScore;
    QWidget *m_nutritionContainer;
    QLabel *m_lblDetailDate, *m_lblDetailExpiry;
    class QProgressBar *m_expiryBar;
    
    // 动态流转区
    QWidget *m_restockBanner;
    QVBoxLayout *m_flowRecordsLayout;
    
    class QPropertyAnimation *m_drawerAnim;
    bool m_drawerOpen = false;
    
    void openDrawer();
    void closeDrawer();
};

#endif
