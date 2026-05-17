#ifndef OPERATIONLOGDIALOG_H
#define OPERATIONLOGDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QTextBrowser>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include "custom_calendar_edit.h"
#include <QComboBox>
#include <QPoint>
#include "logdatamanager.h"

class OperationLogDialog : public QDialog {
    Q_OBJECT
public:
    explicit OperationLogDialog(QWidget *parent = nullptr);

private slots:
    void loadPage(int page);
    void onLogSelected(QListWidgetItem *item);
    void onSearchClicked();
    void onResetClicked();
    void onPrevClicked();
    void onNextClicked();
    void onLogsReceived(const QList<SysOperationLog> &logs, int totalCount);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void setupUi();
    void applyStyles();
    QString renderDiffHtml(const QString &jsonStr);

    LogDataManager *m_dataManager;
    int m_currentPage;
    int m_itemsPerPage;
    int m_totalItems;

    CustomCalendarEdit *m_startDateEdit;
    CustomCalendarEdit *m_endDateEdit;
    QComboBox *m_operatorEdit;
    QComboBox *m_moduleCombo;
    QListWidget *m_listWidget;
    QTextBrowser *m_detailBrowser;
    QLabel *m_pageLabel;
    QPushButton *m_prevBtn;
    QPushButton *m_nextBtn;
    QPushButton *m_searchBtn;
    QPushButton *m_resetBtn;
    QPushButton *m_closeBtn;

    // Frameless window dragging
    bool m_dragging;
    QPoint m_dragPos;
};

#endif // OPERATIONLOGDIALOG_H
