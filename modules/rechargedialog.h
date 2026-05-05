#ifndef RECHARGEDIALOG_H
#define RECHARGEDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QFrame>
#include <QButtonGroup>
#include <QMouseEvent>
#include "addmemberdialog.h" // For MemberInfo

class RechargeDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RechargeDialog(const MemberInfo &info, QWidget *parent = nullptr);

    double getRechargeAmount() const { return m_amount; }
    QString getPayMethod() const { return m_payMethod; }

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void onConfirm();

private:
    void setupUI(const MemberInfo &info);

    double m_amount = 0.0;
    QString m_payMethod = "WeChat";
    
    QLineEdit *m_customInput;
    QButtonGroup *m_presetGroup;
    QButtonGroup *m_payMethodGroup;
    
    QPoint m_dragPosition;
};

#endif // RECHARGEDIALOG_H
