#ifndef EMPLOYEEDETAILDRAWER_H
#define EMPLOYEEDETAILDRAWER_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QPropertyAnimation>
#include <QStackedWidget>
#include <QButtonGroup>
#include "addemployeedialog.h"

class EmployeeDetailDrawer : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int sideWidth READ width WRITE setFixedWidth)

public:
    explicit EmployeeDetailDrawer(QWidget *parent = nullptr);
    void setEmployee(const EmployeeInfo &info);
    
    void showDrawer();
    void hideDrawer();
    bool isOpened() const { return m_isOpened; }

signals:
    void avatarClicked(const QString &path);
    void closeRequested();

private:
    void setupUI();
    bool eventFilter(QObject *obj, QEvent *event) override;

    EmployeeInfo m_currentEmployee;
    bool m_isOpened;

    // UI Elements
    QLabel *m_avatarLabel;
    QLabel *m_nameLabel;
    QLabel *m_roleLabel;
    QLabel *m_idLabel;

    // Details Section
    QLabel *m_genderAgeVal;
    QLabel *m_phoneVal;
    QLabel *m_emailVal;
    QLabel *m_idCardVal;
    QLabel *m_salaryVal;
    QLabel *m_statusVal;
    
    // 新增字段
    QLabel *m_joinDateVal;
    QLabel *m_emergencyVal;
    QLabel *m_deptVal;
    QLabel *m_eduVal;
    QLabel *m_addressVal;
    
    // Tab 导航与页面结构
    QButtonGroup *m_tabGroup;
    QStackedWidget *m_stackedWidget;
    
    QWidget* createProfilePage();
    QWidget* createSchedulePage();

    QPropertyAnimation *m_animation;
};

#endif // EMPLOYEEDETAILDRAWER_H
