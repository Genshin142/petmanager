#include "appointmentitemdelegate.h"
#include <QApplication>
#include <QDateTime>
#include <QPainter>
#include "../utils/imageutils.h"

AppointmentItemDelegate::AppointmentItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
    m_avatarCache.setMaxCost(100); 
    
    m_nameFont = QFont("Microsoft YaHei", 10, QFont::Bold);
    m_timeFont = QFont("Consolas", 11, QFont::Bold);
    m_tagFont = QFont("Microsoft YaHei", 9);
    
    m_circlePath.addEllipse(0, 0, 48, 48);
}

void AppointmentItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid()) return;

    AppointmentInfo info = index.data(Qt::UserRole).value<AppointmentInfo>();
    
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    drawCard(painter, option, info);

    painter->restore();
}

QSize AppointmentItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(0, 88);
}

void AppointmentItemDelegate::drawCard(QPainter *painter, const QStyleOptionViewItem &option, const AppointmentInfo &info) const
{
    QRect rect = option.rect.adjusted(8, 4, -8, -4);
    bool isHovered = (option.state & QStyle::State_MouseOver);
    bool isSelected = (option.state & QStyle::State_Selected);
    
    // ═══ 超时判定 ═══
    bool isOverdue = false;
    if (!info.date.isEmpty() && !info.hour.isEmpty() &&
        (info.status == "Pending" || info.status == "Confirmed")) {
        QDate apptDate = QDate::fromString(info.date, "yyyy-MM-dd");
        if (apptDate < QDate::currentDate()) {
            isOverdue = true;
        } else if (apptDate == QDate::currentDate()) {
            QStringList timeSlots = {"09:00 - 11:00", "11:00 - 14:00", "14:00 - 16:00", "16:00 - 18:00", "18:00 - 21:00"};
            for (const auto &slot : timeSlots) {
                if (info.hour >= slot.left(5) && info.hour < slot.right(5)) {
                    QTime endTime = QTime::fromString(slot.right(5), "HH:mm");
                    if (QTime::currentTime() > endTime) isOverdue = true;
                    break;
                }
            }
        }
    }

    // 1. 卡片背景
    painter->setPen(Qt::NoPen);
    if (isOverdue) {
        painter->setBrush(isHovered ? QColor("#fee2e2") : QColor("#fff5f5"));
    } else if (isSelected) {
        painter->setBrush(QColor("#e8f4ff"));
    } else if (isHovered) {
        painter->setBrush(QColor("#f5f7fa"));
    } else {
        painter->setBrush(Qt::white);
    }
    painter->drawRoundedRect(rect, 10, 10);
    
    // 边框
    QColor borderColor = isOverdue ? QColor("#fecaca") : isSelected ? QColor("#b3d8ff") : QColor(235, 238, 245);
    painter->setPen(QPen(borderColor, 1));
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(rect, 10, 10);

    // ═══ 左侧色块指示条 ═══
    QColor accentColor;
    if (isOverdue) accentColor = QColor("#dc2626");
    else if (info.status == "Pending" || info.status == "待处理") accentColor = QColor("#ea580c");
    else if (info.status == "Confirmed" || info.status == "已确认") accentColor = QColor("#67c23a");
    else if (info.status == "In-Service" || info.status == "服务中") accentColor = QColor("#409eff");
    else if (info.status == "Cancelled" || info.status == "已取消") accentColor = QColor("#c0c4cc");
    else if (info.status == "Expired" || info.status == "已过期") accentColor = QColor("#a3a3a3");
    else accentColor = QColor("#67c23a"); // Completed

    QPainterPath accentPath;
    accentPath.addRoundedRect(QRectF(rect.left(), rect.top(), 5, rect.height()), 3, 3);
    painter->setPen(Qt::NoPen);
    painter->setBrush(accentColor);
    painter->drawPath(accentPath);

    // ═══ 2. 头像区 (48x48, 增大) ═══
    int avatarSize = 48;
    QRect avatarRect(rect.left() + 18, rect.top() + (rect.height() - avatarSize) / 2, avatarSize, avatarSize);
    drawAvatar(painter, avatarRect, info.petAvatar);

    // ═══ 3. 中间信息区 ═══
    int textLeft = avatarRect.right() + 16;
    int textRight = rect.right() - 200; // 给右侧预留200px
    int midY = rect.top() + rect.height() / 2;

    // 第一行：宠物名 + 服务类型标签
    QFont nameFont("Microsoft YaHei", 13, QFont::Bold);
    painter->setFont(nameFont);
    painter->setPen(isOverdue ? QColor("#991b1b") : QColor("#1e293b"));
    painter->drawText(textLeft, midY - 12, info.petName);
    
    int nameWidth = painter->fontMetrics().horizontalAdvance(info.petName);
    
    // 业务标签（紧跟宠物名后面）
    drawTypeTag(painter, QRect(textLeft + nameWidth + 10, midY - 26, 48, 20), info.type);

    // 第二行：会员名 · 手机号 · 服务项目
    QFont subFont("Microsoft YaHei", 11);
    painter->setFont(subFont);
    painter->setPen(QColor("#94a3b8"));
    
    QString subText;
    if (!info.memberName.isEmpty()) {
        subText = info.memberName;
        if (!info.memberPhone.isEmpty()) subText += QString("  ·  %1").arg(info.memberPhone);
        if (!info.service.isEmpty()) subText += QString("  ·  %1").arg(info.service);
    } else {
        subText = info.service.isEmpty() ? "常规服务" : info.service;
    }
    QRect subRect(textLeft, midY + 2, textRight - textLeft, 20);
    painter->drawText(subRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, 
                      painter->fontMetrics().elidedText(subText, Qt::ElideRight, subRect.width()));

    // ═══ 4. 右侧：日期时间 + 状态标签 ═══
    int rightSectionLeft = rect.right() - 220;

    // 日期时间 (上)
    QFont timeFont("Microsoft YaHei", 11, QFont::Bold);
    painter->setFont(timeFont);
    painter->setPen(QColor("#475569"));
    
    QDate apptDate = QDate::fromString(info.date, "yyyy-MM-dd");
    QString dateStr;
    if (apptDate == QDate::currentDate()) dateStr = "今天";
    else if (apptDate == QDate::currentDate().addDays(1)) dateStr = "明天";
    else if (apptDate == QDate::currentDate().addDays(-1)) dateStr = "昨天";
    else dateStr = QString("%1年%2月%3日").arg(apptDate.year()).arg(apptDate.month()).arg(apptDate.day());
    
    QString fullTime;
    if (info.type == "Boarding" && !info.boardingEndDate.isEmpty()) {
        QDate endDate = QDate::fromString(info.boardingEndDate, "yyyy-MM-dd");
        QString endStr = QString("%1/%2").arg(endDate.month()).arg(endDate.day());
        fullTime = QString("%1 → %2 共%3天").arg(dateStr, endStr).arg(info.duration);
    } else {
        fullTime = QString("%1  %2").arg(dateStr, info.hour);
    }
    painter->drawText(QRect(rightSectionLeft, rect.top(), 210, rect.height() / 2), 
                      Qt::AlignRight | Qt::AlignBottom, fullTime);

    // 状态标签 (下)
    QString displayStatus;
    if (info.status == "Pending" || info.status == "待处理") displayStatus = "待处理";
    else if (info.status == "Confirmed" || info.status == "已确认") displayStatus = "已确认";
    else if (info.status == "In-Service" || info.status == "服务中") displayStatus = "服务中";
    else if (info.status == "Cancelled" || info.status == "已取消") displayStatus = "已取消";
    else if (info.status == "Expired" || info.status == "已过期") displayStatus = "已过期";
    else displayStatus = "已完成";

    if (isOverdue) displayStatus = "⚠ 已超时";

    drawStatusTag(painter, QRect(rightSectionLeft, midY + 4, 180, 22), info.status, isOverdue);
}

void AppointmentItemDelegate::drawAvatar(QPainter *painter, const QRect &rect, const QString &path) const
{
    QString key = path.isEmpty() ? ":/images/load_img.jpg" : path;
    QPixmap *cached = m_avatarCache.object(key);
    if (!cached) {
        QPixmap pix = ImageUtils::loadPixmap(key);
        if (pix.isNull()) { pix.load(":/images/load_img.jpg"); key = ":/images/load_img.jpg"; }
        
        int sz = 48;
        QPixmap target(sz, sz);
        target.fill(Qt::transparent);
        QPainter p(&target);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        QPainterPath clipPath;
        clipPath.addEllipse(1, 1, sz - 2, sz - 2);
        p.setClipPath(clipPath);
        p.drawPixmap(1, 1, sz - 2, sz - 2, pix.scaled(sz - 2, sz - 2, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        p.setClipping(false);
        p.setPen(QPen(QColor("#e2e8f0"), 1.5));
        p.drawEllipse(1, 1, sz - 2, sz - 2);
        p.end();
        
        cached = new QPixmap(target);
        m_avatarCache.insert(key, cached);
    }
    painter->drawPixmap(rect, *cached);
}

void AppointmentItemDelegate::drawStatusTag(QPainter *painter, const QRect &rect, const QString &status, bool isOverdue) const
{
    QString text;
    QColor bg, fg;

    if (isOverdue) {
        text = "⚠ 已超时"; bg = QColor("#fef2f2"); fg = QColor("#dc2626");
    } else if (status == "Pending" || status == "待处理") {
        text = "待处理"; bg = QColor("#fff7ed"); fg = QColor("#ea580c");
    } else if (status == "Confirmed" || status == "已确认") {
        text = "已确认"; bg = QColor("#f0f9eb"); fg = QColor("#67c23a");
    } else if (status == "In-Service" || status == "服务中") {
        text = "服务中"; bg = QColor("#eff6ff"); fg = QColor("#2563eb");
    } else if (status == "Cancelled" || status == "已取消") {
        text = "已取消"; bg = QColor("#f4f4f5"); fg = QColor("#909399");
    } else if (status == "Expired" || status == "已过期") {
        text = "已过期"; bg = QColor("#fafafa"); fg = QColor("#a3a3a3");
    } else {
        text = "已完成"; bg = QColor("#f0f9eb"); fg = QColor("#67c23a");
    }

    // 右对齐绘制标签
    QFont tagFont("Microsoft YaHei", 10, QFont::Bold);
    painter->setFont(tagFont);
    int tagWidth = painter->fontMetrics().horizontalAdvance(text) + 20;
    QRect tagRect(rect.right() - tagWidth, rect.top(), tagWidth, rect.height());
    
    painter->setPen(Qt::NoPen);
    painter->setBrush(bg);
    painter->drawRoundedRect(tagRect, 4, 4);

    // 边框
    painter->setPen(QPen(fg.lighter(140), 1));
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(tagRect, 4, 4);

    painter->setPen(fg);
    painter->drawText(tagRect, Qt::AlignCenter, text);
}

void AppointmentItemDelegate::drawTypeTag(QPainter *painter, const QRect &rect, const QString &type) const
{
    QString text = "洗护";
    QColor bg = QColor("#f0f9eb");
    QColor fg = QColor("#67c23a");

    if (type == "Beauty") { text = "美容"; bg = QColor("#fdf6ec"); fg = QColor("#e6a23c"); }
    else if (type == "Transport") { text = "接送"; bg = QColor("#ecf5ff"); fg = QColor("#409eff"); }
    else if (type == "Boarding") { text = "寄养"; bg = QColor("#fef0f0"); fg = QColor("#f56c6c"); }

    painter->setPen(Qt::NoPen);
    painter->setBrush(bg);
    painter->drawRoundedRect(rect, 4, 4);

    QFont tagFont("Microsoft YaHei", 9, QFont::Bold);
    painter->setFont(tagFont);
    painter->setPen(fg);
    painter->drawText(rect, Qt::AlignCenter, text);
}
