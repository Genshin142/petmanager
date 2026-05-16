#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QDebug>
#include <QtCore/QMutex>
#include <functional>
#include "../protocol_codes.h"

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    static NetworkManager& instance();
    
    // 连接服务器 (通常在程序启动时或登录前调用)
    void connectToServer(const QString &host = "127.0.0.1", quint16 port = 8080);
    
    // 发送请求的基本方法
    void sendRequest(int cmdId, const QJsonObject &body);
    
    // 带回调的发送方法 (目前基于 cmdId 简单匹配)
    void sendRequest(int cmdId, const QJsonObject &body, std::function<void(const QJsonObject&)> callback);

    // 检查连接状态
    bool isConnected() const { return m_socket->state() == QAbstractSocket::ConnectedState; }

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &error);
    
    // 收到任何完整包时的通用信号
    void packetReceived(const Protocol::NetPacket &packet);
    
    // 业务特化信号：登录响应
    void loginResponse(int status, const QString &message, const QJsonObject &userInfo);

private slots:
    void onReadyRead();
    void onSocketError();

private:
    explicit NetworkManager(QObject *parent = nullptr);
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

    QTcpSocket *m_socket;
    QByteArray m_buffer;
    class QThreadPool *m_threadPool;
    QMap<int, QList<std::function<void(const QJsonObject&)>>> m_callbacks;
    QMutex m_callbackMutex;
};

#endif // NETWORKMANAGER_H
