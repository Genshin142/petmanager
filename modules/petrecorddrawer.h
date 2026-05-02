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
    
    QWidget* createArchivePage();
    QWidget* createBoardingPage();
    QWidget* createServiceHistoryPage();

    QString m_selectedStartDate;
    QString m_selectedEndDate;
    PetInfo m_currentPet;
    QList<PetActivityLog> m_allLogs;
    QList<PetMedia> m_currentMedia;
    QList<FosterBatch> m_currentBatches;
    bool m_isOpened;

    // Header Elements
    QLabel *m_avatarLabel;
    QLabel *m_nameLabel;
    QLabel *m_breedLabel;
    QLabel *m_ownerLabel;
    QLabel *m_roomBadge; 

    // Tab Navigation
    QButtonGroup *m_tabGroup;
    QStackedWidget *m_stackedWidget;

    // 1. Archive Page Elements
    QLabel *m_valGender;
    QLabel *m_valAge;
    QLabel *m_valOwnerName;
    QLabel *m_valOwnerPhone;
    QLabel *m_valOwnerId;
    QLabel *m_valHealthStatus;
    QPushButton *m_vaccineBtn;
    QLabel *m_valWeight;
    QLabel *m_valMedicalHistory;
    QLabel *m_valDietary;

    // 2. Boarding Page Elements
    QStackedWidget *m_boardingStack;
    QWidget *m_detailCard;
    QLabel *m_weightInVal;
    QLabel *m_weightOutVal;
    QLabel *m_dateInVal;
    QLabel *m_dateOutVal;
    QLabel *m_durationVal;
    QLabel *m_dateOutTitle;
    QPushButton *m_periodBtn;
    PetTimelineWidget *m_timelineWidget;
    QVBoxLayout *m_serviceHistoryLayout; // 新增：服务记录容器

    QStackedWidget *m_bottomStack; 
    QPropertyAnimation *m_animation;
};

#endif // PETRECORDDRAWER_H
