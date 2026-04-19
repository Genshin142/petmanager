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
#include "pettimelinemodel.h"
#include "timelinedelegate.h"
#include "compactcalendar.h"
#include "../common_types.h"

class PetRecordDrawer : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int sideWidth READ width WRITE setFixedWidth)

public:
    explicit PetRecordDrawer(QWidget *parent = nullptr);
    void setPet(const PetInfo &info, const QList<PetActivityLog> &logs, const QList<FosterBatch> &batches = {});
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

    // New Header Elements
    QLabel *m_statusDateLabel;
    QPushButton *m_toggleCalendarBtn;
    QWidget *m_calendarContainer;

    // Body Area
    QWidget *m_bodyArea;
    QWidget *m_emptyPlaceholder;
    QListView *m_timelineView;
    
    PetTimelineModel *m_timelineModel;
    TimelineDelegate *m_timelineDelegate;

    CompactCalendar *m_calendar;

    QStackedWidget *m_bottomStack; // For switching between Record and Departed status
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
