#pragma once

#include <QPointF>
#include "textLabel.h"

// ─────────────────────────────────────────────────────────────────────────
// TextCalculator — Motor matemático puro del módulo Texto.
//
// No tiene estado interno ni dependencias de Qt signals/slots. Resuelve
// dos problemas matemáticos distintos:
//
//   1. Proyección inicial (Sección B): convierte AZ/DT, LAT/LON o la
//      posición de un track en coordenadas DM absolutas — se ejecuta
//      una sola vez al crear el texto.
//
//   2. Recálculo dinámico (Sección C): si el texto está asociado a un
//      track, calcula su posición actual sumando el offset fijo a la
//      posición instantánea del track — se ejecuta cada 80ms.
// ─────────────────────────────────────────────────────────────────────────

class TextCalculator {
public:

    // ── Resolución de posición inicial (Sección B) ───────────────────────

    // Disparador AZ/DT — proyecta desde una posición de referencia.
    // useVerdadero = true → azimut respecto al Norte geográfico.
    // useVerdadero = false → azimut relativo a la proa (requiere ownCourseDeg).
    static QPointF resolveFromBearing(
        QPointF referencePos,
        double  azimuthDeg,
        double  distanceDm,
        bool    useVerdadero,
        double  ownCourseDeg
    );

    // Disparador LAT/LON — convierte lat/lon absoluta a DM usando el
    // origen geográfico del BP (mismo mecanismo que HaService disparador 3).
    static QPointF resolveFromLatLon(
        double originLat, double originLon,
        double targetLat, double targetLon
    );

    // ── Recálculo dinámico (Sección C) ───────────────────────────────────

    // Calcula la posición actual de un label asociado a un track.
    // trackPos es la posición instantánea del track en DM.
    static QPointF resolveAssociatedPosition(
        QPointF trackPos,
        double  offsetXDm,
        double  offsetYDm
    );
};
