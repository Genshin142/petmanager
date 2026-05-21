# 员工排班与刷卡考勤联动系统设计开发计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 实现员工详情抽屉中“考勤”选项卡替代原先的“动态”标签页，展示月度考勤全景日历，并且支持店长双击/点击日期弹窗对考勤计划和实际刷卡时间进行手动补签、修改和备注。

**Architecture:** 
1. **数据模型层**：在客户端 `ScheduleInfo` 结构体中新增 `clockIn`, `clockOut` 以及 `note` 字段。
2. **服务端控制层**：升级 `ScheduleController` 的查询及 upsert SQL 逻辑，查询时包含考勤打卡信息，保存时采用 `ON DUPLICATE KEY UPDATE` 写入打卡数据并支持将 "HH:mm" 输入与日期自动拼合为 SQL `DATETIME` 格式。
3. **客户端数据层**：升级 `ScheduleDataManager` 序列化/反序列化，解析 TCP 网络包中的考勤字段。
4. **客户端 UI 层**：重构 `EmployeeDetailDrawer` 抽屉，设计专用的“考勤补签对话框（`EditAttendanceDialog`）”，实现双击/点击日历单元格进行考勤微调，并支持数据自动广播刷新。

**Tech Stack:** Qt 6.8.2, C++17, MySQL 8.0, QSqlQuery, QStackedWidget, QDialog.

---

### Task 1: 升级数据模型与服务端控制层

**Files:**
- Modify: `common_types.h:333-340` (扩展 ScheduleInfo 结构体)
- Modify: `server/logic/schedule_controller.cpp:25-58` (handleGetSchedule 查询字段升级)
- Modify: `server/logic/schedule_controller.cpp:60-93` (upsertSchedule 支持写入考勤)

- [ ] **Step 1: 在 `common_types.h` 中扩展结构体**
  
  修改 [common_types.h](file:///e:/QT/work/PetManager/common_types.h#L333-L340)，在 `ScheduleInfo` 结构体中添加 `clockIn`、`clockOut` 和 `note` 字段：
  ```cpp
  struct ScheduleInfo {
      QString employeeId;
      QString date;       // yyyy-MM-dd
      ShiftType type;
      QString startTime;  // HH:mm
      QString endTime;    // HH:mm
      QString clockIn;    // HH:mm (新增)
      QString clockOut;   // HH:mm (新增)
      QString note;       // 考勤备注 (新增)
  };
  ```

- [ ] **Step 2: 升级服务端的排班与考勤数据查询 SQL**
  
  修改 [server/logic/schedule_controller.cpp](file:///e:/QT/work/PetManager/server/logic/schedule_controller.cpp#L30-L48)，在 `handleGetSchedule` 中查询 `clock_in`, `clock_out` 和 `note` 字段并格式化：
  ```cpp
  void ScheduleController::handleGetSchedule(ClientHandler *client, const QJsonObject &body)
  {
      QString startDate = body["start_date"].toString();
      QString endDate = body["end_date"].toString();
      
      QSqlDatabase db = ConnectionPool::instance().openConnection();
      QSqlQuery query(db);
      // 使用 DATE_FORMAT 格式化打卡时间为 HH:mm，方便前端渲染
      query.prepare("SELECT s.emp_id, s.work_date, s.shift_type, s.plan_start, s.plan_end, "
                    "DATE_FORMAT(s.clock_in, '%H:%i') as clock_in_time, "
                    "DATE_FORMAT(s.clock_out, '%H:%i') as clock_out_time, s.note "
                    "FROM sys_schedules s "
                    "WHERE s.work_date BETWEEN :start AND :end AND s.is_deleted = 0");
      query.bindValue(":start", startDate);
      query.bindValue(":end", endDate);

      QJsonObject response;
      if (query.exec()) {
          QJsonArray data;
          while (query.next()) {
              QJsonObject item;
              int dbEmpId = query.value("emp_id").toInt();
              item["emp_id"] = QString("E%1").arg(dbEmpId, 3, 10, QChar('0'));
              item["work_date"] = query.value("work_date").toString();
              item["shift_type"] = query.value("shift_type").toString();
              item["plan_start"] = query.value("plan_start").isNull() ? "" : query.value("plan_start").toString().left(5);
              item["plan_end"] = query.value("plan_end").isNull() ? "" : query.value("plan_end").toString().left(5);
              item["clock_in"] = query.value("clock_in_time").toString();
              item["clock_out"] = query.value("clock_out_time").toString();
              item["note"] = query.value("note").toString();
              data.append(item);
          }
          response["status"] = Protocol::STATUS_OK;
          response["data"] = data;
      } else {
          LOG_E("[DB] Get schedule failed: " << query.lastError().text().toStdString());
          response["status"] = Protocol::STATUS_ERROR;
          response["message"] = "Query failed";
      }
      ConnectionPool::instance().closeConnection(db);
      client->sendPacket(Protocol::CMD_GET_SCHEDULE, QJsonDocument(response).toJson(QJsonDocument::Compact));
  }
  ```

- [ ] **Step 3: 升级服务端的排班与考勤数据修改 SQL**
  
  修改 [server/logic/schedule_controller.cpp](file:///e:/QT/work/PetManager/server/logic/schedule_controller.cpp#L72-L92) 中的 `upsertSchedule`，支持在 `ON DUPLICATE KEY UPDATE` 时写入 `clock_in`、`clock_out` 和 `note`。这里要特别处理：如果店长输入的是 "08:52"，我们要拼合为完整的 "yyyy-MM-dd HH:mm:ss" 时间字符串以便 MySQL 写入 `DATETIME` 字段：
  ```cpp
  bool ScheduleController::upsertSchedule(const QJsonObject &obj)
  {
      QString empIdStr = obj["emp_id"].toString();
      int empId = empIdStr.mid(1).toInt(); // "E001" -> 1
      QString workDate = obj["work_date"].toString();
      QString shiftType = obj["shift_type"].toString();
      QString planStart = obj["plan_start"].toString();
      QString planEnd = obj["plan_end"].toString();
      QString clockIn = obj["clock_in"].toString();
      QString clockOut = obj["clock_out"].toString();
      QString note = obj["note"].toString();

      QSqlDatabase db = ConnectionPool::instance().openConnection();
      QSqlQuery query(db);
      
      query.prepare("INSERT INTO sys_schedules (emp_id, work_date, shift_type, plan_start, plan_end, clock_in, clock_out, note) "
                    "VALUES (:emp_id, :work_date, :shift_type, :plan_start, :plan_end, :clock_in, :clock_out, :note) "
                    "ON DUPLICATE KEY UPDATE "
                    "shift_type = VALUES(shift_type), plan_start = VALUES(plan_start), plan_end = VALUES(plan_end), "
                    "clock_in = VALUES(clock_in), clock_out = VALUES(clock_out), note = VALUES(note)");
      
      query.bindValue(":emp_id", empId);
      query.bindValue(":work_date", workDate);
      query.bindValue(":shift_type", shiftType);
      
      if (planStart.isEmpty()) query.bindValue(":plan_start", QVariant(QMetaType::fromType<QString>()));
      else query.bindValue(":plan_start", planStart);
      
      if (planEnd.isEmpty()) query.bindValue(":plan_end", QVariant(QMetaType::fromType<QString>()));
      else query.bindValue(":plan_end", planEnd);

      // 处理实际打卡时间 clock_in：如果输入是 "08:52" 则合成为 "2026-05-17 08:52:00"
      if (clockIn.isEmpty()) {
          query.bindValue(":clock_in", QVariant(QMetaType::fromType<QString>()));
      } else {
          QString fullClockIn = clockIn.contains("-") ? clockIn : (workDate + " " + clockIn + ":00");
          query.bindValue(":clock_in", fullClockIn);
      }

      // 处理实际打卡时间 clock_out
      if (clockOut.isEmpty()) {
          query.bindValue(":clock_out", QVariant(QMetaType::fromType<QString>()));
      } else {
          QString fullClockOut = clockOut.contains("-") ? clockOut : (workDate + " " + clockOut + ":00");
          query.bindValue(":clock_out", fullClockOut);
      }

      query.bindValue(":note", note.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : note);

      bool ok = query.exec();
      if (!ok) {
          LOG_E("[DB] Upsert schedule failed: " << query.lastError().text().toStdString());
      }
      ConnectionPool::instance().closeConnection(db);
      return ok;
  }
  ```

- [ ] **Step 4: 编译验证**
  
  在服务端目录下重新编译，确保无语法错误。

---

### Task 2: 升级客户端数据层 (ScheduleDataManager)

**Files:**
- Modify: `modules/scheduledatamanager.cpp:100-140` (反序列化网络数据包扩展)
- Modify: `modules/scheduledatamanager.cpp:150-180` (发送保存请求序列化升级)

- [ ] **Step 1: 升级客户端的排班与考勤数据解析逻辑**
  
  修改 [modules/scheduledatamanager.cpp](file:///e:/QT/work/PetManager/modules/scheduledatamanager.cpp#L100-L140) 中的 `onPacketReceived` 对应 `Protocol::CMD_GET_SCHEDULE` 的分支，解析考勤打卡信息并填入 `ScheduleInfo`：
  ```cpp
  // 假定解析 JSON 数组时的循环体内部
  QJsonObject obj = arr[i].toObject();
  ScheduleInfo info;
  info.employeeId = obj["emp_id"].toString();
  info.date = obj["work_date"].toString();
  
  QString shiftType = obj["shift_type"].toString();
  if (shiftType == "早班") info.type = SHIFT_MORNING;
  else if (shiftType == "晚班") info.type = SHIFT_EVENING;
  else if (shiftType == "自定义") info.type = SHIFT_CUSTOM;
  else info.type = SHIFT_OFF;

  info.startTime = obj["plan_start"].toString();
  info.endTime = obj["plan_end"].toString();
  info.clockIn = obj["clock_in"].toString();   // 解析打卡上班
  info.clockOut = obj["clock_out"].toString(); // 解析打卡下班
  info.note = obj["note"].toString();         // 解析备注信息
  ```

- [ ] **Step 2: 升级客户端的保存/更新请求构造**
  
  修改 [modules/scheduledatamanager.cpp](file:///e:/QT/work/PetManager/modules/scheduledatamanager.cpp#L150-L180) 中 `saveSchedule` 逻辑，在构建 QJsonObject 时，同步发送考勤打卡信息：
  ```cpp
  QJsonObject body;
  body["emp_id"] = info.employeeId;
  body["work_date"] = info.date;
  
  QString typeStr = "休息";
  if (info.type == SHIFT_MORNING) typeStr = "早班";
  else if (info.type == SHIFT_EVENING) typeStr = "晚班";
  else if (info.type == SHIFT_CUSTOM) typeStr = "自定义";
  body["shift_type"] = typeStr;

  body["plan_start"] = info.startTime;
  body["plan_end"] = info.endTime;
  body["clock_in"] = info.clockIn;   // 新增
  body["clock_out"] = info.clockOut; // 新增
  body["note"] = info.note;         // 新增

  NetworkManager::instance().sendRequest(Protocol::CMD_UPDATE_SCHEDULE, body);
  ```

---

### Task 3: 重构 EmployeeDetailDrawer (展示考勤全景与日历)

**Files:**
- Modify: `modules/employeedetaildrawer.h` (修改 UI 组件声明，更名 Tab)
- Modify: `modules/employeedetaildrawer.cpp:160-210` (重命名 Tab 名称，初始化考勤页组件)
- Modify: `modules/employeedetaildrawer.cpp:510-595` (升级日历刷新逻辑，展示打卡徽章与状态颜色)

- [ ] **Step 1: 在头文件中修改界面组件声明**
  
  在 [modules/employeedetaildrawer.h](file:///e:/QT/work/PetManager/modules/employeedetaildrawer.h) 中添加新看板控件：
  ```cpp
  // 放在私有成员变量区
  QLabel *m_statAbsentLabel = nullptr; // 新增：缺卡统计
  QWidget* createAttendancePage();      // 新增：构建考勤页面 (原 logPage 占位符的替代)
  ```

- [ ] **Step 2: 重构 Drawer 的 Tab 名称及堆栈页面绑定**
  
  在 [modules/employeedetaildrawer.cpp](file:///e:/QT/work/PetManager/modules/employeedetaildrawer.cpp#L161) 中，将 Tab 文本由 `"动态"` 修改为 `"考勤"`：
  ```cpp
  QStringList tabs = {"档案", "排班", "考勤"};
  ```
  在 [modules/employeedetaildrawer.cpp](file:///e:/QT/work/PetManager/modules/employeedetaildrawer.cpp#L196-L206) 中，将原占位符 `logPage` 替换为我们新创建的 `createAttendancePage()`：
  ```cpp
  m_stackedWidget->addWidget(createProfilePage());    // Index 0
  m_stackedWidget->addWidget(createSchedulePage());   // Index 1
  m_stackedWidget->addWidget(createAttendancePage());  // Index 2 (考勤页)
  ```

- [ ] **Step 3: 实现 `createAttendancePage()` 函数构建考勤面板**
  
  在 [modules/employeedetaildrawer.cpp](file:///e:/QT/work/PetManager/modules/employeedetaildrawer.cpp) 中新增 `createAttendancePage()` 的实现：
  包含：
  1. 今日考勤实时状态卡片 (显示排班计划、打卡时间和状态)。
  2. 考勤指标汇总面板 (出勤天数、迟到天数、缺卡天数)。
  
  *(由于逻辑与排班日历类似，可复用日历结构并只读展示，此处重点在于为第四步添加交互支持)*

---

### Task 4: 实现考勤日历点击与“考勤排班微调补签弹窗” (EditAttendanceDialog)

**Files:**
- Create: `modules/editattendancedialog.h` (新建弹窗头文件)
- Create: `modules/editattendancedialog.cpp` (新建弹窗实现文件)
- Modify: `modules/employeedetaildrawer.cpp:510-595` (日历生成时绑定点击事件，调起补签弹窗)

- [ ] **Step 1: 创建 `EditAttendanceDialog` 对话框**
  
  新建 [modules/editattendancedialog.h](file:///e:/QT/work/PetManager/modules/editattendancedialog.h)，声明一个精致的补签修改对话框：
  ```cpp
  #ifndef EDITATTENDANCEDIALOG_H
  #define EDITATTENDANCEDIALOG_H
  
  #include <QDialog>
  #include <QComboBox>
  #include <QLineEdit>
  #include <QTimeEdit>
  #include <QLabel>
  #include "common_types.h"
  
  class EditAttendanceDialog : public QDialog {
      Q_OBJECT
  public:
      explicit EditAttendanceDialog(const ScheduleInfo &info, QWidget *parent = nullptr);
      ScheduleInfo getUpdatedInfo() const;
  private:
      QComboBox *m_typeCombo;
      QTimeEdit *m_planStartEdit;
      QTimeEdit *m_planEndEdit;
      QLineEdit *m_clockInEdit;
      QLineEdit *m_clockOutEdit;
      QLineEdit *m_noteEdit;
      QWidget *m_timeContainer;
      QWidget *m_clockContainer;
      
      ScheduleInfo m_info;
  };
  #endif
  ```

- [ ] **Step 2: 实现 `EditAttendanceDialog` 交互逻辑**
  
  新建 [modules/editattendancedialog.cpp](file:///e:/QT/work/PetManager/modules/editattendancedialog.cpp)，实现排班与考勤打卡字段的同步互斥输入（如切换为休息时隐藏打卡框）：
  ```cpp
  #include "editattendancedialog.h"
  #include <QVBoxLayout>
  #include <QHBoxLayout>
  #include <QPushButton>
  #include <QFormLayout>
  
  EditAttendanceDialog::EditAttendanceDialog(const ScheduleInfo &info, QWidget *parent)
      : QDialog(parent), m_info(info)
  {
      setWindowTitle(QString("编辑排班与考勤 - %1").arg(info.date));
      setFixedWidth(360);
      setStyleSheet("QDialog { background: white; }");
  
      QVBoxLayout *mainLayout = new QVBoxLayout(this);
      QFormLayout *formLayout = new QFormLayout();
  
      m_typeCombo = new QComboBox();
      m_typeCombo->addItems({"休息", "早班", "晚班", "自定义"});
      formLayout->addRow("班次类型:", m_typeCombo);
  
      m_timeContainer = new QWidget();
      QFormLayout *timeForm = new QFormLayout(m_timeContainer);
      timeForm->setContentsMargins(0,0,0,0);
      m_planStartEdit = new QTimeEdit(QTime::fromString(info.startTime.isEmpty() ? "09:00" : info.startTime, "HH:mm"));
      m_planEndEdit = new QTimeEdit(QTime::fromString(info.endTime.isEmpty() ? "18:00" : info.endTime, "HH:mm"));
      timeForm->addRow("计划上班:", m_planStartEdit);
      timeForm->addRow("计划下班:", m_planEndEdit);
      formLayout->addRow(m_timeContainer);
  
      m_clockContainer = new QWidget();
      QFormLayout *clockForm = new QFormLayout(m_clockContainer);
      clockForm->setContentsMargins(0,0,0,0);
      m_clockInEdit = new QLineEdit(info.clockIn);
      m_clockInEdit->setPlaceholderText("格式: HH:mm (如 08:52)");
      m_clockOutEdit = new QLineEdit(info.clockOut);
      m_clockOutEdit->setPlaceholderText("格式: HH:mm (如 18:04)");
      clockForm->addRow("补签上班:", m_clockInEdit);
      clockForm->addRow("补签下班:", m_clockOutEdit);
      formLayout->addRow(m_clockContainer);
  
      m_noteEdit = new QLineEdit(info.note);
      m_noteEdit->setPlaceholderText("如: 忘记刷卡，店长代补签");
      formLayout->addRow("考勤备注:", m_noteEdit);
  
      mainLayout->addLayout(formLayout);
  
      // 联动：如果是休息，隐藏上班时间和打卡框
      auto updateVisibility = [this]() {
          bool isOff = m_typeCombo->currentText() == "休息";
          m_timeContainer->setVisible(!isOff);
          m_clockContainer->setVisible(!isOff);
          adjustSize();
      };
      connect(m_typeCombo, &QComboBox::currentTextChanged, this, updateVisibility);
      
      // 初始化值
      if (info.type == SHIFT_MORNING) m_typeCombo->setCurrentText("早班");
      else if (info.type == SHIFT_EVENING) m_typeCombo->setCurrentText("晚班");
      else if (info.type == SHIFT_CUSTOM) m_typeCombo->setCurrentText("自定义");
      else m_typeCombo->setCurrentText("休息");
      updateVisibility();
  
      QHBoxLayout *btnLayout = new QHBoxLayout();
      QPushButton *cancelBtn = new QPushButton("取消");
      QPushButton *saveBtn = new QPushButton("保存");
      saveBtn->setStyleSheet("background-color: #3b82f6; color: white; border-radius: 4px; padding: 6px 12px;");
      btnLayout->addStretch();
      btnLayout->addWidget(cancelBtn);
      btnLayout->addWidget(saveBtn);
      mainLayout->addLayout(btnLayout);
  
      connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
      connect(saveBtn, &QPushButton::clicked, this, &QDialog::accept);
  }
  
  ScheduleInfo EditAttendanceDialog::getUpdatedInfo() const {
      ScheduleInfo info = m_info;
      QString text = m_typeCombo->currentText();
      if (text == "早班") info.type = SHIFT_MORNING;
      else if (text == "晚班") info.type = SHIFT_EVENING;
      else if (text == "自定义") info.type = SHIFT_CUSTOM;
      else info.type = SHIFT_OFF;
  
      if (info.type != SHIFT_OFF) {
          info.startTime = m_planStartEdit->time().toString("HH:mm");
          info.endTime = m_planEndEdit->time().toString("HH:mm");
          info.clockIn = m_clockInEdit->text().trimmed();
          info.clockOut = m_clockOutEdit->text().trimmed();
      } else {
          info.startTime = "";
          info.endTime = "";
          info.clockIn = "";
          info.clockOut = "";
      }
      info.note = m_noteEdit->text().trimmed();
      return info;
  }
  ```

- [ ] **Step 3: 在 `EmployeeDetailDrawer::refreshCalendar` 中绑定日期点击事件**
  
  修改 [modules/employeedetaildrawer.cpp](file:///e:/QT/work/PetManager/modules/employeedetaildrawer.cpp#L540-L590) 的日历生成循环，将 `dayLabel` 包装在一个带鼠标点击过滤器的复合状态控件（如自定义的 QWidget / QPushButton）中，一旦点击，即调起 `EditAttendanceDialog`，保存后触发更新下发：
  ```cpp
  // 在生成 dayLabel 时：
  QPushButton *dayBtn = new QPushButton(QString::number(day));
  dayBtn->setFixedSize(32, 32);
  dayBtn->setCursor(Qt::PointingHandCursor);
  // 依据排班/考勤打卡状态，为 dayBtn 注入不同的样式 (如绿色、黄色、红色)
  
  // 绑定点击事件，调起编辑框
  connect(dayBtn, &QPushButton::clicked, this, [this, info, curr](){
      EditAttendanceDialog dlg(info, this);
      if (dlg.exec() == QDialog::Accepted) {
          ScheduleInfo newInfo = dlg.getUpdatedInfo();
          ScheduleDataManager::instance()->saveSchedule(newInfo); // 下发修改至服务器！
      }
  });
  ```

---

### 验证与回归测试计划

1. **测试用例 1: 异常打卡显示验证**
   - 运行新服务端及客户端 v1.1.6。
   - 打开店员“李曼妮”的抽屉，切换至“考勤”Tab，确认 5 月 16 号显示为红色“缺卡”。
2. **测试用例 2: 手动考勤补签与数据广播验证**
   - 点击 5 月 16 号的缺卡日期，弹出“编辑排班与考勤”对话框。
   - 将补签上班设为 `08:55`，补签下班设为 `18:02`，备注写上“忘记刷卡，店长补签”，点击保存。
   - 观察客户端的日历网格：该单元格应瞬间自动刷新，背景色变绿，文字显示为正常！
   - 观察后台 MySQL 的 `sys_schedules` 表：确认该员工 5 月 16 日的 `clock_in` 已成功写入为 `2026-05-16 08:55:00`。
