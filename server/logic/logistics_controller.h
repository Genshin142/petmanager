#ifndef LOGISTICS_CONTROLLER_H
#define LOGISTICS_CONTROLLER_H

#include <QObject>
#include <QJsonObject>

class ServerCore;
class ClientHandler;

class LogisticsController : public QObject
{
    Q_OBJECT
public:
    explicit LogisticsController(ServerCore *core, QObject *parent = nullptr);

private:
    void handleGetLogisticsList(ClientHandler *client, const QJsonObject &body);
    void handleUpdateLogisticsStatus(ClientHandler *client, const QJsonObject &body);
    void handleAddLogisticsTask(ClientHandler *client, const QJsonObject &body);

    ServerCore *m_core;
};

#endif // LOGISTICS_CONTROLLER_H
