#ifndef PETMODULE_H
#define PETMODULE_H

#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>

class PetModule : public QWidget
{
    Q_OBJECT
public:
    explicit PetModule(QWidget *parent = nullptr);

private:
    void setupUI();
    void addPetRow(const QString &id, const QString &name, const QString &breed, 
                   const QString &gender, const QString &age, const QString &health, 
                   const QString &vaccine, const QString &status, const QString &ownerId, const QString &owner);
    void updateStats();

private slots:
    void onSearch(const QString &keyword);

private:
    QTableWidget *petTable;
    QLineEdit *searchEdit;

    // 统计卡片价值标签
    QLabel *totalPetsLabel;
    QLabel *boardingPetsLabel;
    QLabel *groomingPetsLabel;
};

#endif // PETMODULE_H
