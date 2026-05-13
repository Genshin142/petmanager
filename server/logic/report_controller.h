#ifndef REPORT_CONTROLLER_H
#define REPORT_CONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include "../network/client_handler.h"

class ServerCore;

class ReportController : public QObject {
    Q_OBJECT
public:
    explicit ReportController(ServerCore* core, QObject* parent = nullptr);

    void handleGetDashboardStats(ClientHandler* client, const QJsonObject& data);
    void handleGetRevenueTrend(ClientHandler* client, const QJsonObject& data);

private:
    ServerCore* m_core;
};

#endif // REPORT_CONTROLLER_H
