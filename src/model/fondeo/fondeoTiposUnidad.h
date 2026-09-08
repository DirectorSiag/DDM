#pragma once

#include "model/enums/enums.h"
#include <QVector>

// Radios de marcha (r1..r5, en yardas, origen en el PF) para la maniobra de
// Fondeo: AD. TODA, AD. MEDIA, AD. DESP., PARA MAQ, MAQ. AT respectivamente.
struct RadiosFondeo {
    double r1 = 0.0;
    double r2 = 0.0;
    double r3 = 0.0;
    double r4 = 0.0;
    double r5 = 0.0;
};

class FondeoTiposUnidad {
public:
    // TODO: MEKO 140 y PATAGONIA están en 0 — faltan los valores reales de
    // doctrina. MEKO 360 confirmado: AD.TODA 1500, AD.MEDIA 1000,
    // AD.DESP. 800, PARA MAQ 100, MAQ.AT 50.
    static inline RadiosFondeo radiosPara(FondeoData::TipoUnidad tipo) {
        switch (tipo) {
        case FondeoData::Meko360:
            return { 1500.0, 1000.0, 800.0, 100.0, 50.0 };
        case FondeoData::Meko140:
            return { 0.0, 0.0, 0.0, 0.0, 0.0 };
        case FondeoData::Patagonia:
            return { 0.0, 0.0, 0.0, 0.0, 0.0 };
        case FondeoData::Otro:
        default:
            return { 0.0, 0.0, 0.0, 0.0, 0.0 };
        }
    }

    static inline QVector<FondeoData::TipoUnidad> todosLosTipos() {
        return { FondeoData::Meko360, FondeoData::Meko140, FondeoData::Patagonia, FondeoData::Otro };
    }
};
