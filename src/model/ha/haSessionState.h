#pragma once

#include <QString>
#include <QPointF>
#include <QDateTime>
#include "haSessionTimer.h"

struct HaSessionState {

    // ── Control ──────────────────────────────────────────────────────────
    bool    active      = false;
    int     slotIndex   = -1;      // 1..10, -1 = sin asignar
    double  fallPointX  = 0.0;
    double  fallPointY  = 0.0;

    // ── Resultados — recalculados cada 80ms ───────────────────────────────
    double  trueAzimuthDeg      = 0.0;
    double  relativeBearingDeg  = 0.0;
    QString banda               = QStringLiteral("-");
    double  distanceYards       = 0.0;
    bool    etaValid            = false;
    double  timeToArrivalMin    = 0.0;

    // ── Componente temporal ───────────────────────────────────────────────
    QString fallTimeLocal   = QStringLiteral("--:--:--");
    QString fallTimeUtc     = QStringLiteral("--:--:--");
    QString elapsedTime     = QStringLiteral("00:00:00");

    // ── Para el radar (LPD) ───────────────────────────────────────────────
    QPointF haIconCenter;

    // ── Timer de la sesión (compartido entre todas las instancias de HaService) ──
    HaSessionTimer timer;

    void reset() { *this = HaSessionState{}; }
};