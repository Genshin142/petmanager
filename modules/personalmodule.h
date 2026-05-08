#ifndef PERSONALMODULE_H
#define PERSONALMODULE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include "../common_types.h"

class PersonalModule : public QWidget {
    Q_OBJECT
public:
    explicit PersonalModule(UserRole role, const QString &userName, QWidget *parent = nullptr);

private:
    void setupUI();
    QWidget* createAdminView();
    QWidget* createStaffView();
    QWidget* createCard(const QString &title, const QString &value, const QString &subValue, const QString &color);
    QWidget* createActionRow(const QString &label, const QString &desc, const QString &btnText);

    UserRole m_role;
    QString m_userName;

private slots:
    void onActionClicked(const QString &actionName);
};

#endif // PERSONALMODULE_H
