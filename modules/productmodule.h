#ifndef PRODUCTMODULE_H
#define PRODUCTMODULE_H
#include <QWidget>

class ProductModule : public QWidget {
    Q_OBJECT
public:
    explicit ProductModule(QWidget *parent = nullptr);
};
#endif
