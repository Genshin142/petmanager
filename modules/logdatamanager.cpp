#include "logdatamanager.h"
#include "../utils/networkmanager.h"
#include "../protocol_codes.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <QDateTime>

// 初始化静态成员
QString LogDataManager::s_currentUser = "";

LogDataManager::LogDataManager(QObject *parent) : QObject(parent) {
    connect(&NetworkManager::instance(), &NetworkManager::packetReceived,
            this, &LogDataManager::onPacketReceived);
}

void LogDataManager::setCurrentUser(const QString &username) {
    s_currentUser = username;
}

void LogDataManager::writeLog(const QString &module, const QString &action, const QString &details) {
    QJsonObject body;
    body["operatorName"] = s_currentUser.isEmpty() ? "未知操作人" : s_currentUser;
    body["module"] = module;
    body["action"] = action;
    body["details"] = details;
    
    qDebug() << "[LOG] Sending log to server:" << s_currentUser << "->" << module << ":" << action;
    NetworkManager::instance().sendRequest(Protocol::CMD_ADD_LOG, body);
}

void LogDataManager::writeLog(const QString &module, const QString &action, const QJsonObject &diffDetails) {
    QJsonDocument doc(diffDetails);
    QString jsonStr = doc.toJson(QJsonDocument::Compact);
    writeLog(module, action, jsonStr);
}

void LogDataManager::fetchLogs(int limit, int offset, const QString &startDate, const QString &endDate, const QString &operatorName, const QString &module) {
    QJsonObject body;
    body["limit"] = limit;
    body["offset"] = offset;
    body["startDate"] = startDate;
    body["endDate"] = endDate;
    body["operatorName"] = operatorName;
    body["module"] = module;
    
    NetworkManager::instance().sendRequest(Protocol::CMD_GET_LOG_LIST, body);
}

void LogDataManager::onPacketReceived(const Protocol::NetPacket &packet) {
    if (packet.cmdId == Protocol::CMD_GET_LOG_LIST) {
        QJsonObject root = packet.jsonObj;
        
        QList<SysOperationLog> logs;
        int totalCount = 0;
        
        if (root["status"].toInt() == Protocol::STATUS_OK) {
            QJsonArray arr = root["data"].toArray();
            totalCount = root["totalCount"].toInt();
            
            for (int i = 0; i < arr.size(); ++i) {
                QJsonObject obj = arr[i].toObject();
                SysOperationLog log;
                log.id = obj["id"].toString();
                log.operatorName = obj["operatorName"].toString();
                log.module = obj["module"].toString();
                log.action = obj["action"].toString();
                log.details = obj["details"].toString();
                log.timestamp = obj["timestamp"].toString();
                logs.append(log);
            }
        }
        
        emit logsReceived(logs, totalCount);
    }
}
