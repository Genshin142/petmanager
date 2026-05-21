#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H

#include <QPixmap>
#include <QByteArray>
#include <QString>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QCache>
#include <QMutex>

class ImageUtils {
private:
    static QCache<QString, QPixmap>& getCache() {
        static QCache<QString, QPixmap> cache(500); // 缓存最多500张图片
        return cache;
    }
    
    static QMutex& getMutex() {
        static QMutex mutex;
        return mutex;
    }

    static QPixmap loadPixmapUncached(const QString &source) {
        // 1. 检查是否为 Data URI 格式 (data:image/xxx;base64,...)
        if (source.startsWith("data:image")) {
            int commaIndex = source.indexOf(',');
            if (commaIndex != -1) {
                QByteArray ba = QByteArray::fromBase64(source.mid(commaIndex + 1).toLatin1());
                QPixmap pix;
                if (pix.loadFromData(ba)) {
                    return pix;
                }
            }
            return QPixmap(); // 若以 data:image 开头但加载失败，避免退回文件系统尝试
        }
        
        // 2. 检查是否为 Base64 (纯 Base64，没有 prefix，可能包含 Base64 字符如 '/' 或 '+')
        // 如果长度较长且成功用 Base64 加载，直接返回
        if (source.length() > 64) {
            QByteArray ba = QByteArray::fromBase64(source.toLatin1());
            QPixmap pix;
            if (pix.loadFromData(ba)) {
                return pix;
            }
        }
        
        // 3. 安全防护：如果字符串过长，绝对不可能是一个合法的 Windows 本地文件路径
        // 直接传入 QPixmap 会导致 Qt 底层 QFileSystemEngine 调用 Win32 API 解析超长路径时发生段错误 (SIGSEGV)
        if (source.length() > 1024) {
            qWarning() << "[ImageUtils] Ignored abnormally long path to prevent SIGSEGV, length:" << source.length();
            return QPixmap();
        }
        
        // 4. 正常文件路径加载
        return QPixmap(source);
    }

public:
    static QPixmap loadPixmap(const QString &source) {
        if (source.isEmpty()) return QPixmap();
        
        // L1 缓存检索
        {
            QMutexLocker locker(&getMutex());
            if (getCache().contains(source)) {
                return *getCache().object(source);
            }
        }
        
        QPixmap pix = loadPixmapUncached(source);
        
        if (!pix.isNull()) {
            QMutexLocker locker(&getMutex());
            getCache().insert(source, new QPixmap(pix));
        }
        return pix;
    }

    static QPixmap getCircularPixmap(const QPixmap &src, int size) {
        if (src.isNull()) return QPixmap();
        
        QPixmap scaled = src.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        QPixmap result(size, size);
        result.fill(Qt::transparent);
        
        QPainter painter(&result);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        
        QPainterPath path;
        path.addEllipse(0, 0, size, size);
        painter.setClipPath(path);
        
        // 居中绘制缩放后的图片
        int x = (size - scaled.width()) / 2;
        int y = (size - scaled.height()) / 2;
        painter.drawPixmap(x, y, scaled);
        
        return result;
    }
};

#endif // IMAGEUTILS_H
