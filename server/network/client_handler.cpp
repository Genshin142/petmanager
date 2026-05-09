#include "client_handler.h"
#include <QtCore/QDataStream>
#include <QtCore/QDebug>

ClientHandler::ClientHandler(QTcpSocket *socket, QObject *parent)
    : QObject(parent), m_socket(socket)
{
    connect(m_socket, &QTcpSocket::readyRead, this, &ClientHandler::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientHandler::disconnected);
}

ClientHandler::~ClientHandler()
{
    if (m_socket) {
        m_socket->deleteLater();
    }
}

void ClientHandler::sendPacket(int cmdId, const QByteArray &jsonBody)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;

    QByteArray block;
    QDataStream ds(&block, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);

    unsigned int totalLength = Protocol::HEADER_SIZE + jsonBody.size();
    
    // 写入 4字节长度 + 4字节指令ID + 包体
    ds << totalLength;
    ds << cmdId;
    block.append(jsonBody);

    m_socket->write(block);
    m_socket->flush();
}

void ClientHandler::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    while (true) {
        // 1. 检查包头是否完整 (至少8字节)
        if (m_buffer.size() < Protocol::HEADER_SIZE) break;

        // 2. 解析包头中的总长度
        QDataStream ds(m_buffer);
        ds.setByteOrder(QDataStream::BigEndian);
        
        unsigned int totalLength;
        ds >> totalLength;

        // 3. 安全检查：防止恶意超大包导致内存溢出 (限制为 10MB)
        if (totalLength > 10 * 1024 * 1024 || totalLength < Protocol::HEADER_SIZE) {
            qWarning() << "[NET] Malformed packet received, closing connection. Size:" << totalLength;
            m_socket->disconnectFromHost();
            return;
        }

        // 4. 检查缓冲区内是否有足够的一个完整包
        if ((unsigned int)m_buffer.size() < totalLength) break;

        // 5. 提取指令 ID 和 包体
        int cmdId;
        ds >> cmdId;

        int bodyLength = (int)totalLength - Protocol::HEADER_SIZE;
        QByteArray bodyData = m_buffer.mid(Protocol::HEADER_SIZE, bodyLength);

        // 6. 组装包并发出信号
        Protocol::NetPacket packet;
        packet.length = totalLength;
        packet.cmdId = cmdId;
        packet.data = bodyData;

        emit packetReady(packet);

        // 7. 从缓冲区中移除已处理的数据
        m_buffer.remove(0, totalLength);
    }
}
