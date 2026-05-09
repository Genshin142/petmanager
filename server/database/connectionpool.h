#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H

#include <QtSql/QSqlDatabase>
#include <QtCore/QMutex>
#include <QtCore/QString>

class ConnectionPool
{
public:
    static ConnectionPool& instance();
    ~ConnectionPool();

    QSqlDatabase openConnection();
    void closeConnection(QSqlDatabase db);

private:
    ConnectionPool();
    ConnectionPool(const ConnectionPool &other) = delete;
    ConnectionPool& operator=(const ConnectionPool &other) = delete;

    QString m_hostName;
    QString m_databaseName;
    QString m_username;
    QString m_password;
    int m_port;
    int m_maxConnections;

    static QMutex m_mutex;
};

#endif // CONNECTIONPOOL_H
