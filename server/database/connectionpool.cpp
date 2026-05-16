#include "connectionpool.h"
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtCore/QThreadStorage>
#include <QtCore/QThread>
#include <QtCore/QDebug>
#include "../logger_compat.h"

QMutex ConnectionPool::m_mutex;
static QThreadStorage<QString> s_threadConnectionName;

ConnectionPool::ConnectionPool()
    : m_hostName("localhost")
    , m_databaseName("petstore")
    , m_username("root")
    , m_password("362345943")
    , m_port(3306)
    , m_maxConnections(20)
{
}

ConnectionPool::~ConnectionPool()
{
}

ConnectionPool& ConnectionPool::instance()
{
    static ConnectionPool pool;
    return pool;
}

QSqlDatabase ConnectionPool::openConnection()
{
    QString name;
    {
        QMutexLocker locker(&m_mutex);
        if (s_threadConnectionName.hasLocalData()) {
            name = s_threadConnectionName.localData();
        } else {
            name = QString("PetConn_%1").arg(QString::number((quintptr)QThread::currentThread(), 16));
            s_threadConnectionName.setLocalData(name);
        }
    }

    if (QSqlDatabase::contains(name)) {
        QSqlDatabase db = QSqlDatabase::database(name);
        if (db.isOpen()) return db;
        if (db.open()) return db;
        
        QMutexLocker locker(&m_mutex);
        QSqlDatabase::removeDatabase(name);
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", name);
    db.setHostName(m_hostName);
    db.setDatabaseName(m_databaseName);
    db.setUserName(m_username);
    db.setPassword(m_password);
    db.setPort(m_port);
    if (!db.open()) {
        LOG(ERROR) << "[DB] Connection failed for " << name.toStdString() << ": " << db.lastError().text().toStdString();
    } else {
        LOG(INFO) << "[DB] Connection opened successfully for " << name.toStdString();
        // 强制使用 utf8mb4 编码，解决中文乱码问题
        QSqlQuery query(db);
        query.exec("SET NAMES utf8mb4");
    }
    return db;
}

void ConnectionPool::closeConnection(QSqlDatabase db)
{
    // Simplified for now
}
