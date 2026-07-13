#pragma once
#include <QPointF>
#include <QtMath>
#include <qmath.h>
#include <QFloat16>

class RadarMath {
public:
    // Distancia desde (0,0) hasta (x,y) en Data Miles (DM)
    static double distanceDm(double xDm, double yDm);

    // Azimut desde (0,0) hacia (x,y)
    // 0° = Norte (+Y), 90° = Este (+X), sentido horario
    static double azimuthDeg(double xDm, double yDm);

    // Nuevos métodos utilitarios para geometria 2D
    static qfloat16 calculateAngle(const QPointF& start, const QPointF& end);
    static qfloat16 calculateLength(const QPointF& start, const QPointF& end);

    // Normaliza angulos al rango [0, 360).
    static double normalizeAngle360(double deg);

    // Conversión de unidades
    static constexpr double kYardsPerDm = 2000.0;
    static double dmToYards(double dm) { return dm * kYardsPerDm; }
    static double yardsToDm(double yards) { return yards / kYardsPerDm; }

    // Convierte diferencia lat/lon a coordenadas DM relativas al origen (BP).
    static void latLonToDm(
        double originLat, double originLon,
        double targetLat, double targetLon,
        double& outXDm,   double& outYDm
        );
    // Convierte coordenada en formato GMS a grados decimales.
    static double dmsToDecimal(int degrees, int minutes, double seconds);
private:
    // Helper interno
    static double normalizeDeg360(double deg);
};
