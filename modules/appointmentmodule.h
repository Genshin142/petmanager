#ifndef APPOINTMENTMODULE_H
#define APPOINTMENTMODULE_H

#include <QWidget>
#include "addappointmentdialog.h"

#include <QListView>
#include <QStandardItemModel>
#include <QStackedWidget>
#include <QDate>
#include <QComboBox>
#include "appointmentitemdelegate.h"
#include "appointmentdetaildrawer.h"

class AppointmentModule : public QWidget {
    Q_OBJECT
public:
    explicit AppointmentModule(QWidget *parent = nullptr);

    void updatePagination();
    void updateStats();

private slots:
    void onAppointmentSelected(const QModelIndex &index);
    void onFilter(const QString &text);
    void onStatusFilterChanged(int index);
    void onPrevDay();
    void onNextDay();
    void onToday();
    void onAddAppointment();
    
    // 业务流转接口
    void handleConfirm(const QString &id);
    void handleStartService(const QString &id, const QString &staff);
    void handleComplete(const QString &id); // 新增
    void handleCancel(const QString &id);
    void handleModify(const AppointmentInfo &info);
    void handleMediaUpload(const QString &id); // 新增
    void handleGallery(const QStringList &paths, int index); // 新增
    void showBigImage(const QString &path);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void setupUI();
    void refreshView(); // 根据当前模式刷新
    void autoExpireStaleAppointments(); // 自动清理过期未闭环单据
    void renderGrid();  // 渲染格栅视图
    void renderList();  // 渲染搜索列表
    bool isAppointmentOverdue(const AppointmentInfo &info) const; // 判断预约是否超时
    void updateDateBtnText();

    // 布局组件
    QStackedWidget *m_stack;
    QWidget *m_gridPage;
    QWidget *m_listPage;

    // 格栅组件
    QVBoxLayout *m_todayGrid;
    QVBoxLayout *m_tomorrowGrid;
    QLabel *m_todayTitle;
    QLabel *m_tomorrowTitle;
    QPushButton *m_dateBtn;

    // 列表组件
    QListView *m_listView;
    QStandardItemModel *m_model;
    AppointmentItemDelegate *m_delegate;
    
    AppointmentDetailDrawer *m_drawer;

    // 统计组件
    QLabel *m_statTotal;
    QLabel *m_statGrooming;
    QLabel *m_statLogistics;
    QLabel *m_statBoarding;

    // 状态
    QDate m_currentDate;
    int m_currentPage = 1;
    int m_pageSize = 8; 
    QString m_filterText;
    QString m_statusFilter = "全部";

    QLineEdit *m_searchEdit;
    QString m_selectedTaskId; // 新增：当前选中的任务ID
    QComboBox *m_statusCombo;
    QLabel *m_pageLabel;
    QLineEdit *m_jumpEdit;
    QPushButton *m_prevBtn;
    QPushButton *m_nextBtn;
};

#endif // APPOINTMENTMODULE_H
