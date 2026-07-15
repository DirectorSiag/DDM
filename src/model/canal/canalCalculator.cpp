#include "canalCalculator.h"
#include "RadarMath.h"
#include <cmath>

void CanalCalculator::calculateCinematica(const QPointF& ownPos, double ownCourse, double ownSpeedDm,
                                          const QPointF& buoyPos, CanalSlot& outSlot)
{
    // Distancia
    double distDM = RadarMath::calculateLength(ownPos, buoyPos);
    outSlot.distanciaYardas = RadarMath::dmToYards(distDM);

    // Azimut Verdadero
    outSlot.azimutVerdadero = RadarMath::normalizeAngle360(RadarMath::calculateAngle(ownPos, buoyPos));

    // Rumbo Verdadero
    double rv = outSlot.azimutVerdadero - ownCourse;
    if (rv > 180.0) rv -= 360.0;
    if (rv < -180.0) rv += 360.0;
    outSlot.rumboVerdadero = rv;

    // ETA
    if (ownSpeedDm > 0.0 && distDM > 0.0) {
        outSlot.timeToArrivalMin = (distDM / ownSpeedDm) * 60.0;
        outSlot.etaValid = true;
    } else {
        outSlot.timeToArrivalMin = 0.0;
        outSlot.etaValid = false;
    }

    // Marcación Relativa
    outSlot.marcacionRelativa = std::fmod((outSlot.azimutVerdadero - ownCourse + 360.0), 360.0);
}

bool CanalCalculator::isWithinAlarmWindow(double relativeBearing) {
    return (relativeBearing >= 85.0 && relativeBearing <= 95.0) ||
           (relativeBearing >= 265.0 && relativeBearing <= 275.0);
}

bool CanalCalculator::isBehind(double relativeBearing) {
    // Retorna true si la boya superó el través
    return (relativeBearing > 95.0 && relativeBearing < 265.0);
}