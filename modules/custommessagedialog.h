#ifndef CUSTOMMESSAGEDIALOG_H
#define CUSTOMMESSAGEDIALOG_H

#include <QDialog>
#include <QString>

class QLabel;
class QPushButton;

class CustomMessageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomMessageDialog(const QString &title, const QString &content, bool isConfirm = false, QWidget *parent = nullptr);
    ~CustomMessageDialog();

    static void showWarning(QWidget *parent, const QString &title, const QString &content);
    static bool confirm(QWidget *parent, const QString &title, const QString &content);

private:
    void setupUI(const QString &title, const QString &content, bool isConfirm);
};

#endif // CUSTOMMESSAGEDIALOG_H
