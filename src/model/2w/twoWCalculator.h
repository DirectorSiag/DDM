#pragma once

#include <QList>
#include <QPointF>

class TwoWCalculator {
public:
    // Factor de conversion tactico: de Millas Nauticas (NM) a Data Miles (DM).
    // 1 NM = 6076.1154 pies / 1 DM = 6000 pies -> Relacion exacta: 1.012685
    static constexpr double kNmToDm = 1.012685;

    static void calculate(
        // -- Entradas --
        QPointF             guidePos,
        QPointF             ownPos,
        double              ownSpeed,
        int                 bpStation,
        double              circleRadiusNm,
        const QList<int>&   selectedStations,
        // -- Salidas
        QPointF&            out_guideCenter,
        QPointF&            out_ownCenter,
        QList<QPointF>&     out_allyCenters,
        bool&               out_etaValid,
        double&             out_courseDeg,
        double&             out_etaMin,
        double&             out_currentAzDeg,
        double&             out_currentDistNm,
        double&             out_expectedAzDeg,
        double&             out_expectedDistNm
        );

};