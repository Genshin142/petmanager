#ifndef FOSTERMODULE_H
#define FOSTERMODULE_H

#include <QWidget>
#include <QLabel>
#include <QGridLayout>

class FosterModule : public QWidget {
    Q_OBJECT
public:
    explicit FosterModule(QWidget *parent = nullptr);

private:
    void setupUI();
    void updateStats();
    QWidget* createRoomCard(int roomNo, const QString &status, 
                            const QString &petId = "", const QString &petName = "");

    QGridLayout *roomGrid;
    QLabel *totalRoomsLabel;
    QLabel *occupiedLabel;
    QLabel *freeLabel;
    QLabel *cleaningLabel;
};

#endif
