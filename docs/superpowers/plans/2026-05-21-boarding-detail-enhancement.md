# 寄养详情显示增强 (Boarding Detail Drawer Enhancement) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在服务预约管理中心的右侧详情抽屉中，为“寄养”类型的预约单补充渲染展示房间号、入住日期、离店日期和总天数。

**Architecture:** 通过修改 `AppointmentDetailDrawer::setAppointment` 中 `m_currentInfo.type == "Boarding"` 的网格布局构建分支，渲染绑定 `AppointmentInfo` 中已有的 `roomNo`、`date`、`boardingEndDate` 和 `duration` 数据字段，保持与其他界面模块的汉化风格统一。

**Tech Stack:** C++, Qt 6.8 (Widgets, QGridLayout)

---

### Task 1: 详情抽屉 Boarding 渲染逻辑扩展

**Files:**
- Modify: `modules/appointmentdetaildrawer.cpp:462-463`

- [ ] **Step 1: 修改 appointmentdetaildrawer.cpp**

  修改 `e:\QT\work\PetManager\modules\appointmentdetaildrawer.cpp` 中的 `setAppointment` 函数，找到 `else if (m_currentInfo.type == "Boarding")` 渲染逻辑：

  ```cpp
  else if (m_currentInfo.type == "Boarding") addInfoRow(g, currentRow++, "房间号", m_currentInfo.roomNo.isEmpty() ? "待分配" : m_currentInfo.roomNo);
  ```

  将其替换为：

  ```cpp
  else if (m_currentInfo.type == "Boarding") {
      addInfoRow(g, currentRow++, "房间号", m_currentInfo.roomNo.isEmpty() ? "待分配" : (m_currentInfo.roomNo + "号房"));
      addInfoRow(g, currentRow++, "入住日期", m_currentInfo.date.isEmpty() ? "--" : m_currentInfo.date);
      addInfoRow(g, currentRow++, "离店日期", m_currentInfo.boardingEndDate.isEmpty() ? "--" : m_currentInfo.boardingEndDate);
      addInfoRow(g, currentRow++, "总天数", m_currentInfo.duration > 0 ? (QString::number(m_currentInfo.duration) + " 天") : "--");
  }
  ```

- [ ] **Step 2: 在 MSYS2 / MinGW 编译环境编译客户端**

  打开 PowerShell 终端并运行客户端编译指令以验证是否有任何语法/编译错误。

  运行：
  ```powershell
  $env:PATH = "E:\QT\Tools\mingw1310_64\bin;" + $env:PATH
  cd e:\QT\work\PetManager\build\Desktop_Qt_6_8_2_MinGW_64_bit-Debug
  mingw32-make -j8
  ```

  Expected: 编译无任何错误，顺利生成 `PetManager.exe` 可执行文件。

- [ ] **Step 3: 运行客户端并进行手动测试验证**

  运行生成的客户端程序：
  ```powershell
  cd e:\QT\work\PetManager\build\Desktop_Qt_6_8_2_MinGW_64_bit-Debug\debug
  .\PetManager.exe
  ```

  在系统主界面左侧导航栏点击**服务预约**。
  - 选择一个**寄养**类型的预约单（例如“奶酪”或“豆包”的寄养订单）。
  - 检查右侧详情界面中“预约详情”卡片内是否按顺序显示：
    - 房间号（如：101号房 或 待分配）
    - 入住日期（如：2026-05-21）
    - 离店日期（如：2026-05-24）
    - 总天数（如：3 天）
  - 切换到一个非寄养的订单（如洗护预约），确认它的右侧详情页依然正常且不受干扰。

- [ ] **Step 4: 提交修改到 Git**

  在 PowerShell 终端执行提交：
  ```powershell
  git add e:\QT\work\PetManager\modules\appointmentdetaildrawer.cpp
  git commit -m "feat: add boarding check-in/out dates and duration details to appointment drawer"
  ```
