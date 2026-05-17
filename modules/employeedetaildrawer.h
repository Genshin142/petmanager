#ifndef EMPLOYEEDETAILDRAWER_H
#define EMPLOYEEDETAILDRAWER_H

#include <QWidget>
#include <QDate>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QPropertyAnimation>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QComboBox>
#include <QGridLayout>
#include "common_types.h"
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
    void editRequested(const EmployeeInfo &info);

private:
    void setupUI();
    bool eventFilter(QObject *obj, QEvent *event) override;

    EmployeeInfo m_currentEmployee;
    bool m_isOpened;
    QWidget *m_emptyWidget = nullptr;
    QWidget *m_contentWidget = nullptr;
    void showEmptyState(bool empty);

    // UI Elements
    QLabel *m_avatarLabel;
    QLabel *m_nameLabel;
    QPushButton *m_editBtn;
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
    QWidget* createAttendancePage();
    void refreshCalendar();
    void refreshAttendCalendar();

    // 动态日历组件 (排班)
    QComboBox *m_yearCombo = nullptr;
    QComboBox *m_monthCombo = nullptr;
    QGridLayout *m_calendarGrid = nullptr;
    QLabel *m_todayShiftLabel = nullptr;
    QLabel *m_statWorkLabel = nullptr;
    QLabel *m_statLeaveLabel = nullptr;
    QLabel *m_statRestLabel = nullptr;

    // 动态日历组件 (考勤)
    QComboBox *m_attendYearCombo = nullptr;
    QComboBox *m_attendMonthCombo = nullptr;
    QGridLayout *m_attendCalendarGrid = nullptr;
    QLabel *m_attendTodayShiftLabel = nullptr;
    QLabel *m_statNormalLabel = nullptr;
    QLabel *m_statLateLabel = nullptr;
    QLabel *m_statAbsentLabel = nullptr;
    QDate m_selectedAttendDate;

    QPropertyAnimation *m_animation;
};

#endif // EMPLOYEEDETAILDRAWER_H
