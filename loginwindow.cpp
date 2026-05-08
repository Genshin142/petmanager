#include "loginwindow.h"
#include "ui_loginwindow.h"
#include "mainwindow.h"
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoginWindow)
{
    ui->setupUi(this);
    setFixedSize(500, 550); // 固定登录窗口的大小，防止形变
    setWindowFlags(Qt::FramelessWindowHint); // 无边框窗口更具现代感
    setAttribute(Qt::WA_TranslucentBackground);// 设置窗口背景透明，配合阴影效果更美观
    //可移动窗口
    this->setMouseTracking(true);

    // 设置阴影效果
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(ui->mainFrame);
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 80));
    shadow->setOffset(0, 0);
    ui->mainFrame->setGraphicsEffect(shadow);

    // 绘制圆角头像
    QPixmap originalPixmap(":/images/load_img.jpg");
    // 假设 label 大小设计为 120x120
    int radius = 60; 
    QPixmap targetPixmap(radius * 2, radius * 2);
    targetPixmap.fill(Qt::transparent); // 背景透明
    
    QPainter painter(&targetPixmap);
    painter.setRenderHint(QPainter::Antialiasing); // 抗锯齿
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    QPainterPath path;
    path.addEllipse(0, 0, radius * 2, radius * 2); // 圆形路径
    painter.setClipPath(path);
    
    // 把原图缩放铺满这个圆形并画上去
    painter.drawPixmap(0, 0, radius * 2, radius * 2, originalPixmap.scaled(radius * 2, radius * 2, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    
    // 给圆形外面加一个白色的描边框 (跟刚才的 QSS border: 2px solid white 一致)
    QPen pen(Qt::white);
    pen.setWidth(4); // 线宽，实际视觉效果会是 2px，因为一半画在外面被裁掉
    painter.setPen(pen);
    painter.drawEllipse(0, 0, radius * 2, radius * 2);
    
    ui->logoLabel->setPixmap(targetPixmap);

    connect(ui->closeBtn, &QPushButton::clicked, this, &QWidget::close);

}

LoginWindow::~LoginWindow()
{
    delete ui;
}

void LoginWindow::on_loginBtn_clicked()
{
    QString user = ui->usernameEdit->text();
    QString pwd = ui->passwordEdit->text();

    if (user == "admin" && pwd == "123456") {
        MainWindow *mw = new MainWindow(ADMIN, "系统管理员 (管理员)");
        mw->show();
        this->close();
    } 
    else if (user == "staff01" && pwd == "123456") {
        MainWindow *mw = new MainWindow(STAFF, "店员小张 (staff01)");
        mw->show();
        this->close();
    }
    else {
        QMessageBox::warning(this, "登录失败", "用户名或密码错误！\n测试账号：admin 或 staff01 (密码均为 123456)");
    }
}

void LoginWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }

}

void LoginWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

void LoginWindow::keyPressEvent(QKeyEvent *event)
{
    //按下回车触发登录
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        on_loginBtn_clicked();
    }
}
