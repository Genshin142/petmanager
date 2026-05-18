#ifndef SERVICEMANAGEMENTMODULE_H
#define SERVICEMANAGEMENTMODULE_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QPropertyAnimation>
#include <QListWidget>
#include <QComboBox>
#include "../common_types.h"

class ServiceManagementModule : public QWidget
{
    Q_OBJECT
public:
    explicit ServiceManagementModule(UserRole role, QWidget *parent = nullptr);

private:
    void setupMasterTable();
    void setupDetailPanel();
    void updateDetailPanel(const ServiceInfo &info);
    void updateTableData();

private slots:
    void onSearchChanged(const QString &text);
    void onCategoryFilterChanged();
    void onAddService();
    void onEditService();
    void onToggleServiceStatus();
    void onTableSelectionChanged();
    void onPrevPage();
    void onNextPage();
    void updatePagination();

private:
    UserRole m_role;
    QLineEdit *m_searchEdit;
    QComboBox *m_statusCombo;
    QWidget *m_categoryContainer;
    QTableWidget *m_serviceTable; // 恢复为表格模式
    
    // 分页组件
    QPushButton *prevBtn;
    QPushButton *nextBtn;
    QLabel *pageLabel;
    int m_currentPage = 1;
    int m_pageSize = 10;

    // 详情面板组件 (右侧高级看板)
    QWidget *m_detailPanel;
    QLabel *m_lblDetailName, *m_lblDetailSales, *m_lblDetailRevenue;
    QLineEdit *m_editCommFixed, *m_editPrice, *m_editDuration, *m_editCategory, *m_editId;
    QLabel *m_lblHistory, *m_lblDetailDesc;
    
    ServiceInfo m_currentService;
    
    // 统计面板组件
    QLabel *m_lblStatTotal;
    QLabel *m_lblStatPopular;
    QLabel *m_lblStatRevenue;
    QLabel *m_lblStatComm;
};

#endif // SERVICEMANAGEMENTMODULE_H
