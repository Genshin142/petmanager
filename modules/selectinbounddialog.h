#ifndef SELECTINBOUNDDIALOG_H
#define SELECTINBOUNDDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include "../common_types.h"

class QLabel;
class QTableWidget;
class QPushButton;

class SelectInboundDialog : public QDialog {
    Q_OBJECT
public:
    explicit SelectInboundDialog(QWidget *parent = nullptr);

signals:
    void shelvedOptimistically(const StockInRecord &rec);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onConfirm();

private:
    void setupUI();
    void showBigImage(const QPixmap &pix);
    void hideBigImage();

    QTableWidget *m_table;
    QPushButton *m_confirmBtn;
    QLabel *m_overlay = nullptr;
    QList<StockInRecord> m_currentList; 
};

#endif
