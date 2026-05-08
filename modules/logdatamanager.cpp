#include "logdatamanager.h"
#include <QVariant>
#include <QDebug>
#include <QDateTime>
#include <algorithm>
#include <QSet>

LogDataManager::LogDataManager(QObject *parent) : QObject(parent) {
    initTable();
}

bool LogDataManager::initTable() {
    // 内存模式下无需建表
    return true;
}

QList<SysOperationLog> LogDataManager::fetchLogs(int limit, int offset, const QString &startDate, const QString &endDate, const QString &operatorName, const QString &module) {
    QList<SysOperationLog> filteredList;
    
    for (const auto &log : m_mockLogs) {
        bool match = true;
        
        // 1. 日期筛选
        if (!startDate.isEmpty() && !endDate.isEmpty()) {
            QString logDate = log.timestamp.left(10);
            if (logDate < startDate || logDate > endDate) match = false;
        }
        
        // 2. 操作人筛选
        if (match && !operatorName.isEmpty()) {
            if (!log.operatorName.contains(operatorName, Qt::CaseInsensitive)) match = false;
        }
        
        // 3. 模块筛选
        if (match && !module.isEmpty()) {
            if (log.module != module) match = false;
        }
        
        if (match) filteredList.append(log);
    }
    
    // 排序 (按时间降序)
    std::sort(filteredList.begin(), filteredList.end(), [](const SysOperationLog &a, const SysOperationLog &b) {
        return a.timestamp > b.timestamp;
    });
    
    // 分页
    QList<SysOperationLog> result;
    for (int i = offset; i < filteredList.size() && result.size() < limit; ++i) {
        result.append(filteredList[i]);
    }
    
    return result;
}

int LogDataManager::getTotalCount(const QString &startDate, const QString &endDate, const QString &operatorName, const QString &module) {
    int count = 0;
    for (const auto &log : m_mockLogs) {
        bool match = true;
        if (!startDate.isEmpty() && !endDate.isEmpty()) {
            QString logDate = log.timestamp.left(10);
            if (logDate < startDate || logDate > endDate) match = false;
        }
        if (match && !operatorName.isEmpty()) {
            if (!log.operatorName.contains(operatorName, Qt::CaseInsensitive)) match = false;
        }
        if (match && !module.isEmpty()) {
            if (log.module != module) match = false;
        }
        if (match) count++;
    }
    return count;
}

QStringList LogDataManager::fetchDistinctModules() {
    QSet<QString> moduleSet;
    for (const auto &log : m_mockLogs) {
        moduleSet.insert(log.module);
    }
    QStringList list = moduleSet.values();
    std::sort(list.begin(), list.end());
    return list;
}

QStringList LogDataManager::fetchDistinctOperators() {
    QSet<QString> opSet;
    for (const auto &log : m_mockLogs) {
        opSet.insert(log.operatorName);
    }
    QStringList list = opSet.values();
    std::sort(list.begin(), list.end());
    return list;
}

bool LogDataManager::insertMockLog(const SysOperationLog &log) {
    // 内存模式：如果是已存在的ID，先删除旧的实现“替换”
    for (int i = 0; i < m_mockLogs.size(); ++i) {
        if (m_mockLogs[i].id == log.id) {
            m_mockLogs.removeAt(i);
            break;
        }
    }
    m_mockLogs.append(log);
    return true;
}
