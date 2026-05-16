#ifndef SERVER_CORE_H
#define SERVER_CORE_H

#include <QtCore/QObject>
#include <functional>
#include <QMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QtNetwork/QTcpServer>
#include <QtCore/QList>
#include <QtCore/QThreadPool>
#include "../logger_compat.h"
#include "../logic/pet_controller.h"
#include "../logic/member_controller.h"
#include "../logic/product_controller.h"
#include <QtCore/QDebug>
#include "client_handler.h"

// 兼容性日志宏：如果 glog 沉默，通过 qDebug 强制输出
#define LOG_I(msg) qDebug().noquote() << "[INFO]" << msg
#define LOG_W(msg) qWarning().noquote() << "[WARN]" << msg
#define LOG_E(msg) qCritical().noquote() << "[ERROR]" << msg

class ServerCore : public QObject
{
    Q_OBJECT
public:
    using HandlerFunc = std::function<void(ClientHandler*, const QJsonObject&)>;

    explicit ServerCore(QObject *parent = nullptr);
    bool start(quint16 port);

    // 路由注册接口
    void registerHandler(int cmdId, HandlerFunc handler);
    void broadcastPacket(int cmdId, const QJsonObject &data); 

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onPacketReady(const Protocol::NetPacket &packet);

private:
    QTcpServer *m_server;
    QList<ClientHandler*> m_clients;
    QMap<int, HandlerFunc> m_handlers;
    QThreadPool *m_threadPool;
};

#endif // SERVER_CORE_H
