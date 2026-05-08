#include "cpa.h"
#include "pppcalculator.h"

CPA::CPA(QObject *parent)
    : QObject{parent}
{}

CPAResult CPA::computeCPA(const Track& a, const Track& b)
{
    PppCalculator::KinematicState aState;
    aState.xDm = a.getX();
    aState.yDm = a.getY();
    aState.speedDmPerHour = a.getVelocidadDmPerHour();
    aState.courseDeg = a.course();

    PppCalculator::KinematicState bState;
    bState.xDm = b.getX();
    bState.yDm = b.getY();
    bState.speedDmPerHour = b.getVelocidadDmPerHour();
    bState.courseDeg = b.course();

    const PppCalculator::Result result = PppCalculator::compute(aState, bState);

    if (result.status != PppCalculator::Result::Valid) {
        return {result.timeHours * 3600.0, result.distanceDm, false};
    }

    return {result.timeHours * 3600.0, result.distanceDm, true};
}

CPAResult CPA::fromCLI(int idTrack1, int idTrack2, CommandContext &ctx)
{
    const Track *track1 = ctx.findTrackById(idTrack1);
    const Track *track2 = ctx.findTrackById(idTrack2);

    if (!track1 || !track2) {
        return {0,0,false};
    }

    return computeCPA(*track1,*track2);
}
