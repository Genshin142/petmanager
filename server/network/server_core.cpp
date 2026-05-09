#include "server_core.h"
#include "../database/connectionpool.h"
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QDebug>

ServerCore::ServerCore(QObject *parent) : QObject(parent)
{
    m_tcpServer = new QTcpServer(this);
}

bool ServerCore::start(quint16 port)
{
    connect(m_tcpServer, &QTcpServer::newConnection, this, &ServerCore::onNewConnection);
    return m_tcpServer->listen(QHostAddress::Any, port);
}

void ServerCore::onNewConnection()
{
    while (m_tcpServer->hasPendingConnections()) {
        QTcpSocket *socket = m_tcpServer->nextPendingConnection();
        ClientHandler *handler = new ClientHandler(socket, this);
        
        connect(handler, &ClientHandler::packetReady, this, [this, handler](const Protocol::NetPacket &p){
            this->dispatchPacket(handler, p);
        });
        connect(handler, &ClientHandler::disconnected, this, &ServerCore::onClientDisconnected);
        
        m_clients.append(handler);
        qDebug() << "[NET] New client connected from" << socket->peerAddress().toString() 
                 << ". Total clients:" << m_clients.size();
    }
}

void ServerCore::dispatchPacket(ClientHandler *client, const Protocol::NetPacket &packet)
{
    qDebug() << "[DISPATCH] Received Command:" << packet.cmdId << "Size:" << packet.length;

    switch (packet.cmdId) {
        case Protocol::CMD_LOGIN:
            handleLogin(client, packet.data);
            break;
        case Protocol::CMD_HEARTBEAT:
            client->sendPacket(Protocol::CMD_HEARTBEAT, "{\"status\":\"alive\"}");
            break;
        default:
            qWarning() << "[DISPATCH] Unknown command ID:" << packet.cmdId;
            break;
    }
}

void ServerCore::handleLogin(ClientHandler *client, const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        qWarning() << "[AUTH] Invalid JSON data received for login.";
        return;
    }

    QJsonObject obj = doc.object();
    QString user = obj["username"].toString();
    QString pwd = obj["password"].toString();

    qDebug() << "[AUTH] Login attempt for user:" << user;

    // 从连接池获取数据库连接
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("SELECT emp_id, real_name, role FROM sys_employees "
                  "WHERE username = ? AND password = ? AND is_deleted = 0");
    query.addBindValue(user);
    query.addBindValue(pwd);

    QJsonObject response;
    if (query.exec() && query.next()) {
        response["status"] = Protocol::STATUS_OK;
        response["message"] = "Login success";
        
        QJsonObject userInfo;
        userInfo["emp_id"] = query.value("emp_id").toInt();
        userInfo["name"] = query.value("real_name").toString();
        userInfo["role"] = query.value("role").toString();
        response["user_info"] = userInfo;
        
        qDebug() << "[AUTH] Success:" << user << "(" << query.value("real_name").toString() << ")";
    } else {
        response["status"] = Protocol::STATUS_AUTH_FAIL;
        response["message"] = "Invalid username or password";
        qDebug() << "[AUTH] Failed for user:" << user;
    }

    client->sendPacket(Protocol::CMD_LOGIN, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void ServerCore::onClientDisconnected()
{
    ClientHandler *handler = qobject_cast<ClientHandler*>(sender());
    if (handler) {
        m_clients.removeAll(handler);
        handler->deleteLater();
        qDebug() << "[NET] Client disconnected. Remaining clients:" << m_clients.size();
    }
}
