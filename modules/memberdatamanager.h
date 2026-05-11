#ifndef MEMBERDATAMANAGER_H
#define MEMBERDATAMANAGER_H

#include <QObject>
#include <QMap>
#include <QList>
#include "../protocol_codes.h"
#include "addmemberdialog.h"
#include <QRecursiveMutex>
#include <QPixmap>
#include <QCache>

class MemberDataManager : public QObject
{
    Q_OBJECT
public:
    static MemberDataManager* instance();
    
    void requestMemberList(); // 从服务器拉取会员列表
    QList<MemberInfo> allMembers() const;
    QList<MemberInfo> activeMembers() const;
    MemberInfo getMember(const QString &id) const;
    
    void addMember(const MemberInfo &info);
    void updateMember(const MemberInfo &info);
    void removeMember(const QString &id); // Logical delete
    void restoreMember(const QString &id);
    void hardDeleteMember(const QString &id);
    void removePetFromMember(const QString &memberId, const QString &petName);
    
    QPixmap getMemberPixmap(const QString &id) const;

signals:
    void dataChanged();

private slots:
    void onPacketReceived(const Protocol::NetPacket &packet);

private:
    explicit MemberDataManager(QObject *parent = nullptr);
    void initMockData();

    static MemberDataManager* m_instance;
    QMap<QString, MemberInfo> m_members;
    mutable QRecursiveMutex m_mutex;
    bool m_isLoading = false;
    
    QString m_cachePath;
    mutable QCache<QString, QPixmap> m_pixmapCache;
    void ensureCacheDir();
    void saveToLocalCache(const QString &id, const QPixmap &pix);
};

#endif // MEMBERDATAMANAGER_H
