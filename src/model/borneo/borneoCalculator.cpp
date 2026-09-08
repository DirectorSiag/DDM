#include "borneoCalculator.h"
#include "borneoSessionState.h"

double BorneoCalculator::calculateRadius(const BorneoConfig& config)
{
    const double grilletesEnMetros = config.grilletes * BorneoConstants::GRILLETE_LENGTH_MTS;
    return config.eslora + grilletesEnMetros + BorneoConstants::MARGEN_SEGURIDAD_MTS - config.profundidad;
}