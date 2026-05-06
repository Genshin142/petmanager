#ifndef PETDATAMANAGER_H
#define PETDATAMANAGER_H

#include <QObject>
#include <QMap>
#include <QList>
#include <QDateTime>
#include "../common_types.h"

class PetDataManager : public QObject
{
    Q_OBJECT
public:
    static PetDataManager* instance();

    // 宠物信息管理
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

    // 日志管理
    void addActivityLog(const QString &petId, const PetActivityLog &log);
    QList<PetActivityLog> getLogs(const QString &petId) const;

    // 影像管理
    void addMedia(const QString &petId, const PetMedia &media);
    void deleteMediaPhoto(const QString &petId, const QString &title, const QString &url);
    QList<PetMedia> getMedia(const QString &petId) const;

    // 疫苗记录管理
    void updateVaccines(const QString &petId, const QList<VaccineRecord> &records);
    QList<VaccineRecord> getVaccines(const QString &petId) const;

    // 寄养记录管理
    QList<FosterBatch> getHistoryBatches(const QString &petId) const;

    // 房间与寄养业务逻辑
    bool isRoomAvailable(int roomId, const QDate &start, const QDate &end) const;
    QString getRoomType(int roomId) const;
    QList<int> getAvailableRooms(const QDate &start, const QDate &end, const QString &type = "") const;
    void executeCheckIn(int roomId, const QString &petId, const QDate &start, const QDate &end, double weight, const QString &note = "");
    void executeBooking(int roomId, const QString &petId, const QDate &start, const QDate &end, double weight);
    void executeCancelBooking(int roomId, const QString &petId);

    // 房间状态周期管理 (维护/清洁)
    void addRoomStatusPeriod(int roomId, const RoomStatusPeriod &period);
    void removeRoomStatusPeriod(int roomId, const QString &type = "");
    QList<RoomStatusPeriod> getRoomStatusPeriods(int roomId) const;

    // 预约管理 (V2 真实分页支持)
    std::pair<QList<AppointmentInfo>, int> getAppointments(int page, int pageSize, const QString &filter = "", const QString &statusFilter = "全部");
    AppointmentInfo getAppointment(const QString &id) const;
    QList<AppointmentInfo> getAppointmentsByGroupId(const QString &groupId) const;
    void addAppointment(const AppointmentInfo &info);
    void updateAppointment(const AppointmentInfo &info);
    void updateAppointmentPhotos(const QString &id, const QStringList &photos); // 新增
    AppointmentStats getAppointmentStats();
    int getBoardingOccupation(const QDate &start, const QDate &end) const;
    QList<AppointmentInfo> getAppointmentsForPet(const QString &petId) const;

    // 订单管理 (新)
    void addOrder(const OrderInfo &info);
    void updateOrder(const OrderInfo &info);
    void cancelOrder(const QString &orderId, const QString &reason);
    QList<OrderInfo> getOrders(const QDate &start, const QDate &end, const QString &filter = "", const QString &moduleFilter = "全部") const;
    OrderInfo getOrder(const QString &id) const;
    struct OrderStats {
        double totalRevenue;
        int pendingCount;
        double avgTicket;
        double successRate;
    };
    OrderStats getOrderStats(const QDate &start, const QDate &end);

signals:
    void petDataChanged(const QString &petId);
    void globalDataChanged();

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
    QMap<QString, OrderInfo> m_orders; // 新增
};

#endif // PETDATAMANAGER_H
