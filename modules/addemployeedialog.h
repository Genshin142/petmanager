#ifndef ADDEMPLOYEEDIALOG_H
#define ADDEMPLOYEEDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include "custom_calendar_edit.h"

struct EmployeeInfo {
    QString id;
    QString name;
    QString role;
    QString gender;
    int age;
    QString phone;
    QString email;
    QString idCard;
    int baseSalary;
    QString status;
    QString imgPath; // 员工头像路径
    
    // 新增专业管理字段
    QString joinDate;          // 入职日期
    QString emergencyContact;  // 紧急联系人
    QString emergencyPhone;    // 紧急联系人电话
    QString address;           // 家庭住址
    QString education;         // 学历
    QString department;        // 所属部门
};

class AddEmployeeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddEmployeeDialog(QWidget *parent = nullptr);
    ~AddEmployeeDialog();

    void setEmployeeInfo(const EmployeeInfo &info);
    EmployeeInfo employeeInfo() const;
    void updateAvatarPreview(const QString &path);

private slots:
    void onSave();
    void onCancel();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupUI();
    
    // UI 元素
    QLineEdit *nameEdit;
    QComboBox *roleCombo;
    QComboBox *genderCombo;
    QSpinBox *ageBox;
    QLineEdit *phoneEdit;
    QLineEdit *emailEdit;
    QLineEdit *idCardEdit;
    QSpinBox *salarySpin;
    QComboBox *statusCombo;
    
    // 新增 HR 控件
    QComboBox *eduCombo;
    QComboBox *deptCombo;
    QLineEdit *emergencyEdit;
    QLineEdit *emergencyPhoneEdit;
    QLineEdit *addressEdit;
    CustomCalendarEdit *joinDateEdit;
    
    QLabel *titleLabel;
    QPushButton *saveBtn;
    
    // 头像相关
    QLabel *avatarPreview;
    QString m_selectedImgPath;
    
    QPoint dragPosition;
    QString m_id;

    // 大图预览交互
    QWidget *m_imagePreviewOverlay;
    QLabel *m_previewLabel;
    void showBigImage(const QString &path);
    void hideBigImage();
};

#endif // ADDEMPLOYEEDIALOG_H
