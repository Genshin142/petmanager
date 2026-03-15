#ifndef ROLEMODULE_H
#define ROLEMODULE_H

#include <QWidget>

class RoleModule : public QWidget
{
    Q_OBJECT
public:
    explicit RoleModule(QWidget *parent = nullptr);
private:
    void setupUI();
};

#endif // ROLEMODULE_H
