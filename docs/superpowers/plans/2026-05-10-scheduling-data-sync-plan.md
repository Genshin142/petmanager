# Scheduling Data Sync Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement real-time client-server data synchronization for the Scheduling Module, allowing single-cell updates and batch template application to be persisted to the MySQL database via TCP commands.

**Architecture:** We are using the "Real-time Single-cell Sync" approach. The client will send `CMD_GET_SCHEDULE`, `CMD_UPDATE_SCHEDULE`, and `CMD_BATCH_UPDATE_SCHEDULE` over TCP. The server will use MySQL `ON DUPLICATE KEY UPDATE` to effortlessly handle upserts.

**Tech Stack:** C++17, Qt 6.x (QTcpSocket, QJsonDocument), MySQL (QSqlQuery)

---

### Task 1: Update Server Protocol and Controller Registration

**Files:**
- Modify: `protocol_codes.h`
- Modify: `server/network/server_core.cpp`

- [ ] **Step 1: Add command IDs to protocol_codes.h**

```cpp
        // 在 enum CommandId 中新增
        CMD_GET_SCHEDULE     = 4101,
        CMD_UPDATE_SCHEDULE  = 4102,
        CMD_BATCH_UPDATE_SCHEDULE = 4103,
```

- [ ] **Step 2: Register ScheduleController in ServerCore**

```cpp
// 引入头文件
#include "../logic/schedule_controller.h"

// 在 ServerCore 构造函数中初始化
new ScheduleController(this, this);
```

- [ ] **Step 3: Commit**

```bash
git add protocol_codes.h server/network/server_core.cpp
git commit -m "feat: add schedule sync protocol codes"
```

### Task 2: Implement Server-Side ScheduleController

**Files:**
- Create: `server/logic/schedule_controller.h`
- Create: `server/logic/schedule_controller.cpp`

- [ ] **Step 1: Create schedule_controller.h**

```cpp
#ifndef SCHEDULE_CONTROLLER_H
#define SCHEDULE_CONTROLLER_H

#include <QObject>
#include "../network/server_core.h"
#include "../network/client_handler.h"
#include <QJsonObject>
#include <QJsonArray>

class ScheduleController : public QObject
{
    QObject *parent;
public:
    explicit ScheduleController(ServerCore *core, QObject *parent = nullptr);

private:
    void handleGetSchedule(ClientHandler *client, const QJsonObject &body);
    void handleUpdateSchedule(ClientHandler *client, const QJsonObject &body);
    void handleBatchUpdateSchedule(ClientHandler *client, const QJsonObject &body);
    
    // 提取的公共逻辑：upsert 单条排班
    bool upsertSchedule(const QJsonObject &obj);
};

#endif // SCHEDULE_CONTROLLER_H
```

- [ ] **Step 2: Create schedule_controller.cpp**

```cpp
#include "schedule_controller.h"
#include "../database/connectionpool.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <glog/logging.h>

ScheduleController::ScheduleController(ServerCore *core, QObject *parent)
    : QObject(parent)
{
    core->registerHandler(Protocol::CMD_GET_SCHEDULE, [this](ClientHandler *client, const QJsonObject &body) {
        handleGetSchedule(client, body);
    });
    core->registerHandler(Protocol::CMD_UPDATE_SCHEDULE, [this](ClientHandler *client, const QJsonObject &body) {
        handleUpdateSchedule(client, body);
    });
    core->registerHandler(Protocol::CMD_BATCH_UPDATE_SCHEDULE, [this](ClientHandler *client, const QJsonObject &body) {
        handleBatchUpdateSchedule(client, body);
    });
}

void ScheduleController::handleGetSchedule(ClientHandler *client, const QJsonObject &body)
{
    QString startDate = body["start_date"].toString();
    QString endDate = body["end_date"].toString();
    
    QSqlDatabase db = ConnectionPool::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("SELECT s.emp_id, s.work_date, s.shift_type, s.plan_start, s.plan_end, e.username "
                  "FROM sys_schedules s JOIN sys_employees e ON s.emp_id = e.emp_id "
                  "WHERE s.work_date BETWEEN :start AND :end AND s.is_deleted = 0");
    query.bindValue(":start", startDate);
    query.bindValue(":end", endDate);

    QJsonObject response;
    if (query.exec()) {
        QJsonArray data;
        while (query.next()) {
            QJsonObject item;
            // 根据实际 emp_id 类型（如果是字符串如M00001，可能需要处理）
            // 假设数据库存的 emp_id 是 INT，但客户端用 "M0000X"
            int dbEmpId = query.value("emp_id").toInt();
            item["emp_id"] = QString("M%1").arg(dbEmpId, 5, 10, QChar('0'));
            item["work_date"] = query.value("work_date").toString();
            item["shift_type"] = query.value("shift_type").toString();
            item["plan_start"] = query.value("plan_start").isNull() ? "" : query.value("plan_start").toString();
            item["plan_end"] = query.value("plan_end").isNull() ? "" : query.value("plan_end").toString();
            data.append(item);
        }
        response["status"] = Protocol::STATUS_OK;
        response["data"] = data;
    } else {
        LOG_E("[DB] Get schedule failed: " << query.lastError().text().toStdString());
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = "Query failed";
    }
    ConnectionPool::instance().releaseConnection(db);
    client->sendPacket(Protocol::CMD_GET_SCHEDULE, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

bool ScheduleController::upsertSchedule(const QJsonObject &obj)
{
    QString empIdStr = obj["emp_id"].toString();
    int empId = empIdStr.mid(1).toInt(); // "M00001" -> 1
    QString workDate = obj["work_date"].toString();
    QString shiftType = obj["shift_type"].toString();
    QString planStart = obj["plan_start"].toString();
    QString planEnd = obj["plan_end"].toString();

    QSqlDatabase db = ConnectionPool::instance().getConnection();
    QSqlQuery query(db);
    
    // ON DUPLICATE KEY UPDATE 语法
    query.prepare("INSERT INTO sys_schedules (emp_id, work_date, shift_type, plan_start, plan_end) "
                  "VALUES (:emp_id, :work_date, :shift_type, :plan_start, :plan_end) "
                  "ON DUPLICATE KEY UPDATE "
                  "shift_type = VALUES(shift_type), plan_start = VALUES(plan_start), plan_end = VALUES(plan_end)");
    
    query.bindValue(":emp_id", empId);
    query.bindValue(":work_date", workDate);
    query.bindValue(":shift_type", shiftType);
    
    if (planStart.isEmpty()) query.bindValue(":plan_start", QVariant(QMetaType::fromType<QString>()));
    else query.bindValue(":plan_start", planStart);
    
    if (planEnd.isEmpty()) query.bindValue(":plan_end", QVariant(QMetaType::fromType<QString>()));
    else query.bindValue(":plan_end", planEnd);

    bool ok = query.exec();
    if (!ok) {
        LOG_E("[DB] Upsert schedule failed: " << query.lastError().text().toStdString());
    }
    ConnectionPool::instance().releaseConnection(db);
    return ok;
}

void ScheduleController::handleUpdateSchedule(ClientHandler *client, const QJsonObject &body)
{
    QJsonObject response;
    if (upsertSchedule(body)) {
        response["status"] = Protocol::STATUS_OK;
        response["message"] = "success";
    } else {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = "Database error";
    }
    client->sendPacket(Protocol::CMD_UPDATE_SCHEDULE, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void ScheduleController::handleBatchUpdateSchedule(ClientHandler *client, const QJsonObject &body)
{
    QJsonArray schedules = body["schedules"].toArray();
    bool allOk = true;
    for (int i = 0; i < schedules.size(); ++i) {
        if (!upsertSchedule(schedules[i].toObject())) {
            allOk = false;
        }
    }
    
    QJsonObject response;
    response["status"] = allOk ? Protocol::STATUS_OK : Protocol::STATUS_ERROR;
    response["message"] = allOk ? "success" : "Partial failure";
    client->sendPacket(Protocol::CMD_BATCH_UPDATE_SCHEDULE, QJsonDocument(response).toJson(QJsonDocument::Compact));
}
```

- [ ] **Step 3: Update database schema if needed**

Ensure `sys_schedules` has a UNIQUE KEY on `(emp_id, work_date)`. If not, we run a query to add it. You can do this via your database client or a small script.
```sql
ALTER TABLE sys_schedules ADD UNIQUE KEY idx_emp_date (emp_id, work_date);
```

- [ ] **Step 4: Commit**

```bash
git add server/logic/schedule_controller.*
git commit -m "feat: implement server ScheduleController with ON DUPLICATE KEY UPDATE"
```

### Task 3: Update Client ScheduleDataManager

**Files:**
- Modify: `modules/scheduledatamanager.h`
- Modify: `modules/scheduledatamanager.cpp`
- Modify: `mainwindow.cpp` (to initialize data)

- [ ] **Step 1: Declare request/response methods in scheduledatamanager.h**

```cpp
// Add public methods
void requestScheduleList(const QString &startDate, const QString &endDate);

// Add public slots
void onPacketReceived(const Protocol::NetPacket &packet);
```

- [ ] **Step 2: Implement network calls in scheduledatamanager.cpp**

```cpp
#include "../utils/networkmanager.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

// In constructor:
connect(&NetworkManager::instance(), &NetworkManager::packetReceived, this, &ScheduleDataManager::onPacketReceived);

// New methods:
void ScheduleDataManager::requestScheduleList(const QString &startDate, const QString &endDate)
{
    QJsonObject body;
    body["start_date"] = startDate;
    body["end_date"] = endDate;
    NetworkManager::instance().sendRequest(Protocol::CMD_GET_SCHEDULE, body);
}

void ScheduleDataManager::onPacketReceived(const Protocol::NetPacket &packet)
{
    if (packet.cmdId == Protocol::CMD_GET_SCHEDULE) {
        QJsonObject root = packet.jsonObj;
        if (root["status"].toInt() == Protocol::STATUS_OK) {
            QJsonArray data = root["data"].toArray();
            m_schedules.clear(); // Need to merge properly, but clear for now or iterate
            for (int i = 0; i < data.size(); ++i) {
                QJsonObject obj = data[i].toObject();
                ScheduleInfo info;
                info.empId = obj["emp_id"].toString();
                info.date = obj["work_date"].toString();
                
                QString typeStr = obj["shift_type"].toString();
                if (typeStr == "早班") info.type = SHIFT_MORNING;
                else if (typeStr == "晚班") info.type = SHIFT_EVENING;
                else if (typeStr == "自定义") info.type = SHIFT_CUSTOM;
                else info.type = SHIFT_OFF;
                
                info.startTime = obj["plan_start"].toString();
                info.endTime = obj["plan_end"].toString();
                
                // Assuming internal structure is a map or similar
                QString key = info.empId + "_" + info.date;
                m_schedules[key] = info;
            }
            emit scheduleDataChanged();
        }
    }
}
```

- [ ] **Step 3: Update `setSchedule` to send network requests**

```cpp
void ScheduleDataManager::setSchedule(const ScheduleInfo &info)
{
    // Local update
    QString key = info.empId + "_" + info.date;
    m_schedules[key] = info;
    
    // Network update
    QJsonObject obj;
    obj["emp_id"] = info.empId;
    obj["work_date"] = info.date;
    
    if (info.type == SHIFT_MORNING) obj["shift_type"] = "早班";
    else if (info.type == SHIFT_EVENING) obj["shift_type"] = "晚班";
    else if (info.type == SHIFT_CUSTOM) obj["shift_type"] = "自定义";
    else obj["shift_type"] = "休息";
    
    obj["plan_start"] = info.startTime;
    obj["plan_end"] = info.endTime;
    
    NetworkManager::instance().sendRequest(Protocol::CMD_UPDATE_SCHEDULE, obj);
}
```

- [ ] **Step 4: Update `saveSchedule` and `applyTemplate` logic for batch update**

Implement logic to collect multiple saves into `CMD_BATCH_UPDATE_SCHEDULE` if needed, or simply let it call `setSchedule` continuously (which uses `CMD_UPDATE_SCHEDULE`). In `ScheduleModule::onApplyTemplate`, it iterates and saves. To make it batch:

```cpp
void ScheduleDataManager::batchSaveSchedules(const QList<ScheduleInfo> &infos) {
    QJsonArray arr;
    for(const auto &info : infos) {
        QString key = info.empId + "_" + info.date;
        m_schedules[key] = info;
        
        QJsonObject obj;
        obj["emp_id"] = info.empId;
        obj["work_date"] = info.date;
        if (info.type == SHIFT_MORNING) obj["shift_type"] = "早班";
        else if (info.type == SHIFT_EVENING) obj["shift_type"] = "晚班";
        else if (info.type == SHIFT_CUSTOM) obj["shift_type"] = "自定义";
        else obj["shift_type"] = "休息";
        obj["plan_start"] = info.startTime;
        obj["plan_end"] = info.endTime;
        arr.append(obj);
    }
    QJsonObject body;
    body["schedules"] = arr;
    NetworkManager::instance().sendRequest(Protocol::CMD_BATCH_UPDATE_SCHEDULE, body);
}
```

- [ ] **Step 5: Init call in MainWindow or Module**

When `ScheduleModule` loads, it should request the data:
`ScheduleDataManager::instance()->requestScheduleList("2026-05-01", "2026-05-31");`

- [ ] **Step 6: Commit**

```bash
git add modules/scheduledatamanager.*
git commit -m "feat: connect ScheduleDataManager to network protocol"
```
