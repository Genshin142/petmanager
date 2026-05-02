#ifndef APPOINTMENTDETAILDRAWER_H
#define APPOINTMENTDETAILDRAWER_H

#include <QWidget>
#include <QPropertyAnimation>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QStyleOption>
#include <QPainter>
#include "../common_types.h"
#include <QInputDialog>

class AppointmentDetailDrawer : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int sideWidth READ width WRITE setFixedWidth)

public:
    explicit AppointmentDetailDrawer(QWidget *parent = nullptr);

    void setAppointment(const QString &id);
    void clearSelection();
    void showDrawer();
    void hideDrawer();
    bool isOpened() const { return m_isOpened; }
    AppointmentInfo currentInfo() const { return m_currentInfo; }

signals:
    void modifyRequested(const AppointmentInfo &info);
    void cancelRequested(const QString &id);
    void confirmRequested(const QString &id); // 新增确认信号
    void startServiceRequested(const QString &id, const QString &staff);
    void completeRequested(const QString &id); // 新增：完成服务信号
    void mediaUploadRequested(const QString &id); // 新增：上传记录照片信号
    void galleryRequested(const QStringList &paths, int index); // 新增：画廊浏览信号
    void imageClicked(const QString &path);

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void setupUI();
    QWidget* createSectionTitle(const QString &text);
    QFrame* createSeparator();
    void addInfoRow(QGridLayout *grid, int row, const QString &label, const QString &value);
    
    bool m_isOpened = false;
    QPropertyAnimation *m_animation;
    AppointmentInfo m_currentInfo;

    // UI 组件
    QLabel *m_avatarLabel;
    QLabel *m_petNameLabel;
    QLabel *m_genderLabel;
    QLabel *m_idLabel;
    QLabel *m_statusTag;
    QLabel *m_logisticsHint; // 新增：物流状态提示
    
    // 布局容器
    QStackedWidget *m_mainStack;
    QVBoxLayout *m_specLayout;

    QPushButton *m_confirmBtn;
    QPushButton *m_startBtn;
    QPushButton *m_mediaBtn; 
    QPushButton *m_completeBtn; // 新增
    QPushButton *m_cancelBtn;
    QPushButton *m_modifyBtn;
    
    // 影像轮播组件
    QWidget *m_photoCarousel;
    QWidget *m_variableWidget; // 新增：变量区域容器
    QLabel *m_photoLabel;
    QPushButton *m_delPhotoBtn; // 新增：影像删除按钮
    int m_photoIndex = 0;
    QStringList m_photos;
    bool m_isDeletingPhoto = false; // 新增：标记位防止自触发刷新闪烁
    void updateCarousel();
    
    QWidget *btnContainer;

private slots:
    void onStartBtnClicked();
};

#endif // APPOINTMENTDETAILDRAWER_H
