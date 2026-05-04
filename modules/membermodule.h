#ifndef MEMBERMODULE_H
#define MEMBERMODULE_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

#include <QComboBox>
#include <QCheckBox>
#include "../common_types.h"
#include "memberdetaildrawer.h"

class MemberModule : public QWidget
{
    Q_OBJECT
public:
    explicit MemberModule(UserRole role, QWidget *parent = nullptr);

signals:
    void sig_petAdded(const PetInfo &pet);
    void sig_requestPetJump(const QString &memberName, const QString &petName);
    void sig_jumpToPetModule(const QString &petId);

private slots:
    void showAddMemberDialog();
    void onCellClicked(int row, int column);
    void onSearchTextChanged(const QString &text);
    void onEditMemberFromDrawer(const MemberInfo &info); // 处理详情页发起的编辑
    void exportData();
    void updateStatistics();
    void onPrevPage();
    void onNextPage();
    void onJumpPage();

    // 批量操作
    void onBatchDelete();

private:
    void setupUI();
    void addSampleData();
    void addRow(const QString &id, const QString &name, const QString &gender, const QString &birthday, const QString &phone, const QString &level, double balance, double consume_amt, int pts, const QString &lastVisit, const QString &pets);
    void updatePagination();
    QString generateNextMemberId();
    
    QTableWidget *memTable;
    QLineEdit *searchEdit;
    QComboBox *levelFilterCombo;
    QString m_currentLevelFilter;
    
    // 统计标签
    QLabel *totalMemberLabel;
    QLabel *regularMemberLabel;
    QLabel *goldMemberLabel;
    QLabel *platinumMemberLabel;
    QLabel *diamondMemberLabel;
    
    // 分页辅助
    QPushButton *prevBtn;
    QPushButton *nextBtn;
    QLineEdit *jumpEdit;
    QIntValidator *jumpValidator;
    QLabel *pageLabel;
    MemberDetailDrawer *m_detailDrawer;
    
    UserRole m_role;
    int m_currentPage;
    int m_pageSize;
};

#endif // MEMBERMODULE_H
