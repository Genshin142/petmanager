#ifndef HISTORYAPPOINTMENTDIALOG_H
#define HISTORYAPPOINTMENTDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QLineEdit>
#include <QDateEdit>
#include "addappointmentdialog.h"

class HistoryAppointmentDialog : public QDialog {
    Q_OBJECT
public:
    explicit HistoryAppointmentDialog(const QList<AppointmentInfo> &history, QWidget *parent = nullptr);

private slots:
    void onSearchChanged(const QString &text);
    void onDateChanged(const QDate &date);

private:
    void setupUI();
    void addRow(const AppointmentInfo &info);
    void updateFilter(); // 统一过滤逻辑
    void validateDate(); // 同步日期下拉列表逻辑
    
    QTableWidget *historyTable;
    QLineEdit *searchEdit;
    class QComboBox *filterModeCombo; 
    class QComboBox *yearCombo, *monthCombo, *dayCombo;
    class QLabel *yLab, *mLab, *dLab;
    QList<AppointmentInfo> m_historyData; 
};

#endif // HISTORYAPPOINTMENTDIALOG_H
