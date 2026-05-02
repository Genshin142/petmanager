#ifndef ORDERDETAILDRAWER_H
#define ORDERDETAILDRAWER_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QStackedWidget>
#include "../common_types.h"

class QLineEdit;

class OrderDetailDrawer : public QWidget
{
    Q_OBJECT
public:
    explicit OrderDetailDrawer(QWidget *parent = nullptr);

    void setOrder(const OrderInfo &order);
    void showEmptyState();

signals:
    void settlementConfirmed(const OrderInfo &order);
    void orderCancelled(const QString &orderId, const QString &reason);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUI();
    void updateUI();

    OrderInfo m_order;

    // UI Components
    QLabel *m_titleLabel;
    QLabel *m_petAvatar;
    QLabel *m_petInfoLabel;
    QLabel *m_memberNameLabel;
    
    QWidget *m_itemsContainer;
    QVBoxLayout *m_itemsLayout;
    
    QLabel *m_totalLabel;
    QLabel *m_finalAmountLabel;
    QLineEdit *m_discountEdit;
    
    QPushButton *m_confirmBtn;
    QPushButton *m_cancelOrderBtn;
    
    QList<QPushButton*> m_payMethodButtons;
    
    QStackedWidget *m_stack;
    QWidget *m_detailWidget;
    QWidget *m_emptyWidget;
    QString m_avatarPathForPreview;
};

#endif // ORDERDETAILDRAWER_H
