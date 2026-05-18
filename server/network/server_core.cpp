#include "server_core.h"
#include "../database/connectionpool.h"
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QDebug>
#include "../logger_compat.h"
#include <QtNetwork/QTcpSocket>
#include "../logic/user_controller.h"
#include "../logic/pet_controller.h"
#include "../logic/member_controller.h"
#include <QtCore/QThread>
#include "../logic/product_controller.h"
#include "../logic/staff_controller.h"
#include "../logic/service_controller.h"
#include "../logic/schedule_controller.h"
#include "../logic/appointment_controller.h"
#include "../logic/order_controller.h"
#include "../logic/finance_controller.h"
#include "../logic/report_controller.h"
#include "../logic/log_controller.h"
#include <QtCore/QMutexLocker>

ServerCore::ServerCore(QObject *parent) : QObject(parent)
{
    m_server = new QTcpServer(this);
    m_threadPool = new QThreadPool(this);
    m_threadPool->setMaxThreadCount(QThread::idealThreadCount()); // 动态使用 CPU 核心数
    
    connect(m_server, &QTcpServer::newConnection, this, &ServerCore::onNewConnection);
    
    // 初始化业务控制器
    new UserController(this, this);
    new PetController(this, this);
    new MemberController(this, this);
    new ProductController(this, this);
    new StaffController(this, this);
    new ServiceController(this, this);
    new ScheduleController(this, this);
    new AppointmentController(this, this);
    new OrderController(this, this);
    new FinanceController(this, this);
    new ReportController(this, this);
    new LogController(this, this);
}

bool ServerCore::start(quint16 port)
{
    return m_server->listen(QHostAddress::Any, port);
}

void ServerCore::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        ClientHandler *handler = new ClientHandler(socket, this);
        
        connect(handler, &ClientHandler::packetReady, this, &ServerCore::onPacketReady);
        connect(handler, &ClientHandler::disconnected, this, &ServerCore::onClientDisconnected);
        
        {
            QMutexLocker locker(&m_clientsMutex);
            m_clients.append(handler);
        }
        LOG_I("[NET] New client connected from " << socket->peerAddress().toString().toStdString() 
                 << ". Total clients: " << m_clients.size());
    }
}

void ServerCore::broadcastPacket(int cmdId, const QJsonObject &data)
{
    QByteArray bytes = QJsonDocument(data).toJson(QJsonDocument::Compact);
    QMutexLocker locker(&m_clientsMutex);
    for (const auto &client : m_clients) {
        client->sendPacket(cmdId, bytes);
    }
}

void ServerCore::registerHandler(int cmdId, HandlerFunc handler)
{
    m_handlers[cmdId] = handler;
}

void ServerCore::onPacketReady(const Protocol::NetPacket &packet)
{
    LOG_I("[DISPATCH] Received Command: " << packet.cmdId << " Size: " << packet.length);

    ClientHandler *client = qobject_cast<ClientHandler*>(sender());
    if (!client) return;

    if (m_handlers.contains(packet.cmdId)) {
        // 解析 JSON 数据并分发给对应的 Handler (使用线程池)
        auto handler = m_handlers[packet.cmdId];
        m_threadPool->start([=]() {
            QJsonDocument doc = QJsonDocument::fromJson(packet.data);
            if (doc.isNull() && packet.cmdId != Protocol::CMD_HEARTBEAT) {
                LOG_W("[DISPATCH] Invalid JSON for command: " << packet.cmdId);
                return;
            }
            handler(client, doc.object());
        });
    } else {
        // 特殊处理心跳
        if (packet.cmdId == Protocol::CMD_HEARTBEAT) {
            client->sendPacket(Protocol::CMD_HEARTBEAT, "{\"status\":\"alive\"}");
        } else {
            LOG_W("[DISPATCH] No handler registered for command: " << packet.cmdId);
        }
    }
}

void ServerCore::onClientDisconnected()
{
    ClientHandler *handler = qobject_cast<ClientHandler*>(sender());
    if (handler) {
        {
            QMutexLocker locker(&m_clientsMutex);
            m_clients.removeAll(handler);
        }
        handler->deleteLater();
        LOG_I("[NET] Client disconnected. Remaining clients: " << m_clients.size());
    }
}
