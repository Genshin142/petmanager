#include "networkmanager.h"
#include <QtCore/QDataStream>
#include <QtCore/QThreadPool>

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
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "[NET] Cannot send request: Not connected.";
        return;
    }

    QByteArray jsonContent = QJsonDocument(body).toJson(QJsonDocument::Compact);
    QByteArray block;
    QDataStream ds(&block, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);

    unsigned int totalLength = Protocol::HEADER_SIZE + jsonContent.size();
    
    // 写入 4字节总长度 + 4字节指令ID
    ds << totalLength;
    ds << cmdId;
    block.append(jsonContent);

    m_socket->write(block);
    m_socket->flush();
    qDebug() << "[NET] Sent CMD:" << cmdId << "Body Size:" << jsonContent.size();
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

        // 在后台线程池中解析并发出信号
        m_threadPool->start([=]() {
            emit packetReceived(packet);

            if (cmdId == Protocol::CMD_LOGIN) {
                QJsonDocument doc = QJsonDocument::fromJson(bodyData);
                QJsonObject resp = doc.object();
                emit loginResponse(resp["status"].toInt(), 
                                   resp["message"].toString(), 
                                   resp["user_info"].toObject());
            }
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
