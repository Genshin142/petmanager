#include "memberdatamanager.h"
#include <QDate>

MemberDataManager* MemberDataManager::m_instance = nullptr;

MemberDataManager* MemberDataManager::instance()
{
    if (!m_instance) {
        m_instance = new MemberDataManager();
    }
    return m_instance;
}

MemberDataManager::MemberDataManager(QObject *parent) : QObject(parent)
{
    initMockData();
}

void MemberDataManager::initMockData()
{
    auto addMock = [&](const QString &id, const QString &name, const QString &gender, const QString &birthday, const QString &phone, const QString &level, double balance, double consume_amt, int pts, const QString &pets = "无") {
        MemberInfo info;
        info.id = id;
        info.name = name;
        info.gender = gender;
        info.birthday = birthday;
        info.phone = phone;
        info.level = level;
        info.balance = balance;
        info.consume_amt = consume_amt;
        info.points = pts;
        info.isActive = true;
        info.status = "正常";
        info.pets = pets;
        m_members[id] = info;
    };

    addMock("M001", "张三", "男", "1990-05-20", "13800138000", "黄金会员", 500.00, 1250.00, 125, "团团（波斯猫）");
    addMock("M002", "李芳", "女", "1995-10-12", "13912345678", "普通会员", 0.00, 100.00, 10, "豆豆（柴犬）, 咪咪（银渐层）");
    addMock("M003", "王五", "男", "1988-03-05", "13777777777", "铂金会员", 1200.00, 3500.00, 350, "旺财（金毛犬）");
    addMock("M004", "赵六", "男", "1992-07-15", "13666666666", "钻石会员", 2500.00, 8800.00, 880);
    addMock("M005", "孙七", "女", "1993-11-20", "18189294306", "普通会员", 50.00, 100.00, 10);
    addMock("M006", "周八", "男", "1991-01-30", "13511112222", "黄金会员", 300.00, 600.00, 60);
    addMock("M007", "吴九", "女", "1994-06-18", "13433334444", "普通会员", 20.00, 50.00, 5);
    addMock("M008", "郑十", "男", "1989-12-25", "13355556666", "铂金会员", 800.00, 2000.00, 200);
    addMock("M009", "钱十一", "男", "1992-03-14", "13277778888", "钻石会员", 1500.00, 5000.00, 500);
    addMock("M010", "陈十二", "女", "1996-08-08", "13199990000", "普通会员", 10.00, 20.00, 2);
    addMock("M011", "林十三", "男", "1990-09-09", "13012123434", "黄金会员", 450.00, 1100.00, 110);
}

QList<MemberInfo> MemberDataManager::allMembers() const
{
    return m_members.values();
}

QList<MemberInfo> MemberDataManager::activeMembers() const
{
    QList<MemberInfo> active;
    for (const auto &info : m_members) {
        if (info.isActive) {
            active.append(info);
        }
    }
    return active;
}

MemberInfo MemberDataManager::getMember(const QString &id) const
{
    return m_members.value(id);
}

void MemberDataManager::addMember(const MemberInfo &info)
{
    m_members[info.id] = info;
    emit dataChanged();
}

void MemberDataManager::updateMember(const MemberInfo &info)
{
    m_members[info.id] = info;
    emit dataChanged();
}

void MemberDataManager::removeMember(const QString &id)
{
    if (m_members.contains(id)) {
        m_members[id].isActive = false;
        m_members[id].status = "已注销";
        emit dataChanged();
    }
}

void MemberDataManager::restoreMember(const QString &id)
{
    if (m_members.contains(id)) {
        m_members[id].isActive = true;
        m_members[id].status = "正常";
        emit dataChanged();
    }
}

void MemberDataManager::hardDeleteMember(const QString &id)
{
    if (m_members.remove(id) > 0) {
        emit dataChanged();
    }
}

void MemberDataManager::removePetFromMember(const QString &memberId, const QString &petName)
{
    if (m_members.contains(memberId)) {
        QString pets = m_members[memberId].pets;
        QStringList list = pets.split(", ", Qt::SkipEmptyParts);
        
        for (int i = 0; i < list.size(); ++i) {
            if (list[i].startsWith(petName)) {
                list.removeAt(i);
                break;
            }
        }
        
        m_members[memberId].pets = list.join(", ");
        emit dataChanged();
    }
}
