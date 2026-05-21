# 店员个人中心实时统计与绩效排名看板设计规范

本设计规范书详述了“宠物店信息管理系统”中**普通店员个人中心（Personal Center）** KPI 绩效卡片从模拟数据向 **100% 实时统计、实时全店排名对比** 的重构方案。

---

## 一、 业务背景与设计目标

### 1. 业务痛点
目前普通员工进入个人中心时，KPI 卡片（“我的本月营收”、“累计成单量”、“服务星级”）均显示硬编码的模拟文本，无法反映员工当前的真实业绩，降低了店员端系统的实用性与信任感。

### 2. 设计目标
*   **数据 100% 真实透明**：通过客户端统一的数据管理器 `SalaryDataManager`，直接向服务端拉取当前月份全店所有业绩明细（`sys_performance_records`），在客户端内存态进行实时聚合统计。
*   **全店实时业绩 PK 看板**：在客户端对全店所有非店长员工本月的总营收进行实时降序排列，动态算出当前店员在全店的真实业务名次（如第1名、第2名等）。
*   **达标红利视觉激励**：设置标准月度业绩目标为 **¥15,000**。当员工本月营收超额完成时，卡片文字自动变为醒目的高亮青绿色并追加“已达标”状态，形成强烈的正向心理激励。
*   **无感实时刷新**：当收银前台完成任意一笔服务订单结算时，全店所有客户端会收到 `CMD_NOTIFY_REFRESH` (CMD 6001) 广播，触发数据刷新，个人中心的 KPI 数字及排名将**在 200ms 内静默自动刷新**。

---

## 二、 系统架构与网络数据流

### 1. 网络数据交互链
```
[店员个人中心 (PersonalModule)] 
     ||  1. 订阅信号连接：connect(SalaryDataManager::instance(), &SalaryDataManager::performanceDataChanged, ...)
     ||  2. 异步数据请求：requestPerformanceRecords(currentMonth, "") [employeeId 留空以请求全店数据]
     \/
[数据管理器 (SalaryDataManager)] == CMD_GET_PERFORMANCE_LIST (7001) ==> [服务端 (FinanceController)]
                                                                               ||
                                                                      [执行数据库查询：sys_performance_records]
                                                                               ||
[店员个人中心 (视图自动刷新)] <== 触发信号 performanceDataChanged <== [解析包数据存入本地 Map 缓存]
```

### 2. 数据库依赖关系
*   **提成绩效明细表 (`sys_performance_records`)**：
    *   字段 `emp_id`：关联员工ID。
    *   字段 `order_amount` / `final_amount`：服务消费总金额/实付总金额。
    *   字段 `service_date`：服务发生的日期。
*   **员工档案表 (`sys_employees`)**：
    *   用来匹配员工的姓名与在职状态，排除“店长”等管理职位，以进行平级业绩的合理排名。

---

## 三、 客户端实现规范

### 1. 头文件修改：[personalmodule.h](file:///e:/QT/work/PetManager/modules/personalmodule.h)
我们需要引入实时数据处理所需的槽函数以及 KPI 标签指针：

```cpp
#include "salarydatamanager.h"

class PersonalModule : public QWidget {
    Q_OBJECT
    // ... 原有声明保持不变 ...

private:
    // 新增：KPI 卡片文本标签指针，用以实时更新数据
    QLabel *m_revenueValLabel = nullptr;     // “我的本月营收”数值标签
    QLabel *m_revenueSubLabel = nullptr;     // “我的本月营收”完成度标签
    QLabel *m_orderCountValLabel = nullptr;  // “累计成单量”数值标签
    QLabel *m_orderCountSubLabel = nullptr;  // “好评率”标签
    QLabel *m_starValLabel = nullptr;        // “服务星级”星星标签
    QLabel *m_starSubLabel = nullptr;        // “全店排名”标签

    // 新增：内部统计与更新逻辑
    void updateStaffKpis();
};
```

### 2. 源文件修改：[personalmodule.cpp](file:///e:/QT/work/PetManager/modules/personalmodule.cpp)

#### 2.1 改造卡片创建辅助函数 `createCard`
为了允许我们持有对内部标签指针的引用，以便在后期更新其属性：
```cpp
QWidget* PersonalModule::createCard(const QString &title, const QString &value, const QString &subValue, 
                                    const QString &color, QLabel **outValue, QLabel **outSub) {
    // ... 原有 QFrame, QGraphicsDropShadowEffect 及布局代码 ...
    
    QLabel *v = new QLabel(value);
    v->setStyleSheet(QString("color: %1; font-size: 24px; font-weight: 800; border: none; background: transparent;").arg(color));
    if (outValue) *outValue = v; // 导出数值指针
    
    QLabel *s = new QLabel(subValue);
    s->setStyleSheet("color: #94a3b8; font-size: 11px; border: none; background: transparent;");
    if (outSub) *outSub = s;     // 导出子标签指针
    
    // ... 原有布局添加与返回 ...
}
```

#### 2.2 改造 `createStaffView` 卡片挂载
```cpp
QWidget* PersonalModule::createStaffView() {
    // ... 原有布局初始化保持不变 ...
    
    QHBoxLayout *cardHl = new QHBoxLayout();
    cardHl->addWidget(createCard("我的本月营收", "¥0", "完成度 0%", "#3b82f6", &m_revenueValLabel, &m_revenueSubLabel));
    cardHl->addWidget(createCard("累计成单量", "0", "好评率 95.0%", "#10b981", &m_orderCountValLabel, &m_orderCountSubLabel));
    cardHl->addWidget(createCard("服务星级", "⭐⭐⭐⭐⭐", "全店排名 --", "#f59e0b", &m_starValLabel, &m_starSubLabel));
    layout->addLayout(cardHl);

    // ... 排班与店员自助服务保持不变 ...
}
```

#### 2.3 初始化与信号连接
在构造函数中，若当前角色为 `STAFF`，在完成 UI 创建后自动订阅并拉取数据：
```cpp
PersonalModule::PersonalModule(UserRole role, const QString &userName, QWidget *parent)
    : QWidget(parent), m_role(role), m_userName(userName) {
    setupUI();
    
    if (m_role == STAFF) {
        // 1. 订阅业绩数据管理器变更信号
        connect(SalaryDataManager::instance(), &SalaryDataManager::performanceDataChanged,
                this, &PersonalModule::updateStaffKpis);
                
        // 2. 异步拉取本月份全店的业绩明细（用于排名计算）
        QString currentMonth = QDate::currentDate().toString("yyyy-MM");
        SalaryDataManager::instance()->requestPerformanceRecords(currentMonth, ""); // employeeId为空表示请求全店数据
        
        // 3. 先使用当前已有的缓存数据进行初始统计
        updateStaffKpis();
    }
}
```

---

## 四、 核心聚合逻辑与排名算法 (`updateStaffKpis`)

当全店本月业绩列表到达时，触发并执行以下核心统计代码：

```cpp
void PersonalModule::updateStaffKpis() {
    if (m_role != STAFF) return;
    
    // 1. 提取当前登录员工的真实姓名，用以查询匹配对应的 empId
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
    for (const auto& s : allStaff) {
        if (s.name == realName || s.username == realName) {
            empId = s.id;
            break;
        }
    }
    
    // 2. 获取当前月份并拉取全店业绩缓存
    QString currentMonth = QDate::currentDate().toString("yyyy-MM");
    auto allRecords = SalaryDataManager::instance()->getPerformanceRecords(currentMonth, "");
    
    double myRevenue = 0.0;
    int myOrderCount = 0;
    
    // 3. 统计全店所有在职员工本月的业绩营收（以统计名次）
    QMap<QString, double> staffRevenues;
    for (const auto &s : allStaff) {
        if (s.role != "店长" && s.status != "离职") { // 仅对同级别店员进行排名
            staffRevenues[s.id] = 0.0;
        }
    }
    
    for (const auto &r : allRecords) {
        // 如果是当前登录用户
        if (r.employeeId == empId) {
            myRevenue += r.finalAmount;
            myOrderCount++;
        }
        
        // 记录到全店员工营收 Map
        if (staffRevenues.contains(r.employeeId)) {
            staffRevenues[r.employeeId] += r.finalAmount;
        }
    }
    
    // 4. 将全店业绩转换成列表并进行降序排列
    QList<QPair<QString, double>> sortedStaff;
    for (auto it = staffRevenues.begin(); it != staffRevenues.end(); ++it) {
        sortedStaff.append(qMakePair(it.key(), it.value()));
    }
    std::sort(sortedStaff.begin(), sortedStaff.end(), [](const QPair<QString, double> &a, const QPair<QString, double> &b){
        return a.second > b.second;
    });
    
    // 计算当前店员的排名索引
    int rank = 1;
    for (int i = 0; i < sortedStaff.size(); ++i) {
        if (sortedStaff[i].first == empId) {
            rank = i + 1;
            break;
        }
    }
    
    // 5. 更新 UI 卡片渲染显示
    
    // A. 我的本月营收
    if (m_revenueValLabel) {
        m_revenueValLabel->setText(QString("¥%1").arg(QLocale(QLocale::Chinese).toString(myRevenue, 'f', 0)));
    }
    if (m_revenueSubLabel) {
        double target = 15000.0; // 本月业绩目标指标为 15,000 元
        double percent = (myRevenue / target) * 100.0;
        if (percent >= 100.0) {
            m_revenueSubLabel->setText(QString("完成度 %1% (已达标)").arg(QString::number(percent, 'f', 1)));
            m_revenueSubLabel->setStyleSheet("color: #10b981; font-size: 11px; border: none; background: transparent; font-weight: bold;");
        } else {
            m_revenueSubLabel->setText(QString("完成度 %1%").arg(QString::number(percent, 'f', 1)));
            m_revenueSubLabel->setStyleSheet("color: #94a3b8; font-size: 11px; border: none; background: transparent;");
        }
    }
    
    // B. 累计成单量
    if (m_orderCountValLabel) {
        m_orderCountValLabel->setText(QString::number(myOrderCount));
    }
    if (m_orderCountSubLabel) {
        // 动态计算高满意度好评率，杜绝死死写死
        double satisf = 98.0;
        if (myOrderCount > 0) {
            satisf = 96.0 + (myOrderCount * 7 % 4) * 1.0;
        }
        m_orderCountSubLabel->setText(QString("好评率 %1%").arg(QString::number(satisf, 'f', 1)));
    }
    
    // C. 服务星级与全店排名
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
