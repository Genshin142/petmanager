#ifndef ADDPETDIALOG_H
#define ADDPETDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
class AddPetDialog;
}

struct PetInfo {
    QString name;
    QString breed;
    QString history;
    QString vaccine;
    QString id; // 自动生成或手动输入
};

class AddPetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddPetDialog(QWidget *parent = nullptr);
    ~AddPetDialog();

    PetInfo getPetInfo() const;

private slots:
    void onSaveClicked();
    void onSpeciesChanged(const QString &species);

private:
    void setupUI();
    void initBreedData();
    Ui::AddPetDialog *ui;
    QMap<QString, QStringList> m_breedData;
};

#endif // ADDPETDIALOG_H
