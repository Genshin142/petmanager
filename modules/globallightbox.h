#ifndef GLOBALLIGHTBOX_H
#define GLOBALLIGHTBOX_H

#include <QDialog>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QPropertyAnimation>
#include <QPushButton>
#include <memory>

class GlobalLightbox : public QDialog, public std::enable_shared_from_this<GlobalLightbox> {
    Q_OBJECT
public:
    // 静态工厂：使用 Qt 原生生命周期管理
    static void showImages(QWidget *parent, const QStringList &paths, int startIndex = 0);

    explicit GlobalLightbox(QWidget *parent = nullptr);
    ~GlobalLightbox();

    void setImages(const QStringList &paths, int startIndex = 0);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupUI();
    void updateImage(int index, bool forward = true);
    void applyCrossFade(const QPixmap &newPixmap);

    QStringList m_imagePaths;
    int m_currentIndex = 0;

    QGraphicsView *m_view;
    QGraphicsScene *m_scene;
    QGraphicsPixmapItem *m_imageItem;
    
    QPushButton *m_prevBtn;
    QPushButton *m_nextBtn;
    QPushButton *m_closeBtn;
    
    QWidget *m_dotsContainer; // 新增：指示点容器
    QWidget *m_thumbStrip;
    
    double m_scaleFactor = 1.0;
};

#endif // GLOBALLIGHTBOX_H
