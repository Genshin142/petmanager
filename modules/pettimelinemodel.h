#ifndef PETTIMELINEMODEL_H
#define PETTIMELINEMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QColor>
#include "../common_types.h"

class PetTimelineModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        TypeRole = Qt::UserRole + 1,
        TimeRole,
        ContentRole,
        IconRole,
        IsHeaderRole,
        IsAlertRole,
        HighlightRole,
        OperatorRole,
        RoomNoRole       // 新增房号索引
    };

    struct TimelineItem {
        bool isHeader;
        PetActivityLog log;
        bool highlight = false;
    };

    explicit PetTimelineModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    void setLogs(const QList<PetActivityLog> &logs) {
        beginResetModel();
        m_items.clear();
        
        if (logs.isEmpty()) {
            endResetModel();
            return;
        }

        // 1. Sort logs by time ascending (oldest first)
        QList<PetActivityLog> sortedLogs = logs;
        std::sort(sortedLogs.begin(), sortedLogs.end(), [](const PetActivityLog &a, const PetActivityLog &b) {
            return a.time < b.time; // Chronological order
        });

        // 2. Group by date and add headers
        QString lastDate;
        for (const auto &log : sortedLogs) {
            QString curDate = QDateTime::fromString(log.time, "yyyy-MM-dd HH:mm").toString("yyyy-MM-dd");
            if (curDate != lastDate) {
                TimelineItem header;
                header.isHeader = true;
                header.log.time = curDate;
                m_items.append(header);
                lastDate = curDate;
            }
            
            TimelineItem item;
            item.isHeader = false;
            item.log = log;
            m_items.append(item);
        }
        endResetModel();
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        Q_UNUSED(parent);
        return m_items.size();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (!index.isValid() || index.row() >= m_items.size()) return QVariant();

        const auto &item = m_items[index.row()];
        switch (role) {
            case TypeRole: return item.isHeader ? "Header" : item.log.type;
            case TimeRole: return item.log.time;
            case ContentRole: return item.log.remark;
            case IconRole: return item.log.icon;
            case IsHeaderRole: return item.isHeader;
            case IsAlertRole: return item.log.isAlert;
            case HighlightRole: return item.highlight;
            case OperatorRole: return item.isHeader ? "" : item.log.operatorName;
            case RoomNoRole: return item.isHeader ? "" : item.log.roomNo;
        }
        return QVariant();
    }

    void setHighlight(int row) {
        for (int i = 0; i < m_items.size(); ++i) {
            if (m_items[i].highlight) {
                m_items[i].highlight = false;
                emit dataChanged(index(i), index(i), {HighlightRole});
            }
        }
        if (row >= 0 && row < m_items.size()) {
            m_items[row].highlight = true;
            emit dataChanged(index(row), index(row), {HighlightRole});
        }
    }

    int findFirstIndexForDate(const QString &date) const {
        for (int i = 0; i < m_items.size(); ++i) {
            if (m_items[i].isHeader && m_items[i].log.time == date) {
                return i;
            }
        }
        return -1;
    }

private:
    QList<TimelineItem> m_items;
};

#endif // PETTIMELINEMODEL_H
