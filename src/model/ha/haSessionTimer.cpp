#include "haSessionTimer.h"

void HaSessionTimer::start() {
    m_fallTimestamp = QDateTime::currentDateTime();
    m_started = true;
}

QString HaSessionTimer::fallTimeLocal() const {
    if (!m_started) return QStringLiteral("--:--:--");
    return m_fallTimestamp.time().toString(QStringLiteral("HH:mm:ss"));
}

QString HaSessionTimer::fallTimeUtc() const {
    if (!m_started) return QStringLiteral("--:--:--");
    return m_fallTimestamp.toUTC().time().toString(QStringLiteral("HH:mm:ss"));
}

QString HaSessionTimer::elapsedTime() const {
    if (!m_started) return QStringLiteral("00:00:00");
    const qint64 secs = m_fallTimestamp.secsTo(QDateTime::currentDateTime());
    const int hh = static_cast<int>(secs / 3600);
    const int mm = static_cast<int>((secs % 3600) / 60);
    const int ss = static_cast<int>(secs % 60);
    return QString::asprintf("%02d:%02d:%02d", hh, mm, ss);
}

void HaSessionTimer::reset() {
    m_fallTimestamp = QDateTime();
    m_started = false;
}