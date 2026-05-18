#include "globallightbox.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QGraphicsOpacityEffect>
#include <QApplication>
#include <QScreen>
#include <QDateTime>
#include "../utils/imageutils.h"

void GlobalLightbox::showImages(QWidget *parent, const QStringList &paths, int startIndex)
{
    GlobalLightbox *lightbox = new GlobalLightbox(parent);
    lightbox->setImages(paths, startIndex);
    lightbox->show();
    // 依赖 Qt 对象树和 DeleteOnClose 机制管理生命周期
    lightbox->setAttribute(Qt::WA_DeleteOnClose);
}

GlobalLightbox::GlobalLightbox(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    // 移除 Qt::Window，使其作为子窗口存在，跟随父窗口缩放/移动
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    
    // 如果有父窗口，大小对齐父窗口
    if (parent) {
        setGeometry(parent->rect());
        parent->installEventFilter(this); // 监听父窗口大小变化
    }
}

GlobalLightbox::~GlobalLightbox()
{
}

void GlobalLightbox::setupUI()
{
    resize(parentWidget() ? parentWidget()->size() : qApp->primaryScreen()->size());

    m_scene = new QGraphicsScene(this);
    m_view = new QGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setRenderHint(QPainter::SmoothPixmapTransform);
    m_view->setDragMode(QGraphicsView::ScrollHandDrag);
    m_view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    m_view->setStyleSheet("background: transparent; border: none;");
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->installEventFilter(this);
    m_view->viewport()->installEventFilter(this);

    m_imageItem = new QGraphicsPixmapItem();
    m_scene->addItem(m_imageItem);

    // 叠加半透明背景
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(0, 0, 0, 215));
    setPalette(pal);
    setAutoFillBackground(true);

    // 操作按钮
    m_prevBtn = new QPushButton("<", this);
    m_nextBtn = new QPushButton(">", this);
    m_closeBtn = new QPushButton("×", this);

    QString btnStyle = "QPushButton { background: rgba(255,255,255,30); color: white; border-radius: 25px; font-size: 24px; border: 1px solid rgba(255,255,255,50); } "
                       "QPushButton:hover { background: rgba(255,255,255,60); }";
    
    m_prevBtn->setFixedSize(50, 50);
    m_nextBtn->setFixedSize(50, 50);
    m_closeBtn->setFixedSize(40, 40);
    
    m_prevBtn->setStyleSheet(btnStyle);
    m_nextBtn->setStyleSheet(btnStyle);
    m_closeBtn->setStyleSheet("QPushButton { background: transparent; color: rgba(255,255,255,180); font-size: 32px; border: none; } QPushButton:hover { color: white; }");

    connect(m_prevBtn, &QPushButton::clicked, this, [this](){ updateImage(m_currentIndex - 1, false); });
    connect(m_nextBtn, &QPushButton::clicked, this, [this](){ updateImage(m_currentIndex + 1, true); });
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::close);

    // 指示点容器
    m_dotsContainer = new QWidget(this);
    m_dotsContainer->setFixedHeight(40);
    m_dotsContainer->setStyleSheet("background: transparent;");
    QHBoxLayout *dotsLayout = new QHBoxLayout(m_dotsContainer);
    dotsLayout->setContentsMargins(0, 0, 0, 0);
    dotsLayout->setSpacing(10);
    dotsLayout->setAlignment(Qt::AlignCenter);
}

void GlobalLightbox::setImages(const QStringList &paths, int startIndex)
{
    m_imagePaths = paths;
    m_currentIndex = startIndex;
    updateImage(m_currentIndex);
}

void GlobalLightbox::updateImage(int index, bool forward)
{
    Q_UNUSED(forward);
    if (index < 0 || index >= m_imagePaths.size()) return;
    m_currentIndex = index;

    QPixmap pix = ImageUtils::loadPixmap(m_imagePaths[m_currentIndex]);
    if (pix.isNull()) pix.load(":/images/load_img.jpg");

    applyCrossFade(pix);

    // 更新指示点
    if (m_dotsContainer && m_dotsContainer->layout()) {
        QLayout *layout = m_dotsContainer->layout();
        QLayoutItem *child;
        while ((child = layout->takeAt(0)) != nullptr) {
            if (child->widget()) child->widget()->deleteLater();
            delete child;
        }

        if (m_imagePaths.size() > 1) {
            for (int i = 0; i < m_imagePaths.size(); ++i) {
                QLabel *dot = new QLabel();
                dot->setFixedSize(8, 8);
                if (i == m_currentIndex) {
                    dot->setStyleSheet("background: #409eff; border-radius: 4px;");
                } else {
                    dot->setStyleSheet("background: rgba(255,255,255,100); border-radius: 4px;");
                }
                layout->addWidget(dot);
            }
        }
    }

    m_prevBtn->setVisible(m_currentIndex > 0);
    m_nextBtn->setVisible(m_currentIndex < m_imagePaths.size() - 1);
}

void GlobalLightbox::applyCrossFade(const QPixmap &newPixmap)
{
    // 简单的交叉淡入淡出模拟
    QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);
    m_view->setGraphicsEffect(eff);
    
    QPropertyAnimation *anim = new QPropertyAnimation(eff, "opacity");
    anim->setDuration(250);
    anim->setStartValue(0.2);
    anim->setEndValue(1.0);
    
    m_imageItem->setPixmap(newPixmap);
    
    // 初始缩放自适应
    m_scene->setSceneRect(newPixmap.rect());
    double viewW = width() * 0.8;
    double viewH = height() * 0.8;
    double scale = qMin(viewW / newPixmap.width(), viewH / newPixmap.height());
    m_view->resetTransform();
    m_view->scale(scale, scale);
    
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void GlobalLightbox::wheelEvent(QWheelEvent *event)
{
    if (m_imagePaths.size() <= 1) return;
    
    // 阈值判断，防止滚动过快瞬间跳过多张
    static qint64 lastWheelTime = 0;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (currentTime - lastWheelTime < 200) return; // 200ms 冷却时间
    
    if (event->angleDelta().y() > 0) {
        // 向上滚：上一张
        int prevIdx = (m_currentIndex - 1 + m_imagePaths.size()) % m_imagePaths.size();
        updateImage(prevIdx, false);
    } else if (event->angleDelta().y() < 0) {
        // 向下滚：下一张
        int nextIdx = (m_currentIndex + 1) % m_imagePaths.size();
        updateImage(nextIdx, true);
    }
    lastWheelTime = currentTime;
}

void GlobalLightbox::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) close();
    else if (event->key() == Qt::Key_Left) m_prevBtn->click();
    else if (event->key() == Qt::Key_Right) m_nextBtn->click();
}

void GlobalLightbox::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    m_view->setGeometry(rect());
    m_closeBtn->move(width() - 60, 20);
    m_prevBtn->move(30, height() / 2 - 25);
    m_nextBtn->move(width() - 80, height() / 2 - 25);
    if (m_dotsContainer) {
        m_dotsContainer->move(0, height() - 60);
        m_dotsContainer->setFixedWidth(width());
    }
}

bool GlobalLightbox::eventFilter(QObject *watched, QEvent *event)
{
    if (m_view && (watched == m_view || watched == m_view->viewport()) && event->type() == QEvent::Wheel) {
        // 转发滚轮事件给弹窗主体处理切图
        QWheelEvent *we = static_cast<QWheelEvent*>(event);
        this->wheelEvent(we);
        return true;
    }
    
    if (parent() && watched == parent() && event->type() == QEvent::Resize) {
        // 跟随父窗口大小变化
        setGeometry(static_cast<QWidget*>(parent())->rect());
    }
    
    return QDialog::eventFilter(watched, event);
}
