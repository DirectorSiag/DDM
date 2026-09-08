#include "textCalculator.h"
#include "model/utils/RadarMath.h"
#include <QtMath>
#include <cmath>

QPointF TextCalculator::resolveFromBearing(
    QPointF referencePos,
    double  azimuthDeg,
    double  distanceDm,
    bool    useVerdadero,
    double  ownCourseDeg)
{
    // Si es Relativo (R), el azimut se mide desde la proa del BP —
    // se le suma el rumbo actual para obtener el azimut verdadero
    // antes de proyectar.
    double effectiveAzimuth = azimuthDeg;
    if (!useVerdadero) {
        effectiveAzimuth = RadarMath::normalizeAngle360(azimuthDeg + ownCourseDeg);
    }

    const double azRad = qDegreesToRadians(effectiveAzimuth);
    const double x = referencePos.x() + distanceDm * std::sin(azRad);
    const double y = referencePos.y() + distanceDm * std::cos(azRad);

    return QPointF(x, y);
}

QPointF TextCalculator::resolveFromLatLon(
    double originLat, double originLon,
    double targetLat, double targetLon)
{
    double xDm = 0.0, yDm = 0.0;
    RadarMath::latLonToDm(originLat, originLon, targetLat, targetLon, xDm, yDm);
    return QPointF(xDm, yDm);
}

QPointF TextCalculator::resolveAssociatedPosition(
    QPointF trackPos,
    double  offsetXDm,
    double  offsetYDm)
{
    return QPointF(trackPos.x() + offsetXDm, trackPos.y() + offsetYDm);
}
