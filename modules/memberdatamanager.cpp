#include "memberdatamanager.h"
#include "../utils/networkmanager.h"
#include "../protocol_codes.h"
#include "logdatamanager.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <QDate>

#include <QDir>
#include <QCoreApplication>
#include <QFile>

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
    m_pixmapCache.setMaxCost(200); 
    m_cachePath = QCoreApplication::applicationDirPath() + "/cache/members";
    ensureCacheDir();

    connect(&NetworkManager::instance(), &NetworkManager::packetReceived,
            this, &MemberDataManager::onPacketReceived);
    requestMemberList(); 
}

void MemberDataManager::requestMemberList()
{
    {
        QMutexLocker locker(&m_mutex);
        if (m_isLoading) {
            return; // Already fetching
        }
        m_isLoading = true;
    }
    
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
            
            {
                QMutexLocker locker(&m_mutex);
                m_members.clear();
                for (int i = 0; i < arr.size(); ++i) {
                    QJsonObject obj = arr[i].toObject();
                    MemberInfo info;
                    // 格式化 ID：1 -> M00001
                    info.id = QString("M%1").arg(obj["member_id"].toVariant().toLongLong(), 5, 10, QChar('0'));
                    info.name = obj["name"].toString();
                    info.gender = obj["gender"].toString();
                    info.phone = obj["phone"].toString();
                    info.level = obj["level_name"].toString(); // 修正 Key 名
                    info.status = obj["status"].toString();
                    info.isDeleted = obj["is_deleted"].toBool();
                    info.isActive = !info.isDeleted;
                    info.balance = obj["balance"].toDouble();
                    info.consume_amt = obj["consume_amt"].toDouble();
                    info.points = obj["points"].toInt();
                    info.birthday = obj["birthday"].toString();
                    info.pets = obj["pets"].toString();
                    info.imgData = obj["img_data"].toString();
                    
                    // qDebug() << "[DEBUG] Member ID:" << info.id << "Name:" << info.name << "Gender:" << info.gender;
                    
                    m_members[info.id] = info;
                }
            }
            
            qDebug() << "[MEMBER] Member list updated. Count:" << m_members.size();
            m_isLoading = false;
            emit dataChanged();
        } else {
            QMutexLocker locker(&m_mutex);
            m_isLoading = false;
        }
    } else if (packet.cmdId == Protocol::CMD_NOTIFY_REFRESH) {
        QJsonDocument doc = QJsonDocument::fromJson(packet.data);
        QJsonObject root = doc.object();
        QString module = root["module"].toString();
        if (module == "member" || module.isEmpty()) {
            qDebug() << "[MEMBER] Received refresh broadcast, reloading member list...";
            // Prevent redundant loads if already loading
            if (!m_isLoading) {
                requestMemberList();
            }
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
    QMutexLocker locker(&m_mutex);
    return m_members.values();
}

QList<MemberInfo> MemberDataManager::activeMembers() const
{
    QMutexLocker locker(&m_mutex);
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
    QMutexLocker locker(&m_mutex);
    return m_members.value(id);
}

void MemberDataManager::addMember(const MemberInfo &info)
{
    QJsonObject data;
    data["name"] = info.name;
    data["phone"] = info.phone;
    data["gender"] = info.gender;
    data["birthday"] = info.birthday;
    data["level"] = info.level;
    data["balance"] = info.balance;
    
    NetworkManager::instance().sendRequest(Protocol::CMD_ADD_MEMBER, data);

    // 记录新增日志
    QJsonObject diff;
    diff["field"] = "姓名, 电话, 性别, 生日, 会员等级, 卡余额";
    diff["old"] = "空";
    diff["new"] = QString("%1, %2, %3, %4, %5, %6")
                  .arg(info.name, info.phone, info.gender, info.birthday, info.level)
                  .arg(QString::number(info.balance, 'f', 2));
    LogDataManager::writeLog("会员管理", "新增会员: " + info.name, diff);
}

void MemberDataManager::updateMember(const MemberInfo &info)
{
    MemberInfo oldInfo = getMember(info.id);

    QJsonObject data;
    // 解析 ID，M00001 -> 1
    int dbId = info.id.mid(1).toInt();
    data["member_id"] = dbId;
    data["name"] = info.name;
    data["phone"] = info.phone;
    data["gender"] = info.gender;
    data["birthday"] = info.birthday;
    data["level"] = info.level;
    data["balance"] = info.balance;
    
    NetworkManager::instance().sendRequest(Protocol::CMD_UPDATE_MEMBER, data);

    // 记录修改日志 (Diff 对比)
    QJsonArray diffs;
    if (oldInfo.name != info.name) {
        QJsonObject d; d["field"] = "姓名"; d["old"] = oldInfo.name; d["new"] = info.name;
        diffs.append(d);
    }
    if (oldInfo.phone != info.phone) {
        QJsonObject d; d["field"] = "电话"; d["old"] = oldInfo.phone; d["new"] = info.phone;
        diffs.append(d);
    }
    if (oldInfo.gender != info.gender) {
        QJsonObject d; d["field"] = "性别"; d["old"] = oldInfo.gender; d["new"] = info.gender;
        diffs.append(d);
    }
    if (oldInfo.birthday != info.birthday) {
        QJsonObject d; d["field"] = "生日"; d["old"] = oldInfo.birthday; d["new"] = info.birthday;
        diffs.append(d);
    }
    if (oldInfo.level != info.level) {
        QJsonObject d; d["field"] = "会员等级"; d["old"] = oldInfo.level; d["new"] = info.level;
        diffs.append(d);
    }
    if (qAbs(oldInfo.balance - info.balance) > 0.001) {
        QJsonObject d; d["field"] = "卡余额"; d["old"] = QString::number(oldInfo.balance, 'f', 2); d["new"] = QString::number(info.balance, 'f', 2);
        diffs.append(d);
    }

    if (!diffs.isEmpty()) {
        LogDataManager::writeLog("会员管理", "编辑会员: " + info.name, QJsonDocument(diffs).toJson(QJsonDocument::Compact));
    }
}

void MemberDataManager::removeMember(const QString &id)
{
    MemberInfo m = getMember(id);

    QJsonObject data;
    data["member_id"] = id.mid(1).toInt();
    NetworkManager::instance().sendRequest(Protocol::CMD_DELETE_MEMBER, data);

    QJsonObject diff;
    diff["field"] = "会员状态";
    diff["old"] = "正常";
    diff["new"] = "已软删除";
    LogDataManager::writeLog("会员管理", "软删除会员: " + m.name, diff);
}

void MemberDataManager::restoreMember(const QString &id)
{
    MemberInfo m = getMember(id);

    QJsonObject data;
    data["member_id"] = id.mid(1).toInt();
    NetworkManager::instance().sendRequest(Protocol::CMD_RESTORE_MEMBER, data);

    QJsonObject diff;
    diff["field"] = "会员状态";
    diff["old"] = "已软删除";
    diff["new"] = "正常";
    LogDataManager::writeLog("会员管理", "恢复会员: " + m.name, diff);
}

void MemberDataManager::hardDeleteMember(const QString &id)
{
    MemberInfo m = getMember(id);

    QJsonObject data;
    data["member_id"] = id.mid(1).toInt();
    NetworkManager::instance().sendRequest(Protocol::CMD_HARD_DELETE_MEMBER, data);

    QJsonObject diff;
    diff["field"] = "数据档案";
    diff["old"] = "存在于数据库";
    diff["new"] = "已永久物理删除";
    LogDataManager::writeLog("会员管理", "彻底删除会员: " + m.name, diff);
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
QPixmap MemberDataManager::getMemberPixmap(const QString &id) const
{
    // L1: Memory
    if (m_pixmapCache.contains(id)) {
        return *m_pixmapCache.object(id);
    }

    // L2: Disk
    QString localPath = m_cachePath + "/" + id + ".png";
    if (QFile::exists(localPath)) {
        QPixmap pix(localPath);
        if (!pix.isNull()) {
            m_pixmapCache.insert(id, new QPixmap(pix));
            return pix;
        }
    }

    // L3: Data
    MemberInfo info = getMember(id);
    if (info.imgData.isEmpty()) return QPixmap();

    QPixmap pix;
    QByteArray ba = QByteArray::fromBase64(info.imgData.toUtf8());
    if (pix.loadFromData(ba)) {
        m_pixmapCache.insert(id, new QPixmap(pix));
        const_cast<MemberDataManager*>(this)->saveToLocalCache(id, pix);
    }
    return pix;
}

void MemberDataManager::ensureCacheDir()
{
    QDir dir(m_cachePath);
    if (!dir.exists()) dir.mkpath(".");
}

void MemberDataManager::saveToLocalCache(const QString &id, const QPixmap &pix)
{
    if (pix.isNull()) return;
    QString localPath = m_cachePath + "/" + id + ".png";
    if (!QFile::exists(localPath)) {
        pix.save(localPath, "PNG");
    }
}
