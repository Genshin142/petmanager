#ifndef FOSTERMODULE_H
#define FOSTERMODULE_H
#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>

class FosterModule : public QWidget {
    Q_OBJECT
public:
    explicit FosterModule(QWidget *parent = nullptr);
private:
    void initRoomView();
};
#endif
