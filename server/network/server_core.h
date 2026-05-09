#ifndef SERVER_CORE_H
#define SERVER_CORE_H

#include <QtCore/QObject>
#include <QtNetwork/QTcpServer>
#include <QtCore/QList>
#include "client_handler.h"

class ServerCore : public QObject
{
    Q_OBJECT
public:
    explicit ServerCore(QObject *parent = nullptr);
    bool start(quint16 port);

private slots:
    void onNewConnection();
    void onClientDisconnected();

private:
    QTcpServer *m_tcpServer;
    QList<ClientHandler*> m_clients;

    // 业务分发路由函数
    void dispatchPacket(ClientHandler *client, const Protocol::NetPacket &packet);
    
    // 具体的业务处理模块
    void handleLogin(ClientHandler *client, const QByteArray &data);
};

#endif // SERVER_CORE_H
