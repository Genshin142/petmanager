#ifndef FINANCE_CONTROLLER_H
#define FINANCE_CONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include "../network/client_handler.h"

class ServerCore;

class FinanceController : public QObject {
    Q_OBJECT
public:
    explicit FinanceController(ServerCore* core, QObject* parent = nullptr);

    void handleGetPerformanceList(ClientHandler* client, const QJsonObject& data);
    void handleVerifyPerformance(ClientHandler* client, const QJsonObject& data);
    void handleGetSalaryList(ClientHandler* client, const QJsonObject& data);
    void handleApproveSalary(ClientHandler* client, const QJsonObject& data);
    void handlePaySalary(ClientHandler* client, const QJsonObject& data);

private:
    ServerCore* m_core;
};

#endif // FINANCE_CONTROLLER_H
