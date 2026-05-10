#ifndef SERVICE_CONTROLLER_H
#define SERVICE_CONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

class ServerCore;
class ClientHandler;

class ServiceController : public QObject {
    Q_OBJECT
public:
    explicit ServiceController(ServerCore* core, QObject* parent = nullptr);

    void handleGetServiceList(ClientHandler* client, const QJsonObject& data);
    void handleAddService(ClientHandler* client, const QJsonObject& data);
    void handleUpdateService(ClientHandler* client, const QJsonObject& data);
    void handleDeleteService(ClientHandler* client, const QJsonObject& data);

private:
    ServerCore* m_core;
};

#endif // SERVICE_CONTROLLER_H
