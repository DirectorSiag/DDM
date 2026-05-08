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
