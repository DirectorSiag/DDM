#include "fondeoCalculator.h"
#include "RadarMath.h"
#include <cmath>

QPointF FondeoCalculator::resolvePuntoFondeo(const FondeoConfig& config, const QPointF& trackPos)
{
        // --- MODO TRACK ---
        const double kMnToDm = 1.012685;
        double distanceDm = config.trackDt * kMnToDm;
        double rad = config.trackAz * (M_PI / 180.0);

        return QPointF(
            trackPos.x() + distanceDm * std::sin(rad),
            trackPos.y() + distanceDm * std::cos(rad)
            );
}

QPointF FondeoCalculator::resolvePuntoFondeo(const FondeoConfig& config, double ownLatDec, double ownLonDec)
{
    double pfLatDec = RadarMath::dmsToDecimal(config.pfLatDeg, config.pfLatMin, config.pfLatSec);
    double pfLonDec = RadarMath::dmsToDecimal(config.pfLonDeg, config.pfLonMin, config.pfLonSec);

    double xDm = 0.0;
    double yDm = 0.0;

    RadarMath::latLonToDm(ownLatDec, ownLonDec, pfLatDec, pfLonDec, xDm, yDm);

    return QPointF(xDm, yDm);
}

QPointF FondeoCalculator::resolvePuntoAuxiliar(const QPointF& pf, double paAz, double paDt)
{
    const double kMnToDm = 1.012685;
    double distanceDm = paDt * kMnToDm;
    double rad = paAz * (M_PI / 180.0);

    return QPointF(
        pf.x() + distanceDm * std::sin(rad),
        pf.y() + distanceDm * std::cos(rad)
        );
}

void FondeoCalculator::calculateDistAzPfPa(
    const QPointF& ownPos,
    FondeoSessionState& out_state)
{
    // Se calculan las distancias en DM y luego se pasa a yardas
    double distPF_DM = RadarMath::calculateLength(ownPos, out_state.puntoFondeo);
    out_state.distanciaPF = RadarMath::dmToYards(distPF_DM);
    out_state.azimutPF = RadarMath::normalizeAngle360(RadarMath::calculateAngle(ownPos, out_state.puntoFondeo));

    double distPA_DM = RadarMath::calculateLength(ownPos, out_state.puntoAuxiliar);
    out_state.distanciaPA = RadarMath::dmToYards(distPA_DM);
    out_state.azimutPA = RadarMath::normalizeAngle360(RadarMath::calculateAngle(ownPos, out_state.puntoAuxiliar));
}

void FondeoCalculator::calculateMarcacionRelativa(
    double ownCourseDeg,
    FondeoSessionState& out_state)
{
    double azimutVerdaderoObjetivo = 0.0;

    if (!out_state.paAlcanzado) {
        // Fase 1: Aproximación al Punto Auxiliar (PA)
        azimutVerdaderoObjetivo = out_state.azimutPA;
        out_state.distanciaRelativa = out_state.distanciaPA;
    } else {
        // Fase 2: Aproximación final al Punto de Fondeo (PF)
        azimutVerdaderoObjetivo = out_state.azimutPF;
        out_state.distanciaRelativa = out_state.distanciaPF;
    }

    // Calculamos el Azimut Relativo
    out_state.azimutRelativo = std::fmod((azimutVerdaderoObjetivo - ownCourseDeg + 360.0), 360.0);
}

void FondeoCalculator::calculatePanelPredictivo(
    FondeoSessionState& out_state)
{

    double d = out_state.distanciaPF;
    const FondeoConfig& c = out_state.config;

    if (d > c.r1) {
        out_state.movimientoActual = {"AD. TODA", d - c.r1};
        out_state.proximoMovimiento = {"AD. MEDIA", d - c.r2};
    }
    else if (d > c.r2) {
        out_state.movimientoActual = {"AD. MEDIA", d - c.r2};
        out_state.proximoMovimiento = {"AD. DESP.", d - c.r3};
    }
    else if (d > c.r3) {
        out_state.movimientoActual = {"AD. DESP.", d - c.r3};
        out_state.proximoMovimiento = {"PARA MAQ", d - c.r4};
    }
    else if (d > c.r4) {
        out_state.movimientoActual = {"PARA MAQ", d - c.r4};
        out_state.proximoMovimiento = {"MAQ. AT", d - c.r5};
    }
    else if (d > c.r5) {
        out_state.movimientoActual = {"MAQ. AT", d - c.r5};
        out_state.proximoMovimiento = {"DETENCIÓN", d};
    }
    else {
        out_state.movimientoActual = {"DETENCIÓN", d};
        out_state.proximoMovimiento = {"FONDEADO", 0.0};
    }
}