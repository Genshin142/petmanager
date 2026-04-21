# 宠物与寄养数据统一同步设计文档 (2026-04-20)

## 1. 目标 (Goal)
实现宠物档案模块（PetModule）与寄养房态模块（FosterModule）之间的数据深度同步。消除目前两个模块中 Mock 数据的割裂感，确保房间号、寄养状态、行为日志及影像资料在全系统内实时一致。

## 2. 核心架构：PetDataManager (单例)

### 2.1 职责
- 存储全量宠物信息 (`PetInfo`)、日志 (`PetActivityLog`)、影像 (`PetMedia`)。
- 提供 CRUD 接口供各模块调用。
- 发射信号通知各模块进行 UI 局部或整体刷新。

### 2.2 关键数据结构
```cpp
class PetDataManager : public QObject {
    Q_OBJECT
private:
    QMap<QString, PetInfo> m_pets;               // PetId -> Info
    QMap<QString, QList<PetActivityLog>> m_logs;  // PetId -> Logs
    QMap<QString, QList<PetMedia>> m_media;       // PetId -> Media
public:
    static PetDataManager* instance();
    
    // 接口
    void updatePet(const PetInfo &info);
    void addActivityLog(const QString &petId, const PetActivityLog &log);
    void addMedia(const QString &petId, const PetMedia &media);
    
    QList<PetInfo> allPets() const;
    PetInfo getPet(const QString &id) const;
    
signals:
    void petDataChanged(const QString &petId);   // 特定宠物更新
    void globalDataChanged();                    // 列表/房态大幅变更
};
```

## 3. 模块适配逻辑

### 3.1 寄养模块 (FosterModule)
- **取消随机生成**：`onForecastDateChanged` 不再使用 `QRandomGenerator` 生成随机宠物。
- **房态渲染**：遍历 `allPets()`，若宠物 `status == "寄养中"` 且 `roomNo` 为当前房间号，则该房间显示为“占用”并关联该宠物。
- **操作同步**：在寄养看板点击“录入影像/日志”时，调用 `PetDataManager::addActivityLog/Media`，这会自动触发信号。

### 3.2 宠物模块 (PetModule)
- **初始化**：从 `PetDataManager` 获取初始列表进行 `addPetRow`。
- **状态联动**：修改宠物档案中的房号或状态后，调用 `PetDataManager::updatePet`，寄养看板会因收到信号而重绘。

## 4. 实施计划 (Implementation Plan)
1. **[NEW]** 创建 `modules/petdatamanager.h/cpp`。
2. **[MODIFY]** 迁移 `PetModule` 中的 `addDemo` 逻辑至 `PetDataManager` 初始化函数中。
3. **[MODIFY]** 重构 `PetModule` 的数据加载与更新逻辑，对接 `PetDataManager`。
4. **[MODIFY]** 重构 `FosterModule` 的看板渲染逻辑，基于真实宠物数据展示房态。
5. **[MODIFY]** 统一 `MediaUploadDialog` 和 `PetRecordDrawer` 的保存逻辑，调用中央接口。

## 5. 验证标准 (Success Criteria)
- 在“宠物档案”修改某只宠物的房号为 105，切换到“寄养看板”看到 Room 105 立即变为该宠物。
- 在“寄养看板”为某宠物录入一张洗澡照片，点击“宠物档案”查看该宠物全纪录，能即时看到新增的照片。
