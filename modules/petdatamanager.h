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
    void addPet(const PetInfo &info);
    void removePet(const QString &id);
    void notifyGlobalDataChanged();
    void notifyPetDataChanged(const QString &petId);

    // 日志管理
    void addActivityLog(const QString &petId, const PetActivityLog &log);
    QList<PetActivityLog> getLogs(const QString &petId) const;

    // 影像管理
    void addMedia(const QString &petId, const PetMedia &media);
    QList<PetMedia> getMedia(const QString &petId) const;

    // 疫苗记录管理
    void updateVaccines(const QString &petId, const QList<VaccineRecord> &records);
    QList<VaccineRecord> getVaccines(const QString &petId) const;

    // 寄养记录管理
    QList<FosterBatch> getHistoryBatches(const QString &petId) const;

    // 房间与寄养业务逻辑
    bool isRoomAvailable(int roomId, const QDate &start, const QDate &end) const;
    void executeCheckIn(int roomId, const QString &petId, const QDate &start, const QDate &end, double weight, const QString &note = "");

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
};

#endif // PETDATAMANAGER_H
