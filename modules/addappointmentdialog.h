#ifndef ADDAPPOINTMENTDIALOG_H
#define ADDAPPOINTMENTDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTextEdit>

#include "../common_types.h"

class AddAppointmentDialog : public QDialog {
    Q_OBJECT
public:
    explicit AddAppointmentDialog(QWidget *parent = nullptr);
    void setInitialData(const AppointmentInfo &info);
    QList<AppointmentInfo> getAppointmentInfos() const;
    void accept() override; // 重写 accept 实现拦截校验

private:
    void setupUI();
    void updateDays(); // 动态更新天数
    void validateDate(); // 校验日期合法性
    void onPetChanged(int index);
    void onAddServiceRow();
    bool eventFilter(QObject *obj, QEvent *event) override;
    
    QLineEdit *m_petSearchEdit;
    QComboBox *m_petCombo;
    // 主人信息
    QLabel *m_ownerNameLabel;
    QLabel *m_ownerPhoneLabel;
    
    // 多业务支持
    struct ServiceRowItems {
        QComboBox *serviceCombo;
        QWidget *dateEdit;  // CustomCalendarEdit
        QComboBox *hourCombo;
        QWidget *tagsContainer;
        QWidget *returnParamWidget; // 返程参数容器
        QWidget *returnDateEdit;    // 返程日期
        QComboBox *returnHourCombo; // 返程时间
        QWidget *boardingEndDateEdit; // 寄养离店日期
        QLabel  *boardingDaysLabel;   // 寄养天数显示
        QLabel  *boardingStatusLabel; // 寄养状态显示(空闲/紧张/已满)
        QLabel  *roomHint;            // 房号提示
        QComboBox *roomCombo;         // 房号选择
        QLineEdit *amountEdit;        // 新增：金额输入框
        QWidget *container;
    };
    QVBoxLayout *m_serviceContainer;
    QList<ServiceRowItems> m_serviceRows;
    QPushButton *m_addServiceBtn;

    // 基础字段
    QTextEdit *m_notesEdit;
    
    // 按钮与标签
    QPushButton *saveBtn;
    QPushButton *cancelBtn;
    QLabel *m_avatarLabel;
    QString m_currentAvatarPath;
    QLabel *m_errorLabel; // 非侵入式错误提示
};

#endif // ADDAPPOINTMENTDIALOG_H
