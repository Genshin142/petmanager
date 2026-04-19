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
    enum DialogType { Warning, Success, Confirm };
    explicit CustomMessageDialog(const QString &title, const QString &content, DialogType type = Warning, QWidget *parent = nullptr);
    ~CustomMessageDialog();

    static void showWarning(QWidget *parent, const QString &title, const QString &content);
    static void showSuccess(QWidget *parent, const QString &title, const QString &content);
    static bool confirm(QWidget *parent, const QString &title, const QString &content);

private:
    void setupUI(const QString &title, const QString &content, DialogType type);
};

#endif // CUSTOMMESSAGEDIALOG_H
