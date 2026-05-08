#pragma once

class RadarMath {
public:
    // Distancia desde (0,0) hasta (x,y) en Data Miles (DM)
    static double distanceDm(double xDm, double yDm);

    // Azimut desde (0,0) hacia (x,y)
    // 0° = Norte (+Y), 90° = Este (+X), sentido horario
    static double azimuthDeg(double xDm, double yDm);

private:
    // Helper interno
    static double normalizeDeg360(double deg);
};
