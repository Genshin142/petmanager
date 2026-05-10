# Appointment & Logistics Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the client-server data synchronization for the Appointment and Logistics Dispatch modules. Ensure that creating an appointment auto-generates a logistics task if transport is needed, and completing an appointment auto-generates a pending checkout order.

**Architecture:** 
- Centralized transaction control in the server's `AppointmentController`.
- Weekly caching on the client's `AppointmentDataManager`.

**Tech Stack:** C++17, Qt 6.x (QTcpSocket, QJsonDocument), MySQL (QSqlQuery, QSqlDatabase::transaction)

---

### Task 1: Protocol & Server Structure Setup

**Files:**
- Modify: `protocol_codes.h`
- Modify: `server/network/server_core.cpp`
- Modify: `server/CMakeLists.txt`

- [ ] **Step 1: Add new command IDs to protocol_codes.h**
Add the following commands to `Protocol::CommandId`:
```cpp
        // 预约与物流模块
        CMD_GET_APPOINTMENT_LIST = 5101,
        CMD_ADD_APPOINTMENT      = 5102,
        CMD_UPDATE_APPT_STATUS   = 5103,
        
        CMD_GET_LOGISTICS_LIST        = 5201,
        CMD_UPDATE_LOGISTICS_STATUS   = 5202,
```

- [ ] **Step 2: Add AppointmentController to CMakeLists.txt**
In `server/CMakeLists.txt`, add `logic/appointment_controller.cpp` to the `SOURCES` list.

- [ ] **Step 3: Register AppointmentController in ServerCore**
Create `new AppointmentController(this, this);` in the `ServerCore` constructor. Include the header.

- [ ] **Step 4: Commit**
```bash
git add .
git commit -m "feat: add protocol codes and register appointment controller"
```

---

### Task 2: Implement AppointmentController (Server-side)

**Files:**
- Create: `server/logic/appointment_controller.h`
- Create: `server/logic/appointment_controller.cpp`

- [ ] **Step 1: Create appointment_controller.h**
Declare the class inheriting `QObject`, registering handlers for 5101, 5102, 5103. Include `handleGetAppointments`, `handleAddAppointment`, `handleUpdateStatus`.

- [ ] **Step 2: Implement handleAddAppointment with DB Transaction**
Use `ConnectionPool::instance().openConnection()`.
Call `db.transaction()`.
Insert into `appointments`.
If `body["need_transport"].toBool()` is true, use `query.lastInsertId().toInt()` to insert a row into `logistics_tasks`.
If both succeed, call `db.commit()`. If error, call `db.rollback()`.

- [ ] **Step 3: Implement handleUpdateStatus with Order Auto-generation**
Check if `body["status"] == "已完成"`.
If yes, run a transaction:
1. Update `appointments` status.
2. Query `services` to get the base `price` (if missing from body).
3. Insert into `orders` with `source_module='Appointment'`, `related_id=appt_id`, `status='待支付'`.
Call `db.commit()`.

- [ ] **Step 4: Implement handleGetAppointments**
Query `appointments` for the `start_date` to `end_date` range and return as JSON array.

- [ ] **Step 5: Commit**
```bash
git add server/logic/appointment_controller.*
git commit -m "feat: implement AppointmentController with atomic DB transactions"
```

---

### Task 3: Client Data Manager Integration

**Files:**
- Modify: `modules/appointmentdatamanager.h`
- Modify: `modules/appointmentdatamanager.cpp`

- [ ] **Step 1: Connect networking in Constructor**
Listen to `NetworkManager::packetReceived`.

- [ ] **Step 2: Implement network requests**
Add `requestAppointments(const QDate &start, const QDate &end)`.
Add `addAppointment(const AppointmentInfo &info)`.
Add `updateAppointmentStatus(int apptId, const QString &status)`.

- [ ] **Step 3: Parse response in onPacketReceived**
On `CMD_GET_APPOINTMENT_LIST` success, parse the array, cache them in a `QMap<QDate, QList<AppointmentInfo>>` structure, and emit `appointmentDataChanged()`.
On `CMD_ADD_APPOINTMENT` / `CMD_UPDATE_APPT_STATUS` success, re-fetch the week's data.

- [ ] **Step 4: Commit**
```bash
git add modules/appointmentdatamanager.*
git commit -m "feat: hook up AppointmentDataManager to TCP protocol"
```
