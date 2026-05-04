#ifndef QUICKORDERDIALOG_H
#define QUICKORDERDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include "../common_types.h"

#include "boardingdatamanager.h"

class QuickOrderDialog : public QDialog
{
    Q_OBJECT
public:
    explicit QuickOrderDialog(QWidget *parent = nullptr);

signals:
    void orderCreated(const QString &orderId);

private slots:
    void onCategoryChanged(int id);
    void onTileClicked(); // 点击磁贴时触发
    void onSearchItems(const QString &text);
    void onRemoveCartItem(int row);
    void onCreateOrder();
    void onMemberSearch(const QString &text);
    void onMemberChanged(int index);

private:
    void setupUI();
    void updateCartUI();
    void updateTilePanel(const QString &category = "全部", const QString &searchKw = "");

    QLineEdit *m_itemSearch;
    QListWidget *m_cartList;
    
    QLabel *m_totalLabel;
    
    // UI 组件：分类与磁贴
    class QButtonGroup *m_categoryGroup;
    QWidget *m_tileContainer;
    class QGridLayout *m_tileLayout;

    // 新增：底部会员与宠物选择
    class QComboBox *m_memberCombo;
    class QComboBox *m_petCombo;
    QWidget *m_petWrapper;

    // 当前选中的会员
    QString m_selectedMemberId;
    QString m_selectedMemberName;
    
    // 购物车数据结构
    struct CartItem {
        QString id;
        QString name;
        double price;
        int qty;
        bool isService;
        QString startDate;
        QString endDate;
        QString roomId;
    };
    QList<CartItem> m_cart;
};

#endif // QUICKORDERDIALOG_H
