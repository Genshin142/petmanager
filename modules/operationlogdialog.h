#ifndef OPERATIONLOGDIALOG_H
#define OPERATIONLOGDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QTextBrowser>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QDateEdit>
#include "logdatamanager.h"

class OperationLogDialog : public QDialog {
    Q_OBJECT
public:
    explicit OperationLogDialog(QWidget *parent = nullptr);

private slots:
    void loadPage(int page);
    void onLogSelected(QListWidgetItem *item);
    void onSearchClicked();
    void onPrevClicked();
    void onNextClicked();

private:
    void setupUi();
    QString renderDiffHtml(const QString &jsonStr);
    void addMockDataIfNeeded();

    LogDataManager *m_dataManager;
    int m_currentPage;
    int m_itemsPerPage;
    int m_totalItems;

    QDateEdit *m_startDateEdit;
    QDateEdit *m_endDateEdit;
    QLineEdit *m_operatorEdit;
    QListWidget *m_listWidget;
    QTextBrowser *m_detailBrowser;
    QLabel *m_pageLabel;
    QPushButton *m_prevBtn;
    QPushButton *m_nextBtn;
    QPushButton *m_searchBtn;
};

#endif // OPERATIONLOGDIALOG_H
