#include "trackpppservice.h"

#include "commandContext.h"
#include "model/pppcalculator.h"

TrackPppService::TrackPppService(CommandContext* context)
    : m_context(context)
{
    Q_ASSERT(m_context);
}

Track::SitrepPppData TrackPppService::buildNoOwnShipResult() const
{
    Track::SitrepPppData out;
    out.status = Track::SitrepPppData::NoOwnShip;
    out.reason = QStringLiteral("ownship_not_set");
    return out;
}

Track::SitrepPppData TrackPppService::computeTrackAgainstOwnShip(const Track& track) const
{
    if (!m_context->ownShip.valid) {
        return buildNoOwnShipResult();
    }

    const auto& own = m_context->ownShip;

    PppCalculator::KinematicState ownState;
    ownState.xDm = own.xDm;
    ownState.yDm = own.yDm;
    ownState.speedDmPerHour = own.speedKnots / Track::kDmToNm;
    ownState.courseDeg = own.courseDeg;

    PppCalculator::KinematicState trackState;
    trackState.xDm = track.getX();
    trackState.yDm = track.getY();
    trackState.speedDmPerHour = 0.0;
    trackState.courseDeg = track.course();

    // Etapa actual: los tracks se consideran estaticos para PPP de SITREP.
    // Por eso forzamos speed=0 y calculamos una sola vez al setear OwnShip.
    // Si en el futuro los tracks pasan a moverse, usar aqui la velocidad real
    // del track y disparar recalc desde updateTracks() o punto equivalente.
    const PppCalculator::Result calc = PppCalculator::compute(ownState, trackState);

    Track::SitrepPppData out;
    out.azDeg = calc.azDeg;
    out.distanceDm = calc.distanceDm;
    out.timeHours = calc.timeHours;
    out.reason = calc.reason;

    if (calc.status == PppCalculator::Result::Valid) {
        out.status = Track::SitrepPppData::Valid;
    } else if (calc.status == PppCalculator::Result::DegenerateRelativeMotion) {
        out.status = Track::SitrepPppData::DegenerateRelativeMotion;
    } else {
        out.status = Track::SitrepPppData::DegenerateRelativeMotion;
    }

    return out;
}

void TrackPppService::recalculateAllTracksAgainstOwnShip() const
{
    for (Track& track : m_context->tracks) {
        recalculateTrackAgainstOwnShip(track);
    }
}

void TrackPppService::recalculateTrackAgainstOwnShip(Track& track) const
{
    track.setSitrepPpp(computeTrackAgainstOwnShip(track));
}
