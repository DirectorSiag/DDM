#include "haCalculator.h"
#include "model/utils/RadarMath.h"
#include <cmath>

void HaCalculator::calculate(
    QPointF         ownPos,
    double          ownCourseDeg,
    double          ownSpeedDm,
    QPointF         fallPoint,
    HaSessionState& outState)
{
    outState.haIconCenter = fallPoint;

    // Azimut verdadero BP → punto de caída (cálculo directo, sin ajuste de pantalla)
    double dx = fallPoint.x() - ownPos.x();
    double dy = fallPoint.y() - ownPos.y();
    double distDm = std::sqrt(dx * dx + dy * dy);
    outState.trueAzimuthDeg = RadarMath::normalizeAngle360(
        std::atan2(dx, dy) * (180.0 / M_PI)
        );

    // Distancia en yardas
    outState.distanceYards = RadarMath::dmToYards(distDm);

    // Marcación relativa [0–180°]
    double rel = RadarMath::normalizeAngle360(outState.trueAzimuthDeg - ownCourseDeg);
    if (rel > 180.0) rel = 360.0 - rel;
    outState.relativeBearingDeg = rel;

    // Banda
    outState.banda = computeBanda(
        RadarMath::normalizeAngle360(outState.trueAzimuthDeg - ownCourseDeg)
        );

    // Tiempo de arribo — cálculo directo (destino estático sobre el punto)
    if (ownSpeedDm > 0.0 && distDm > 0.0) {
        outState.timeToArrivalMin = (distDm / ownSpeedDm) * 60.0;
        outState.etaValid = true;
    } else {
        outState.timeToArrivalMin = 0.0;
        outState.etaValid = false;
    }
}

QString HaCalculator::computeBanda(double angleFromProw) {
    if (angleFromProw < 0.5 || angleFromProw > 359.5)
        return QStringLiteral("PROA");
    if (std::abs(angleFromProw - 180.0) < 0.5)
        return QStringLiteral("POPA");
    if (angleFromProw < 180.0)
        return QStringLiteral("ESTRIBOR");
    return QStringLiteral("BABOR");
}