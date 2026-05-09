#include "memberdatamanager.h"
#include "../utils/networkmanager.h"
#include "../protocol_codes.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
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
    // 连接网络包接收信号
    connect(&NetworkManager::instance(), &NetworkManager::packetReceived,
            this, &MemberDataManager::onPacketReceived);
    
    initMockData();
}

void MemberDataManager::requestMemberList()
{
    qDebug() << "[MEMBER] Requesting member list from server...";
    NetworkManager::instance().sendRequest(Protocol::CMD_GET_MEMBER_LIST, QJsonObject());
}

void MemberDataManager::onPacketReceived(const Protocol::NetPacket &packet)
{
    if (packet.cmdId == Protocol::CMD_GET_MEMBER_LIST) {
        QJsonDocument doc = QJsonDocument::fromJson(packet.data);
        QJsonObject root = doc.object();
        
        if (root["status"].toInt() == Protocol::STATUS_OK) {
            QJsonArray arr = root["data"].toArray();
            
            // 全量更新
            m_members.clear();
            
            for (int i = 0; i < arr.size(); ++i) {
                QJsonObject obj = arr[i].toObject();
                MemberInfo info;
                // 格式化 ID：1 -> M00001
                info.id = QString("M%1").arg(obj["member_id"].toInt(), 5, 10, QChar('0'));
                info.name = obj["name"].toString();
                info.gender = obj["gender"].toString();
                info.birthday = obj["birthday"].toString();
                info.phone = obj["phone"].toString();
                info.balance = obj["balance"].toDouble();
                info.consume_amt = obj["consume_amt"].toDouble();
                info.points = obj["points"].toInt();
                info.level = obj["level_name"].toString();
                info.isActive = true;
                info.status = "正常";
                
                qDebug() << "[DEBUG] Member ID:" << info.id << "Name:" << info.name << "Gender:" << info.gender;
                
                m_members[info.id] = info;
            }
            
            qDebug() << "[MEMBER] Member list updated. Count:" << m_members.size();
            emit dataChanged();
        }
    }
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

    /*
    addMock("M001", "张三", "男", "1990-05-20", "13800138000", "黄金会员", 500.00, 1250.00, 125, "团团（波斯猫）");
    addMock("M002", "李芳", "女", "1995-10-12", "13912345678", "普通会员", 0.00, 100.00, 10, "豆豆（柴犬）, 咪咪（银渐层）");
    ...
    addMock("M011", "林十三", "男", "1990-09-09", "13012123434", "黄金会员", 450.00, 1100.00, 110);
    */
    Q_UNUSED(addMock);
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
