#pragma once

#include <QString>

// Constantes fijas del cálculo de Círculo de Borneo en metros
namespace BorneoConstants {
constexpr double GRILLETE_LENGTH_MTS = 27.43;
constexpr double MARGEN_SEGURIDAD_MTS = 10.0;
}

// Configuración inmutable de la sesión, ingresada por el operador
struct BorneoConfig {
    double eslora = 0.0;       // ESLORA BP, en metros
    int grilletes = 0;         // GRILLETES AL AGUA
    double profundidad = 0.0;  // PROFUNDIDAD, en metros
};

// Estado dinámico de la sesión de Borneo
struct BorneoSessionState {
    bool active = false;
    BorneoConfig config;
    double radioCalculado = 0.0; // RADIO DE BORNEO, en metros

    void reset() {
        active = false;
        config = BorneoConfig();
        radioCalculado = 0.0;
    }
};

