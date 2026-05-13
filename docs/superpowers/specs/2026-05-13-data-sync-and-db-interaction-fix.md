# 数据同步与数据库交互修复方案 (2026-05-13)

## 1. 核心目标
修复客户端与服务端在资产数据（库存、余额、订单）上的同步断层，确保所有核心业务操作（结账、入住、离店）均具备数据库持久化能力，并严格执行 FIFO 库存扣减。

## 2. 待修复模块与方案

### 2.1 财务结账与库存/余额联动 (核心优先级)
- **问题**：目前创建订单不扣库存、不扣会员余额。
- **方案**：
    - 在服务端 `OrderController::handleCreateOrder` 中启用事务。
    - **FIFO 逻辑实现**：
        1. 遍历订单项。
        2. 查询 `product_batches` 表，按 `expiry_date`（过期日期）升序排序。
        3. 逐个批次扣减 `current_qty`，直到满足需求。
        4. 同步更新 `products` 表的总库存。
    - **余额联动**：
        1. 如果是会员订单，更新 `members` 表中的 `balance = balance - actual_pay`。
        2. 更新 `consume_amt = consume_amt + actual_pay`。

### 2.2 寄养结账与财务同步
- **问题**：寄养离店 (`executeCheckOut`) 仅更新本地内存。
- **方案**：
    - 客户端 `PetDataManager::executeCheckOut` 发送 `CMD_CREATE_ORDER` 指令。
    - 服务端更新 `rooms` 状态为 `空闲` 或 `清理中`。
    - 服务端更新 `pets` 表的 `current_status = '空闲'`。

### 2.3 寄养/预约状态持久化
- **问题**：`executeCheckIn` 和 `executeBooking` 缺失网络请求。
- **方案**：
    - 客户端发送 `CMD_UPDATE_PET_STATUS` 指令。
    - 服务端更新 `pets` 表的 `room_no`、`foster_start_time`、`foster_end_time`。

### 2.4 库存批次持久化
- **问题**：批次管理仅在客户端内存。
- **方案**：
    - 补全服务端 `CMD_ADD_BATCH` 和 `CMD_GET_BATCHES` 接口。
    - 在入库上架时，自动向服务端写入批次数据。

## 3. 验证方案
- **库存验证**：下单购买 10 件商品，验证 `product_batches` 中最早到期的批次是否被优先清空。
- **余额验证**：会员余额 100 元，购买 20 元商品，验证数据库 `members` 表是否变为 80 元。
- **持久化验证**：重启客户端后，寄养房间的状态和宠物位置依然正确。
