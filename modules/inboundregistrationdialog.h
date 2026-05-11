#ifndef INBOUNDREGISTRATIONDIALOG_H
#define INBOUNDREGISTRATIONDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QFrame>
#include <QScrollArea>
#include <QDateTime>
#include "../common_types.h"

class InboundRegistrationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit InboundRegistrationDialog(QWidget *parent = nullptr);
    void setEditMode(const StockInRecord &rec, const ProductInfo &pInfo);

signals:
    void recordAdded();
    void recordUpdated();

private slots:
    void onBarcodeEntered();
    void onConfirmInbound();
    void onAddImage();
    void switchImage(bool next);

private:
    void setupUI();
    void updatePreviewCard(const QString &barcode);
    void updateDots();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QLineEdit *m_barcodeEdit;
    
    // Preview Card Components
    QFrame *m_previewCard;
    QLabel *m_previewImg;
    QWidget *m_dotsContainer;
    QHBoxLayout *m_dotsLayout;
    QStringList m_imagePaths;
    int m_currentImgIndex = 0;

    QLabel *m_previewName;
    QLabel *m_previewSpec;
    QLabel *m_previewStock;

    // Inbound Form
    QLineEdit *m_nameEdit;
    QComboBox *m_categoryCombo;
    QLineEdit *m_specEdit;
    QLineEdit *m_originEdit;
    QLineEdit *m_qtyEdit;
    QLineEdit *m_costEdit;
    QLineEdit *m_supplierEdit;
    QLineEdit *m_supplierPhoneEdit;
    QLineEdit *m_priceEdit; // 零售价
    QComboBox *m_operatorCombo;
    class CustomCalendarEdit *m_dateEdit;
    QLineEdit *m_shelfLifeEdit;
    QPushButton *m_confirmBtn;
    bool m_isEditMode = false;
    StockInRecord m_currentRecord;
};

#endif // INBOUNDREGISTRATIONDIALOG_H
