#include "connectionpool.h"
#include <QtSql/QSqlError>
#include <QtCore/QThreadStorage>
#include <QtCore/QThread>
#include <QtCore/QDebug>

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
    QMutexLocker locker(&m_mutex);
    
    if (!s_threadConnectionName.hasLocalData()) {
        QString name = QString("PetConn_%1").arg(QString::number((quintptr)QThread::currentThread(), 16));
        s_threadConnectionName.setLocalData(name);

        QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", name);
        db.setHostName(m_hostName);
        db.setDatabaseName(m_databaseName);
        db.setUserName(m_username);
        db.setPassword(m_password);
        db.setPort(m_port);

        if (!db.open()) {
            qCritical() << "Thread" << QThread::currentThread() << "could not open database:" << db.lastError().text();
        } else {
            qDebug() << "Database connection opened for thread:" << QThread::currentThread();
        }
        return db;
    }
    
    return QSqlDatabase::database(s_threadConnectionName.localData());
}

void ConnectionPool::closeConnection(QSqlDatabase db)
{
    // Simplified for now
}
