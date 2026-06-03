#include "twoWCalculator.h"
#include "2w/twoWStationTable.h"
#include "RadarMath.h"
#include <cmath>

void TwoWCalculator::calculate(
    QPointF guidePos,
    QPointF ownPos, double ownSpeed,
    int bpStation, double circleRadiusNm, const QList<int>& selectedStations,
    QPointF& out_guideCenter, QPointF& out_ownCenter, QList<QPointF>& out_allyCenters,
    bool& out_etaValid, double& out_courseDeg, double& out_etaMin,
    double& out_currentAzDeg, double& out_currentDistNm,
    double& out_expectedAzDeg, double& out_expectedDistNm)
{
    // FACTOR DE CONVERSIÓN TÁCTICO: de Millas Náuticas (NM) a Data Miles (DM)
    // 1 NM = 6076.1154 pies / 1 DM = 6000 pies -> Relación exacta: 1.012685
    const double kNmToDm = 1.012685;

    // 1. Centro del Guía
    out_guideCenter = guidePos;

    // 2. Datos de Tabla A para la estación del BP
    const TwoWStationEntry& stationData = TwoWStationTable::stationAt(bpStation);
    out_expectedAzDeg  = RadarMath::normalizeAngle360(stationData.azimuthDeg);
    out_expectedDistNm = stationData.distanceNm;

    // 3. Proyectar centro de la estación del BP
    double expectedDistDm = out_expectedDistNm * circleRadiusNm * kNmToDm;
    double rad = out_expectedAzDeg * (M_PI / 180.0);
    out_ownCenter = QPointF(
        guidePos.x() + expectedDistDm * std::sin(rad),
        guidePos.y() + expectedDistDm * std::cos(rad)
        );

    // 4. Azimut y distancia ACTUAL BP → Guía
    double distRealDm = RadarMath::calculateLength(guidePos, ownPos);
    out_currentDistNm = distRealDm / kNmToDm;
    out_currentAzDeg  = RadarMath::normalizeAngle360(RadarMath::calculateAngle(guidePos, ownPos));

    // 5. Rumbo y ETA hacia la estación
    double dxEstacion     = out_ownCenter.x() - ownPos.x();
    double dyEstacion     = out_ownCenter.y() - ownPos.y();
    double distEstacionDm = std::sqrt(dxEstacion*dxEstacion + dyEstacion*dyEstacion);

    out_courseDeg = RadarMath::normalizeAngle360(RadarMath::calculateAngle(ownPos, out_ownCenter));

    if (ownSpeed > 0.0 && distEstacionDm > 0.0) {
        double tiempoHoras = distEstacionDm / ownSpeed;
        out_etaMin = tiempoHoras * 60.0;
        out_etaValid = true;
    } else {
        out_etaMin = 0.0;
        out_etaValid = false;
    }

    // 6. Estaciones aliadas
    out_allyCenters.clear();
    for (int estId : selectedStations) {
        const TwoWStationEntry& ally = TwoWStationTable::stationAt(estId);
        double allyDistDm = ally.distanceNm * circleRadiusNm * kNmToDm;
        double allyRad    = ally.azimuthDeg * (M_PI / 180.0);
        out_allyCenters.append(QPointF(
            guidePos.x() + allyDistDm * std::sin(allyRad),
            guidePos.y() + allyDistDm * std::cos(allyRad)
            ));
    }
}