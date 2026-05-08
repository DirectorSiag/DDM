#include "estacionamientocalculator.h"

#include "utils/RadarMath.h"

#include <QtMath>

#include <algorithm>
#include <cmath>

namespace {
constexpr double kEpsilon = 1e-9;

bool isFiniteKinematicState(const EstacionamientoCalculator::KinematicState& state)
{
    return std::isfinite(state.xDm)
        && std::isfinite(state.yDm)
        && std::isfinite(state.speedDmPerHour)
        && std::isfinite(state.courseDeg)
        && state.speedDmPerHour >= 0.0;
}

void velocityFromCourseAndSpeed(const EstacionamientoCalculator::KinematicState& state,
                                double& vx,
                                double& vy)
{
    const double courseRad = qDegreesToRadians(state.courseDeg);
    vx = state.speedDmPerHour * std::sin(courseRad);
    vy = state.speedDmPerHour * std::cos(courseRad);
}

bool isPositiveTime(double timeHours)
{
    return std::isfinite(timeHours) && timeHours > kEpsilon;
}
}

EstacionamientoCalculator::Result EstacionamientoCalculator::compute(const Input& input)
{
    Result out;

    if (!isFiniteKinematicState(input.trackA)
        || !isFiniteKinematicState(input.trackB)
        || !input.trackA.valid
        || !input.trackB.valid
        || !std::isfinite(input.azRelativeDeg)
        || !std::isfinite(input.distanceDm)
        || input.distanceDm <= 0.0) {
        out.status = Result::InvalidInput;
        out.reason = QStringLiteral("invalid_input");
        return out;
    }

    double bVx = 0.0;
    double bVy = 0.0;
    velocityFromCourseAndSpeed(input.trackB, bVx, bVy);

    const double stationAzAbsDeg = RadarMath::normalizeAngle360(input.trackB.courseDeg + input.azRelativeDeg);
    const double stationAzAbsRad = qDegreesToRadians(stationAzAbsDeg);
    const double offX = input.distanceDm * std::sin(stationAzAbsRad);
    const double offY = input.distanceDm * std::cos(stationAzAbsRad);

    const double rX = (input.trackB.xDm + offX) - input.trackA.xDm;
    const double rY = (input.trackB.yDm + offY) - input.trackA.yDm;

    double chosenTimeHours = 0.0;

    if (input.useSpeedMode) {
        if (!std::isfinite(input.vdDmPerHour) || input.vdDmPerHour <= 0.0) {
            out.status = Result::InvalidInput;
            out.reason = QStringLiteral("invalid_vd");
            return out;
        }

        const double vd2 = input.vdDmPerHour * input.vdDmPerHour;
        const double vb2 = (bVx * bVx) + (bVy * bVy);
        const double rv = (rX * bVx) + (rY * bVy);
        const double r2 = (rX * rX) + (rY * rY);

        const double a = vb2 - vd2;
        const double b = 2.0 * rv;
        const double c = r2;

        double t1 = -1.0;
        double t2 = -1.0;

        if (std::abs(a) < kEpsilon) {
            if (std::abs(b) < kEpsilon) {
                out.status = Result::NoPositiveTimeSolution;
                out.reason = QStringLiteral("no_time_solution");
                return out;
            }
            t1 = -c / b;
        } else {
            const double discriminant = (b * b) - (4.0 * a * c);
            if (!std::isfinite(discriminant) || discriminant < 0.0) {
                out.status = Result::NoPositiveTimeSolution;
                out.reason = QStringLiteral("no_real_solution");
                return out;
            }

            const double sqrtDisc = std::sqrt(std::max(0.0, discriminant));
            t1 = (-b - sqrtDisc) / (2.0 * a);
            t2 = (-b + sqrtDisc) / (2.0 * a);
        }

        const bool validT1 = isPositiveTime(t1);
        const bool validT2 = isPositiveTime(t2);
        if (!validT1 && !validT2) {
            out.status = Result::NoPositiveTimeSolution;
            out.reason = QStringLiteral("no_positive_time_solution");
            return out;
        }

        if (validT1 && validT2) {
            chosenTimeHours = std::min(t1, t2);
        } else {
            chosenTimeHours = validT1 ? t1 : t2;
        }
    } else {
        if (!std::isfinite(input.duHours) || input.duHours <= 0.0) {
            out.status = Result::InvalidInput;
            out.reason = QStringLiteral("invalid_du");
            return out;
        }
        chosenTimeHours = input.duHours;
    }

    if (!isPositiveTime(chosenTimeHours)) {
        out.status = Result::NoPositiveTimeSolution;
        out.reason = QStringLiteral("invalid_time_solution");
        return out;
    }

    const double aVx = (rX + bVx * chosenTimeHours) / chosenTimeHours;
    const double aVy = (rY + bVy * chosenTimeHours) / chosenTimeHours;

    const double speed = std::sqrt((aVx * aVx) + (aVy * aVy));
    if (!std::isfinite(speed) || speed < kEpsilon) {
        out.status = Result::DegenerateSolution;
        out.reason = QStringLiteral("degenerate_velocity_solution");
        return out;
    }

    out.status = Result::Valid;
    out.reason = QStringLiteral("ok");
    out.rumboDeg = RadarMath::azimuthDeg(aVx, aVy);
    out.timeHours = chosenTimeHours;
    out.resultingSpeedDmPerHour = speed;
    return out;
}
