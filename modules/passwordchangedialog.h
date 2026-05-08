#ifndef PASSWORDCHANGEDIALOG_H
#define PASSWORDCHANGEDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>

class PasswordChangeDialog : public QDialog {
    Q_OBJECT
public:
    explicit PasswordChangeDialog(QWidget *parent = nullptr);

signals:
    void passwordUpdated(const QString &newPassword);

private slots:
    void validateForm();
    void updateStrength(const QString &text);

private:
    void setupUI();
    void applyStyles();

    QLineEdit *m_currentPwdEdit;
    QLineEdit *m_newPwdEdit;
    QLineEdit *m_confirmPwdEdit;
    QProgressBar *m_strengthBar;
    QLabel *m_strengthLabel;
    QLabel *m_errorLabel;
    QPushButton *m_saveBtn;
    QPushButton *m_cancelBtn;
};

#endif // PASSWORDCHANGEDIALOG_H
