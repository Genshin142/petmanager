#ifndef ADDEMPLOYEEDIALOG_H
#define ADDEMPLOYEEDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>

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
};

class AddEmployeeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddEmployeeDialog(QWidget *parent = nullptr);
    ~AddEmployeeDialog();

    void setEmployeeInfo(const EmployeeInfo &info);
    EmployeeInfo employeeInfo() const;

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
    
    QLabel *titleLabel;
    QPushButton *saveBtn;
    
    // 头像相关
    QLabel *avatarPreview;
    QString m_selectedImgPath;
    
    QPoint dragPosition;
    QString m_id;
};

#endif // ADDEMPLOYEEDIALOG_H
