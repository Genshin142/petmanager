#pragma once
#include <QDebug>

#define INFO "INFO"
#define WARNING "WARN"
#define ERROR "ERROR"
#define FATAL "FATAL"

#define LOG(level) qDebug().noquote() << "[" level "]"
