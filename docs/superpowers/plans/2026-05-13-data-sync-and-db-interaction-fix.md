# 数据同步与数据库交互修复实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 打通客户端与服务端在结账、库存、会员余额及寄养状态上的数据闭环，确保 FIFO 库存扣减和余额同步。

**Architecture:** 
- 服务端：`OrderController` 增加事务处理，实现 FIFO 库存扣减逻辑。
- 客户端：`PetDataManager` 补全网络请求触发器。

**Tech Stack:** Qt 6.8.2, C++17, MySQL, QSqlDatabase Transactions.

---

### Task 1: 协议号补全与服务端基础接口
**Files:**
- Modify: `protocol_codes.h`
- Modify: `server/logic/pet_controller.h/cpp`

- [ ] **Step 1: 在 protocol_codes.h 中添加新指令**
```cpp
// Add to protocol_codes.h
static const int CMD_UPDATE_PET_STATUS = 2005; 
static const int CMD_ADD_BATCH = 3005;
static const int CMD_GET_BATCHES = 3006;
```
- [ ] **Step 2: 在 PetController 中注册状态更新处理函数**
```cpp
// server/logic/pet_controller.cpp
m_core->registerHandler(Protocol::CMD_UPDATE_PET_STATUS, [this](ClientHandler *client, const QJsonObject &body) {
    handleUpdatePetStatus(client, body);
});
```
- [ ] **Step 3: 实现 handleUpdatePetStatus**
```cpp
void PetController::handleUpdatePetStatus(ClientHandler *client, const QJsonObject &body) {
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("UPDATE pets SET current_status = ?, room_no = ?, foster_start_time = ?, foster_end_time = ? WHERE pet_id = ?");
    query.bindValue(0, body["status"].toString());
    query.bindValue(1, body["room_no"].toString());
    query.bindValue(2, body["start_time"].toString());
    query.bindValue(3, body["end_time"].toString());
    query.bindValue(4, body["pet_id"].toString().mid(1).toInt());
    
    QJsonObject res;
    if (query.exec()) {
        res["status"] = Protocol::STATUS_OK;
        m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, QJsonObject{{"module", "pet"}});
    } else {
        res["status"] = Protocol::STATUS_ERROR;
    }
    client->sendPacket(Protocol::CMD_UPDATE_PET_STATUS, QJsonDocument(res).toJson());
    ConnectionPool::instance().closeConnection(db);
}
```

---

### Task 2: 服务端 FIFO 库存扣减与余额联动
**Files:**
- Modify: `server/logic/order_controller.cpp`

- [ ] **Step 1: 修改 handleCreateOrder 启用事务并实现 FIFO**
```cpp
// server/logic/order_controller.cpp 核心逻辑替换
db.transaction();
try {
    // 1. 插入订单 (已有逻辑)
    // 2. FIFO 扣减库存
    QJsonArray items = QJsonDocument::fromJson(data["itemDetails"].toString().toUtf8()).array();
    for(int i=0; i<items.size(); ++i) {
        QJsonObject item = items[i].toObject();
        QString barcode = item["barcode"].toString();
        int needed = item["count"].toInt();
        
        QSqlQuery batchQuery(db);
        batchQuery.prepare("SELECT batch_id, current_qty FROM product_batches WHERE product_id = (SELECT product_id FROM products WHERE barcode = ?) AND current_qty > 0 ORDER BY expiry_date ASC FOR UPDATE");
        batchQuery.addBindValue(barcode);
        if(batchQuery.exec()) {
            while(batchQuery.next() && needed > 0) {
                QString bid = batchQuery.value(0).toString();
                int available = batchQuery.value(1).toInt();
                int take = qMin(needed, available);
                
                QSqlQuery updateBatch(db);
                updateBatch.prepare("UPDATE product_batches SET current_qty = current_qty - ? WHERE batch_id = ?");
                updateBatch.addBindValue(take);
                updateBatch.addBindValue(bid);
                updateBatch.exec();
                
                needed -= take;
            }
        }
        if(needed > 0) throw std::runtime_error("Insufficient stock for " + barcode.toStdString());
    }
    
    // 3. 更新会员余额
    if (!data["memberId"].toString().isEmpty()) {
        QSqlQuery memQuery(db);
        memQuery.prepare("UPDATE members SET balance = balance - ?, consume_amt = consume_amt + ? WHERE member_id = ?");
        memQuery.addBindValue(data["finalAmount"].toDouble());
        memQuery.addBindValue(data["finalAmount"].toDouble());
        memQuery.addBindValue(data["memberId"].toString().mid(1).toInt());
        if(!memQuery.exec()) throw std::runtime_error("Update member balance failed");
    }
    db.commit();
} catch (...) {
    db.rollback();
    // 发送错误响应
}
```

---

### Task 3: 客户端寄养业务逻辑同步
**Files:**
- Modify: `modules/petdatamanager.cpp`

- [ ] **Step 1: 在 executeCheckIn 中发送状态同步请求**
```cpp
void PetDataManager::executeCheckIn(...) {
    // ... 原有内存逻辑 ...
    QJsonObject body;
    body["pet_id"] = petId;
    body["status"] = "寄养中";
    body["room_no"] = QString::number(roomId);
    body["start_time"] = start.toString("yyyy-MM-dd");
    body["end_time"] = end.toString("yyyy-MM-dd");
    NetworkManager::instance().sendRequest(Protocol::CMD_UPDATE_PET_STATUS, body);
}
```
- [ ] **Step 2: 修正 executeCheckOut 触发服务端订单创建**
```cpp
void PetDataManager::executeCheckOut(...) {
    // ... 原有逻辑 ...
    QJsonObject order;
    order["id"] = "ORD-BO-" + QDateTime::currentDateTime().toString("yyyyMMddHHmmss");
    order["sourceModule"] = "Boarding";
    order["memberId"] = info.ownerId;
    order["totalAmount"] = totalAmount;
    order["finalAmount"] = totalAmount;
    order["payMethod"] = paymentMethod;
    order["status"] = "已支付";
    NetworkManager::instance().sendRequest(Protocol::CMD_CREATE_ORDER, order);
}
```

---

### Task 4: 验证与测试
- [ ] **Step 1: 验证 FIFO 扣减**
测试步骤：入库两批次商品（过期日期分别为 2026-06 和 2026-12），下单购买。检查数据库 `product_batches` 确保 2026-06 批次先被消耗。
- [ ] **Step 2: 验证会员余额**
测试步骤：使用余额充裕的会员账号下单，检查 `members` 表余额是否减少，累计消费是否增加。
- [ ] **Step 3: 验证寄养状态**
测试步骤：在客户端办理入住，重启客户端，验证房间状态是否保持“入住中”。
