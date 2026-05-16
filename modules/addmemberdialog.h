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
    QString gender;      // 性别 (男/女)
    QString phone;
    QString level;
    QString birthday;    // 生日 (yyyy-MM-dd)
    double balance;      // 储值余额
    double consume_amt;  // 累计消费金额
    int points;         // 当前可用积分
    bool isActive = true; // 是否有效（逻辑删除标志）
    bool isDeleted = false; // 是否已注销
    QString status = "正常"; // 状态：正常、已注销、锁定等
    QString pets = "无";    // 关联的宠物信息字符串
    QString imgData;        // 新增：头像 Base64 数据
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
    MemberInfo m_info;
    class CustomCalendarEdit *m_birthdayEdit;
};

Q_DECLARE_METATYPE(MemberInfo)

#endif // ADDMEMBERDIALOG_H
