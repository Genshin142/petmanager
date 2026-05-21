#ifndef PERSONALMODULE_H
#define PERSONALMODULE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include "../common_types.h"

#include <QDialog>
#include <QEvent>
#include <QPixmap>

class TodoDetailsDialog : public QDialog {
    Q_OBJECT
public:
    explicit TodoDetailsDialog(QWidget *parent = nullptr);
signals:
    void requestJumpTo(int index);
private:
    QWidget* createStockTab();
    QWidget* createApptTab();
    QWidget* createLogisticsTab();
};

class PersonalModule : public QWidget {
    Q_OBJECT
public:
    explicit PersonalModule(UserRole role, const QString &userName, QWidget *parent = nullptr);

signals:
    void requestJumpToModule(int index);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupUI();
    QWidget* createAdminView();
    QWidget* createStaffView();
    QWidget* createCard(const QString &title, const QString &value, const QString &subValue, const QString &color);
    QWidget* createActionRow(const QString &label, const QString &desc, const QString &btnText);
    void showEnlargedAvatar();
    void showTodoDetailsDialog();

    UserRole m_role;
    QString m_userName;
    QLabel *m_avatarLabel = nullptr;
    QPixmap m_originalPixmap;
    QDialog *m_enlargedDialog = nullptr;

    QFrame *m_todoCard = nullptr;
    QWidget *m_loginCard = nullptr;

public slots:
    void updateTodoCard();

private slots:
    void onActionClicked(const QString &actionName);
};

#endif // PERSONALMODULE_H
