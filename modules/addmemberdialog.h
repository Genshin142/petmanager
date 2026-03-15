#ifndef ADDMEMBERDIALOG_H
#define ADDMEMBERDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
class AddMemberDialog;
}

struct MemberInfo {
    QString name;
    QString phone;
    QString level;
    int points;
};

class AddMemberDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddMemberDialog(QWidget *parent = nullptr);
    ~AddMemberDialog();

    MemberInfo getMemberInfo() const;
    void setInitialData(const MemberInfo &info); // 新增：设置初始数据进入编辑模式

private slots:
    void on_buttonBox_accepted();

private:
    Ui::AddMemberDialog *ui;
};

#endif // ADDMEMBERDIALOG_H
