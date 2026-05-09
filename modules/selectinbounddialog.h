#ifndef SELECTINBOUNDDIALOG_H
#define SELECTINBOUNDDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include "../common_types.h"

class SelectInboundDialog : public QDialog {
    Q_OBJECT
public:
    explicit SelectInboundDialog(QWidget *parent = nullptr);

private slots:
    void onConfirm();
    void onRefresh();

private:
    void setupUI();
    void loadData();

    QTableWidget *m_table;
    QPushButton *m_confirmBtn;
};

#endif
