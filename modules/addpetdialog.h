#ifndef ADDPETDIALOG_H
#define ADDPETDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QMap>
#include "../common_types.h"

namespace Ui {
class AddPetDialog;
}

class AddPetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddPetDialog(QWidget *parent = nullptr);
    ~AddPetDialog();

    void setPetInfo(const PetInfo &info);
    PetInfo getPetInfo() const;

private slots:
    void onSaveClicked();
    void onSpeciesChanged(const QString &species);
    void onSelectImageClicked();
    void showBigImage();
    void hideBigImage();

private:
    void setupUI();
    void initBreedData();
    Ui::AddPetDialog *ui;
    QMap<QString, QStringList> m_breedData;
    
    QComboBox *genderCombo;
    QComboBox *healthCombo;
    QLineEdit *ageYearEdit;
    QComboBox *ageMonthCombo;
    QComboBox *statusCombo;
    QLineEdit *joinTimeEdit;
    QLineEdit *ownerIdEdit;
    QLineEdit *ownerNameEdit;
    class QTextEdit *historyTextEdit;
    class QTextEdit *dietaryTextEdit;

    QLabel *avatarLabel;
    QPushButton *selectImageBtn;
    QString m_avatarPath;
    QString m_currentId;

    // 大图预览交互
    QWidget *m_imagePreviewOverlay;
    QLabel *m_previewLabel;

    void setupBigImageOverlay();
    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // ADDPETDIALOG_H
