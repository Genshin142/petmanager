#include "fostermodule.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>

FosterModule::FosterModule(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);

    QLabel *title = new QLabel("可视化寄养房态图", this);
    title->setStyleSheet("font-size: 20px; font-weight: bold;");
    layout->addWidget(title);

    QLabel *subTitle = new QLabel("蓝色：已占用 | 绿色：空闲 | 灰色：清洁中", this);
    subTitle->setStyleSheet("color: #606266; margin-bottom: 10px;");
    layout->addWidget(subTitle);

    initRoomView();
}

void FosterModule::initRoomView() {
    QGraphicsView *view = new QGraphicsView(this);
    QGraphicsScene *scene = new QGraphicsScene(this);
    view->setScene(scene);
    view->setStyleSheet("background-color: #fcfcfc; border-radius: 4px;");

    // 绘制房间格
    int rows = 3, cols = 5;
    int w = 120, h = 80, gap = 20;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int x = c * (w + gap);
            int y = r * (h + gap);
            
            QGraphicsRectItem *rect = scene->addRect(x, y, w, h, QPen(Qt::gray));
            
            QString statusText;
            if ((r + c) % 3 == 0) {
                rect->setBrush(QBrush(QColor(64, 158, 255))); // 占用
                statusText = "已占用\n(二哈)";
            } else if ((r + c) % 5 == 0) {
                rect->setBrush(QBrush(QColor(144, 147, 153))); // 清洁
                statusText = "清洁中";
            } else {
                rect->setBrush(QBrush(QColor(103, 194, 58))); // 空闲
                statusText = "空闲";
            }

            QGraphicsTextItem *text = scene->addText(QString("Room %1\n%2").arg(r*cols + c + 101).arg(statusText));
            text->setPos(x + 5, y + 20);
            text->setDefaultTextColor(Qt::white);
        }
    }

    QVBoxLayout *layout = static_cast<QVBoxLayout*>(this->layout());
    layout->addWidget(view);
}
