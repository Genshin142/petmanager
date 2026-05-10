#ifndef SCHEDULEDATAMANAGER_H
#define SCHEDULEDATAMANAGER_H

#include <QObject>
#include <QMap>
#include <QDate>
#include "common_types.h"
#include "protocol_codes.h"

class ScheduleDataManager : public QObject
{
    Q_OBJECT
public:
    static ScheduleDataManager* instance();

    // 获取某天某员工的班次
    ScheduleInfo getSchedule(const QString &employeeId, const QDate &date);
    QList<ScheduleInfo> getWeeklySchedule(const QString &staffId, const QDate &monday);

    // 设置/更新班次
    void saveSchedule(const ScheduleInfo &info);
    void setSchedule(const ScheduleInfo &info); // 兼容性别名

    // 模板管理
    QList<ScheduleInfo> getTemplateSchedule(const QString &staffId);
    void saveTemplateSchedule(const QString &staffId, const QList<ScheduleInfo> &weeklyTemplate);

    // Network methods
    void requestScheduleList(const QString &startDate, const QString &endDate);

public slots:
    void onPacketReceived(const Protocol::NetPacket &packet);

    void clearAllData();   // 批量设置班次
    void setSchedules(const QList<ScheduleInfo> &infos);

    // 获取某段时间内所有员工的班次
    QList<ScheduleInfo> getSchedulesInRange(const QDate &start, const QDate &end);

    // 生成模拟数据
    void initMockData();

signals:
    void scheduleChanged();

private:
    explicit ScheduleDataManager(QObject *parent = nullptr);
    static ScheduleDataManager* m_instance;

    // Key: employeeId_yyyy-MM-dd
    QMap<QString, ScheduleInfo> m_schedules;
};

#endif // SCHEDULEDATAMANAGER_H
