#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <QtCore/QObject>
#include <QtNetwork/QTcpSocket>
#include "../../protocol_codes.h"

class ClientHandler : public QObject
{
    Q_OBJECT
public:
    explicit ClientHandler(QTcpSocket *socket, QObject *parent = nullptr);
    ~ClientHandler();

    // 发送数据包的便捷方法
    void sendPacket(int cmdId, const QByteArray &jsonBody);

    // 用户 ID 管理
    void setUserId(int id) { m_userId = id; }
    int userId() const { return m_userId; }

signals:
    // 当一个完整的包解析出来时抛出此信号
    void packetReady(const Protocol::NetPacket &packet);
    void disconnected();

private slots:
    void onReadyRead();

private:
    QTcpSocket *m_socket;
    QByteArray m_buffer; // 接收缓冲区，用于解决粘包/半包
    int m_userId = 0;    // 关联的员工 ID
};

#endif // CLIENT_HANDLER_H
