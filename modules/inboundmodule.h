#ifndef INBOUNDMODULE_H
#define INBOUNDMODULE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDateEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QFrame>
#include <QButtonGroup>
#include "../common_types.h"

class InboundModule : public QWidget
{
    Q_OBJECT
public:
    explicit InboundModule(UserRole role, QWidget *parent = nullptr);

private slots:
    void onNewRegistration();
    void onFilterChanged();
    void onRecordSelected();
    void updateRecordList();
    void updateStats();
    void onPrevPage();
    void onNextPage();
    void onDeleteRecord();
    void onRestoreRecord();
    void onHardDeleteRecord();
    void onEditProductFromDrawer();

private:
    void setupUI();
    void setupDetailDrawer();
    void openDrawer();
    void closeDrawer();
    
    // Image Preview & Dots helpers (for Detail Panel)
    void switchDetailImage(bool next);
    void updateDetailDots();
    void showLargePreview();
    void updateLargeDots();
    void switchImage(bool next);
    void updateDots();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    // Filter Bar Components
    QLineEdit *m_searchEdit;
    class CustomCalendarEdit *m_startDateEdit;
    class CustomCalendarEdit *m_endDateEdit;
    QButtonGroup *m_categoryGroup;
    QString m_selectedCategory = "全部";

    QTableWidget *m_recordTable;

    // Stats Labels
    QLabel *m_totalCategoriesLabel;
    QLabel *m_todayItemsLabel;
    QLabel *m_pendingShelvesLabel;
    QLabel *m_totalCostLabel;

    // UI Components - Detail Drawer
    QWidget *m_detailDrawer;
    QFrame *m_drawerContainer;
    QPushButton *m_editBtn;
    QVBoxLayout *m_detailContentLayout;
    
    QLabel *m_detailPreviewImg;
    QWidget *m_detailDotsContainer;
    QHBoxLayout *m_detailDotsLayout;
    QStringList m_detailImagePaths;
    int m_detailImgIndex = 0;

    bool m_isDrawerOpen = false;

    // Image Viewer Overlay
    QWidget *m_backdrop;
    QLabel *m_largePreviewImg;
    QWidget *m_largeDotsContainer;
    QHBoxLayout *m_largeDotsLayout;
    
    // Temporary storage for image viewer
    QStringList m_imagePaths;
    int m_currentImgIndex = 0;
    
    // Pagination
    QPushButton *prevBtn;
    QPushButton *nextBtn;
    QLabel *pageLabel;
    int m_currentPage = 1;
    int m_pageSize = 15;
    UserRole m_role;
};

#endif // INBOUNDMODULE_H
