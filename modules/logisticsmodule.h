#ifndef LOGISTICSMODULE_H
#define LOGISTICSMODULE_H

#include <QWidget>

class QVBoxLayout;
class QFlowLayout;
class QComboBox;
class QScrollArea;
class QLabel;

class LogisticsModule : public QWidget
{
    Q_OBJECT
public:
    explicit LogisticsModule(QWidget *parent = nullptr);

public slots:
    void refreshTasks();
    void showCreateTaskDialog();
    void showHistoryDialog();
    void showBigImage(const QString &path);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

private:
    void setupUI();
    void renderTaskCards(const QString &filterStatus = "全部");

    QVBoxLayout *m_mainLayout;
    QComboBox *m_filterCombo;
    QScrollArea *m_scrollArea;
    QWidget *m_cardsContainer;

    QLabel *m_lblTotal;
    QLabel *m_lblPending;
    QLabel *m_lblTransit;
    QLabel *m_lblCompleted;

    void updateStatistics();
};

#endif // LOGISTICSMODULE_H
