#ifndef PETRECORDDRAWER_H
#define PETRECORDDRAWER_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTextEdit>
#include <QComboBox>
#include <QPropertyAnimation>
#include <QListView>
#include <QStackedWidget>
#include "pettimelinewidget.h"
#include "compactcalendar.h"
#include "../common_types.h"
#include "fostermodule.h"

class PetRecordDrawer : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int sideWidth READ width WRITE setFixedWidth)

public:
    explicit PetRecordDrawer(QWidget *parent = nullptr);
    void setPet(const PetInfo &info, const QList<PetActivityLog> &logs, const QList<PetMedia> &media = {}, const QList<FosterBatch> &batches = {});
    void addLogItem(const PetActivityLog &log);
    
    void updateBatchStatus(const QString &status);
    
    void showDrawer();
    void hideDrawer();
    bool isOpened() const { return m_isOpened; }

signals:
    void logAdded(const QString &petId, const PetActivityLog &log);
    void avatarClicked(const QString &path);
    void closeRequested();

private slots:
    void onSubmitQuickAction();
    void onCalendarClicked(const QDate &date);
    void onBatchChanged(int index);
    void onToggleCalendar();
    void onEditLog(const QModelIndex &index);
    void onDeleteLog(const QModelIndex &index);

private:
    void setupUI();
    void updateEmptyState();
    bool eventFilter(QObject *obj, QEvent *event) override;

    PetInfo m_currentPet;
    QList<PetActivityLog> m_allLogs;
    bool m_isOpened;

    // UI Elements
    QLabel *m_avatarLabel;
    QLabel *m_nameLabel;
    QLabel *m_breedLabel;
    QLabel *m_ownerLabel;
    QLabel *m_statusBadge; // 新增寄养房号标签
    QPushButton *m_archiveBtn; // 影像留档入口按钮

    // Body Area
    PetTimelineWidget *m_timelineWidget;
    QList<PetMedia> m_currentMedia;
    QList<FosterBatch> m_currentBatches;

    // 新增：与寄养模块一致的摘要卡片成员
    QWidget *m_detailCard;
    QLabel *m_weightInVal;
    QLabel *m_weightOutVal;
    QLabel *m_dateInVal;
    QLabel *m_dateOutVal;
    QLabel *m_durationVal;
    QLabel *m_dateOutTitle;
    QPushButton *m_periodBtn;

    QStackedWidget *m_bottomStack; 
    QWidget *m_recordPanel;
    QComboBox *m_typeCombo;
    QComboBox *m_operatorCombo;
    QTextEdit *m_remarkEdit;
    QPushButton *m_addBtn;
    
    QWidget *m_tipPanel;
    QLabel *m_tipLabel;
    
    
    QPropertyAnimation *m_animation;
    QPropertyAnimation *m_calendarAnimation;
};

#endif // PETRECORDDRAWER_H
