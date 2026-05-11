#ifndef SCHEDULE_CONTROLLER_H
#define SCHEDULE_CONTROLLER_H

#include <QObject>
#include "../network/server_core.h"
#include "../network/client_handler.h"
#include <QJsonObject>
#include <QJsonArray>

class ScheduleController : public QObject
{
    Q_OBJECT
public:
    explicit ScheduleController(ServerCore *core, QObject *parent = nullptr);

private:
    void handleGetSchedule(ClientHandler *client, const QJsonObject &body);
    void handleUpdateSchedule(ClientHandler *client, const QJsonObject &body);
    void handleBatchUpdateSchedule(ClientHandler *client, const QJsonObject &body);
    
    bool upsertSchedule(const QJsonObject &obj);
    
    ServerCore *m_core;
};

#endif // SCHEDULE_CONTROLLER_H
