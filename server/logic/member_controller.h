#ifndef MEMBER_CONTROLLER_H
#define MEMBER_CONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include "../network/server_core.h"

class MemberController : public QObject
{
    Q_OBJECT
public:
    explicit MemberController(ServerCore *server, QObject *parent = nullptr);

    // 处理获取会员列表请求 (CMD 2002)
    void handleGetMemberList(ClientHandler *client, const QJsonObject &data);

private:
    void handleAddMember(ClientHandler *client, const QJsonObject &data);
    void handleUpdateMember(ClientHandler *client, const QJsonObject &data);
    void handleDeleteMember(ClientHandler *client, const QJsonObject &data);
    void handleRestoreMember(ClientHandler *client, const QJsonObject &data);
    void handleHardDeleteMember(ClientHandler *client, const QJsonObject &data);

    ServerCore *m_server;
};

#endif // MEMBER_CONTROLLER_H
