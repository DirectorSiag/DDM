#pragma once

#include "entities/track.h"

class CommandContext;

// Integracion de aplicacion: calcula y persiste PPP en Track para consumo de SITREP.
// La matematica se resuelve en PppCalculator y es reutilizable para otros casos.
class TrackPppService
{
public:
    explicit TrackPppService(CommandContext* context);

    Track::SitrepPppData computeTrackAgainstOwnShip(const Track& track) const;
    void recalculateAllTracksAgainstOwnShip() const;
    void recalculateTrackAgainstOwnShip(Track& track) const;

private:
    Track::SitrepPppData buildNoOwnShipResult() const;

    CommandContext* m_context;
};
