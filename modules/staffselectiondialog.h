#ifndef STAFFSELECTIONDIALOG_H
#define STAFFSELECTIONDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include "addemployeedialog.h"

class StaffSelectionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit StaffSelectionDialog(QWidget *parent = nullptr);
    QString selectedStaff() const;

private slots:
    void onSearchChanged(const QString &text);
    void onItemDoubleClicked(QListWidgetItem *item);
    void onConfirm();

private:
    void setupUI();
    void renderStaffList(const QString &filter = "");
    QPixmap createCircularAvatar(const QPixmap &src, int size);

    QLineEdit *m_searchEdit;
    QListWidget *m_listWidget;
    QString m_selectedName;
};

#endif // STAFFSELECTIONDIALOG_H
