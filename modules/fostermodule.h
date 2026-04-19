#ifndef FOSTERMODULE_H
#define FOSTERMODULE_H

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QDateEdit>
#include <QFrame>
#include <QDialog>
#include <QScrollArea>
#include <QPainter>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>

class QPropertyAnimation;

class FosterCard : public QFrame {
    Q_OBJECT
public:
    explicit FosterCard(int roomNo, const QString &status, const QString &petId, const QString &petName, const QString &petBreed = "", const QString &ownerName = "", QWidget *parent = nullptr);
    QString petId() const { return m_petId; }
    QString petName() const { return m_petName; }
    QString petBreed() const { return m_petBreed; }
    QString ownerName() const { return m_ownerName; }
    QString status() const { return m_status; }
    int roomId() const { return m_roomNo; }

signals:
    void clicked();

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    int m_roomNo;
    QString m_status;
    QString m_petId;
    QString m_petName;
    QString m_petBreed;
    QString m_ownerName;
    QGraphicsDropShadowEffect *m_shadow;
    QLabel *m_avatar = nullptr;
};

// 巨幕预览：头像放大弹窗
class AvatarZoomDialog : public QDialog {
    Q_OBJECT
public:
    explicit AvatarZoomDialog(const QString &iconText, QWidget *parent = nullptr) : QDialog(parent) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        setAttribute(Qt::WA_TranslucentBackground);
        
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignCenter);
        
        QLabel *bigAvatar = new QLabel(iconText);
        bigAvatar->setFixedSize(320, 320);
        bigAvatar->setAlignment(Qt::AlignCenter);
        bigAvatar->setStyleSheet(
            "font-size: 160px; background: white; border-radius: 160px; "
            "border: 4px solid white;"
        );
        
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(50);
        shadow->setColor(QColor(0, 0, 0, 150));
        bigAvatar->setGraphicsEffect(shadow);
        
        layout->addWidget(bigAvatar);
        if (parent) resize(parent->size());
    }

protected:
    void mousePressEvent(QMouseEvent *) override { accept(); }
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.fillRect(rect(), QColor(0, 0, 0, 180));
    }
};

// 带遮罩的动态详情弹窗（根据房间状态展示不同内容）
class FosterDetailDialog : public QDialog {
    Q_OBJECT
public:
    explicit FosterDetailDialog(int roomId, const QString &status, const QString &petId, const QString &petName, const QString &petBreed = "", const QString &ownerName = "", QWidget *parent = nullptr);
signals:
    void requestBooking(int roomId);
    void requestHistory(int roomId);
protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
private:
    void buildOccupiedView(QVBoxLayout *layout, int roomId, const QString &petId, const QString &petName, const QString &petBreed, const QString &ownerName);
    void buildFreeView(QVBoxLayout *layout, int roomId);
    void buildCleaningView(QVBoxLayout *layout, int roomId);
    QString m_status;
};

class FosterModule : public QWidget {
    Q_OBJECT
public:
    explicit FosterModule(QWidget *parent = nullptr);

signals:
    void signal_openBooking(int roomId); // 点击空闲房间时发出

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void setupUI();
    void updateStats();
    void relayoutGrid(); // 核心：根据当前宽度重排网格

private slots:
    void onForecastDateChanged(const QDate &date);
    void onCardClicked();
    void onToggleViewMode(); // 切换看板/时间轴模式

private:
    QGridLayout *roomGrid;
    QLabel *totalRoomsLabel;
    QLabel *occupiedLabel;
    QLabel *freeLabel;
    QLabel *cleaningLabel;
    QDateEdit *forecastDateEdit;
    
    QScrollArea *m_scrollArea;
    QWidget *m_gridContainer;
    bool m_isTimelineMode = false;
    QDate m_currentForecastDate;
    class CompactCalendar *m_calendar;
};

#endif
