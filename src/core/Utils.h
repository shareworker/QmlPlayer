#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QtGlobal>

namespace Utils {

inline QString formatTimeMs(qint64 ms)
{
    if (ms <= 0)
        return QStringLiteral("00:00");

    const qint64 totalSeconds = ms / 1000;
    const qint64 minutes = totalSeconds / 60;
    const qint64 seconds = totalSeconds % 60;

    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}

inline qreal scaleToRange(qreal value, qreal inMin, qreal inMax, qreal outMin, qreal outMax)
{
    if (qFuzzyCompare(inMax, inMin))
        return outMin;

    const qreal t = (value - inMin) / (inMax - inMin);
    return outMin + t * (outMax - outMin);
}

}

#endif
