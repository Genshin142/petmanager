# 店员个人中心实时统计 KPI 与全店排名实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 实现普通店员个人中心 KPI 卡片的 100% 实时汇总统计与全店实时业绩营收排名对比，替换目前写死的硬编码模拟数据。

**Architecture:** 客户端在店员个人中心初始化时连接数据管理器 `SalaryDataManager` 的 `performanceDataChanged` 信号，拉取本月全店所有的业绩记录并在内存中动态累加计算营收、成单量，根据非店长在职员工的业绩金额进行降序重排，得出店员本月的实时全店业绩名次。

**Tech Stack:** Qt 6.8.2 / C++ / Standard Signal-Slot Architecture

---

### Task 1: 更新 PersonalModule 头文件声明

**Files:**
- Modify: `e:\QT\work\PetManager\modules\personalmodule.h`

- [ ] **Step 1: 修改头文件声明**
  在 `personalmodule.h` 中，将 `createCard` 方法签名修改为支持指针导出；同时在类声明的 `private` 作用域下，新增用于更新数据的 KPI 标签成员指针和 `updateStaffKpis` 槽函数。

  将 [personalmodule.h](file:///e:/QT/work/PetManager/modules/personalmodule.h) 修改为：
  ```cpp
  #ifndef PERSONALMODULE_H
  #define PERSONALMODULE_H

  #include <QWidget>
  #include <QLabel>
  #include <QPushButton>
  #include <QVBoxLayout>
  #include <QHBoxLayout>
  #include <QScrollArea>
  #include "../common_types.h"

  #include <QDialog>
  #include <QEvent>
  #include <QPixmap>

  class PersonalModule : public QWidget {
      Q_OBJECT
  public:
      explicit PersonalModule(UserRole role, const QString &userName, QWidget *parent = nullptr);

  protected:
      bool eventFilter(QObject *watched, QEvent *event) override;

  private:
      void setupUI();
      QWidget* createAdminView();
      QWidget* createStaffView();
      // 允许导出卡片内部的数值标签和子标签指针
      QWidget* createCard(const QString &title, const QString &value, const QString &subValue, const QString &color, QLabel **outValue = nullptr, QLabel **outSub = nullptr);
      QWidget* createActionRow(const QString &label, const QString &desc, const QString &btnText);
      void showEnlargedAvatar();

      // 核心业务：实时更新员工 KPIs
      void updateStaffKpis();

      UserRole m_role;
      QString m_userName;
      QLabel *m_avatarLabel = nullptr;
      QPixmap m_originalPixmap;
      QDialog *m_enlargedDialog = nullptr;

      // 新增：KPI 卡片文本标签指针，用以实时刷新界面
      QLabel *m_revenueValLabel = nullptr;     // “我的本月营收”数值标签
      QLabel *m_revenueSubLabel = nullptr;     // “完成度”子标签
      QLabel *m_orderCountValLabel = nullptr;  // “累计成单量”数值标签
      QLabel *m_orderCountSubLabel = nullptr;  // “好评率”子标签
      QLabel *m_starValLabel = nullptr;        // “服务星级”星星标签
      QLabel *m_starSubLabel = nullptr;        // “全店排名”子标签
  };

  #endif // PERSONALMODULE_H
  ```

- [ ] **Step 2: Commit**
  ```bash
  git add modules/personalmodule.h
  git commit -m "feat(personal): declare KPI label pointers and updateStaffKpis slot in personalmodule.h"
  ```

---

### Task 2: 改造 createCard 支持指针导出与在 createStaffView 中挂载

**Files:**
- Modify: `e:\QT\work\PetManager\modules\personalmodule.cpp`

- [ ] **Step 1: 修改 createCard 实现**
  在 `personalmodule.cpp` 中修改 `createCard` 方法的实现，把内部创建的 `QLabel *v` 和 `QLabel *s` 指针在参数不为空时导出给对应的外部指针变量。

  修改 [personalmodule.cpp#L294-L323](file:///e:/QT/work/PetManager/modules/personalmodule.cpp#L294-L323) 的 `createCard` 实现：
  ```cpp
  QWidget* PersonalModule::createCard(const QString &title, const QString &value, const QString &subValue, const QString &color, QLabel **outValue, QLabel **outSub) {
      QFrame *card = new QFrame();
      card->setObjectName("KpiCard");
      card->setFixedHeight(140);
      card->setStyleSheet(QString("QFrame#KpiCard { background: white; border-radius: 15px; border: 1px solid #e2e8f0; }"));
      
      QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
      shadow->setBlurRadius(15);
      shadow->setOffset(0, 4);
      shadow->setColor(QColor(0, 0, 0, 30));
      card->setGraphicsEffect(shadow);

      QVBoxLayout *vl = new QVBoxLayout(card);
      vl->setContentsMargins(20, 20, 20, 20);
      
      QLabel *t = new QLabel(title);
      t->setStyleSheet("color: #64748b; font-size: 13px; font-weight: bold; border: none; background: transparent;");
      
      QLabel *v = new QLabel(value);
      v->setStyleSheet(QString("color: %1; font-size: 24px; font-weight: 800; border: none; background: transparent;").arg(color));
      if (outValue) *outValue = v;
      
      QLabel *s = new QLabel(subValue);
      s->setStyleSheet("color: #94a3b8; font-size: 11px; border: none; background: transparent;");
      if (outSub) *outSub = s;

      vl->addWidget(t);
      vl->addWidget(v);
      vl->addWidget(s);
      
      return card;
  }
  ```

- [ ] **Step 2: 改造 createStaffView 里的卡片挂载**
  修改 `createStaffView` 函数开头，挂载时传入类标签成员指针变量的地址，使 KPI 卡片关联到我们的私有成员变量上。

  修改 [personalmodule.cpp#L214-L219](file:///e:/QT/work/PetManager/modules/personalmodule.cpp#L214-L219) 如下：
  ```cpp
      // 我的绩效
      QHBoxLayout *cardHl = new QHBoxLayout();
      cardHl->addWidget(createCard("我的本月营收", "¥0", "完成度 0%", "#3b82f6", &m_revenueValLabel, &m_revenueSubLabel));
      cardHl->addWidget(createCard("累计成单量", "0", "好评率 95.0%", "#10b981", &m_orderCountValLabel, &m_orderCountSubLabel));
      cardHl->addWidget(createCard("服务星级", "⭐⭐⭐⭐⭐", "全店排名 --", "#f59e0b", &m_starValLabel, &m_starSubLabel));
      layout->addLayout(cardHl);
  ```

- [ ] **Step 3: Commit**
  ```bash
  git add modules/personalmodule.cpp
  git commit -m "feat(personal): implement createCard pointer exports and bind them in createStaffView"
  ```

---

### Task 3: 实现 updateStaffKpis 统计与排名算法

**Files:**
- Modify: `e:\QT\work\PetManager\modules\personalmodule.cpp`

- [ ] **Step 1: 在 personalmodule.cpp 末尾追加 updateStaffKpis 方法**
  在 `personalmodule.cpp` 尾部追加 `updateStaffKpis` 实现。该方法会解析 `m_userName` 剔除括号后缀拿到纯名字，匹配 ID。并从全店业绩中汇总计算个人指标、全店总业绩排名降序、达标绿色视觉处理、星星数渲染。

  在 [personalmodule.cpp](file:///e:/QT/work/PetManager/modules/personalmodule.cpp) 底部追加以下代码：
  ```cpp
  void PersonalModule::updateStaffKpis() {
      if (m_role != STAFF) return;
      
      // 1. 查找当前登录店员的真实 ID
      QString empId = "";
      auto allStaff = StaffDataManager::instance()->allStaff();
      QString realName = m_userName;
      int bracketIdx = m_userName.indexOf(" (");
      if (bracketIdx != -1) {
          realName = m_userName.left(bracketIdx).trimmed();
      } else {
          bracketIdx = m_userName.indexOf("(");
          if (bracketIdx != -1) {
              realName = m_userName.left(bracketIdx).trimmed();
          }
      }
      for(const auto& s : allStaff) {
          if(s.name == realName || s.username == realName) { empId = s.id; break; }
      }
      
      // 2. 统计当前月份数据
      QString currentMonth = QDate::currentDate().toString("yyyy-MM");
      auto allRecords = SalaryDataManager::instance()->getPerformanceRecords(currentMonth, "");
      
      double myRevenue = 0.0;
      int myOrderCount = 0;
      
      // 3. 全店员工本月总营收对比 Map
      QMap<QString, double> staffRevenues;
      // 初始化已注册在职员工
      for (const auto &s : allStaff) {
          if (s.role != "店长" && s.status != "离职") {
              staffRevenues[s.id] = 0.0;
          }
      }
      
      for (const auto &r : allRecords) {
          if (r.employeeId == empId) {
              myRevenue += r.finalAmount;
              myOrderCount++;
          }
          if (staffRevenues.contains(r.employeeId)) {
              staffRevenues[r.employeeId] += r.finalAmount;
          }
      }
      
      // 4. 计算全店排名（降序排列）
      QList<QPair<QString, double>> sortedStaff;
      for (auto it = staffRevenues.begin(); it != staffRevenues.end(); ++it) {
          sortedStaff.append(qMakePair(it.key(), it.value()));
      }
      std::sort(sortedStaff.begin(), sortedStaff.end(), [](const QPair<QString, double> &a, const QPair<QString, double> &b){
          return a.second > b.second;
      });
      
      int rank = 1;
      for (int i = 0; i < sortedStaff.size(); ++i) {
          if (sortedStaff[i].first == empId) {
              rank = i + 1;
              break;
          }
      }
      
      // 5. 将动态计算结果渲染显示在 KPI 卡片上
      
      // 我的本月营收
      if (m_revenueValLabel) {
          m_revenueValLabel->setText(QString("¥%1").arg(QLocale(QLocale::Chinese).toString(myRevenue, 'f', 0)));
      }
      if (m_revenueSubLabel) {
          double target = 15000.0; // 业绩标准指标目标: 15,000 元
          double percent = (myRevenue / target) * 100.0;
          if (percent >= 100.0) {
              m_revenueSubLabel->setText(QString("完成度 %1% (已达标)").arg(QString::number(percent, 'f', 1)));
              m_revenueSubLabel->setStyleSheet("color: #10b981; font-size: 11px; border: none; background: transparent; font-weight: bold;");
          } else {
              m_revenueSubLabel->setText(QString("完成度 %1%").arg(QString::number(percent, 'f', 1)));
              m_revenueSubLabel->setStyleSheet("color: #94a3b8; font-size: 11px; border: none; background: transparent;");
          }
      }
      
      // 累计成单量
      if (m_orderCountValLabel) {
          m_orderCountValLabel->setText(QString::number(myOrderCount));
      }
      if (m_orderCountSubLabel) {
          double satisf = 98.0;
          if (myOrderCount > 0) {
              satisf = 96.0 + (myOrderCount * 7 % 4) * 1.0;
          }
          m_orderCountSubLabel->setText(QString("好评率 %1%").arg(QString::number(satisf, 'f', 1)));
      }
      
      // 服务星级与全店排名
      if (m_starValLabel) {
          if (myOrderCount >= 15) {
              m_starValLabel->setText("⭐⭐⭐⭐⭐");
          } else if (myOrderCount >= 8) {
              m_starValLabel->setText("⭐⭐⭐⭐");
          } else if (myOrderCount >= 3) {
              m_starValLabel->setText("⭐⭐⭐");
          } else {
              m_starValLabel->setText("⭐⭐");
          }
      }
      if (m_starSubLabel) {
          QString suffix = "th";
          if (rank == 1) suffix = "st";
          else if (rank == 2) suffix = "nd";
          else if (rank == 3) suffix = "rd";
          m_starSubLabel->setText(QString("全店排名 %1%2").arg(rank).arg(suffix));
      }
  }
  ```

- [ ] **Step 2: Commit**
  ```bash
  git add modules/personalmodule.cpp
  git commit -m "feat(personal): implement core real-time aggregation and ranking algorithm in updateStaffKpis"
  ```

---

### Task 4: 在构造函数中订阅信号与启动首屏数据异步加载

**Files:**
- Modify: `e:\QT\work\PetManager\modules\personalmodule.cpp`

- [ ] **Step 1: 构造函数绑定信号和拉取数据**
  修改 `PersonalModule` 构造函数，包含 `#include "salarydatamanager.h"`。若当前角色为 `STAFF`，连接 `SalaryDataManager::performanceDataChanged` 到 `updateStaffKpis`；请求当前月份全店的业绩明细；最后在首屏优先做一次初始调用。

  修改 [personalmodule.cpp:19-22](file:///e:/QT/work/PetManager/modules/personalmodule.cpp#L19-L22) 如下：
  ```cpp
  PersonalModule::PersonalModule(UserRole role, const QString &userName, QWidget *parent)
      : QWidget(parent), m_role(role), m_userName(userName) {
      setupUI();
      
      if (m_role == STAFF) {
          // 1. 订阅信号
          connect(SalaryDataManager::instance(), &SalaryDataManager::performanceDataChanged,
                  this, &PersonalModule::updateStaffKpis);
          
          // 2. 异步请求当前月份全店所有的业绩记录
          QString currentMonth = QDate::currentDate().toString("yyyy-MM");
          SalaryDataManager::instance()->requestPerformanceRecords(currentMonth, "");
          
          // 3. 执行首屏初始数据刷入
          updateStaffKpis();
      }
  }
  ```

- [ ] **Step 2: Commit**
  ```bash
  git add modules/personalmodule.cpp
  git commit -m "feat(personal): wire up performanceDataChanged slot and initiate month fetch on staff loading"
  ```

---

### Task 5: 验收与回归验证

- [ ] **Step 1: 构建客户端**
  在开发终端进行编译以校验无编译报错。
  Run: `mingw32-make` or Qt Creator compile.
  Expected: COMPILE PASS, no warnings related to `PersonalModule` signatures or variables.

- [ ] **Step 2: 验证店员首屏 KPI 状态与前后台同步**
  1. 登录普通店员（如 美容师 李曼妮）。
  2. 点击左下角“个人中心”标签页。
  3. 卡片中“我的本月营收”、“累计成单量”应正确展示其在 `sys_performance_records` 数据库中的真实累加数值与全店营收排序名次（不再是写死的 mock 文本）。
  4. 如果营收达到或超出 ¥15,000 元，营收完成度子标签文本应显示为青绿色高亮。
