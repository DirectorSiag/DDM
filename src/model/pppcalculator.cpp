#include "pppcalculator.h"

#include "utils/RadarMath.h"

#include <QtMath>

#include <cmath>

namespace {
constexpr double kEpsilonSpeedDmPerHour = 1e-9;

bool isFiniteState(const PppCalculator::KinematicState& s)
{
    return std::isfinite(s.xDm)
        && std::isfinite(s.yDm)
        && std::isfinite(s.speedDmPerHour)
        && std::isfinite(s.courseDeg)
        && s.speedDmPerHour >= 0.0;
}

void velocityFromCourseAndSpeed(const PppCalculator::KinematicState& s,
                                double& vx,
                                double& vy)
{
    const double courseRad = qDegreesToRadians(s.courseDeg);
    vx = s.speedDmPerHour * std::sin(courseRad);
    vy = s.speedDmPerHour * std::cos(courseRad);
}
}

PppCalculator::Result PppCalculator::compute(const KinematicState& reference,
                                             const KinematicState& target)
{
    Result out;

    if (!reference.valid || !target.valid || !isFiniteState(reference) || !isFiniteState(target)) {
        out.status = Result::InvalidInput;
        out.reason = QStringLiteral("invalid_kinematic_input");
        return out;
    }

    const double r0x = target.xDm - reference.xDm;
    const double r0y = target.yDm - reference.yDm;

    double refVx = 0.0;
    double refVy = 0.0;
    double tgtVx = 0.0;
    double tgtVy = 0.0;
    velocityFromCourseAndSpeed(reference, refVx, refVy);
    velocityFromCourseAndSpeed(target, tgtVx, tgtVy);

    const double vrx = tgtVx - refVx;
    const double vry = tgtVy - refVy;
    const double vr2 = vrx * vrx + vry * vry;

    if (!std::isfinite(vr2) || vr2 < kEpsilonSpeedDmPerHour) {
        out.azDeg = RadarMath::azimuthDeg(r0x, r0y);
        out.distanceDm = RadarMath::distanceDm(r0x, r0y);
        out.timeHours = 0.0;
        out.status = Result::DegenerateRelativeMotion;
        out.reason = QStringLiteral("degenerate_relative_motion");
        return out;
    }

    double tcpaHours = -((r0x * vrx) + (r0y * vry)) / vr2;
    if (!std::isfinite(tcpaHours) || tcpaHours < 0.0) {
        tcpaHours = 0.0;
    }

    const double cpaX = r0x + vrx * tcpaHours;
    const double cpaY = r0y + vry * tcpaHours;

    out.azDeg = RadarMath::azimuthDeg(cpaX, cpaY);
    out.distanceDm = RadarMath::distanceDm(cpaX, cpaY);
    out.timeHours = tcpaHours;
    out.status = Result::Valid;
    out.reason = QStringLiteral("ok");
    return out;
}
