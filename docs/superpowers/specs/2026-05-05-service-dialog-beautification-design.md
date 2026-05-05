# 服务项目对话框美化设计文档

## 1. 背景与目标
为了提升 PetManager 的整体视觉统一性，需要将“新增/编辑服务项目”对话框从传统的 Qt 风格升级为现代化的卡片式风格，并增加“服务描述”字段。

## 2. 界面设计方案
### 2.1 整体结构
- **窗体样式**：采用无边框 (Frameless) 窗口，四周带圆角 (12px) 和阴影效果。
- **背景层**：半透明背景，主内容区为纯白卡片 (`#ffffff`)。
- **标题栏**：蓝色背景 (`#3b82f6`)，白色文字，包含标题和关闭按钮。

### 2.2 字段布局 (分组模式)
采用双列网格布局，将字段分为三个逻辑区块：

1. **基础信息 (Basic Info)**
   - 服务名称 (`QLineEdit`)
   - 所属分类 (`QComboBox`)
   - 服务编码 (`QLineEdit`, 占满一行，提示语：自动生成可留空)

2. **财务与时间 (Finance & Time)**
   - 服务价格 (`QLineEdit`, 带单位“元”)
   - 标准时长 (`QLineEdit`, 带单位“分钟”)
   - 固定提成 (`QLineEdit`, 占满一行，带单位“元”)

3. **详细说明 (Description)**
   - 服务描述 (`QTextEdit`, 占满一行)

### 2.3 视觉细节 (QSS)
- **输入框**：背景色 `#f8fafc`，边框 `#e2e8f0`，圆角 6px。焦点状态边框变为蓝色。
- **单位标签**：使用辅助色 `#94a3b8`，字体略小，紧跟输入框。
- **分组标题**：蓝色文字，左侧带 4px 粗实线装饰。

## 3. 技术实现细节
### 3.1 核心组件
- **`ServiceDialog`**: 继承自 `QDialog`。
- **`ServiceInfo`**: 更新结构体以支持 `description` 字段。

### 3.2 样式代码
```css
QDialog { background: transparent; }
QFrame#bgFrame { background: #ffffff; border-radius: 12px; }
QLabel#titleLabel { font-size: 18px; color: #ffffff; font-weight: bold; }
QLineEdit, QComboBox, QTextEdit { 
    border: 1px solid #e2e8f0; 
    border-radius: 6px; 
    padding: 8px; 
    background: #f8fafc; 
}
QLineEdit:focus, QComboBox:focus, QTextEdit:focus { 
    border: 2px solid #3b82f6; 
}
```

### 3.3 逻辑变动
- 更新 `getServiceInfo()` 和 `setServiceInfo()` 以处理 `description` 字段。
- 确保 `common_types.h` (或相关的) 中的 `ServiceInfo` 结构体包含 `QString description`。

## 4. 验收标准
- 界面风格与“新增会员档案”对话框完全一致。
- 能够正常保存和回显“服务描述”信息。
- 所有输入框和按钮的交互状态（悬浮、焦点、点击）工作正常。
