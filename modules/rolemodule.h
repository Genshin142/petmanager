#ifndef ROLEMODULE_H
#define ROLEMODULE_H

#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QIntValidator>
#include "addemployeedialog.h"
#include "employeedetaildrawer.h"

class RoleModule : public QWidget
{
    Q_OBJECT
public:
    explicit RoleModule(QWidget *parent = nullptr);
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
private:
    void setupUI();
    void addEmployeeRow(const QString &id, const QString &name, const QString &role, const QString &status, 
                        const QString &gender, int age, const QString &phone, const QString &email, const QString &idCard,
                        double baseSalary, double performance, double commission, const QString &imgPath = "");
    void addEmployeeRowInPlace(int row, struct EmployeeInfo const &info);
    void setEmployeeRowData(int row, const QString &id, const QString &name, const QString &role, const QString &status, 
                            const QString &gender, int age, const QString &phone, const QString &email, const QString &idCard,
                            double baseSalary, double performance, double commission, const QString &imgPath);
    void updateStats();
    void addSampleData();
    QPixmap createCircularAvatar(const QPixmap &src, int size);
    
private slots:
    void onAddEmployee();
    void onEditEmployee();
    void onResetPassword(const QString &id);
    void onEditEmployeeFromDrawer(const struct EmployeeInfo &info);
    void onFilterChanged();
    void onSearchTextChanged(const QString &text);
    void onPrevPage();
    void onNextPage();
    void updatePagination();
    void showBigImage(const QString &path);
    void hideBigImage();
    void onCurrentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

private:
    QTableWidget *empTable;
    QLineEdit *searchEdit;
    QComboBox *roleFilterCombo;
    QWidget *statusFilterContainer;
    QString m_currentRoleFilter;
    QString m_currentStatusFilter;

    // 统计卡片数值与趋势标签
    QLabel *totalEmpLabel;
    QLabel *todayAttendLabel;
    QLabel *onLeaveLabel;
    QLabel *attendRateLabel;

    // 分页
    QPushButton *prevBtn;
    QPushButton *nextBtn;
    QLabel *pageLabel;
    int m_currentPage;
    int m_pageSize;

    // 默认头像缓存
    QPixmap m_maleAvatar;
    QPixmap m_femaleAvatar;

    // 大图预览交互
    QWidget *m_imagePreviewOverlay;
    QLabel *m_previewLabel;

    EmployeeDetailDrawer *m_drawer;
    bool m_isRefreshing = false;
    void refreshTablePreservingSelection(const QString &targetId);
};

#endif // ROLEMODULE_H
