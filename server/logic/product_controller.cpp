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
        m_server->registerHandler(Protocol::CMD_ADD_INBOUND_RECORD, 
            std::bind(&ProductController::handleAddInboundRecord, this, std::placeholders::_1, std::placeholders::_2));
        m_server->registerHandler(Protocol::CMD_UPDATE_INBOUND_RECORD, 
            std::bind(&ProductController::handleUpdateInboundRecord, this, std::placeholders::_1, std::placeholders::_2));
        m_server->registerHandler(Protocol::CMD_DELETE_INBOUND_RECORD, 
            std::bind(&ProductController::handleDeleteInboundRecord, this, std::placeholders::_1, std::placeholders::_2));
        m_server->registerHandler(Protocol::CMD_UPDATE_PRODUCT, 
            std::bind(&ProductController::handleUpdateProduct, this, std::placeholders::_1, std::placeholders::_2));
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
    query.prepare("SELECT p.*, "
                  "  COALESCE(( "
                  "    SELECT SUM(CAST(item.count AS SIGNED)) "
                  "    FROM orders o, "
                  "    JSON_TABLE(CASE WHEN JSON_VALID(o.item_details) THEN o.item_details ELSE '[]' END, '$[*]' COLUMNS ( "
                  "      barcode VARCHAR(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci PATH '$.barcode', "
                  "      count INT PATH '$.count' "
                  "    )) as item "
                  "    WHERE o.status = 'Paid' AND o.source_module = 'Product' AND item.barcode = p.barcode "
                  "  ), 0) as sales_count "
                  "FROM products p WHERE p.is_deleted = 0");
    
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
        p["sales_count"] = getValue("sales_count").toInt();

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
            item["is_active"] = query.value("is_deleted").toInt() == 0;
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
    QSqlQuery query(db);

    // 1. 获取入库单详情
    query.prepare("SELECT * FROM product_inbound WHERE id = ?");
    query.addBindValue(inboundId);
    if (!query.exec() || !query.next()) {
        client->sendPacket(Protocol::CMD_SHELVE_PRODUCT, "{\"status\":1, \"message\":\"Inbound record not found\"}");
        return;
    }

    QSqlRecord rec = query.record();
    int isShelved = rec.value("is_shelved").toInt();
    if (isShelved == 1) {
        client->sendPacket(Protocol::CMD_SHELVE_PRODUCT, "{\"status\":1, \"message\":\"This inbound record has already been shelved\"}");
        return;
    }
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
        LOG_E("[PRODUCT] Batch insert failed: " << batchQuery.lastError().text().toStdString());
        client->sendPacket(Protocol::CMD_SHELVE_PRODUCT, "{\"status\":1, \"message\":\"Batch info failed\"}");
        return;
    }

    // 3. 标记入库单为已上架
    QSqlQuery finalQuery(db);
    finalQuery.prepare("UPDATE product_inbound SET is_shelved = 1 WHERE id = ?");
    finalQuery.addBindValue(inboundId);
    finalQuery.exec();

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
    QSqlQuery query(db);

    // 1. 获取入库单详情
    query.prepare("SELECT * FROM product_inbound WHERE id = ?");
    query.addBindValue(inboundId);
    if (!query.exec() || !query.next()) {
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
        client->sendPacket(Protocol::CMD_UNSHELVE_PRODUCT, "{\"status\":1, \"message\":\"Update master failed\"}");
        return;
    }

    // 3. 删除对应的批次记录
    QSqlQuery batchQuery(db);
    batchQuery.prepare("DELETE FROM product_batches WHERE batch_id = ?");
    batchQuery.addBindValue(inboundNo);
    if (!batchQuery.exec()) {
        client->sendPacket(Protocol::CMD_UNSHELVE_PRODUCT, "{\"status\":1, \"message\":\"Delete batch failed\"}");
        return;
    }

    // 4. 标记入库单为待上架
    QSqlQuery finalQuery(db);
    finalQuery.prepare("UPDATE product_inbound SET is_shelved = 0 WHERE id = ?");
    finalQuery.addBindValue(inboundId);
    finalQuery.exec();

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

void ProductController::handleAddInboundRecord(ClientHandler* client, const QJsonObject &data)
{
    LOG_I("[PRODUCT] Adding new inbound record.");
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    // 生成入库单号 IN + yyyyMMdd + 4位序号
    QString today = QDate::currentDate().toString("yyyyMMdd");
    QString prefix = "IN" + today;
    query.prepare("SELECT inbound_no FROM product_inbound WHERE inbound_no LIKE ? ORDER BY inbound_no DESC LIMIT 1");
    query.addBindValue(prefix + "%");
    
    QString newNo = prefix + "0001";
    if (query.exec() && query.next()) {
        QString lastNo = query.value(0).toString();
        int seq = lastNo.right(4).toInt();
        newNo = prefix + QString("%1").arg(seq + 1, 4, 10, QChar('0'));
    }

    query.prepare("INSERT INTO product_inbound (inbound_no, barcode, product_name, spec, category, supplier, quantity, cost_price, sale_price, production_date, shelf_life_days, supplier_phone, operator_name, img_data, is_shelved) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0)");
    query.addBindValue(newNo);
    query.addBindValue(data["barcode"].toString());
    query.addBindValue(data["productName"].toString());
    query.addBindValue(data["spec"].toString());
    query.addBindValue(data["category"].toString());
    query.addBindValue(data["supplier"].toString());
    query.addBindValue(data["quantity"].toInt());
    query.addBindValue(data["cost_price"].toDouble());
    query.addBindValue(data["sale_price"].toDouble());
    query.addBindValue(data["productionDate"].toString());
    query.addBindValue(data["shelfLifeDays"].toInt());
    query.addBindValue(data["supplierPhone"].toString());
    query.addBindValue(data["operatorName"].toString());
    query.addBindValue(data["imgData"].toString());

    QJsonObject resp;
    if (query.exec()) {
        resp["status"] = Protocol::STATUS_OK;
        resp["inbound_no"] = newNo;
        // 广播刷新
        QJsonObject notify;
        notify["module"] = "product";
        m_server->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        resp["status"] = Protocol::STATUS_ERROR;
        resp["message"] = query.lastError().text();
    }
    client->sendPacket(Protocol::CMD_ADD_INBOUND_RECORD, QJsonDocument(resp).toJson(QJsonDocument::Compact));
}

void ProductController::handleUpdateInboundRecord(ClientHandler* client, const QJsonObject &data)
{
    int id = data["id"].toInt();
    LOG_I("[PRODUCT] Updating inbound record ID: " << id);
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("UPDATE product_inbound SET product_name=?, spec=?, category=?, supplier=?, quantity=?, cost_price=?, sale_price=?, production_date=?, shelf_life_days=?, supplier_phone=?, operator_name=?, img_data=? WHERE id=?");
    query.addBindValue(data["productName"].toString());
    query.addBindValue(data["spec"].toString());
    query.addBindValue(data["category"].toString());
    query.addBindValue(data["supplier"].toString());
    query.addBindValue(data["quantity"].toInt());
    query.addBindValue(data["cost_price"].toDouble());
    query.addBindValue(data["sale_price"].toDouble());
    query.addBindValue(data["productionDate"].toString());
    query.addBindValue(data["shelfLifeDays"].toInt());
    query.addBindValue(data["supplierPhone"].toString());
    query.addBindValue(data["operatorName"].toString());
    query.addBindValue(data["imgData"].toString());
    query.addBindValue(id);

    QJsonObject resp;
    if (query.exec()) {
        resp["status"] = Protocol::STATUS_OK;
        // 广播刷新
        QJsonObject notify;
        notify["module"] = "product";
        m_server->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        resp["status"] = Protocol::STATUS_ERROR;
        resp["message"] = query.lastError().text();
    }
    client->sendPacket(Protocol::CMD_UPDATE_INBOUND_RECORD, QJsonDocument(resp).toJson(QJsonDocument::Compact));
}

void ProductController::handleDeleteInboundRecord(ClientHandler* client, const QJsonObject &data)
{
    QString dateTime = data["dateTime"].toString();
    QString barcode = data["barcode"].toString();
    bool isHard = data["hard"].toBool();
    bool isRestore = data["restore"].toBool();
    
    LOG_I("[PRODUCT] Delete/Restore inbound record. Barcode: " << barcode.toStdString() << " Date: " << dateTime.toStdString() << " Restore: " << isRestore);
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    bool execSuccess = false;
    QString errorMsg;

    if (isHard) {
        query.prepare("DELETE FROM product_inbound WHERE barcode = ? AND created_at = ?");
        query.addBindValue(barcode);
        query.addBindValue(dateTime);
        if (query.exec()) {
            // Check if any inbound records still exist for this barcode
            QSqlQuery checkQuery(db);
            checkQuery.prepare("SELECT COUNT(*) FROM product_inbound WHERE barcode = ?");
            checkQuery.addBindValue(barcode);
            if (checkQuery.exec() && checkQuery.next()) {
                if (checkQuery.value(0).toInt() == 0) {
                    // No more inbound records, delete from products catalog
                    QSqlQuery delProdQuery(db);
                    delProdQuery.prepare("DELETE FROM products WHERE barcode = ?");
                    delProdQuery.addBindValue(barcode);
                    delProdQuery.exec();
                    
                    // Also cleanup orphaned batches
                    QSqlQuery delBatchQuery(db);
                    delBatchQuery.prepare("DELETE FROM product_batches WHERE product_id NOT IN (SELECT product_id FROM products)");
                    delBatchQuery.exec();
                }
            }
            execSuccess = true;
        } else {
            errorMsg = query.lastError().text();
        }
    } else {
        if (isRestore) {
            query.prepare("UPDATE product_inbound SET is_deleted = 0 WHERE barcode = ? AND created_at = ?");
        } else {
            query.prepare("UPDATE product_inbound SET is_deleted = 1 WHERE barcode = ? AND created_at = ?");
        }
        query.addBindValue(barcode);
        query.addBindValue(dateTime);
        execSuccess = query.exec();
        if (!execSuccess) errorMsg = query.lastError().text();
    }

    QJsonObject resp;
    if (execSuccess) {
        resp["status"] = Protocol::STATUS_OK;
        // 广播刷新
        QJsonObject notify;
        notify["module"] = "product";
        m_server->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        resp["status"] = Protocol::STATUS_ERROR;
        resp["message"] = errorMsg;
    }
    client->sendPacket(Protocol::CMD_DELETE_INBOUND_RECORD, QJsonDocument(resp).toJson(QJsonDocument::Compact));
}

void ProductController::handleUpdateProduct(ClientHandler* client, const QJsonObject &data)
{
    QString barcode = data["barcode"].toString();
    LOG_I("[PRODUCT] Updating product archive: " << barcode.toStdString());
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    // 检查是否存在
    query.prepare("SELECT product_id FROM products WHERE barcode = ?");
    query.addBindValue(barcode);
    
    bool exists = false;
    if (query.exec() && query.next()) exists = true;
    
    if (exists) {
        query.prepare("UPDATE products SET name=?, brand=?, origin=?, category=?, spec=?, unit=?, sale_price=?, stock_min=?, cost_price=?, supplier=?, supplier_phone=?, description=?, img_data=? WHERE barcode=?");
    } else {
        query.prepare("INSERT INTO products (name, brand, origin, category, spec, unit, sale_price, stock_min, cost_price, supplier, supplier_phone, description, img_data, barcode) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    }
    
    query.addBindValue(data["name"].toString());
    query.addBindValue(data["brand"].toString());
    query.addBindValue(data["origin"].toString());
    query.addBindValue(data["category"].toString());
    query.addBindValue(data["spec"].toString());
    query.addBindValue(data["unit"].toString());
    query.addBindValue(data["sale_price"].toDouble());
    query.addBindValue(data["stock_min"].toInt());
    query.addBindValue(data["cost_price"].toDouble());
    query.addBindValue(data["supplier"].toString());
    query.addBindValue(data["supplier_phone"].toString());
    query.addBindValue(data["description"].toString());
    query.addBindValue(data["img_data"].toString());
    query.addBindValue(barcode);

    QJsonObject resp;
    if (query.exec()) {
        resp["status"] = Protocol::STATUS_OK;
        // 广播刷新
        QJsonObject notify;
        notify["module"] = "product";
        m_server->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        resp["status"] = Protocol::STATUS_ERROR;
        resp["message"] = query.lastError().text();
    }
    client->sendPacket(Protocol::CMD_UPDATE_PRODUCT, QJsonDocument(resp).toJson(QJsonDocument::Compact));
}
