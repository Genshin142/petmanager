# 数据报表模块扇形图汉化 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将数据报表模块中的营收项目构成和支付渠道分布两个扇形图（饼图）中的分类标签翻译为对应的中文，解决当前显示为英文的问题。

**Architecture:** 在客户端 `modules/statsmodule.cpp` 的 `updatePie` lambda 函数中引入映射字典，在循环提取分类名称时进行条件拦截并翻译为中文标签，保证底层数据库存储键名和网络通信数据结构不被改动。

**Tech Stack:** C++ / Qt 6.8.2 / Qt Charts (QPieSeries, QPieSlice)

---

### Task 1: 汉化 `StatsModule::onDashboardStatsReceived` 饼图渲染逻辑

**Files:**
- Modify: `modules/statsmodule.cpp:2359-2386`

- [ ] **Step 1: 添加翻译转换映射逻辑**

修改 `modules/statsmodule.cpp` 中的 `updatePie` lambda 函数，在循环中提取分类名称后添加汉化转换字典。

```cpp
    // 2. 更新饼图
    auto updatePie = [&](QChartView* view, const QJsonArray &arr, const QList<QColor> &colors) {
        if (!view) return;
        QChart *chart = new QChart();
        chart->setBackgroundVisible(false);
        QPieSeries *series = new QPieSeries();
        series->setHoleSize(0.4); 
        series->setPieSize(0.7); 
        
        for (int i = 0; i < arr.size(); ++i) {
            QJsonObject obj = arr[i].toObject();
            QString name = obj["name"].toString();
            
            // 翻译模块和支付渠道的英文名称为中文
            if (name == "Appointment") name = "服务预约";
            else if (name == "Product") name = "商品零售";
            else if (name == "Service") name = "服务项目";
            else if (name == "Foster" || name == "Boarding") name = "宠物寄养";
            else if (name == "Logistics") name = "宠物接送";
            else if (name == "MemberCard" || name == "Balance") name = "会员卡余额";
            else if (name == "Alipay") name = "支付宝";
            else if (name == "Wechat" || name == "WeChat") name = "微信支付";
            else if (name == "Cash") name = "现金";
            else if (name == "Card") name = "银行卡";

            double val = obj["value"].toDouble();
            if (val <= 0) continue;
            
            QPieSlice *slice = series->append(name, val);
            if (i < colors.size()) slice->setBrush(colors[i]);
            slice->setLabelVisible(true);
            slice->setLabelPosition(QPieSlice::LabelOutside);
            slice->setLabelFont(QFont("Microsoft YaHei", 10, QFont::Bold));
        }
        for (auto slice : series->slices()) {
            slice->setLabel(QString("%1 %2%").arg(slice->label()).arg(QString::number(slice->percentage() * 100, 'f', 1)));
        }
        chart->addSeries(series);
        chart->setMargins(QMargins(0, 0, 0, 0));
        chart->legend()->hide();
        view->setChart(chart);
    };
```

- [ ] **Step 2: 编译项目验证代码正确性**

通过构建命令编译项目，确保修改语法正确、无编译错误。

- [ ] **Step 3: 运行客户端并核对效果**

运行 `PetManager`，登录后进入“数据报表”模块，截屏或通过肉眼确认“营收项目构成”与“支付渠道分布”两个扇形图的分类标签成功汉化，比例百分比计算正确且排版整齐。
