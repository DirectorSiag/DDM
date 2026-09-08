#include "futuraCalculator.h"
#include "RadarMath.h"
#include <cmath>

void FuturaCalculator::calculateProjection(
    QPointF trackPos,
    double trackCourseDeg,
    double trackSpeedKn,
    int timeMinutes,
    QList<QPointF>& out_crossPoints,
    double& out_totalDistanceDm)
{
    // Distancia total
    const double timeHours = timeMinutes / 60.0;
    const double totalDistanceNm = trackSpeedKn * timeHours;

    // Conversion NM -> DM
    out_totalDistanceDm = totalDistanceNm * kNmToDm;

    // 10 puntos equidistantes sobre el rumbo del track
    out_crossPoints.clear();
    const double rad = trackCourseDeg * (M_PI / 180.0);
    for (int i = 1; i <= 10; ++i) {
        const double distDm = out_totalDistanceDm * (static_cast<double>(i) / 10.0);
        out_crossPoints.append(QPointF(
            trackPos.x() + distDm * std::sin(rad),
            trackPos.y() + distDm * std::cos(rad)
            ));
    }
}

bool FuturaCalculator::evaluateAlarm(
    double baseCourseDeg,
    double baseSpeedKn,
    double currentCourseDeg,
    double currentSpeedKn,
    double thresholdDeg,
    double thresholdKn)
{
    // Diferencia angular mas corta, normalizada a (-180, 180]
    double courseDelta = std::fmod(currentCourseDeg - baseCourseDeg, 360.0);
    if (courseDelta < -180.0) courseDelta += 360.0;
    else if (courseDelta > 180.0) courseDelta -= 360.0;

    const bool courseExceeded = std::fabs(courseDelta) > thresholdDeg;
    const bool speedExceeded  = std::fabs(currentSpeedKn - baseSpeedKn) > thresholdKn;

    return courseExceeded || speedExceeded;
}