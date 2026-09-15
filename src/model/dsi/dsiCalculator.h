#pragma once

#include <QPointF>
// ─────────────────────────────────────────────────────────────────────────
// DSICalculator — Motor matemático puro del módulo DSI.
//
// No tiene estado interno ni dependencias de Qt signals/slots. Resuelve
// dos problemas matemáticos distintos:
//
//   1. Proyección inicial (Sección B): convierte AZ/DT o LAT/LON en
//      coordenadas DM absolutas — se ejecuta una sola vez al crear la DSI.
//
//   2. Recálculo dinámico (Sección C): si la DSI está asociada a un
//      track, adopta directamente su posición instantánea (sin offset,
//      a diferencia de Texto) — se ejecuta cada 80ms.
//
// También resuelve el chequeo geométrico de la alarma: si un track dado
// está dentro del radio de una DSI.
// ─────────────────────────────────────────────────────────────────────────

class DSICalculator {
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
    // origen geográfico del BP (mismo mecanismo que TextCalculator).
    static QPointF resolveFromLatLon(
        double originLat, double originLon,
        double targetLat, double targetLon
    );

    // ── Recálculo dinámico (Sección C) ───────────────────────────────────

    // Adopción directa: la posición actual de una DSI asociada ES la
    // posición instantánea del track — sin offset (REQ-DSI-UI-017).
    static QPointF resolveAssociatedPosition(QPointF trackPos);

    // ── Evaluación de alarma (F1/F2, REQ-DSI-CAL-001) ────────────────────

    // True si trackPos está dentro (o exactamente sobre) el radio de la
    // DSI centrada en zoneCenter.
    static bool isTrackInsideZone(
        QPointF zoneCenter,
        double  radiusDm,
        QPointF trackPos
    );
};
