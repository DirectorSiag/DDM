#pragma once

#include <QString>
#include <QDateTime>


// Responsabilidades:
//   - Captura y congela la hora de caída (local y UTC) al iniciarse.
//   - Calcula el tiempo transcurrido desde el inicio en formato HH:MM:SS.


class HaSessionTimer {
public:

    void start();

    QString fallTimeLocal() const;
    QString fallTimeUtc() const;

    // Calcula el tiempo transcurrido desde start() hasta ahora.
    QString elapsedTime() const;

    void reset();

private:
    QDateTime m_fallTimestamp;  // Timestamp congelado al inicio
    bool      m_started = false;
};