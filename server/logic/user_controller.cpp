#include "user_controller.h"
#include "../network/server_core.h"
#include "../database/connectionpool.h"
#include "../../protocol_codes.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

UserController::UserController(ServerCore* core, QObject* parent) 
    : QObject(parent), m_core(core) 
{
    // 在这里向 ServerCore 注册它负责的指令
    m_core->registerHandler(Protocol::CMD_LOGIN, std::bind(&UserController::handleLogin, this, std::placeholders::_1, std::placeholders::_2));
}

void UserController::handleLogin(ClientHandler* client, const QJsonObject& data) {
    QString user = data["username"].toString();
    QString pwd = data["password"].toString();

    LOG_I("[AUTH] Controller handling login for: " << user.toStdString());

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("SELECT * FROM sys_employees WHERE username = ? AND password = ? AND is_deleted = 0");
    query.addBindValue(user);
    query.addBindValue(pwd);

    QJsonObject response;
    if (query.exec() && query.next()) {
        int empId = query.value("emp_id").toInt();
        client->setUserId(empId); // 绑定用户ID到连接对象
        
        response["status"] = Protocol::STATUS_OK;
        response["message"] = "Login successful";
        
        QJsonObject userInfo;
        userInfo["emp_id"] = empId;
        userInfo["real_name"] = query.value("real_name").toString();
        userInfo["role"] = query.value("role").toString();
        response["user_info"] = userInfo;
        
        LOG_I("[AUTH] Success: " << user.toStdString() << " (ID: " << empId << ")");
    } else {
        response["status"] = Protocol::STATUS_AUTH_FAIL;
        response["message"] = "Invalid username or password";
        LOG_I("[AUTH] Failed: " << user.toStdString());
    }

    client->sendPacket(Protocol::CMD_LOGIN, QJsonDocument(response).toJson(QJsonDocument::Compact));
}
