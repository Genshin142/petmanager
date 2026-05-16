#include "logger.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QMutex>
#include <iostream>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dbghelp.h>

namespace {
    LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo) {
        QString dumpName = QString("logs/crash_%1.dmp").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
        HANDLE hFile = CreateFileW((LPCWSTR)dumpName.utf16(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            MINIDUMP_EXCEPTION_INFORMATION exInfo;
            exInfo.ThreadId = GetCurrentThreadId();
            exInfo.ExceptionPointers = exceptionInfo;
            exInfo.ClientPointers = FALSE;
            MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &exInfo, NULL, NULL);
            CloseHandle(hFile);
        }
        return EXCEPTION_EXECUTE_HANDLER;
    }
}
#endif

namespace Logger {
    QMutex mutex;
    QString logFilePath;

    void init(const QString &fileName) {
        QDir().mkpath("logs");
        logFilePath = "logs/" + fileName;
        
        // 启动时清空旧日志
        QFile file(logFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.close();
        }

#ifdef Q_OS_WIN
        SetUnhandledExceptionFilter(unhandledExceptionFilter);
#endif

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
