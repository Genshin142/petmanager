#ifndef PRODUCT_CONTROLLER_H
#define PRODUCT_CONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include "../network/client_handler.h"

class ServerCore;

class ProductController : public QObject
{
    Q_OBJECT
public:
    explicit ProductController(ServerCore *server, QObject *parent = nullptr);

    // 路由分发映射
    void handleRequest(int cmdId, ClientHandler* client, const QJsonObject &data);

private:
    void handleGetProductList(ClientHandler* client, const QJsonObject &data);
    void handleGetInboundList(ClientHandler* client, const QJsonObject &data);
    void handleShelveProduct(ClientHandler* client, const QJsonObject &data);
    
    ServerCore *m_server;
};

#endif // PRODUCT_CONTROLLER_H
