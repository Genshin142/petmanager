#ifndef PROTOCOL_CODES_H
#define PROTOCOL_CODES_H

#include <QtCore/QMetaType>
#include <QtCore/QByteArray>

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

        // 业务与订单类 (3000-3999)
        CMD_CREATE_ORDER    = 3001,
        CMD_GET_APPOINTMENTS = 3002,

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
    };
}

Q_DECLARE_METATYPE(Protocol::NetPacket)

#endif // PROTOCOL_CODES_H
