# 排班管理中心数据同步架构设计 (Scheduling Data Sync Design)

## 1. 业务目标
打通 PetManager 客户端（`ScheduleModule`）与后端服务器之间的排班数据流，实现排班数据的云端持久化存储，并支持客户端通过现有的“无缝点击”操作进行实时更新。

## 2. 架构模式
采用 **“单点实时直连 (Real-time Single-cell Sync)”** 模式。
因为使用对象限定为“单一店长”，并发冲突压力极低，因此放弃防抖队列，每次格子状态改变时，立刻向服务器发送更新指令，服务端采用“有则更新，无则插入”的原子操作，保证操作极简且状态精确一致。

## 3. 通信协议设计 (TCP JSON)

在 `protocol_codes.h` 中新增以下指令 ID：
- `CMD_GET_SCHEDULE = 4101`: 获取排班表。
- `CMD_UPDATE_SCHEDULE = 4102`: 更新单条排班。
- `CMD_BATCH_UPDATE_SCHEDULE = 4103`: 批量更新排班（用于应用模板）。

### 3.1 获取排班 (CMD_GET_SCHEDULE)
**Request:**
```json
{
  "start_date": "2026-05-04",
  "end_date": "2026-05-10"
}
```
**Response:**
```json
{
  "status": 0,
  "data": [
    {
      "emp_id": "M00001",
      "work_date": "2026-05-04",
      "shift_type": "早班",
      "plan_start": "09:00",
      "plan_end": "18:00"
    }
  ]
}
```

### 3.2 单点更新 (CMD_UPDATE_SCHEDULE)
**Request:**
```json
{
  "emp_id": "M00001",
  "work_date": "2026-05-04",
  "shift_type": "晚班",
  "plan_start": "13:00",
  "plan_end": "22:00"
}
```
**Response:** `{ "status": 0, "message": "success" }`

### 3.3 批量更新 (CMD_BATCH_UPDATE_SCHEDULE)
**Request:**
```json
{
  "schedules": [
     { "emp_id": "M00001", "work_date": "2026-05-04", "shift_type": "早班", "plan_start": "09:00", "plan_end": "18:00" },
     ...
  ]
}
```

## 4. 客户端实现 (ScheduleDataManager)
1. **拉取逻辑 (`getScheduleList`)**: 客户端向服务器发送 `CMD_GET_SCHEDULE`，收到后解析 `packet.jsonObj`，更新本地 `m_schedules` (QMap 或 Hash 结构)，并发出 `scheduleDataChanged` 信号。
2. **单点更新 (`saveSchedule`)**: 用户点击单元格触发状态流转后，`ScheduleDataManager` 发送 `CMD_UPDATE_SCHEDULE` 请求。本地依然同步变色。
3. **批量更新 (`applyTemplate`)**: 当点击“应用排班模板”时，组装所有变动行，发送 `CMD_BATCH_UPDATE_SCHEDULE`，清空或覆盖相关天的缓存。

## 5. 服务端实现 (ScheduleController)
1. **路由注册**: 在 `ServerCore` 注册 4101, 4102, 4103 的 Handler。
2. **处理 4102/4103 更新逻辑**:
   核心 SQL 将采用 `ON DUPLICATE KEY UPDATE` 语法：
   ```sql
   INSERT INTO sys_schedules (emp_id, work_date, shift_type, plan_start, plan_end)
   VALUES (?, ?, ?, ?, ?)
   ON DUPLICATE KEY UPDATE
   shift_type = VALUES(shift_type), plan_start = VALUES(plan_start), plan_end = VALUES(plan_end)
   ```
   *注意：数据库的 `sys_schedules` 需要存在基于 `(emp_id, work_date)` 的 UNIQUE 索引，才能触发此语法。*

## 6. 边缘情况处理
- **网络延迟与断网**：客户端保持乐观更新（点击即变色）。若服务端返回错误或超时，当前架构接受弱一致性（重新拉取数据或刷新页面即可复原），不实现复杂的本地事务回滚。
- **休息日**：`shift_type` 为 "休息" 时，`plan_start` 与 `plan_end` 可传空字符串，服务端入库时存为 NULL。
