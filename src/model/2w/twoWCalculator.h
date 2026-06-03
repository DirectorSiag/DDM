#pragma once

#include <QList>
#include <QPointF>

class TwoWCalculator {
public:
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