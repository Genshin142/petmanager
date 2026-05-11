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
        m_server->registerHandler(Protocol::CMD_UNSHELVE_PRODUCT, 
            std::bind(&ProductController::handleUnshelveProduct, this, std::placeholders::_1, std::placeholders::_2));
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
    LOG_I("[PRODUCT] Client requested inbound list.");

    bool onlyUnshelved = data["onlyUnshelved"].toBool();
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    if (onlyUnshelved) {
        query.prepare("SELECT * FROM product_inbound WHERE is_shelved = 0 ORDER BY created_at DESC");
    } else {
        query.prepare("SELECT * FROM product_inbound ORDER BY created_at DESC");
    }
    
    QJsonObject response;
    response["status"] = Protocol::STATUS_OK;
    QJsonArray inboundList;

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
            item["sale_price"] = query.value("sale_price").toDouble();
            item["production_date"] = query.value("production_date").toDate().toString("yyyy-MM-dd");
            item["shelf_life_days"] = query.value("shelf_life_days").toInt();
            item["supplier_phone"] = query.value("supplier_phone").toString();
            item["operator_name"] = query.value("operator_name").toString();
            item["is_shelved"] = query.value("is_shelved").toInt() == 1;
            item["is_active"] = true; // 确保客户端不过滤掉
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
    double sale = rec.value("sale_price").toDouble();
    QString name = rec.value("product_name").toString();
    QString spec = rec.value("spec").toString();
    QString category = rec.value("category").toString();
    QString supplier = rec.value("supplier").toString();
    QString imgData = rec.value("img_data").toString();

    // 2. 处理商品档案 (products 表)
    QSqlQuery checkQuery(db);
    checkQuery.prepare("SELECT product_id FROM products WHERE barcode = ?");
    checkQuery.addBindValue(barcode);
    
    int productId = -1;
    if (checkQuery.exec() && checkQuery.next()) {
        productId = checkQuery.value(0).toInt();
        // 更新现有商品：累加库存，并同步更新最后一次入库确定的售价
        QSqlQuery updateQuery(db);
        updateQuery.prepare("UPDATE products SET stock_current = stock_current + ?, stock_curr = stock_curr + ?, cost_price = ?, sale_price = ?, supplier = ? WHERE product_id = ?");
        updateQuery.addBindValue(qty);
        updateQuery.addBindValue(qty);
        updateQuery.addBindValue(cost);
        updateQuery.addBindValue(sale);
        updateQuery.addBindValue(supplier);
        updateQuery.addBindValue(productId);
        if (!updateQuery.exec()) {
            db.rollback();
            client->sendPacket(Protocol::CMD_SHELVE_PRODUCT, "{\"status\":1, \"message\":\"Update master failed\"}");
            return;
        }
    } else {
        // 新建商品档案
        QSqlQuery insertQuery(db);
        insertQuery.prepare("INSERT INTO products (barcode, name, spec, category, img_data, cost_price, sale_price, stock_current, stock_curr, supplier, is_active, unit) "
                            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 1, '个')");
        insertQuery.addBindValue(barcode);
        insertQuery.addBindValue(name);
        insertQuery.addBindValue(spec);
        insertQuery.addBindValue(category);
        insertQuery.addBindValue(imgData);
        insertQuery.addBindValue(cost);
        insertQuery.addBindValue(sale); 
        insertQuery.addBindValue(qty);
        insertQuery.addBindValue(qty);
        insertQuery.addBindValue(supplier);
        if (!insertQuery.exec()) {
            db.rollback();
            client->sendPacket(Protocol::CMD_SHELVE_PRODUCT, "{\"status\":1, \"message\":\"Insert master failed\"}");
            return;
        }
        productId = insertQuery.lastInsertId().toInt();
    }

    // 3. 计算到期日并写入批次表 (product_batches)
    QDate prodDate = rec.value("production_date").toDate();
    int shelfLife = rec.value("shelf_life_days").toInt();
    QDate expiryDate = prodDate.addDays(shelfLife);
    QString inboundNo = rec.value("inbound_no").toString();

    QSqlQuery batchQuery(db);
    batchQuery.prepare("INSERT INTO product_batches (batch_id, product_id, expiry_date, initial_qty, current_qty) "
                       "VALUES (?, ?, ?, ?, ?)");
    batchQuery.addBindValue(inboundNo);
    batchQuery.addBindValue(productId);
    batchQuery.addBindValue(expiryDate.toString("yyyy-MM-dd"));
    batchQuery.addBindValue(qty);
    batchQuery.addBindValue(qty);
    
    if (!batchQuery.exec()) {
        db.rollback();
        LOG_E("[PRODUCT] Batch insert failed: " << batchQuery.lastError().text().toStdString());
        client->sendPacket(Protocol::CMD_SHELVE_PRODUCT, "{\"status\":1, \"message\":\"Batch info failed\"}");
        return;
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
    
    // 广播通知
    QJsonObject notify;
    notify["module"] = "product";
    m_server->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
}

void ProductController::handleUnshelveProduct(ClientHandler* client, const QJsonObject &data)
{
    int inboundId = data["id"].toInt();
    LOG_I("[PRODUCT] Unshelving inbound record ID:" << inboundId);

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    db.transaction();
    QSqlQuery query(db);

    // 1. 获取入库单详情
    query.prepare("SELECT * FROM product_inbound WHERE id = ?");
    query.addBindValue(inboundId);
    if (!query.exec() || !query.next()) {
        db.rollback();
        client->sendPacket(Protocol::CMD_UNSHELVE_PRODUCT, "{\"status\":1, \"message\":\"Inbound record not found\"}");
        return;
    }

    QSqlRecord rec = query.record();
    QString barcode = rec.value("barcode").toString();
    int qty = rec.value("quantity").toInt();
    QString inboundNo = rec.value("inbound_no").toString();

    // 2. 从 products 表扣除库存
    QSqlQuery updateQuery(db);
    updateQuery.prepare("UPDATE products SET stock_current = stock_current - ?, stock_curr = stock_curr - ? WHERE barcode = ?");
    updateQuery.addBindValue(qty);
    updateQuery.addBindValue(qty);
    updateQuery.addBindValue(barcode);
    if (!updateQuery.exec()) {
        db.rollback();
        client->sendPacket(Protocol::CMD_UNSHELVE_PRODUCT, "{\"status\":1, \"message\":\"Update master failed\"}");
        return;
    }

    // 3. 删除对应的批次记录
    QSqlQuery batchQuery(db);
    batchQuery.prepare("DELETE FROM product_batches WHERE batch_id = ?");
    batchQuery.addBindValue(inboundNo);
    if (!batchQuery.exec()) {
        db.rollback();
        client->sendPacket(Protocol::CMD_UNSHELVE_PRODUCT, "{\"status\":1, \"message\":\"Delete batch failed\"}");
        return;
    }

    // 4. 标记入库单为待上架
    QSqlQuery finalQuery(db);
    finalQuery.prepare("UPDATE product_inbound SET is_shelved = 0 WHERE id = ?");
    finalQuery.addBindValue(inboundId);
    finalQuery.exec();

    db.commit();
    LOG_I("[PRODUCT] Unshelve success for barcode:" << barcode.toStdString());

    QJsonObject resp;
    resp["status"] = Protocol::STATUS_OK;
    resp["message"] = "Success";
    client->sendPacket(Protocol::CMD_UNSHELVE_PRODUCT, QJsonDocument(resp).toJson(QJsonDocument::Compact));

    // 广播通知
    QJsonObject notify;
    notify["module"] = "product";
    m_server->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
}
