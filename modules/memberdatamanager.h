#ifndef MEMBERDATAMANAGER_H
#define MEMBERDATAMANAGER_H

#include <QObject>
#include <QMap>
#include <QList>
#include "addmemberdialog.h"

class MemberDataManager : public QObject
{
    Q_OBJECT
public:
    static MemberDataManager* instance();

    QList<MemberInfo> allMembers() const;
    QList<MemberInfo> activeMembers() const;
    MemberInfo getMember(const QString &id) const;
    
    void addMember(const MemberInfo &info);
    void updateMember(const MemberInfo &info);
    void removeMember(const QString &id); // Logical delete
    void restoreMember(const QString &id);
    void hardDeleteMember(const QString &id);
    void removePetFromMember(const QString &memberId, const QString &petName);

signals:
    void dataChanged();

private:
    explicit MemberDataManager(QObject *parent = nullptr);
    void initMockData();

    static MemberDataManager* m_instance;
    QMap<QString, MemberInfo> m_members;
};

#endif // MEMBERDATAMANAGER_H
