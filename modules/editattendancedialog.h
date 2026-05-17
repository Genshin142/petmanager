#ifndef EDITATTENDANCEDIALOG_H
#define EDITATTENDANCEDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include "common_types.h"

class EditAttendanceDialog : public QDialog {
    Q_OBJECT
public:
    explicit EditAttendanceDialog(const ScheduleInfo &info, QWidget *parent = nullptr);
    ScheduleInfo getUpdatedInfo() const;
private:
    QComboBox *m_typeCombo;
    QLineEdit *m_planStartEdit;
    QLineEdit *m_planEndEdit;
    QLineEdit *m_clockInEdit;
    QLineEdit *m_clockOutEdit;
    QLineEdit *m_noteEdit;
    QWidget *m_timeContainer;
    QWidget *m_clockContainer;
    
    ScheduleInfo m_info;
};

#endif // EDITATTENDANCEDIALOG_H
