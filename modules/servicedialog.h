#ifndef SERVICEDIALOG_H
#define SERVICEDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include "servicedatamanager.h"
#include "../common_types.h"

class ServiceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ServiceDialog(QWidget *parent = nullptr);
    ServiceInfo getServiceInfo() const;
    void setServiceInfo(const ServiceInfo &info);

private:
    void setupUi();
    QWidget* createInputWithUnit(QWidget* input, const QString &unit);

    QLineEdit *m_nameEdit;
    QComboBox *m_categoryCombo;
    QLineEdit *m_durationEdit;
    QLineEdit *m_priceEdit;
    QLineEdit *m_idEdit;
    QLineEdit *m_commAmountEdit;
    
    QLabel *m_priceTitleLabel;
    QLabel *m_durationTitleLabel;
    QLabel *m_durationUnitLabel;
    QLabel *m_commTitleLabel;
    QWidget *m_durationGroup;
    
    QString m_currentId;
};

#endif // SERVICEDIALOG_H
