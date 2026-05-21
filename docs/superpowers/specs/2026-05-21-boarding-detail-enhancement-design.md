# 2026-05-21 服务预约界面寄养详情信息展示增强设计方案

## 1. 目标与背景
当前在服务预约界面的“预约管理中心”，当用户选中一个“寄养 (Boarding)”类型的预约单时，右侧的“预约详情”抽屉中仅展示了宠物档案、主客关系，以及预约详情里的“房间号”。
为了提供更完整、直观的寄养单据详情展示，需要在此抽屉中补充展示寄养核心的三个关键属性：
1. **入住日期** (Check-in Date)
2. **离店日期** (Check-out Date)
3. **总天数** (Total Duration Days)

这有助于商家和前台服务人员快速了解寄养周期，避免频繁切换回寄养管理中心查看详情。

## 2. 设计方案
在 `AppointmentDetailDrawer::setAppointment` 函数中，对“预约详情”卡片的数据渲染逻辑进行扩展。

### 2.1 涉及数据结构与字段
在 `AppointmentInfo` 结构体中，这些属性均已包含且已在网络层和数据库层完成同步：
- `m_currentInfo.date` (QString) — 预约/入住日期，格式为 `yyyy-MM-dd`
- `m_currentInfo.boardingEndDate` (QString) — 离店日期，格式为 `yyyy-MM-dd`
- `m_currentInfo.duration` (int) — 寄养总天数
- `m_currentInfo.roomNo` (QString) — 房位编号

### 2.2 界面扩展逻辑
修改 `modules/appointmentdetaildrawer.cpp` 的 `setAppointment` 函数。
在 `m_currentInfo.type == "Boarding"` 的分支，将原来的单行添加修改为四行：

```cpp
else if (m_currentInfo.type == "Boarding") {
    addInfoRow(g, currentRow++, "房间号", m_currentInfo.roomNo.isEmpty() ? "待分配" : (m_currentInfo.roomNo + "号房"));
    addInfoRow(g, currentRow++, "入住日期", m_currentInfo.date.isEmpty() ? "--" : m_currentInfo.date);
    addInfoRow(g, currentRow++, "离店日期", m_currentInfo.boardingEndDate.isEmpty() ? "--" : m_currentInfo.boardingEndDate);
    addInfoRow(g, currentRow++, "总天数", m_currentInfo.duration > 0 ? (QString::number(m_currentInfo.duration) + " 天") : "--");
}
```

- **房间号**：若为空，则显示“待分配”；否则追加“号房”字样，如“101号房”。
- **入住日期**：即预约的起始时间。
- **离店日期**：寄养预订的截止日期。
- **总天数**：根据天数值动态拼接“ 天”。

这样能确保与系统其他寄养相关的视觉设计与语言一致，且能够整齐排列在“预约详情”卡片中。

## 3. 验证计划
1. **编译运行客户端**：使用 MinGW 进行完整增量编译，确保无语法与链接错误。
2. **UI 功能核对**：
   - 进入“服务预约”中心。
   - 点击一个“寄养”类型的预约。
   - 检查右侧详情面板，确认“预约详情”卡片内正确按顺序渲染出：
     - **房间号**
     - **入住日期**
     - **离店日期**
     - **总天数**
   - 检查非寄养类型预约（如洗护、美容），确认其预约详情不受影响。
