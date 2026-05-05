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
    void onSearchItems(const QString &text);
    void onTileClicked(const QString &id, bool isService);
    void onRemoveCartItem(int row);
    void onCreateOrder();
    void onMemberChanged(int index);

private:
    void setupUI();
    void initMemberData();
    void updateCartUI();
    void updateTilePanel(const QString &category, const QString &searchKw = "");

    struct CartItem {
        QString id;
        QString name;
        double price;
        int qty;
        bool isService;
    };

    // UI 组件
    class QButtonGroup *m_categoryGroup;
    QWidget *m_tileContainer;
    class QGridLayout *m_tileLayout;
    class QTableWidget *m_cartTable;
    QLineEdit *m_memberSearch;
    class QComboBox *m_memberCombo;
    class QComboBox *m_petCombo;
    QLabel *m_totalLabel;
    
    QList<CartItem> m_cart;
    QString m_currentCategory;
};

#endif // QUICKORDERDIALOG_H
