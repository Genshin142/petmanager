#include <QApplication>
#include "loginwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 设置通用样式
    a.setStyleSheet("QWidget { font-family: 'Microsoft YaHei'; font-size: 14px; }"
                    "QHeaderView::section { background-color: #f2f6fc; color: #606266; padding: 4px; border: none; font-weight: bold; }");

    LoginWindow w;
    w.show();
    
    return a.exec();
}
