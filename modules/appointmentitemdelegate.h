#ifndef APPOINTMENTITEMDELEGATE_H
#define APPOINTMENTITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QPainterPath>
#include <QCache>
#include "../common_types.h"

class AppointmentItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit AppointmentItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    void drawCard(QPainter *painter, const QStyleOptionViewItem &option, const AppointmentInfo &info) const;
    void drawAvatar(QPainter *painter, const QRect &rect, const QString &path) const;
    void drawStatusTag(QPainter *painter, const QRect &rect, const QString &status, bool isOverdue = false) const;
    void drawTypeTag(QPainter *painter, const QRect &rect, const QString &type) const;

    // 缓存资源以榨干性能
    mutable QCache<QString, QPixmap> m_avatarCache;
    mutable QPainterPath m_circlePath;
    
    // 预定义颜色与字体
    QColor m_primaryColor = QColor("#409eff");
    QColor m_pendingColor = QColor("#3498db");
    QColor m_processingColor = QColor("#f1c40f");
    QColor m_completedColor = QColor("#2ecc71");
    QColor m_canceledColor = QColor("#e74c3c");
    
    QFont m_nameFont;
    QFont m_timeFont;
    QFont m_tagFont;
};

#endif // APPOINTMENTITEMDELEGATE_H
