#ifndef LOG_CONTROLLER_H
#define LOG_CONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include "../network/server_core.h"

class LogController : public QObject
{
    Q_OBJECT
public:
    explicit LogController(ServerCore *server, QObject *parent = nullptr);

    // 处理获取系统日志列表 (CMD 9001)
    void handleGetLogList(ClientHandler *client, const QJsonObject &data);
    
    // 处理添加系统日志 (CMD 9002)
    void handleAddLog(ClientHandler *client, const QJsonObject &data);

private:
    ServerCore *m_server;
};

#endif // LOG_CONTROLLER_H
