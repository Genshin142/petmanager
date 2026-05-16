#ifndef PET_CONTROLLER_H
#define PET_CONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include "../network/client_handler.h"

class ServerCore;

class PetController : public QObject {
    Q_OBJECT
public:
    explicit PetController(ServerCore* core, QObject* parent = nullptr);

    // 获取宠物列表
    void handleGetPetList(ClientHandler* client, const QJsonObject& data);
    void handleGetRoomList(ClientHandler* client, const QJsonObject& data);
    void handleAddPet(ClientHandler* client, const QJsonObject& data);
    void handleUpdatePet(ClientHandler* client, const QJsonObject& data);
    void handleGetVaccines(ClientHandler* client, const QJsonObject& data);
    void handleUpdateVaccines(ClientHandler* client, const QJsonObject& data);
    void handleUpdatePetStatus(ClientHandler* client, const QJsonObject& data);

private:
    ServerCore* m_core;
};

#endif // PET_CONTROLLER_H
