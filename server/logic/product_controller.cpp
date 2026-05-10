#include "product_controller.h"
#include "../network/server_core.h"
#include "../database/connectionpool.h"
#include "../../protocol_codes.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <functional>

ProductController::ProductController(ServerCore *server, QObject *parent) 
    : QObject(parent), m_server(server)
{
    if (m_server) {
        m_server->registerHandler(Protocol::CMD_GET_PRODUCT_LIST, 
            std::bind(&ProductController::handleGetProductList, this, std::placeholders::_1, std::placeholders::_2));
        m_server->registerHandler(Protocol::CMD_GET_INBOUND_LIST, 
            std::bind(&ProductController::handleGetInboundList, this, std::placeholders::_1, std::placeholders::_2));
        m_server->registerHandler(Protocol::CMD_SHELVE_PRODUCT, 
            std::bind(&ProductController::handleShelveProduct, this, std::placeholders::_1, std::placeholders::_2));
    }
}

void ProductController::handleRequest(int cmdId, ClientHandler* client, const QJsonObject &data)
{
    // 这个方法可以保留作为手动分发的备选，但我们现在使用 registerHandler 机制
    if (cmdId == Protocol::CMD_GET_PRODUCT_LIST) {
        handleGetProductList(client, data);
    }
}

void ProductController::handleGetProductList(ClientHandler* client, const QJsonObject &data)
{
    Q_UNUSED(data);
    LOG_I("[PRODUCT] Client requested product list.");

    QJsonObject response;
    response["status"] = Protocol::STATUS_OK;
    QJsonArray productList;

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("SELECT * FROM products WHERE is_deleted = 0");
    
    if (query.exec()) {
        while (query.next()) {
        QJsonObject p;
        QSqlRecord rec = query.record();
        
        auto getValue = [&](const char* field) {
            int idx = rec.indexOf(field);
            return (idx != -1) ? query.value(idx) : QVariant();
        };

        p["product_id"] = getValue("product_id").toInt();
        p["barcode"] = getValue("barcode").toString();
        p["name"] = getValue("name").toString();
        p["brand"] = getValue("brand").toString();
        p["origin"] = getValue("origin").toString();
        p["category"] = getValue("category").toString();
        p["spec"] = getValue("spec").toString();
        p["unit"] = getValue("unit").toString();
        p["sale_price"] = getValue("sale_price").toDouble();
        p["cost_price"] = getValue("cost_price").toDouble();
        p["stock_curr"] = getValue("stock_current").toInt();
        p["stock_min"] = getValue("stock_min").toInt();
        p["production_date"] = getValue("production_date").toString();
        p["shelf_life"] = getValue("shelf_life_days").toInt();
        p["supplier"] = getValue("supplier").toString();
        p["supplier_phone"] = getValue("supplier_phone").toString();
        p["description"] = getValue("description").toString();
        p["ingredients"] = getValue("ingredients").toString();
        p["storage_req"] = getValue("storage_req").toString();
        p["tags"] = getValue("tags").toString();
        p["img_data"] = getValue("img_data").toString();
        p["is_active"] = getValue("is_active").toInt() == 1;
        
        // 自动计算预警状态
        int curr = p["stock_curr"].toInt();
        int min = p["stock_min"].toInt();
        p["is_warning"] = (curr <= min);

        productList.append(p);
    }
    }

    if (query.lastError().isValid()) {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
        LOG_E("[PRODUCT] Database error: " << query.lastError().text());
    } else {
        response["data"] = productList;
        LOG_I("[PRODUCT] Successfully fetched " << productList.size() << " products.");
    }

    client->sendPacket(Protocol::CMD_GET_PRODUCT_LIST, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void ProductController::handleGetInboundList(ClientHandler* client, const QJsonObject &data)
{
    Q_UNUSED(data);
    LOG_I("[PRODUCT] Client requested inbound list.");

    QJsonObject response;
    response["status"] = Protocol::STATUS_OK;
    QJsonArray inboundList;

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("SELECT * FROM product_inbound ORDER BY created_at DESC");
    
    if (query.exec()) {
        while (query.next()) {
            QJsonObject item;
            QSqlRecord rec = query.record();
            item["id"] = query.value("id").toInt();
            item["inbound_no"] = query.value("inbound_no").toString();
            item["barcode"] = query.value("barcode").toString();
            item["product_name"] = query.value("product_name").toString();
            item["spec"] = query.value("spec").toString();
            item["category"] = query.value("category").toString();
            item["supplier"] = query.value("supplier").toString();
            item["quantity"] = query.value("quantity").toInt();
            item["cost_price"] = query.value("cost_price").toDouble();
            item["production_date"] = query.value("production_date").toDate().toString("yyyy-MM-dd");
            item["shelf_life_days"] = query.value("shelf_life_days").toInt();
            item["supplier_phone"] = query.value("supplier_phone").toString();
            item["operator_name"] = query.value("operator_name").toString();
            item["is_shelved"] = query.value("is_shelved").toInt() == 1;
            item["img_data"] = query.value("img_data").toString();
            item["created_at"] = query.value("created_at").toDateTime().toString("yyyy-MM-dd HH:mm:ss");
            inboundList.append(item);
        }
    }

    response["data"] = inboundList;
    client->sendPacket(Protocol::CMD_GET_INBOUND_LIST, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void ProductController::handleShelveProduct(ClientHandler* client, const QJsonObject &data)
{
    int inboundId = data["id"].toInt();
    LOG_I("[PRODUCT] Shelving inbound record ID:" << inboundId);

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    db.transaction();
    QSqlQuery query(db);

    // 1. 获取入库单详情
    query.prepare("SELECT * FROM product_inbound WHERE id = ?");
    query.addBindValue(inboundId);
    if (!query.exec() || !query.next()) {
        db.rollback();
        client->sendPacket(Protocol::CMD_SHELVE_PRODUCT, "{\"status\":1, \"message\":\"Inbound record not found\"}");
        return;
    }

    QSqlRecord rec = query.record();
    QString barcode = rec.value("barcode").toString();
    int qty = rec.value("quantity").toInt();
    double cost = rec.value("cost_price").toDouble();
    QString name = rec.value("product_name").toString();
    QString spec = rec.value("spec").toString();
    QString category = rec.value("category").toString();
    QString supplier = rec.value("supplier").toString();
    QString imgData = rec.value("img_data").toString();

    // 2. 更新或插入商品档案
    QSqlQuery checkQuery(db);
    checkQuery.prepare("SELECT product_id FROM products WHERE barcode = ?");
    checkQuery.addBindValue(barcode);
    
    if (checkQuery.exec() && checkQuery.next()) {
        // 更新现有商品：累加库存，更新最后进价
        QSqlQuery updateQuery(db);
        updateQuery.prepare("UPDATE products SET stock_current = stock_current + ?, cost_price = ?, supplier = ?, img_data = ? WHERE barcode = ?");
        updateQuery.addBindValue(qty);
        updateQuery.addBindValue(cost);
        updateQuery.addBindValue(supplier);
        updateQuery.addBindValue(imgData);
        updateQuery.addBindValue(barcode);
        updateQuery.exec();
    } else {
        // 新建商品档案
        QSqlQuery insertQuery(db);
        insertQuery.prepare("INSERT INTO products (barcode, name, spec, category, img_data, cost_price, sale_price, stock_current, supplier, is_active) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, 1)");
        insertQuery.addBindValue(barcode);
        insertQuery.addBindValue(name);
        insertQuery.addBindValue(spec);
        insertQuery.addBindValue(category);
        insertQuery.addBindValue(imgData);
        insertQuery.addBindValue(cost);
        insertQuery.addBindValue(cost * 1.5); // 默认 1.5 倍加价作为起步价
        insertQuery.addBindValue(qty);
        insertQuery.addBindValue(supplier);
        insertQuery.exec();
    }

    // 3. 标记入库单为已上架
    QSqlQuery finalQuery(db);
    finalQuery.prepare("UPDATE product_inbound SET is_shelved = 1 WHERE id = ?");
    finalQuery.addBindValue(inboundId);
    finalQuery.exec();

    db.commit();
    LOG_I("[PRODUCT] Shelve success for barcode:" << barcode.toStdString());

    QJsonObject resp;
    resp["status"] = Protocol::STATUS_OK;
    resp["message"] = "Success";
    client->sendPacket(Protocol::CMD_SHELVE_PRODUCT, QJsonDocument(resp).toJson(QJsonDocument::Compact));
    
    // 广播：通知所有客户端更新商品列表
    // 这里简化处理，客户端可以通过后续的业务逻辑感知或轮询
}
