#ifndef PROTOCOL_CODES_H
#define PROTOCOL_CODES_H

#include <QtCore/QMetaType>
#include <QtCore/QByteArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>

namespace Protocol {
    // 基础包头长度: 4(Length) + 4(CommandID) = 8字节
    const int HEADER_SIZE = 8;

    enum Command : int {
        // 系统类 (1000-1999)
        CMD_HEARTBEAT = 1000,
        CMD_LOGIN     = 1001,
        CMD_LOGOUT    = 1002,

        // 宠物与会员类 (2000-2999)
        CMD_GET_PET_LIST    = 2001,
        CMD_GET_MEMBER_LIST = 2002,
        CMD_GET_PRODUCT_LIST = 2003, // 获取商品列表
        CMD_GET_INBOUND_LIST = 2004, // 获取入库单据列表
        CMD_SHELVE_PRODUCT = 2005,   // 上架商品
        CMD_UPDATE_PET      = 2006,
        CMD_GET_ROOM_LIST   = 2007,

        // 业务与订单类 (3000-3999)
        CMD_CREATE_ORDER    = 3001,
        CMD_GET_APPOINTMENTS = 3002,

        // 员工管理类 (4000-4999)
        CMD_GET_STAFF_LIST = 4001,
        CMD_ADD_STAFF      = 4002,
        CMD_UPDATE_STAFF   = 4003,
        CMD_DELETE_STAFF   = 4004,
        CMD_RESTORE_STAFF  = 4005,
        
        // 服务管理类 (5000-5999)
        CMD_GET_SERVICE_LIST = 5001,
        CMD_ADD_SERVICE      = 5002,
        CMD_UPDATE_SERVICE   = 5003,
        CMD_DELETE_SERVICE   = 5004,

        // 错误/响应类
        CMD_ERROR_RESPONSE  = 9999
    };

    enum Status : int {
        STATUS_OK      = 0,
        STATUS_ERROR   = 1,
        STATUS_AUTH_FAIL = 2
    };

    struct NetPacket {
        unsigned int length; // 总长度 (包含头)
        int cmdId;           // 指令ID
        QByteArray data;     // JSON 包体
        QJsonObject jsonObj; // 预解析的 JSON 对象（在后台线程完成解析）
    };
}

Q_DECLARE_METATYPE(Protocol::NetPacket)

#endif // PROTOCOL_CODES_H
