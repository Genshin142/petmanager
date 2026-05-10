#ifndef STAFF_CONTROLLER_H
#define STAFF_CONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include "../network/client_handler.h"

class ServerCore;

class StaffController : public QObject {
    Q_OBJECT
public:
    explicit StaffController(ServerCore* core, QObject* parent = nullptr);

    void handleGetStaffList(ClientHandler* client, const QJsonObject& data);
    void handleAddStaff(ClientHandler* client, const QJsonObject& data);
    void handleUpdateStaff(ClientHandler* client, const QJsonObject& data);
    void handleDeleteStaff(ClientHandler* client, const QJsonObject& data);
    void handleRestoreStaff(ClientHandler* client, const QJsonObject& data);

private:
    ServerCore* m_core;
};

#endif // STAFF_CONTROLLER_H
