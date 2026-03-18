#ifndef ADDMEMBERDIALOG_H
#define ADDMEMBERDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
class AddMemberDialog;
}

struct MemberInfo {
    QString id;
    QString name;
    QString phone;
    QString level;
    double balance;      // 储值余额
    double consume_amt;  // 累计消费金额
    int points;         // 当前可用积分
};

class AddMemberDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddMemberDialog(QWidget *parent = nullptr);
    ~AddMemberDialog();

    MemberInfo getMemberInfo() const;
    void setInitialData(const MemberInfo &info);
    
    void accept() override; // 重写 accept 实现拦截校验

private:
    Ui::AddMemberDialog *ui;
};

#endif // ADDMEMBERDIALOG_H
