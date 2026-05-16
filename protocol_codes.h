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
        CMD_ADD_MEMBER      = 2008,
        CMD_UPDATE_MEMBER   = 2009,
        CMD_DELETE_MEMBER   = 2010,
        CMD_RESTORE_MEMBER  = 2021,
        CMD_HARD_DELETE_MEMBER = 2022,
        CMD_ADD_PET         = 2023, // 新增宠物档案
        CMD_GET_PRODUCT_LIST = 2003, // 获取商品列表
        CMD_GET_INBOUND_LIST = 2004, // 获取入库单据列表
        CMD_ADD_INBOUND_RECORD = 2015, // 新增入库记录
        CMD_UPDATE_INBOUND_RECORD = 2016, // 更新入库记录
        CMD_DELETE_INBOUND_RECORD = 2017, // 删除/作废入库记录
        CMD_UPDATE_PRODUCT = 2018, // 更新/新增商品档案
        CMD_SHELVE_PRODUCT = 2005,   // 上架商品
        CMD_UNSHELVE_PRODUCT = 2011, // 下架商品
        CMD_UPDATE_PET      = 2006,
        CMD_GET_ROOM_LIST   = 2007,
        CMD_GET_VACCINES    = 2012, // 获取疫苗记录明细
        CMD_UPDATE_VACCINES = 2013, // 更新疫苗记录明细
        CMD_UPDATE_PET_STATUS = 2020, // 更新宠物在店状态(入住/预约等)

        // 业务与订单类 (3000-3999)
        CMD_CREATE_ORDER    = 3001,
        CMD_GET_APPOINTMENTS = 3002,
        CMD_GET_ORDER_LIST  = 3003, // 获取订单列表
        CMD_UPDATE_ORDER    = 3004, // 更新订单
        CMD_CANCEL_ORDER    = 3005, // 作废订单
        CMD_ADD_BATCH       = 3101, // 新增库存批次
        CMD_GET_BATCHES     = 3102, // 获取商品批次明细

        // 员工管理类 (4000-4999)
        CMD_GET_STAFF_LIST = 4001,
        CMD_ADD_STAFF      = 4002,
        CMD_UPDATE_STAFF   = 4003,
        CMD_DELETE_STAFF   = 4004,
        CMD_RESTORE_STAFF  = 4005,
        CMD_HARD_DELETE_STAFF = 4006,
        
        // 服务管理类 (5000-5999)
        CMD_GET_SERVICE_LIST = 5001,
        CMD_ADD_SERVICE      = 5002,
        CMD_UPDATE_SERVICE   = 5003,
        CMD_DELETE_SERVICE   = 5004,

        // 排班管理模块
        CMD_GET_SCHEDULE     = 4101,
        CMD_UPDATE_SCHEDULE  = 4102,
        CMD_BATCH_UPDATE_SCHEDULE = 4103,

        // 预约与物流模块
        CMD_GET_APPOINTMENT_LIST      = 5101,
        CMD_ADD_APPOINTMENT           = 5102,
        CMD_UPDATE_APPT_STATUS        = 5103,
        
        CMD_GET_LOGISTICS_LIST        = 5201,
        CMD_UPDATE_LOGISTICS_STATUS   = 5202,

        // 系统通知 (6000+)
        CMD_NOTIFY_REFRESH = 6001, // 服务端通知客户端刷新数据
        
        // 财务与薪资类 (7000-7999)
        CMD_GET_PERFORMANCE_LIST = 7001,
        CMD_VERIFY_PERFORMANCE   = 7002,
        CMD_GET_SALARY_LIST      = 7003,
        CMD_APPROVE_SALARY       = 7004,
        CMD_PAY_SALARY           = 7005,
        
        // 统计报表类 (8000-8999)
        CMD_GET_STATS_DASHBOARD = 8001, // 获取仪表盘核心指标
        CMD_GET_STATS_REVENUE   = 8002, // 获取营收趋势数据 (趋势图)

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
