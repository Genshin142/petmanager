#ifndef MEMBERMODULE_H
#define MEMBERMODULE_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QStackedLayout>

#include "common_types.h"

class MemberModule : public QWidget
{
    Q_OBJECT
public:
    explicit MemberModule(UserRole role, QWidget *parent = nullptr);

private slots:
    void showAddMemberDialog();
    void onCellClicked(int row, int column);
    void onSearchTextChanged(const QString &text);
    void exportData();
    void updateStatistics();

private:
    void setupUI();
    void addSampleData();
    void addRow(const QString &id, const QString &name, const QString &phone, const QString &level, double balance, double consume_amt, int pts, const QString &pets);
    void styleRow(int row);
    
    QTableWidget *memTable;
    QLineEdit *searchEdit;
    
    // 进阶功能组件
    QStackedLayout *stackLayout;
    QWidget *emptyStateWidget;
    QLabel *totalMemberLabel;
    QLabel *goldMemberLabel;
    QLabel *platinumMemberLabel;
    QLabel *diamondMemberLabel;
    UserRole m_role;
};

#endif // MEMBERMODULE_H
