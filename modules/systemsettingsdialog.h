#ifndef SYSTEMSETTINGSDIALOG_H
#define SYSTEMSETTINGSDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QStackedWidget>
#include <QPushButton>

class SystemSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SystemSettingsDialog(QWidget *parent = nullptr);

private slots:
    void onSaveClicked();

private:
    void setupUI();
    void applyStyles();
    
    QWidget* createStoreInfoPage();
    QWidget* createPrintConfigPage();
    QWidget* createPointsRulePage();

    QListWidget *m_sidebar;
    QStackedWidget *m_stackedWidget;
    QPushButton *m_saveBtn;
    QPushButton *m_cancelBtn;
};

#endif // SYSTEMSETTINGSDIALOG_H
