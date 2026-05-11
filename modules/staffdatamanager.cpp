#include "staffdatamanager.h"
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include "../utils/networkmanager.h"

#include <QDir>
#include <QCoreApplication>
#include <QFile>
#include <QMutexLocker>

StaffDataManager* StaffDataManager::m_instance = nullptr;

StaffDataManager* StaffDataManager::instance()
{
    if (!m_instance) {
        m_instance = new StaffDataManager();
    }
    return m_instance;
}

StaffDataManager::StaffDataManager(QObject *parent) : QObject(parent)
{
    m_pixmapCache.setMaxCost(50);
    m_cachePath = QCoreApplication::applicationDirPath() + "/cache/staff";
    ensureCacheDir();

    connect(&NetworkManager::instance(), &NetworkManager::packetReceived, 
            this, &StaffDataManager::onPacketReceived);
            
    // 初始加载
    requestStaffList();
}

void StaffDataManager::initMockData()
{
    // Mock data is now handled by server
}

void StaffDataManager::requestStaffList()
{
    if (!m_staff.isEmpty()) {
        emit staffDataChanged();
        return;
    }
    if (m_isLoading) return;
    m_isLoading = true;
    qDebug() << "[STAFF] Requesting staff list from server...";
    NetworkManager::instance().sendRequest(Protocol::CMD_GET_STAFF_LIST, QJsonObject());
}

void StaffDataManager::onPacketReceived(const Protocol::NetPacket &packet)
{
    if (packet.cmdId == Protocol::CMD_GET_STAFF_LIST) {
        QJsonDocument doc = QJsonDocument::fromJson(packet.data);
        QJsonObject root = doc.object();
        
        if (root["status"].toInt() == Protocol::STATUS_OK) {
            QJsonArray arr = root["data"].toArray();
            m_staff.clear();
            
            for (int i = 0; i < arr.size(); ++i) {
                QJsonObject obj = arr[i].toObject();
                EmployeeInfo info;
                info.id = QString("E%1").arg(obj["id"].toVariant().toLongLong(), 3, 10, QChar('0'));
                info.name = obj["name"].toString();
                info.role = obj["role"].toString();
                info.gender = obj["gender"].toString();
                info.age = obj["age"].toInt();
                info.phone = obj["phone"].toString();
                info.email = obj["email"].toString();
                info.idCard = obj["idCard"].toString();
                info.baseSalary = obj["baseSalary"].toInt();
                info.status = obj["status"].toString();
                info.imgPath = obj["imgPath"].toString();
                info.joinDate = obj["joinDate"].toString();
                info.emergencyContact = obj["emergencyContact"].toString();
                info.emergencyPhone = obj["emergencyPhone"].toString();
                info.address = obj["address"].toString();
                info.education = obj["education"].toString();
                info.department = obj["department"].toString();
                info.username = obj["username"].toString();
                info.imgData = obj["img_data"].toString();
                
                m_staff[info.id] = info;
            }
            
            qDebug() << "[STAFF] Staff list updated from server. Count: " << m_staff.size();
            m_isLoading = false;
            emit staffDataChanged();
        } else {
            m_isLoading = false;
        }
    } else if (packet.cmdId == Protocol::CMD_ADD_STAFF || 
               packet.cmdId == Protocol::CMD_UPDATE_STAFF || 
               packet.cmdId == Protocol::CMD_DELETE_STAFF || 
               packet.cmdId == Protocol::CMD_RESTORE_STAFF) {
        
        QJsonDocument doc = QJsonDocument::fromJson(packet.data);
        QJsonObject root = doc.object();
        
        if (root["status"].toInt() == Protocol::STATUS_OK) {
            // 操作成功后刷新列表
            requestStaffList();
        } else {
            qWarning() << "[STAFF] Server operation failed: " << root["message"].toString();
        }
    }
}

QList<EmployeeInfo> StaffDataManager::allStaff() const
{
    return m_staff.values();
}

EmployeeInfo StaffDataManager::getStaff(const QString &id) const
{
    return m_staff.value(id);
}

void StaffDataManager::addStaff(const EmployeeInfo &info)
{
    QJsonObject obj;
    obj["username"] = info.username;
    obj["password"] = info.password;
    obj["name"] = info.name;
    obj["role"] = info.role;
    obj["department"] = info.department;
    obj["gender"] = info.gender;
    obj["age"] = info.age;
    obj["phone"] = info.phone;
    obj["email"] = info.email;
    obj["idCard"] = info.idCard;
    obj["baseSalary"] = info.baseSalary;
    obj["joinDate"] = info.joinDate;
    obj["emergencyContact"] = info.emergencyContact;
    obj["emergencyPhone"] = info.emergencyPhone;
    obj["address"] = info.address;
    obj["education"] = info.education;
    obj["status"] = info.status;
    obj["imgPath"] = info.imgPath;

    NetworkManager::instance().sendRequest(Protocol::CMD_ADD_STAFF, obj);
}

void StaffDataManager::updateStaff(const EmployeeInfo &info)
{
    QJsonObject obj;
    obj["id"] = info.id;
    obj["username"] = info.username;
    if (!info.password.isEmpty()) obj["password"] = info.password;
    obj["name"] = info.name;
    obj["role"] = info.role;
    obj["department"] = info.department;
    obj["gender"] = info.gender;
    obj["age"] = info.age;
    obj["phone"] = info.phone;
    obj["email"] = info.email;
    obj["idCard"] = info.idCard;
    obj["baseSalary"] = info.baseSalary;
    obj["joinDate"] = info.joinDate;
    obj["emergencyContact"] = info.emergencyContact;
    obj["emergencyPhone"] = info.emergencyPhone;
    obj["address"] = info.address;
    obj["education"] = info.education;
    obj["status"] = info.status;
    obj["imgPath"] = info.imgPath;

    NetworkManager::instance().sendRequest(Protocol::CMD_UPDATE_STAFF, obj);
}

void StaffDataManager::removeStaff(const QString &id)
{
    QJsonObject obj;
    obj["id"] = id;
    NetworkManager::instance().sendRequest(Protocol::CMD_DELETE_STAFF, obj);
}

void StaffDataManager::restoreStaff(const QString &id)
{
    QJsonObject obj;
    obj["id"] = id;
    NetworkManager::instance().sendRequest(Protocol::CMD_RESTORE_STAFF, obj);
}

void StaffDataManager::hardDeleteStaff(const QString &id)
{
    removeStaff(id); 
}

QStringList StaffDataManager::activeStaffNames() const
{
    QStringList names;
    for (const auto &info : m_staff.values()) {
        if (info.status != "离职") {
            names << info.name;
        }
    }
    return names;
}
QPixmap StaffDataManager::getStaffPixmap(const QString &id) const
{
    if (m_pixmapCache.contains(id)) return *m_pixmapCache.object(id);

    QString localPath = m_cachePath + "/" + id + ".png";
    if (QFile::exists(localPath)) {
        QPixmap pix(localPath);
        if (!pix.isNull()) {
            m_pixmapCache.insert(id, new QPixmap(pix));
            return pix;
        }
    }

    EmployeeInfo info = getStaff(id);
    if (info.imgData.isEmpty()) return QPixmap();

    QPixmap pix;
    QByteArray ba = QByteArray::fromBase64(info.imgData.toUtf8());
    if (pix.loadFromData(ba)) {
        m_pixmapCache.insert(id, new QPixmap(pix));
        const_cast<StaffDataManager*>(this)->saveToLocalCache(id, pix);
    }
    return pix;
}

void StaffDataManager::ensureCacheDir()
{
    QDir dir(m_cachePath);
    if (!dir.exists()) dir.mkpath(".");
}

void StaffDataManager::saveToLocalCache(const QString &id, const QPixmap &pix)
{
    if (pix.isNull()) return;
    QString localPath = m_cachePath + "/" + id + ".png";
    if (!QFile::exists(localPath)) {
        pix.save(localPath, "PNG");
    }
}
