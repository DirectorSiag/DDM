#include "RadarMath.h"
#include <cmath>

double RadarMath::normalizeDeg360(double deg)
{
    double x = std::fmod(deg, 360.0);
    if (x < 0.0) x += 360.0;
    if (x >= 360.0) x -= 360.0;
    return x;
}

double RadarMath::distanceDm(double xDm, double yDm)
{
    return std::sqrt((xDm * xDm) + (yDm * yDm));
}

double RadarMath::azimuthDeg(double xDm, double yDm)
{
    if (xDm == 0.0 && yDm == 0.0)
        return 0.0;

    const double rad = std::atan2(xDm, yDm);
    const double deg = rad * (180.0 / M_PI);
    return normalizeDeg360(deg);
}

qfloat16 RadarMath::calculateAngle(const QPointF& start, const QPointF& end) {
    double dx = end.x() - start.x();
    double dy = end.y() - start.y();

    // En Qt, Y aumenta hacia abajo.
    // Para que 0° sea ARRIBA y el ángulo crezca HORARIO (CW):
    // Usamos atan2(dx, -dy).
    // El signo negativo en dy compensa el eje invertido de la pantalla.

    double rad = std::atan2(dx, -dy);
    double deg = rad * (180.0 / M_PI);

    // normalizeDeg360 asegura que el resultado esté entre 0 y 360
    return 180-static_cast<qfloat16>(normalizeDeg360(deg));
}

qfloat16 RadarMath::calculateLength(const QPointF& start, const QPointF& end) {
    return qSqrt(qPow(end.x() - start.x(), 2) + qPow(end.y() - start.y(), 2));
}

double RadarMath::normalizeAngle360(double deg)
{
    return normalizeDeg360(deg);
}
