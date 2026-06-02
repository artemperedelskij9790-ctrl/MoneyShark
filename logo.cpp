#include "logo.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QSizePolicy>

namespace {

void drawMoneySharkLogo(QPainter &painter, const QRectF &target)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.translate(target.topLeft());
    const qreal scale = qMin(target.width() / 520.0, target.height() / 360.0);
    painter.scale(scale, scale);
    const qreal xOffset = (target.width() / scale - 520.0) / 2.0;
    const qreal yOffset = (target.height() / scale - 360.0) / 2.0;
    painter.translate(xOffset, yOffset);

    const QColor rim("#e9e2d5");
    const QColor badge("#8bc4cf");
    const QColor badgeDark("#5f9da9");
    const QColor shark("#1d2228");
    const QColor text("#f5f0e8");

    painter.setPen(Qt::NoPen);
    painter.setBrush(rim);
    painter.drawEllipse(QRectF(100, 24, 318, 318));
    painter.setBrush(badgeDark);
    painter.drawEllipse(QRectF(116, 40, 286, 286));
    painter.setBrush(badge);
    painter.drawPie(QRectF(142, 62, 234, 234), 20 * 16, 190 * 16);

    QPainterPath sharkPath;
    sharkPath.moveTo(28, 116);
    sharkPath.cubicTo(70, 74, 184, 114, 252, 124);
    sharkPath.cubicTo(300, 132, 336, 78, 396, 70);
    sharkPath.cubicTo(372, 98, 352, 138, 342, 190);
    sharkPath.cubicTo(380, 214, 450, 200, 500, 176);
    sharkPath.cubicTo(468, 222, 424, 254, 352, 260);
    sharkPath.cubicTo(312, 264, 282, 246, 248, 232);
    sharkPath.cubicTo(182, 206, 112, 188, 44, 178);
    sharkPath.cubicTo(58, 160, 68, 144, 72, 132);
    sharkPath.cubicTo(54, 130, 40, 124, 28, 116);
    sharkPath.closeSubpath();

    painter.setPen(QPen(rim, 13, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(shark);
    painter.drawPath(sharkPath);
    painter.setPen(QPen(shark, 1.5));
    painter.drawPath(sharkPath);

    QPainterPath bellyPath;
    bellyPath.moveTo(62, 123);
    bellyPath.cubicTo(92, 153, 164, 167, 240, 218);
    bellyPath.cubicTo(174, 204, 102, 188, 48, 177);
    bellyPath.cubicTo(62, 159, 72, 144, 74, 132);
    bellyPath.cubicTo(70, 130, 66, 127, 62, 123);
    painter.setPen(Qt::NoPen);
    painter.setBrush(text);
    painter.drawPath(bellyPath);

    painter.setPen(QPen(rim, 5, Qt::SolidLine, Qt::RoundCap));
    for (int i = 0; i < 5; ++i) {
        painter.drawLine(QPointF(190 + i * 11, 160), QPointF(175 + i * 10, 204));
    }

    painter.setPen(QPen(rim, 4, Qt::SolidLine, Qt::RoundCap));
    painter.drawArc(QRectF(86, 100, 22, 22), -90 * 16, 260 * 16);

    painter.setPen(QPen(shark, 3, Qt::SolidLine, Qt::RoundCap));
    for (int i = 0; i < 12; ++i) {
        const qreal x = 48 + i * 9;
        painter.drawLine(QPointF(x, 138), QPointF(x + 4, 150));
    }

    painter.setPen(QPen(rim, 20, Qt::SolidLine, Qt::RoundCap));
    painter.drawArc(QRectF(136, 96, 246, 246), 204 * 16, 86 * 16);

    QFont font = painter.font();
    font.setFamily(QStringLiteral("Arial"));
    font.setBold(true);
    font.setPointSize(30);
    painter.setFont(font);
    painter.setPen(text);
    painter.drawText(QRectF(118, 248, 285, 76), Qt::AlignCenter, QStringLiteral("MONEY SHARK"));

    painter.restore();
}

QString rasterLogoPath()
{
    const QStringList candidates = {
        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("assets/moneyshark_logo.png")),
        QDir::current().absoluteFilePath(QStringLiteral("assets/moneyshark_logo.png")),
        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("../assets/moneyshark_logo.png"))
    };

    for (const QString &candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }
    return QString();
}

QPixmap rasterLogoPixmap()
{
    const QString path = rasterLogoPath();
    if (path.isEmpty()) {
        return {};
    }
    return QPixmap(path);
}

} // namespace

QPixmap renderMoneySharkLogo(const QSize &size)
{
    const QSize safeSize(qMax(32, size.width()), qMax(32, size.height()));
    QPixmap pixmap(safeSize);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QPixmap rasterLogo = rasterLogoPixmap();
    if (!rasterLogo.isNull()) {
        const QPixmap scaled = rasterLogo.scaled(safeSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        const QPoint topLeft((safeSize.width() - scaled.width()) / 2, (safeSize.height() - scaled.height()) / 2);
        painter.drawPixmap(topLeft, scaled);
    } else {
        drawMoneySharkLogo(painter, QRectF(QPointF(0, 0), safeSize));
    }
    return pixmap;
}

QIcon moneySharkIcon()
{
    QIcon icon;
    const QList<int> sizes = {16, 24, 32, 48, 64, 128, 256};
    for (int size : sizes) {
        icon.addPixmap(renderMoneySharkLogo(QSize(size, size)));
    }
    return icon;
}

MoneySharkLogoWidget::MoneySharkLogoWidget(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

QSize MoneySharkLogoWidget::sizeHint() const
{
    return QSize(360, 250);
}

QSize MoneySharkLogoWidget::minimumSizeHint() const
{
    return QSize(220, 150);
}

void MoneySharkLogoWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QRect targetRect = rect().adjusted(6, 6, -6, -6);
    const QPixmap rasterLogo = rasterLogoPixmap();
    if (!rasterLogo.isNull()) {
        const QPixmap scaled = rasterLogo.scaled(targetRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        const QPoint topLeft(
            targetRect.x() + (targetRect.width() - scaled.width()) / 2,
            targetRect.y() + (targetRect.height() - scaled.height()) / 2);
        painter.drawPixmap(topLeft, scaled);
        return;
    }

    drawMoneySharkLogo(painter, targetRect);
}
