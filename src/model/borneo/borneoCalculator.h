#pragma once

struct BorneoConfig;

// Motor de cálculo puro para el radio del Círculo de Borneo.
class BorneoCalculator
{
public:
    // CB = Eslora + (Grilletes * 27.43) + MargenSeguridad(10) - Profundidad
    static double calculateRadius(const BorneoConfig& config);
};