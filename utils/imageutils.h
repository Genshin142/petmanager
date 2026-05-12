#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H

#include <QPixmap>
#include <QByteArray>
#include <QString>
#include <QDebug>

#include <QPainter>
#include <QPainterPath>

class ImageUtils {
public:
    static QPixmap loadPixmap(const QString &source) {
        if (source.isEmpty()) return QPixmap();
        
        // 检查是否为 Data URI 格式 (data:image/xxx;base64,...)
        if (source.startsWith("data:image")) {
            int commaIndex = source.indexOf(',');
            if (commaIndex != -1) {
                QByteArray ba = QByteArray::fromBase64(source.mid(commaIndex + 1).toLatin1());
                QPixmap pix;
                if (pix.loadFromData(ba)) {
                    return pix;
                }
            }
        }
        
        // 检查是否为纯 Base64 (没有 prefix)
        if (source.length() > 100 && !source.contains('/') && !source.contains('\\') && !source.contains(':')) {
             QByteArray ba = QByteArray::fromBase64(source.toLatin1());
             QPixmap pix;
             if (pix.loadFromData(ba)) {
                 return pix;
             }
        }
        
        return QPixmap(source);
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
