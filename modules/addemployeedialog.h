#ifndef ADDEMPLOYEEDIALOG_H
#define ADDEMPLOYEEDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include "custom_calendar_edit.h"

#include "common_types.h"

class AddEmployeeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddEmployeeDialog(QWidget *parent = nullptr);
    ~AddEmployeeDialog();

    void setEmployeeInfo(const EmployeeInfo &info);
    void setNextAccount(const QString &account); // 确保这里有声明
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
    QLineEdit *emergencyEdit;
    QLineEdit *emergencyPhoneEdit;
    QLineEdit *addressEdit;
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
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
