#ifndef FOSTERMODULE_H
#define FOSTERMODULE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QAbstractButton>
#include <QGridLayout>
#include <QDateEdit>
#include <QFrame>
#include <QDialog>
#include <QScrollArea>
#include <QPainter>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QComboBox>
#include <QTextEdit>
#include "../common_types.h"

class QPropertyAnimation;
class CompactCalendar;

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
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.fillRect(rect(), QColor(0, 0, 0, 180));
    }
    void mousePressEvent(QMouseEvent *) override {
        accept();
    }
};

// 全屏图片预览弹窗
class FullImagePreviewDialog : public QDialog {
    Q_OBJECT
public:
    explicit FullImagePreviewDialog(const QString &path, QWidget *parent = nullptr);
protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override { accept(); }
};

// 可点击的图片标签
class ClickableLabel : public QLabel {
    Q_OBJECT
public:
    explicit ClickableLabel(QWidget *parent = nullptr);
signals:
    void clicked();
protected:
    void mouseReleaseEvent(QMouseEvent *e) override;
};

// 内部使用的记录条目卡片
class HistoryItemCard : public QFrame {
    Q_OBJECT
public:
    HistoryItemCard(const QString &petName, const QString &roomInfo, const QString &period, const QString &status = "已完成", QWidget *parent = nullptr);
    void setSelected(bool selected);
signals:
    void clicked();
protected:
    void mousePressEvent(class QMouseEvent *event) override;
};

// 顶部总结卡片
class SummaryCard : public QFrame {
    Q_OBJECT
public:
    explicit SummaryCard(QWidget *parent = nullptr);
};

// 影像记录展示区
class MediaGallery : public QWidget {
    Q_OBJECT
public:
    explicit MediaGallery(QWidget *parent = nullptr);
    void setMedia(const QList<PetMedia> &media);
};

// 影像详情预览弹窗
class MediaDetailDialog : public QDialog {
    Q_OBJECT
public:
    explicit MediaDetailDialog(const PetMedia &media, QWidget *parent = nullptr);
protected:
    void paintEvent(class QPaintEvent *event) override;
    void mousePressEvent(class QMouseEvent *event) override;
};

// 影像与日志录入弹窗
class MediaUploadDialog : public QDialog {
    Q_OBJECT
public:
    explicit MediaUploadDialog(const QString &petId, QWidget *parent = nullptr);
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
signals:
    void recordAdded(const QString &category, const QString &log, const QStringList &images);
private slots:
    void onUploadClicked();
private:
    void setupUI();

    QFrame *m_previewArea;
    QLabel *m_uIcon;
    QLabel *m_uText;
    QComboBox *m_typeCombo;
    QComboBox *m_logTypeCombo;
    QComboBox *m_operatorCombo;
    QTextEdit *m_logEdit;
    QString m_petId;
    QStringList m_filePaths;
};

// 影像留档专用的沉浸式弹窗 (与宠物模块保持一致)
class PetMediaArchiveDialog : public QDialog {
    Q_OBJECT
public:
    PetMediaArchiveDialog(const QString &petName, const QList<PetMedia> &media, QWidget *parent = nullptr);
protected:
    void paintEvent(QPaintEvent *event) override;
};

// 寄养明细容器：整合 Summary, Media, Timeline 和 Financials
class FosterHistoryDetailWidget : public QWidget {
    Q_OBJECT
public:
    explicit FosterHistoryDetailWidget(QWidget *parent = nullptr);
    void setDetails(const QList<PetActivityLog> &logs, const QList<PetMedia> &media);
private:
    class PetTimelineWidget *m_timeline;
    MediaGallery *m_gallery;
};

// 往期记录详情弹窗：解决“弹窗叠抽屉”的层级冲突，提供更宽的历史纵览视图
class HistoryRecordDialog : public QDialog {
    Q_OBJECT
public:
    explicit HistoryRecordDialog(int roomId, const QString &petId = "", const QString &petName = "", QWidget *parent = nullptr);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    void setupUI(int roomId, const QString &petId, const QString &petName);
    QList<HistoryItemCard*> m_cards;
};

// 带遮罩的动态详情弹窗（根据房间状态展示不同内容）
class FosterDetailDialog : public QDialog {
    Q_OBJECT
public:
    explicit FosterDetailDialog(int roomId, const QString &status, const QString &petId, const QString &petName, const QString &petBreed = "", const QString &ownerName = "", QWidget *parent = nullptr);
signals:
    void requestBooking(int roomId);
    void requestHistory(int roomId, const QString &petId, const QString &petName);
protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
private:
    void buildOccupiedView(QVBoxLayout *layout, int roomId, const QString &petId, const QString &petName, const QString &petBreed, const QString &ownerName);
    void buildBookedView(QVBoxLayout *layout, int roomId, const QString &petId, const QString &petName, const QString &petBreed, const QString &ownerName);
    void buildFreeView(QVBoxLayout *layout, int roomId);
    void buildCleaningView(QVBoxLayout *layout, int roomId);
    QString m_status;
};

class PillButton : public QPushButton {
    Q_OBJECT
public:
    explicit PillButton(const QString &text, const QString &accentColor, QWidget *parent = nullptr);
private slots:
    void updateStyle();
private:
    QString m_accent;
};

class FosterActionPanel : public QFrame {
    Q_OBJECT
public:
    explicit FosterActionPanel(QWidget *parent = nullptr);
    void updatePanel(int roomId, const QString &status, const QString &petId = "", const QString &petName = "");
    bool hasUnsavedChanges() const;
    void resetChanges();

signals:
    void avatarClicked(const QString &path);

private:
    void setupUI();
    void showEmptyPlaceholder();
    void clearContentLayout();
public:
    void showCheckInForm(int roomId = -1, const QDate &startDate = QDate());
    void showManagementView(int roomId, const QString &petId, const QString &status);
    void showMaintenanceView(int roomId, const QString &status);
    bool eventFilter(QObject *obj, QEvent *event) override;

    QVBoxLayout *m_contentLayout;
    QWidget *m_currentWidget = nullptr;
    QLabel *m_avatarLabel = nullptr;
    PetInfo m_currentInfo;
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
    void showBigImage(const QString &path);
    void hideBigImage();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QGridLayout *roomGrid;
    QLabel *totalRoomsLabel;
    QLabel *occupiedLabel;
    QLabel *freeLabel;
    QLabel *bookedLabel;
    QLabel *cleaningLabel;
    QDateEdit *forecastDateEdit;
    
    QScrollArea *m_scrollArea;
    QWidget *m_gridContainer;
    class QStackedWidget *m_viewStack;
    class QTableView *m_ganttView;
    class FosterGanttModel *m_ganttModel;
    
    bool m_isTimelineMode = false;
    QDate m_currentForecastDate;
    CompactCalendar *m_calendar;
    FosterActionPanel *m_actionPanel;

    // 大图预览组件
    QWidget *m_imagePreviewOverlay = nullptr;
    QLabel *m_previewLabel = nullptr;
};

#endif
