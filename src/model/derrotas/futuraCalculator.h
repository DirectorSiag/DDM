#pragma once
#include <QList>
#include <QPointF>

class FuturaCalculator {
public:
    // Factor de conversion tactico: de Millas Nauticas (NM) a Data Miles (DM).
    // 1 NM = 6076.1154 pies / 1 DM = 6000 pies -> Relacion exacta: 1.012685
    static constexpr double kNmToDm = 1.012685;

    // Calcula la distancia total, la convierte a DM, y la fracciona en 10 puntos equidistantes sobre el rumbo del track (las 10 cruces).
    static void calculateProjection(
        // -- Entradas --
        QPointF             trackPos,
        double              trackCourseDeg,
        double              trackSpeedKn,
        int                 timeMinutes,
        // -- Salidas --
        QList<QPointF>&     out_crossPoints,
        double&             out_totalDistanceDm
        );

    // Compara el estado base (tomado al INICIAR)y lo contrasta con el estado actual del track. OR logico entre desvio de rumbo y de velocidad.
    static bool evaluateAlarm(
        double baseCourseDeg,
        double baseSpeedKn,
        double currentCourseDeg,
        double currentSpeedKn,
        double thresholdDeg,
        double thresholdKn
        );
};