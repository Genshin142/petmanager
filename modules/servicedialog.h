#ifndef SERVICEDIALOG_H
#define SERVICEDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QFrame>
#include <QVBoxLayout>
#include <QGridLayout>
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
    QWidget* createFormItem(const QString &label, QWidget *widget);
    QWidget* createGroupTitle(const QString &title);

    QFrame *m_bgFrame;
    QLabel *m_titleLabel;
    QLineEdit *m_nameEdit;
    QComboBox *m_categoryCombo;
    QLineEdit *m_idEdit;
    QLineEdit *m_priceEdit;
    QLineEdit *m_durationEdit;
    QLineEdit *m_commAmountEdit;
    QTextEdit *m_descriptionEdit;
    
    QString m_currentId;
    ServiceInfo m_originalServiceInfo;
};


#endif // SERVICEDIALOG_H
