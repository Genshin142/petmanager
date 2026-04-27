# 宠物性别辨识度优化设计文档

## 1. 背景与目标
在 PetManager 的寄养房态卡片中，已入住（Occupied）状态使用蓝色作为主背景色。目前的性别符号直接渲染为蓝色（公）或粉色（母），导致公犬/公猫的蓝色符号在蓝色背景下对比度极低，难以辨认。
本设计的目标是通过引入“高亮色块点”方案，确保性别信息在任何背景下都具备极高的辨识度。

## 2. 设计方案：高亮色块点 (High Contrast Dot)
该方案将性别符号包裹在一个实心的圆形背景块中，符号本身统一使用白色，通过背景块的颜色来区分性别。

### 2.1 视觉规范
- **公 (Male)**:
  - 圆点背景色: `#409eff` (明亮的系统蓝)
  - 符号: `♂` (白色)
- **母 (Female)**:
  - 圆点背景色: `#f56c6c` (明亮的系统粉/红)
  - 符号: `♀` (白色)
- **尺寸**: 18px x 18px (圆形)
- **对齐**: 垂直居中对齐于品种文字右侧

### 2.2 渲染实现
由于 Qt 的 `QLabel` 支持富文本（HTML），我们将通过内联 CSS 实现该效果：
```html
<span style='display: inline-flex; align-items: center; justify-content: center; width: 18px; height: 18px; background: %1; border-radius: 9px; color: white; font-size: 12px; font-weight: bold;'>%2</span>
```
其中 `%1` 为背景色，`%2` 为性别符号。

## 3. 影响范围
需要更新 `fostermodule.cpp` 中所有显示性别符号的地方：
1. **FosterCard**: 房态主看板卡片。
2. **FosterDetailDialog::buildBookedView**: 预约详情视图。
3. **FosterDetailDialog::buildOccupiedView**: 入住详情视图。
4. **FosterActionPanel::showManagementView**: 右侧操作面板详情。

## 4. 验收标准
- [ ] 在蓝色背景的卡片上，公性符号（蓝色圆点+白色符号）清晰可见。
- [ ] 符号垂直位置与品种名称文字对齐。
- [ ] 全局所有性别符号显示风格保持统一。
