#ifndef MEMBERDETAILDRAWER_H
#define MEMBERDETAILDRAWER_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QStackedWidget>
#include <QButtonGroup>
#include "addmemberdialog.h" // For MemberInfo

class MemberDetailDrawer : public QWidget
{
    Q_OBJECT
signals:
    void sig_addPetRequested(const QString &memberId, const QString &memberName);
    void sig_jumpToPetRequested(const QString &petId);

public:
    explicit MemberDetailDrawer(QWidget *parent = nullptr);

public slots:
    void setMember(const MemberInfo &info, const QString &lastVisit, const QString &pets);
    void showDrawer();
    void hideDrawer();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void setupUI();
    QWidget* createProfilePage();
    QWidget* createPetPage();
    QWidget* createOrderPage();

    // Image Preview
    void setupImagePreview();
    void showBigImage(const QString &path);
    void hideBigImage();
    
    QWidget *m_imagePreviewOverlay;
    QLabel *m_previewLabel;
    QString m_currentPreviewPath;

    // Header components
    QLabel *m_nameLabel;
    QLabel *m_levelLabel;
    QLabel *m_idLabel;
    QLabel *m_petCountLabel;

    // Content pages
    QStackedWidget *m_stackedWidget;
    QButtonGroup *m_tabGroup;

    // Detail labels for Profile
    QLabel *m_valGender;
    QLabel *m_valBirthday;
    QLabel *m_valPhone;
    QLabel *m_valLevel;
    QLabel *m_valBalance;
    QLabel *m_valPoints;
    QLabel *m_valTotalConsume;
    QLabel *m_valLastVisit;
    QPushButton *m_viewMoreVisitBtn;

    // Detail labels for Pets
    QVBoxLayout *m_petCardsLayout;
    QLabel *m_petListLabel; // Keep for backward compatibility or placeholder
    QPushButton *m_addPetBtn;

    MemberInfo m_currentMember;
    bool m_isOpened;
};

#endif // MEMBERDETAILDRAWER_H
