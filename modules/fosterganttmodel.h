#ifndef FOSTERGANTTMODEL_H
#define FOSTERGANTTMODEL_H

#include <QAbstractTableModel>
#include <QDate>
#include <QColor>
#include "petdatamanager.h"

class FosterGanttModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Roles {
        PetIdRole = Qt::UserRole + 1,
        StatusRole,      // "occupied", "booked", "free"
        StayStartRole,
        StayEndRole,
        IsStartRole,     // Is this the first day of the stay
        IsEndRole        // Is this the last day of the stay
    };

    explicit FosterGanttModel(QObject *parent = nullptr) : QAbstractTableModel(parent) {
        m_startDate = QDate::currentDate().addDays(-2); // 默认从前两天开始显示
        m_dayCount = 35; // 显示 5 周左右
    }

    void setStartDate(const QDate &date) {
        beginResetModel();
        m_startDate = date;
        endResetModel();
    }

    QDate startDate() const { return m_startDate; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        Q_UNUSED(parent);
        return 20; // 20个房间 (101-120)
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override {
        Q_UNUSED(parent);
        return m_dayCount;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override {
        if (role == Qt::DisplayRole) {
            if (orientation == Qt::Horizontal) {
                QDate d = m_startDate.addDays(section);
                static const QStringList weekDays = {"", "周一", "周二", "周三", "周四", "周五", "周六", "周日"};
                return QString("%1\n%2").arg(d.toString("MM-dd"), weekDays[d.dayOfWeek()]);
            } else {
                return QString("Room %1").arg(101 + section);
            }
        }
        return QVariant();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (!index.isValid()) return QVariant();

        int roomId = 101 + index.row();
        QDate date = m_startDate.addDays(index.column());
        QString roomIdStr = QString::number(roomId);

        // 获取该房间在该日期的房态
        auto allPets = PetDataManager::instance()->allPets();
        for (const auto &pet : allPets) {
            bool roomMatch = (pet.roomNo == roomIdStr) || (pet.roomNo.toInt() == roomId);
            if (!roomMatch) continue;

            QDate s = QDate::fromString(pet.fosterStartTime, "yyyy-MM-dd");
            QDate e = pet.fosterEndTime == "至今" ? QDate::currentDate().addYears(1) : QDate::fromString(pet.fosterEndTime, "yyyy-MM-dd");

            if (date >= s && date <= e) {
                if (role == Qt::DisplayRole) return pet.name;
                if (role == PetIdRole) return pet.id;
                if (role == StatusRole) return (pet.status == "已预约" || pet.status == "待寄养") ? "booked" : "occupied";
                if (role == StayStartRole) return s;
                if (role == StayEndRole) return e;
                if (role == IsStartRole) return date == s;
                if (role == IsEndRole) return date == e;
                
                if (role == Qt::BackgroundRole) {
                    return (pet.status == "已预约" || pet.status == "待寄养") ? QColor("#f9f0ff") : QColor("#f0f7ff");
                }
                if (role == Qt::ForegroundRole) {
                    return (pet.status == "已预约" || pet.status == "待寄养") ? QColor("#722ed1") : QColor("#1890ff");
                }
            }
        }

        // 如果没有宠物，检查是否处于维护/清洁状态 (逻辑同步 FosterModule)
        auto periods = PetDataManager::instance()->getRoomStatusPeriods(roomId);
        for (const auto &p : periods) {
            QDate s = QDate::fromString(p.startTime.split(" ").first(), "yyyy-MM-dd");
            QDate e = QDate::fromString(p.endTime.split(" ").first(), "yyyy-MM-dd");
            
            if (date >= s && date <= e) {
                if (role == Qt::DisplayRole) return (p.type == "maintenance") ? "维护" : "清洁";
                if (role == StatusRole) return p.type;
                if (role == StayStartRole) return s;
                if (role == StayEndRole) return e;
                if (role == IsStartRole) return date == s;
                if (role == IsEndRole) return date == e;
                
                if (role == Qt::BackgroundRole) {
                    return (p.type == "maintenance") ? QColor("#fff7e6") : QColor("#f6ffed");
                }
                if (role == Qt::ForegroundRole) {
                    return (p.type == "maintenance") ? QColor("#fa8c16") : QColor("#52c41a");
                }
            }
        }

        return QVariant();
    }

private:
    QDate m_startDate;
    int m_dayCount;
};

#endif // FOSTERGANTTMODEL_H
