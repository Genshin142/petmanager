#ifndef ORDER_CONTROLLER_H
#define ORDER_CONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QtSql/QSqlDatabase>
#include "../network/server_core.h"

class OrderController : public QObject
{
    Q_OBJECT
public:
    explicit OrderController(ServerCore *server, QObject *parent = nullptr);

    // 处理获取订单列表 (CMD 3003)
    void handleGetOrderList(ClientHandler *client, const QJsonObject &data);
    
    // 处理创建订单 (CMD 3001)
    void handleCreateOrder(ClientHandler *client, const QJsonObject &data);
    
    // 处理更新订单 (CMD 3004)
    void handleUpdateOrder(ClientHandler *client, const QJsonObject &data);
    
    // 处理作废订单 (CMD 3005)
    void handleCancelOrder(ClientHandler *client, const QJsonObject &data);

private:
    ServerCore *m_server;
    void deductProductStock(QSqlDatabase &db, const QString &itemDetailsJson);
};

#endif // ORDER_CONTROLLER_H
