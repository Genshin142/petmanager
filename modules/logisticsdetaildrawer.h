#ifndef LOGISTICSDETAILDRAWER_H
#define LOGISTICSDETAILDRAWER_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPropertyAnimation>
#include "../common_types.h"

class LogisticsDetailDrawer : public QWidget
{
    Q_OBJECT
public:
    explicit LogisticsDetailDrawer(QWidget *parent = nullptr);
    void showTask(const LogisticsTask &task);
    void showEmpty();
    void showBigImage(const QString &path);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void closeDrawer();

signals:
    void taskCompleted(const QString &taskId);
    void taskModified(const QString &taskId);

private:
    void setupUI();
    QWidget* createInfoBlock(const QString &title, const QString &content, const QString &icon = "");
    
    QString m_selectedTaskId;
    QString m_currentAvatarPath;
    LogisticsTask m_currentTask;
    QLabel *m_avatarLabel;
    QLabel *m_nameLabel;
    QLabel *m_idLabel;
    QLabel *m_statusTag;
    
    QWidget *m_contentArea;
    QVBoxLayout *m_contentLayout;
    
    QPropertyAnimation *m_anim;
    QPushButton *m_primaryBtn;
    QWidget *m_footer;
};

#endif // LOGISTICSDETAILDRAWER_H
