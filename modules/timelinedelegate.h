#ifndef TIMELINEDELEGATE_H
#define TIMELINEDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QPainterPath>
#include "pettimelinemodel.h"

class TimelineDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit TimelineDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        bool isHeader = index.data(PetTimelineModel::IsHeaderRole).toBool();
        bool isHighlight = index.data(PetTimelineModel::HighlightRole).toBool();
        
        if (isHighlight) {
            painter->fillRect(option.rect, QColor(240, 247, 255));
        }

        if (isHeader) {
            drawHeader(painter, option, index);
        } else {
            drawItem(painter, option, index);
        }

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        bool isHeader = index.data(PetTimelineModel::IsHeaderRole).toBool();
        if (isHeader) {
            return QSize(option.rect.width(), 40);
        } else {
            QString content = index.data(PetTimelineModel::ContentRole).toString();
            QString typeStr = index.data(PetTimelineModel::TypeRole).toString();
            
            // 重新拉宽气泡以适配整个宽度
            int textWidth = option.rect.width() - 119; 
            if (textWidth < 50) textWidth = 50;

            QTextDocument doc;
            doc.setDefaultFont(QFont("Microsoft YaHei", 10));
            doc.setTextWidth(textWidth);
            doc.setHtml(QString("<div style='line-height: 1.2;'><b>%1:</b> %2</div>").arg(typeStr, content));
            // 返回高度: 文字高度 + 经办人高度(20) + 气泡上下Padding(10*2) + 外部Margin(10*2)
            int calculatedHeight = (int)doc.size().height() + 60;
            return QSize(option.rect.width(), qMax(85, calculatedHeight));
        }
    }

private:
    void drawHeader(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
        QString dateStr = index.data(PetTimelineModel::TimeRole).toString();
        
        // 轴线左移 (从 30 移至 15)
        painter->setPen(QPen(QColor("#f0f2f5"), 2));
        painter->drawLine(option.rect.left() + 15, option.rect.top(), option.rect.left() + 15, option.rect.bottom());

        // Header Background 左移 (从 50 移至 35)
        QRect headerRect(option.rect.left() + 35, option.rect.top() + 8, 100, 24);
        painter->setBrush(QColor("#f5f7fa"));
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(headerRect, 12, 12);

        // Header Text
        painter->setPen(QColor("#909399"));
        painter->setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
        painter->drawText(headerRect, Qt::AlignCenter, dateStr);
    }

    void drawItem(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
        QString fullTime = index.data(PetTimelineModel::TimeRole).toString();
        QString timeStr = fullTime.contains(' ') ? fullTime.split(' ').last() : fullTime;
        if (timeStr.length() > 5) timeStr = timeStr.right(5); // 确保只显示 HH:mm
        QString typeStr = index.data(PetTimelineModel::TypeRole).toString();
        QString contentStr = index.data(PetTimelineModel::ContentRole).toString();
        QString iconStr = index.data(PetTimelineModel::IconRole).toString();
        bool isAlert = index.data(PetTimelineModel::IsAlertRole).toBool();

        // 1. 轴线
        painter->setPen(QPen(QColor("#f0f2f5"), 2));
        painter->drawLine(option.rect.left() + 15, option.rect.top(), option.rect.left() + 15, option.rect.bottom());

        // 2. 轴点 (改为小圆点，去除大图标)
        QRect dotRect(option.rect.left() + 11, option.rect.top() + 20, 8, 8);
        QColor themeColor = "#409eff";
        if (typeStr == "投喂") themeColor = "#67c23a";
        else if (typeStr == "洗护") themeColor = "#e6a23c";
        else if (isAlert) themeColor = "#f56c6c";

        painter->setBrush(themeColor);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(dotRect);

        // 3. 时间
        painter->setPen(QColor("#C0C4CC"));
        painter->setFont(QFont("Arial", 9));
        painter->drawText(option.rect.left() + 30, option.rect.top() + 28, timeStr);

        // 4. 气泡
        QRect bubbleRect(option.rect.left() + 75, option.rect.top() + 10, option.rect.width() - 95, option.rect.height() - 20);
        painter->setBrush(QColor(248, 249, 251));
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(bubbleRect, 8, 8);

        // Bubble Triangle
        QPainterPath path;
        path.moveTo(bubbleRect.left(), bubbleRect.top() + 15);
        path.lineTo(bubbleRect.left() - 6, bubbleRect.top() + 20);
        path.lineTo(bubbleRect.left(), bubbleRect.top() + 25);
        painter->setPen(Qt::NoPen);
        painter->drawPath(path);
        painter->fillPath(path, QColor(248, 249, 251));

        // Text Content (将图标移动到文字内部)
        painter->setPen(QColor("#606266"));
        painter->setFont(QFont("Microsoft YaHei", 10));
        QRect textRect = bubbleRect.adjusted(12, 10, -12, -10);
        
        QTextDocument doc;
        doc.setHtml(QString("<div style='line-height: 1.4;'>%1 <b>%2:</b> %3</div>").arg(iconStr, typeStr, contentStr));
        doc.setTextWidth(textRect.width());
        
        painter->save();
        painter->translate(textRect.left(), textRect.top());
        doc.drawContents(painter);
        painter->restore();

        // 5. Footer Info
        QString opStr = index.data(PetTimelineModel::OperatorRole).toString();
        QString roomStr = index.data(PetTimelineModel::RoomNoRole).toString();
        if (opStr.isEmpty()) opStr = "系统生成";
        
        QString footerText = QString("经办人: %1").arg(opStr);
        if (!roomStr.isEmpty()) {
            footerText = QString("房间: %1 | 经办人: %2").arg(roomStr, opStr);
        }

        painter->setPen(QColor("#909399"));
        painter->setFont(QFont("Microsoft YaHei", 9));
        painter->drawText(bubbleRect.adjusted(12, 0, -12, -8), Qt::AlignBottom | Qt::AlignRight, footerText);
    }

    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override {
        if (event->type() == QEvent::MouseButtonRelease) {
            // 已移除修改/删除按钮交互
        }
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }

signals:
    void editRequested(const QModelIndex &index);
    void deleteRequested(const QModelIndex &index);
};

#endif // TIMELINEDELEGATE_H
