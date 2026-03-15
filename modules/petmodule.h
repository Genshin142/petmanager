#ifndef PETMODULE_H
#define PETMODULE_H

#include <QWidget>

class PetModule : public QWidget
{
    Q_OBJECT
public:
    explicit PetModule(QWidget *parent = nullptr);
};

#endif // PETMODULE_H
