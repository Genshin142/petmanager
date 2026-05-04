#include "logger.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QMutex>
#include <iostream>

namespace Logger {
    QMutex mutex;
    QString logFilePath;

    void init() {
        QDir().mkpath("logs");
        logFilePath = "logs/app_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".log";
        qInstallMessageHandler(messageHandler);
        qDebug() << "Logger initialized. Logging to:" << logFilePath;
    }

    void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
        QMutexLocker locker(&mutex);

        QString level;
        switch (type) {
            case QtDebugMsg:    level = "DEBUG"; break;
            case QtInfoMsg:     level = "INFO "; break;
            case QtWarningMsg:  level = "WARN "; break;
            case QtCriticalMsg: level = "CRIT "; break;
            case QtFatalMsg:    level = "FATAL"; break;
        }

        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        QString logLine = QString("[%1] [%2] (%3:%4, %5) - %6")
                            .arg(timestamp, level, context.file, QString::number(context.line), context.function, msg);

        // Print to console
        std::cerr << logLine.toStdString() << std::endl;

        // Write to file
        QFile file(logFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out << logLine << "\n";
            file.close();
        }

        if (type == QtFatalMsg) {
            abort();
        }
    }
}
