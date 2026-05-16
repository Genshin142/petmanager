#ifndef QUICKORDERDIALOG_H
#define QUICKORDERDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include "../common_types.h"
#include "custom_calendar_edit.h"

#include "boardingdatamanager.h"
#include <QFrame>
#include <QVBoxLayout>
#include <QShowEvent>

// --- 自定义商品/服务磁贴组件 ---
class ItemTile : public QFrame {
    Q_OBJECT
public:
    ItemTile(const QString &id, const QString &name, double price, const QString &icon, bool isService, const QString &category, QWidget *parent = nullptr);

    void setQuantity(int qty);
    QString id() const { return m_id; }
    void clickedSignal(std::function<void(QString, bool)> callback) { m_callback = callback; }

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void updateStyle();

private:
    QString m_id;
    QString m_category;
    bool m_isService;
    bool m_isSelected;
    int m_qty;
    QLabel *m_qtyBadge;
    std::function<void(QString, bool)> m_callback;
};

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
        QString category; // 新增分类存储
    };

    // UI 组件
    class QButtonGroup *m_categoryGroup;
    QWidget *m_tileContainer;
    class QGridLayout *m_tileLayout;
    class QTableWidget *m_cartTable;
    QLineEdit *m_searchEdit;
    QLineEdit *m_memberSearch;
    class QComboBox *m_memberCombo;
    class QComboBox *m_petCombo;
    QLabel *m_totalLabel;
    
    QWidget *m_serviceDetailContainer;
    QVBoxLayout *m_serviceDetailLayout;
    // 动态控件指针
    QLineEdit *m_addrEdit1;
    QLineEdit *m_addrEdit2;
    CustomCalendarEdit *m_dateEdit1;
    CustomCalendarEdit *m_dateEdit2;
    QComboBox *m_timeSlotCombo1;
    QComboBox *m_timeSlotCombo2;
    
    // 寄养详情控件
    CustomCalendarEdit *m_checkInEdit;
    CustomCalendarEdit *m_checkOutEdit;
    QComboBox *m_roomCombo;
    QLabel *m_boardingStockLabel;
    QLabel *m_boardingDaysLabel;
    
    QList<CartItem> m_cart;
    QString m_currentCategory;

protected:
    void showEvent(QShowEvent *event) override;
};

#endif // QUICKORDERDIALOG_H
