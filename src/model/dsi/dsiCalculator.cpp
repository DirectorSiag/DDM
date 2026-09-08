#include "dsiCalculator.h"
#include "model/utils/RadarMath.h"
#include <QtMath>
#include <cmath>

QPointF DSICalculator::resolveFromBearing(
    QPointF referencePos,
    double  azimuthDeg,
    double  distanceDm,
    bool    useVerdadero,
    double  ownCourseDeg)
{
    double effectiveAzimuth = azimuthDeg;
    if (!useVerdadero) {
        effectiveAzimuth = RadarMath::normalizeAngle360(azimuthDeg + ownCourseDeg);
    }

    const double azRad = qDegreesToRadians(effectiveAzimuth);
    const double x = referencePos.x() + distanceDm * std::sin(azRad);
    const double y = referencePos.y() + distanceDm * std::cos(azRad);

    return QPointF(x, y);
}

QPointF DSICalculator::resolveFromLatLon(
    double originLat, double originLon,
    double targetLat, double targetLon)
{
    double xDm = 0.0, yDm = 0.0;
    RadarMath::latLonToDm(originLat, originLon, targetLat, targetLon, xDm, yDm);
    return QPointF(xDm, yDm);
}

QPointF DSICalculator::resolveAssociatedPosition(QPointF trackPos)
{
    // Adopción directa — sin offset. Ver REQ-DSI-UI-017.
    return trackPos;
}

bool DSICalculator::isTrackInsideZone(
    QPointF zoneCenter,
    double  radiusDm,
    QPointF trackPos)
{
    const double dx = trackPos.x() - zoneCenter.x();
    const double dy = trackPos.y() - zoneCenter.y();
    const double distDm = std::sqrt(dx * dx + dy * dy);
    return distDm <= radiusDm;
}
