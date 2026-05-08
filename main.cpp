#include <QApplication>
#include "loginwindow.h"
#include "mainwindow.h"
#include "utils/logger.h"

int main(int argc, char *argv[])
{
    Logger::init();
    QApplication a(argc, argv);
    
    // 设置全局字体，确保全程序一致（特别是表格项等非 Widget 元素）
    QFont globalFont("Microsoft YaHei", 10);
    a.setFont(globalFont);
    
    a.setStyleSheet(
        "QWidget { font-family: 'Microsoft YaHei'; } "
        "QHeaderView::section { background-color: white; color: #64748b; font-weight: bold; font-size: 13px; padding: 12px 4px; border: none; border-bottom: 1px solid #f1f5f9; }"
        
        // 全局约束：强制取消所有表格的交替底色与选中行的高亮变化
        "QTableView, QTableWidget {"
        "   alternate-background-color: white !important;" 
        "   selection-background-color: transparent !important;"
        "   selection-color: black !important;"
        "}"
        "QTableView::item:selected, QTableWidget::item:selected {"
        "   background-color: transparent !important;"
        "   color: black !important;"
        "}"
        
        // 全局 ComboBox 下拉列表美化 (同步薪资管理中心样式)
        "QComboBox QAbstractItemView {"
        "   border: 1px solid #e4e7ed;"
        "   background-color: #ffffff;"
        "   border-radius: 4px;"
        "   selection-background-color: #f5f7fa;"
        "   selection-color: #409eff;"
        "   outline: none;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "   height: 35px;"
        "   padding-left: 10px;"
        "   color: #606266;"
        "}"
        "QComboBox QAbstractItemView::item:selected {"
        "   background-color: #f5f7fa;"
        "   color: #409eff;"
        "   border-left: 3px solid #409eff;"
        "}"
        
        // 下拉列表滚动条美化
        "QComboBox QScrollBar:vertical {"
        "   width: 0px;"
        "   background: transparent;"
        "   margin: 0px;"
        "}"
        "QComboBox QScrollBar::handle:vertical {"
        "   background: #dcdfe6;"
        "   border-radius: 4px;"
        "   min-height: 20px;"
        "}"
        "QComboBox QScrollBar::add-line:vertical, QComboBox QScrollBar::sub-line:vertical {"
        "   height: 0px;"
        "}"
    );

    LoginWindow lw;
    lw.show();
    
    return a.exec();
}
