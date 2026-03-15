#ifndef SELECTPETDIALOG_H
#define SELECTPETDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QListWidget>

class SelectPetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SelectPetDialog(const QStringList &allPets, const QStringList &selectedPets, QWidget *parent = nullptr);
    QStringList getSelectedPets() const;

private:
    void setupUI(const QStringList &allPets, const QStringList &selectedPets);
    QListWidget *petListWidget;
};

#endif // SELECTPETDIALOG_H
