#ifndef SALARYMODULE_H
#define SALARYMODULE_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QEvent>
#include <QDialog>

class SalaryModule : public QWidget
{
    Q_OBJECT
public:
    explicit SalaryModule(QWidget *parent = nullptr);

private slots:
    void refreshData();
    void onMonthChanged(int index);
    void onSearchEmployee(const QString &text);
    void onPaySalary(const QString &salaryId);
    void onEditSalary(const QString &salaryId);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupUI();
    void updateStats();
    void updateTable();
    void onPrevPage();
    void onNextPage();

    // UI 元素
    QComboBox *m_monthCombo;
    QLineEdit *m_searchEdit;
    QTableWidget *m_salaryTable;
    
    // 分页 UI
    QLabel *m_pageInfoLabel;
    QPushButton *m_prevBtn;
    QPushButton *m_nextBtn;

    // 统计卡片组件
    QLabel *m_totalPayrollVal;
    QLabel *m_paidCountVal;
    QLabel *m_pendingCountVal;

    // 详情卡片 UI (增强版)
    QLabel *m_detailAvatar;
    QLabel *m_detailName;
    QLabel *m_detailEmpId;
    QLabel *m_detailRole;
    QLabel *m_detailBaseVal;
    QLabel *m_detailCommVal;
    
    // 五险一金明细
    QLabel *m_detailInsBaseVal; // 参保基数
    QLabel *m_detailPensionVal; // 养老
    QLabel *m_detailMedicalVal; // 医疗
    QLabel *m_detailUnemploymentVal; // 失业
    QLabel *m_detailHousingFundVal; // 公积金
    
    QLabel *m_detailInsuranceVal; // 总计代缴
    QLabel *m_detailTaxVal;
    QLabel *m_detailNetVal;
    QPushButton *m_payBtn;

    // Image Preview (复刻会员模块)
    void setupImagePreview();
    void showBigImage(const QString &path, const QString &text = "");
    void hideBigImage();
    
    QWidget *m_imagePreviewOverlay = nullptr;
    QLabel *m_previewLabel = nullptr;

    QString m_currentMonth;
    QString m_currentSalaryId;
    QString m_currentStaffImgPath;

    int m_currentPage = 1;
    const int m_pageSize = 11;
};

#endif // SALARYMODULE_H
