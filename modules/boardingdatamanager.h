#ifndef BOARDINGDATAMANAGER_H
#define BOARDINGDATAMANAGER_H

#include <QObject>
#include <QDate>
#include <QMap>
#include <QStringList>

struct BoardingRecord {
    QString orderId;
    QString roomId;
    QDate startDate;
    QDate endDate;
};

class BoardingDataManager : public QObject
{
    Q_OBJECT
public:
    static BoardingDataManager* instance();

    // 获取在指定日期区间内空闲的房间
    QStringList getAvailableRooms(const QDate &start, const QDate &end);

    // 记录一笔新的寄养（用于模拟占用）
    void addRecord(const BoardingRecord &record);

    // 获取所有房间列表
    QStringList allRooms() const { return m_rooms; }

private:
    explicit BoardingDataManager(QObject *parent = nullptr);
    static BoardingDataManager* m_instance;

    QStringList m_rooms;
    QList<BoardingRecord> m_records;
};

#endif // BOARDINGDATAMANAGER_H
