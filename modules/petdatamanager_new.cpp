#include "petdatamanager.h"
#include "../utils/networkmanager.h"
#include "memberdatamanager.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QMutexLocker>
#include <QDir>
#include <QStandardPaths>

PetDataManager* PetDataManager::m_instance = nullptr;

PetDataManager* PetDataManager::instance() {
    if (!m_instance) {
        m_instance = new PetDataManager();
    }
    return m_instance;
}

PetDataManager::PetDataManager(QObject *parent) : QObject(parent) {
    m_cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/pet_images";
    ensureCacheDir();
    
    connect(&NetworkManager::instance(), &NetworkManager::packetReceived, this, &PetDataManager::onPacketReceived);
}

void PetDataManager::ensureCacheDir() {
    QDir dir(m_cachePath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

void PetDataManager::requestPetList() {
    NetworkManager::instance().sendRequest(Protocol::CMD_GET_PET_LIST, QJsonObject());
}

void PetDataManager::requestRoomList() {
    NetworkManager::instance().sendRequest(Protocol::CMD_GET_ROOM_LIST, QJsonObject());
}

void PetDataManager::requestOrderList() {
    NetworkManager::instance().sendRequest(Protocol::CMD_GET_ORDER_LIST, QJsonObject());
}

void PetDataManager::notifyGlobalDataChanged() {
    emit globalDataChanged();
}

void PetDataManager::notifyPetDataChanged(const QString &petId) {
    emit petDataChanged(petId);
}

void PetDataManager::executeCheckOut(int roomId, const QString &petId, const QDate &checkOutDate, double weight, double totalAmount, const QString &paymentMethod, std::function<void(bool, QString)> callback) {
    qDebug() << "[DEBUG] CMD_CREATE_ORDER value:" << (int)Protocol::CMD_CREATE_ORDER;
    Q_UNUSED(checkOutDate);
    PetInfo info;
    {
        QMutexLocker locker(&m_mutex);
        if (m_pets.contains(petId)) {
            info = m_pets[petId];
        } else {
            if (callback) callback(false, "找不到宠物数据: " + petId);
            return;
        }
    }

    // 1. 同步到服务器：更新宠物在店状态
    QJsonObject petStatus;
    petStatus["pet_id"] = petId;
    petStatus["status"] = "在家";
    petStatus["room_no"] = "";
    petStatus["start_time"] = "";
    petStatus["end_time"] = "";
    
    qDebug() << "[CHECKOUT] Step 1: Sending CMD_UPDATE_PET_STATUS for pet:" << petId;
    NetworkManager::instance().sendRequest(Protocol::CMD_UPDATE_PET_STATUS, petStatus, [=](const QJsonObject &response){
        qDebug() << "[CHECKOUT] Step 1 Response received:" << response["status"].toInt();
        if (response["status"].toInt() != Protocol::STATUS_OK) {
            if (callback) callback(false, "更新宠物状态失败: " + response["message"].toString());
            return;
        }

        // 2. 同步到服务器：创建正式订单
        qDebug() << "[CHECKOUT] Step 2: Preparing CMD_CREATE_ORDER...";
        QJsonObject orderObj;
        orderObj["id"] = "ORD-BO-" + QDateTime::currentDateTime().toString("yyyyMMddHHmmss");
        orderObj["sourceModule"] = "Boarding";
        orderObj["relatedId"] = QString::number(roomId);
        orderObj["memberId"] = info.ownerId;
        orderObj["totalAmount"] = totalAmount;
        orderObj["discount"] = 0.0;
        orderObj["finalAmount"] = totalAmount;
        orderObj["itemDetails"] = QString("[{\"name\":\"寄养服务\",\"barcode\":\"S001\",\"price\":%1,\"count\":1}]").arg(totalAmount);
        orderObj["payMethod"] = paymentMethod;
        orderObj["status"] = "已支付";
        orderObj["operator_id"] = 1;

        qDebug() << "[CHECKOUT] Step 2: Sending CMD_CREATE_ORDER now.";
        NetworkManager::instance().sendRequest(Protocol::CMD_CREATE_ORDER, orderObj, [=](const QJsonObject &resp2){
            qDebug() << "[CHECKOUT] Step 2 Response received:" << resp2["status"].toInt();
            if (resp2["status"].toInt() == Protocol::STATUS_OK) {
                // 本地更新内存状态
                {
                    QMutexLocker locker(&m_mutex);
                    if (m_pets.contains(petId)) {
                        m_pets[petId].status = "在家";
                        m_pets[petId].roomNo = "";
                        m_pets[petId].weight = weight;
                    }
                    removeRoomStatusPeriod(roomId, "寄养");
                }
                
                notifyGlobalDataChanged();
                notifyPetDataChanged(petId);
                requestOrderList(); // 立即刷新订单列表

                if (callback) callback(true, "");
            } else {
                if (callback) callback(false, "创建订单失败: " + resp2["message"].toString());
            }
        });
    });
}
