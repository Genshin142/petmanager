#ifndef USER_CONTROLLER_H
#define USER_CONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include "../network/client_handler.h"

class ServerCore;

class UserController : public QObject {
    Q_OBJECT
public:
    explicit UserController(ServerCore* core, QObject* parent = nullptr);

    // 登录逻辑
    void handleLogin(ClientHandler* client, const QJsonObject& data);

private:
    ServerCore* m_core;
};

#endif // USER_CONTROLLER_H
