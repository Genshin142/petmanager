#include "networkmanager.h"
#include <QtCore/QDataStream>
#include <QtCore/QThreadPool>
#include <QtCore/QMutexLocker>
#include <QtCore/QTimer>

NetworkManager::NetworkManager(QObject *parent) : QObject(parent)
{
    m_socket = new QTcpSocket(this);
    m_threadPool = new QThreadPool(this);
    m_threadPool->setMaxThreadCount(4);
    
    connect(m_socket, &QTcpSocket::connected, this, &NetworkManager::connected);
    connect(m_socket, &QTcpSocket::disconnected, this, &NetworkManager::disconnected);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), 
            this, &NetworkManager::onSocketError);
    connect(m_socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
}

NetworkManager& NetworkManager::instance()
{
    static NetworkManager manager;
    return manager;
}

void NetworkManager::connectToServer(const QString &host, quint16 port)
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
    }
    qDebug() << "[NET] Connecting to server:" << host << ":" << port;
    m_socket->connectToHost(host, port);
}

void NetworkManager::sendRequest(int cmdId, const QJsonObject &body)
{
    QByteArray jsonContent = QJsonDocument(body).toJson(QJsonDocument::Compact);
    QByteArray block;
    QDataStream ds(&block, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);

    unsigned int totalLength = Protocol::HEADER_SIZE + jsonContent.size();
    ds << totalLength;
    ds << (int)cmdId;
    block.append(jsonContent);

    // 确保异步写入，绝不阻塞当前线程
    QMetaObject::invokeMethod(this, [=]() {
        if (m_socket->state() == QAbstractSocket::ConnectedState) {
            m_socket->write(block);
            m_socket->flush();
            qDebug() << "[NET] >>> SENT CMD:" << cmdId << "at" << QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        } else {
            qWarning() << "[NET] Cannot send: Socket not connected. CMD:" << cmdId;
        }
    }, Qt::QueuedConnection);
}

void NetworkManager::sendRequest(int cmdId, const QJsonObject &body, std::function<void(const QJsonObject&)> callback)
{
    {
        QMutexLocker locker(&m_callbackMutex);
        m_callbacks[cmdId].append(callback);
    }
    sendRequest(cmdId, body);
}

void NetworkManager::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    while (true) {
        if (m_buffer.size() < Protocol::HEADER_SIZE) break;

        QDataStream ds(m_buffer);
        ds.setByteOrder(QDataStream::BigEndian);
        
        unsigned int totalLength;
        ds >> totalLength;

        if ((unsigned int)m_buffer.size() < totalLength) break;

        int cmdId;
        ds >> cmdId;

        int bodyLen = (int)totalLength - Protocol::HEADER_SIZE;
        QByteArray bodyData = m_buffer.mid(Protocol::HEADER_SIZE, bodyLen);
        
        Protocol::NetPacket packet;
        packet.length = totalLength;
        packet.cmdId = cmdId;
        packet.data = bodyData;

        // 立即解析 JSON（在主线程或后台线程均可，这里为了防止卡顿选后台）
        m_threadPool->start([=]() {
            QJsonDocument doc = QJsonDocument::fromJson(bodyData);
            Protocol::NetPacket parsedPacket = packet;
            parsedPacket.jsonObj = doc.object();
            
            // 回到主线程分发数据和回调
            QMetaObject::invokeMethod(this, [=]() {
                // 1. 触发回调
                QList<std::function<void(const QJsonObject&)>> toCall;
                {
                    QMutexLocker locker(&m_callbackMutex);
                    if (m_callbacks.contains(cmdId)) {
                        toCall = m_callbacks.take(cmdId);
                    }
                }
                
                for (auto &cb : toCall) {
                    cb(parsedPacket.jsonObj);
                }

                // 2. 发射信号
                emit packetReceived(parsedPacket);

                if (cmdId == Protocol::CMD_LOGIN) {
                    QJsonObject resp = parsedPacket.jsonObj;
                    emit loginResponse(resp["status"].toInt(), 
                                       resp["message"].toString(), 
                                       resp["user_info"].toObject());
                }
            }, Qt::QueuedConnection);
        });

        m_buffer.remove(0, (int)totalLength);
    }
}

void NetworkManager::onSocketError()
{
    QString errorMsg = m_socket->errorString();
    qWarning() << "[NET] Socket Error:" << errorMsg;
    emit errorOccurred(errorMsg);
}
