#ifndef SCHEDULETEMPLATEDIALOG_H
#define SCHEDULETEMPLATEDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QLabel>
#include "common_types.h"

class ScheduleTemplateDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ScheduleTemplateDialog(QWidget *parent = nullptr);

private slots:
    void onCellClicked(int row, int col);
    void onSaveAndApply();

private:
    void setupUI();
    void loadTemplate();
    QWidget* createShiftWidget(const ScheduleInfo &info);
    QPixmap createCircularAvatar(const QPixmap &src, int size);

    QTableWidget *m_table;
    QList<EmployeeInfo> m_allStaff;
    // 内存中维护的当前编辑中的模板数据: Map<staffId, QList<ScheduleInfo>(size 7)>
    QMap<QString, QList<ScheduleInfo>> m_editingTemplate;
};

#endif // SCHEDULETEMPLATEDIALOG_H
