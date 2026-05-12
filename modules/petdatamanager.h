#ifndef PETDATAMANAGER_H
#define PETDATAMANAGER_H

#include <QObject>
#include <QMap>
#include <QList>
#include <QDateTime>
#include <QRecursiveMutex>
#include <QPixmap>
#include <QCache>
#include "../common_types.h"
#include "../protocol_codes.h"

struct OrderStats {
    double totalRevenue;
    int pendingCount;
    double avgTicket;
    double successRate;
};

class PetDataManager : public QObject
{
    Q_OBJECT
public:
    static PetDataManager* instance();

    // 宠物信息管理
    void requestPetList(); 
    void updatePet(const PetInfo &info);
    PetInfo getPet(const QString &id) const;
    QList<PetInfo> allPets() const;
    QList<PetInfo> getPetsByOwner(const QString &ownerId) const;
    void addPet(const PetInfo &info);
    void removePet(const QString &id);
    void restorePet(const QString &id);
    void hardDeletePet(const QString &id);
    void notifyGlobalDataChanged();
    void notifyPetDataChanged(const QString &petId);
    
    QPixmap getPetPixmap(const QString &id) const;

    // 日志与影像
    void addActivityLog(const QString &petId, const PetActivityLog &log);
    QList<PetActivityLog> getLogs(const QString &petId) const;
    void addMedia(const QString &petId, const PetMedia &media);
    void deleteMediaPhoto(const QString &petId, const QString &title, const QString &url);
    QList<PetMedia> getMedia(const QString &petId) const;

    // 疫苗与寄养
    void updateVaccines(const QString &petId, const QList<VaccineRecord> &records);
    void requestVaccines(const QString &petId);
    QList<VaccineRecord> getVaccines(const QString &petId) const;
    QList<FosterBatch> getHistoryBatches(const QString &petId) const;

    // 房间管理
    void requestRoomList();
    QList<BoardingRoom> allRooms() const;
    BoardingRoom getRoom(int id) const;
    bool isRoomAvailable(int roomId, const QDate &start, const QDate &end) const;
    QString getRoomType(int roomId) const;
    QList<int> getAvailableRooms(const QDate &start, const QDate &end, const QString &type = "") const;
    void executeCheckIn(int roomId, const QString &petId, const QDate &start, const QDate &end, double weight, const QString &note = "");
    void executeBooking(int roomId, const QString &petId, const QDate &start, const QDate &end, double weight);
    void executeCancelBooking(int roomId, const QString &petId);
    
    // 房态管理
    void addRoomStatusPeriod(int roomId, const RoomStatusPeriod &period);
    void removeRoomStatusPeriod(int roomId, const QString &type = "");
    QList<RoomStatusPeriod> getRoomStatusPeriods(int roomId) const;

    // 预约管理
    std::pair<QList<AppointmentInfo>, int> getAppointments(int page, int pageSize, const QString &filter = "", const QString &statusFilter = "全部");
    AppointmentInfo getAppointment(const QString &id) const;
    QList<AppointmentInfo> getAppointmentsByGroupId(const QString &groupId) const;
    void addAppointment(const AppointmentInfo &info);
    void updateAppointment(const AppointmentInfo &info);
    void updateAppointmentPhotos(const QString &id, const QStringList &photos);
    AppointmentStats getAppointmentStats();
    int getBoardingOccupation(const QDate &start, const QDate &end) const;
    QList<AppointmentInfo> getAppointmentsForPet(const QString &petId) const;
    void requestAppointmentList(const QDate &startDate, const QDate &endDate);
    void updateAppointmentStatus(const QString &apptId, const QString &status);

    // 订单管理
    void requestOrderList();
    QList<OrderInfo> allOrders() const;
    OrderInfo getOrder(const QString &id) const;
    void addOrder(const OrderInfo &order);
    void updateOrder(const OrderInfo &order);
    void cancelOrder(const QString &orderId, const QString &reason);
    QList<OrderInfo> getOrders(const QDate &start, const QDate &end, const QString &filter = "", const QString &moduleFilter = "全部") const;
    OrderStats getOrderStats(const QDate &start, const QDate &end);

signals:
    void petDataChanged(const QString &petId);
    void globalDataChanged();

private slots:
    void onPacketReceived(const Protocol::NetPacket &packet);

private:
    explicit PetDataManager(QObject *parent = nullptr);
    void initMockData();

    static PetDataManager *m_instance;
    
    QMap<QString, PetInfo> m_pets;
    QMap<QString, QList<PetActivityLog>> m_activityLogs;
    QMap<QString, QList<PetMedia>> m_petMedia;
    QMap<QString, QList<VaccineRecord>> m_vaccineRecords;
    QMap<QString, QList<FosterBatch>> m_historyBatches;
    QMap<int, QList<RoomStatusPeriod>> m_roomStatusPeriods;
    QMap<QString, AppointmentInfo> m_appointments;
    QMap<int, BoardingRoom> m_rooms;
    QMap<QString, OrderInfo> m_orders;
    mutable QRecursiveMutex m_mutex;
    
    QString m_cachePath;
    mutable QCache<QString, QPixmap> m_pixmapCache;
    void ensureCacheDir();
    void saveToLocalCache(const QString &id, const QPixmap &pix);
};

#endif // PETDATAMANAGER_H
