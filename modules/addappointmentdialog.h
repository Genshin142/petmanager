#ifndef ADDAPPOINTMENTDIALOG_H
#define ADDAPPOINTMENTDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>

struct AppointmentInfo {
    QString memberName;
    QString memberPhone;
    QString date;
    QString hour;
    QString service;
    QString station;
    QString staff;
    QString status; // "Pending", "Completed", "Canceled"
};

class AddAppointmentDialog : public QDialog {
    Q_OBJECT
public:
    explicit AddAppointmentDialog(QWidget *parent = nullptr);
    void setInitialData(const AppointmentInfo &info);
    AppointmentInfo getAppointmentInfo() const;
    void accept() override; // 重写 accept 实现拦截校验

private:
    void setupUI();
    void updateDays(); // 动态更新天数
    void validateDate(); // 校验日期合法性
    
    QLineEdit *nameEdit;
    QLineEdit *phoneEdit;
    QComboBox *yearCombo;
    QComboBox *monthCombo;
    QComboBox *dayCombo;
    QComboBox *hourCombo;
    QComboBox *serviceCombo;
    QComboBox *stationCombo;
    QComboBox *staffCombo;
    
    QPushButton *saveBtn;
    QPushButton *cancelBtn;
    QLabel *titleLabel;
};

#endif // ADDAPPOINTMENTDIALOG_H
