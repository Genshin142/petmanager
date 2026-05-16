#include "report_controller.h"
#include "../network/server_core.h"
#include "../database/connectionpool.h"
#include "../../protocol_codes.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QJsonDocument>
#include "../logger_compat.h"

ReportController::ReportController(ServerCore *core, QObject *parent)
    : QObject(parent), m_core(core)
{
    m_core->registerHandler(Protocol::CMD_GET_STATS_DASHBOARD, std::bind(&ReportController::handleGetDashboardStats, this, std::placeholders::_1, std::placeholders::_2));
    m_core->registerHandler(Protocol::CMD_GET_STATS_REVENUE, std::bind(&ReportController::handleGetRevenueTrend, this, std::placeholders::_1, std::placeholders::_2));
}

void ReportController::handleGetDashboardStats(ClientHandler *client, const QJsonObject &data)
{
    Q_UNUSED(data);
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    QJsonObject result;
    
    // 1. 核心指标 (总营收、订单量、客单价)
    if (query.exec("SELECT SUM(actual_pay), COUNT(*) FROM orders WHERE status = 'Paid'")) {
        if (query.next()) {
            double total = query.value(0).toDouble();
            int count = query.value(1).toInt();
            result["totalRevenue"] = total;
            result["totalOrders"] = count;
            result["avgOrder"] = count > 0 ? total / count : 0;
        }
    }

    // 2. 营收项目构成
    QJsonArray finComp;
    if (query.exec("SELECT source_module, SUM(actual_pay) FROM orders WHERE status = 'Paid' GROUP BY source_module")) {
        while (query.next()) {
            QJsonObject item;
            item["name"] = query.value(0).toString();
            item["value"] = query.value(1).toDouble();
            finComp.append(item);
        }
    }
    result["finComp"] = finComp;

    // 3. 支付渠道分布
    QJsonArray payComp;
    if (query.exec("SELECT payment_method, SUM(actual_pay) FROM orders WHERE status = 'Paid' GROUP BY payment_method")) {
        while (query.next()) {
            QJsonObject item;
            item["name"] = query.value(0).toString();
            item["value"] = query.value(1).toDouble();
            payComp.append(item);
        }
    }
    result["payComp"] = payComp;

    QJsonObject response;
    response["status"] = Protocol::STATUS_OK;
    response["data"] = result;
    
    client->sendPacket(Protocol::CMD_GET_STATS_DASHBOARD, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void ReportController::handleGetRevenueTrend(ClientHandler *client, const QJsonObject &data)
{
    QString range = data["range"].toString(); // "year", "month", "all_years"
    int year = data["year"].toInt();
    int month = data["month"].toInt();
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    QString sql;
    if (range == "all_years") {
        sql = "SELECT DATE_FORMAT(created_at, '%Y年') as label, SUM(actual_pay) FROM orders WHERE status = 'Paid' GROUP BY label ORDER BY label DESC LIMIT 5";
    } else if (range == "year") {
        sql = QString("SELECT DATE_FORMAT(created_at, '%%M月') as label, SUM(actual_pay) FROM orders WHERE status = 'Paid' AND YEAR(created_at) = %1 GROUP BY label ORDER BY MONTH(created_at)").arg(year);
    } else { // month
        sql = QString("SELECT DATE_FORMAT(created_at, '%%Y-%%m-%%d') as label, SUM(actual_pay) FROM orders WHERE status = 'Paid' AND YEAR(created_at) = %1 AND MONTH(created_at) = %2 GROUP BY label ORDER BY label").arg(year).arg(month);
    }

    QJsonArray trendData;
    if (query.exec(sql)) {
        while (query.next()) {
            QJsonObject item;
            item["label"] = query.value(0).toString();
            item["value"] = query.value(1).toDouble();
            trendData.append(item);
        }
    }

    QJsonObject response;
    response["status"] = Protocol::STATUS_OK;
    response["data"] = trendData;
    
    client->sendPacket(Protocol::CMD_GET_STATS_REVENUE, QJsonDocument(response).toJson(QJsonDocument::Compact));
}
