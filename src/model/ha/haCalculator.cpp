#include "haCalculator.h"
#include "model/utils/RadarMath.h"
#include "model/estacionamientocalculator.h"
#include <cmath>

void HaCalculator::calculate(
    QPointF         ownPos,
    double          ownCourseDeg,
    double          ownSpeedDm,
    QPointF         fallPoint,
    HaSessionState& outState)
{
    outState.haIconCenter = fallPoint;

    //Azimut verdadero BP → punto de caída
    outState.trueAzimuthDeg = RadarMath::normalizeAngle360(
        RadarMath::calculateAngle(ownPos, fallPoint)
        );

    double distDm = RadarMath::calculateLength(ownPos, fallPoint);
    outState.distanceYards = RadarMath::dmToYards(distDm);

    double rel = RadarMath::normalizeAngle360(outState.trueAzimuthDeg - ownCourseDeg);
    if (rel > 180.0) rel = 360.0 - rel;
    outState.relativeBearingDeg = rel;

    outState.banda = computeBanda(
        RadarMath::normalizeAngle360(outState.trueAzimuthDeg - ownCourseDeg)
        );

    //Tiempo de arribo
    EstacionamientoCalculator::KinematicState stateA;
    stateA.xDm            = ownPos.x();
    stateA.yDm            = ownPos.y();
    stateA.speedDmPerHour = ownSpeedDm;
    stateA.courseDeg      = ownCourseDeg;
    stateA.valid          = (ownSpeedDm > 0.0);

    EstacionamientoCalculator::KinematicState stateB;
    stateB.xDm            = fallPoint.x();
    stateB.yDm            = fallPoint.y();
    stateB.speedDmPerHour = 0.0;
    stateB.courseDeg      = 0.0;
    stateB.valid          = true;

    EstacionamientoCalculator::Input input;
    input.trackA        = stateA;
    input.trackB        = stateB;
    input.azRelativeDeg = 0.0;
    input.distanceDm    = 0.0;
    input.useSpeedMode  = true;
    input.vdDmPerHour   = ownSpeedDm;

    if (ownSpeedDm > 0.0 && distDm > 0.0) {
        const EstacionamientoCalculator::Result result =
            EstacionamientoCalculator::compute(input);

        if (result.status == EstacionamientoCalculator::Result::Valid) {
            outState.timeToArrivalMin = result.timeHours * 60.0;
            outState.etaValid = true;
        } else {
            outState.timeToArrivalMin = 0.0;
            outState.etaValid = false;
        }
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