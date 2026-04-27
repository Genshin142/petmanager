#ifndef FOSTERGANTTDELEGATE_H
#define FOSTERGANTTDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QPainterPath>
#include "fosterganttmodel.h"

class FosterGanttDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit FosterGanttDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QString status = index.data(FosterGanttModel::StatusRole).toString();
        if (status.isEmpty()) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        bool isStart = index.data(FosterGanttModel::IsStartRole).toBool();
        bool isEnd = index.data(FosterGanttModel::IsEndRole).toBool();
        QColor bgColor = index.data(Qt::BackgroundRole).value<QColor>();
        QColor textColor = index.data(Qt::ForegroundRole).value<QColor>();

        QRect rect = option.rect;
        // 稍微收缩一点，留出网格间隙
        rect.adjust(0, 4, 0, -4);

        QPainterPath path;
        int radius = 6;
        
        // 根据是否是开头/结尾决定圆角
        if (isStart && isEnd) {
            path.addRoundedRect(rect.adjusted(4, 0, -4, 0), radius, radius);
        } else if (isStart) {
            path.addRoundedRect(rect.adjusted(4, 0, 10, 0), radius, radius); // 向右延伸
        } else if (isEnd) {
            path.addRoundedRect(rect.adjusted(-10, 0, -4, 0), radius, radius); // 向左延伸
        } else {
            path.addRect(rect.adjusted(-1, 0, 1, 0)); // 中间部分完全填充
        }

        if (status == "maintenance") {
            painter->fillPath(path, bgColor);
            painter->save();
            painter->setClipPath(path);
            QPen hatchPen(textColor, 1);
            QColor c = textColor;
            c.setAlpha(80);
            hatchPen.setColor(c);
            painter->setPen(hatchPen);
            for (int x = rect.left() - rect.height(); x < rect.right(); x += 8) {
                painter->drawLine(x, rect.bottom(), x + rect.height(), rect.top());
            }
            painter->restore();
        } else {
            painter->fillPath(path, bgColor);
        }
        
        // 绘制文字 (仅在开头或中间绘制)
        if (isStart || index.column() % 3 == 0) {
            painter->setPen(textColor);
            QFont font = painter->font();
            font.setBold(true);
            font.setPointSize(9);
            painter->setFont(font);
            QString name = index.data(Qt::DisplayRole).toString();
            painter->drawText(rect.adjusted(8, 0, -8, 0), Qt::AlignVCenter | Qt::AlignLeft, name);
        }

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        Q_UNUSED(option); Q_UNUSED(index);
        return QSize(60, 44);
    }
};

#endif // FOSTERGANTTDELEGATE_H
