# PetManager 项目开发状态总结 (2026-05-09)

## 1. 核心架构说明
### 后端 (PetServer)
- **控制器模式**：采用模块化控制器架构。`ServerCore` 负责网络分发，具体业务由 `PetController`、`MemberController` 等处理。
- **通信协议**：基于 `Protocol::NetPacket`。
    - `CMD 2001`: 获取宠物列表。
    - `CMD 2002`: 获取会员列表。
- **数据库**：MySQL (petstore)，使用 `QSqlDatabase` 连接池。

### 前端 (PetManager)
- **数据管理层**：`PetDataManager` 和 `MemberDataManager` 采用单例模式，通过 `NetworkManager` 异步接收服务器推送的数据包。
- **UI 组件**：基于原生 Qt 6.8.2 控件美化，支持响应式布局、侧边详情抽屉。

## 2. 数据库状态 (MySQL)
- **当前库名**：`petstore`
- **关键表结构已升级**：
    - `members`: 包含 `gender`, `birthday`, `consume_amt`, `balance` 等。
    - `pets`: 包含 `age_months`, `health_status`, `join_time`, `avatar_path` 等。
- **维护工具**：
    - `db_fix_all.py`: 自动补全数据库字段（兼容旧版 MySQL）。
    - `seed_db.py`: 注入标准测试数据。

## 3. 已实现的功能模块
- **[宠物档案中心]**：全量网络同步，支持头像展示、关联主人跳转、时间格式化展示。
- **[会员信息管理]**：全量网络同步，支持业务 ID 格式化 (`M00001`)，具备数据空状态占位、启动自动选中首行。

## 4. 关键变量与常量定义
- **ID 规则**：
    - 会员：`M` + 5位数字 (如 `M00001`)。
    - 宠物：`P` + 5位数字 (如 `P00001`)。
- **协议状态**：`Protocol::STATUS_OK` (200), `Protocol::STATUS_ERROR` (400)。

## 5. 待办事项 (Next Steps)
1. **商品管理 (Inventory)**：目前为 Mock 数据，需实现 `ProductController` 及网络对接。
2. **预约服务 (Services)**：对接 `appointments` 表，实现服务排期逻辑。
3. **财务/订单 (Orders)**：实现订单流水记录与数据报表动态化。

## 6. 环境信息
- **Qt版本**: 6.8.2 MinGW 64-bit
- **数据库账号**: `root / 362345943`
- **项目路径**: `E:\QT\work\PetManager`
