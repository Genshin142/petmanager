#ifndef PERFORMANCEMODULE_H
#define PERFORMANCEMODULE_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QEvent>
#include <QDialog>

class PerformanceModule : public QWidget
{
    Q_OBJECT
public:
    explicit PerformanceModule(QWidget *parent = nullptr);

private slots:
    void refreshData();
    void onVerifySingle(const QString &recordId);
    void onFilterChanged();
    void onPrevPage();
    void onNextPage();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupUI();
    void updateStats();
    void updateTable();

    // UI 元素
    class CustomCalendarEdit *m_startDateEdit;
    class CustomCalendarEdit *m_endDateEdit;
    QComboBox *m_employeeCombo;
    QComboBox *m_statusCombo;
    QLineEdit *m_searchEdit;
    QTableWidget *m_perfTable;
    QPushButton *m_confirmVerifyBtn;

    // 统计卡片
    QLabel *m_totalRevenueVal;
    QLabel *m_totalCommVal;
    QLabel *m_pendingVerifyCountVal;

    // 详情卡片 UI
    QLabel *m_detailHeaderAvatar;
    QLabel *m_detailHeaderName;
    QLabel *m_detailHeaderEmpId;
    QLabel *m_detailHeaderRole;
    
    QLabel *m_detailOrderIdVal;
    QLabel *m_detailCustomerVal;
    QLabel *m_detailPaymentVal;
    QLabel *m_detailPetVal;
    QLabel *m_detailOrderTotalVal;
    QLabel *m_detailActualPaidVal;
    QLabel *m_detailCommFormulaVal;

    // 分页控制
    QLabel *m_pageInfoLabel;
    QPushButton *m_prevBtn;
    QPushButton *m_nextBtn;

    // Image Preview (复刻会员模块)
    void setupImagePreview();
    void showBigImage(const QString &path, const QString &text = "");
    void hideBigImage();
    
    QWidget *m_imagePreviewOverlay = nullptr;
    QLabel *m_previewLabel = nullptr;

    int m_currentPage = 1;
    const int m_pageSize = 11;

    QString m_currentEmployeeId;
    QString m_currentRecordId; // 当前选中的记录 ID
    QString m_currentStaffImgPath; // 当前头像路径
};

#endif // PERFORMANCEMODULE_H
