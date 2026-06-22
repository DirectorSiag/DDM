#pragma once

#include <QPointF>
#include "model/fondeo/FondeoSessionState.h"

class FondeoCalculator {
public:
    static QPointF resolvePuntoFondeo(const FondeoConfig& config, const QPointF& trackPos, double ownLatDec, double ownLonDec);
    static QPointF resolvePuntoAuxiliar(const QPointF& pf, double paAz, double paDt);
    static void calculateDistAzPfPa(const QPointF& ownPos, FondeoSessionState& out_state);
    static void calculateMarcacionRelativa(double ownCourseDeg, FondeoSessionState& out_state);
    static void calculatePanelPredictivo(FondeoSessionState& out_state);
};