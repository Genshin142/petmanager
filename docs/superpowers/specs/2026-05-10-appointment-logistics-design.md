# 预约服务与车辆调度 数据同步架构设计 (Appointment & Logistics Sync Design)

## 1. 业务目标
实现前台预约业务与车辆接送业务的数据闭环。通过“服务端集中处理事务，客户端按周缓存展示”的架构，确保多表关联数据（预约表、物流表、订单表）的原子性与一致性。

## 2. 核心架构与流转逻辑

### 2.1 车辆调度单底层自动触发
- **触发机制**：客户端在提交新建预约（`CMD_ADD_APPOINTMENT`）时，若数据包中包含接送地址（`address` 非空）且包含接送标识，服务端在持久化 `appointments` 表后，处于同一个 **SQL 事务 (Transaction)** 中，自动向 `logistics_tasks` 表插入关联数据。
- **一致性保障**：使用 `db.transaction()`。如果物流单生成失败，预约单将自动 `rollback()` 回滚，避免产生幽灵数据。

### 2.2 自动转入“待结算”池
- **触发机制**：店长在客户端将预约状态更新为 `已完成`（触发 `CMD_UPDATE_APPT_STATUS`）。
- **服务端处理**：`AppointmentController` 监听到状态变更后，除了更新 `appointments.status`，还要跨表查询服务的价格，并自动向 `orders` 表插入一条数据：
  - `source_module` = 'Appointment'
  - `related_id` = `appt_id`
  - `status` = '待支付'
- 这样店长去“财务结算中心”时，直接能看到这条待结算的订单。

### 2.3 “按周缓存，按天展示”机制
- 客户端的 `AppointmentDataManager` 会以周一至周日为区间，向服务端发送 `CMD_GET_APPOINTMENT_LIST`。
- 获取后保存在内存的 `QHash<QDate, QList<AppointmentInfo>>` 缓存池中。
- UI 点击周二时，不发网络请求，直接调用 `getDataManager()->getAppointmentsByDay(Tuesday)` 实现无缝切换渲染。

## 3. 网络通信协议 (TCP JSON)

将在 `protocol_codes.h` 新增：
- **预约类 (51xx)**:
  - `CMD_GET_APPOINTMENT_LIST = 5101`
  - `CMD_ADD_APPOINTMENT = 5102`
  - `CMD_UPDATE_APPT_STATUS = 5103`
- **物流类 (52xx)**:
  - `CMD_GET_LOGISTICS_LIST = 5201`
  - `CMD_UPDATE_LOGISTICS_STATUS = 5202`

### 3.1 客户端请求规范 (Req & Res)
**添加预约单 Request (CMD_ADD_APPOINTMENT)**:
```json
{
  "member_id": 1001,
  "pet_id": 2001,
  "service_id": 5,
  "staff_id": "E002",
  "appt_time": "2026-05-10 14:00:00",
  "need_transport": true,
  "address": "高新南区XXX单元",
  "amount": 128.00
}
```
*服务端响应:*
如果是 true，除了返回预约成功，底层会自动在 `logistics_tasks` 写一条状态为“待处理”的接宠单。

**更新预约状态 Request (CMD_UPDATE_APPT_STATUS)**:
```json
{
  "appt_id": 105,
  "status": "已完成"
}
```
*服务端处理:* 返回成功，并底层自动插入 `orders` 表。

## 4. 服务端目录规约
- 创建 `AppointmentController` 处理 51xx 协议。
- 创建 `LogisticsController` 处理 52xx 协议。
- 这两个 Controller 都将持有 `QSqlDatabase` 连接，并严格使用 `QSqlDatabase::transaction()` 与 `commit()`。
