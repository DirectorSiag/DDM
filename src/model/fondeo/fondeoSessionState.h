#pragma once

#include <QPointF>
#include <QString>

struct FondeoConfig {
    bool useTrack = false;
    bool useGms = false;

    // --- VARIABLES MODO TRACK ---
    int trackId = 0;
    double trackAz = 0.0; // Grados - Azimut del track respecto al BP
    double trackDt = 0.0; // MN - Distancia del track

    // --- VARIABLES MODO GMS ---
    int pfLatDeg = 0;
    int pfLatMin = 0;
    double pfLatSec = 0.0;

    int pfLonDeg = 0;
    int pfLonMin = 0;
    double pfLonSec = 0.0;

    // --- PUNTO AUXILIAR Y RADIOS ---
    double paAz = 0.0; // Grados - Azimut verdadero
    double paDt = 0.0; // MN - Distancia del PA al PF
    double r1 = 0.0; // Yardas - Origen en el PF
    double r2 = 0.0; // Yardas - Origen en el PF
    double r3 = 0.0; // Yardas - Origen en el PF
    double r4 = 0.0; // Yardas - Origen en el PF
    double r5 = 0.0; // Yardas - Origen en el PF
};

struct FondeoSessionState {
    // ---- Datos estáticos    
    FondeoConfig config;
    QPointF puntoFondeo;
    QPointF puntoAuxiliar;

    // ---- Datos dinámicos
    bool active = false;
    bool paAlcanzado = false; // Controla la transición PA -> PF

    double distanciaPF = 0.0; // Yardas - Distancia del BP al PF
    double azimutPF = 0.0; // Grados - Azimut verdadero del PF
    double distanciaPA = 0.0; // Yardas - Distancia del BP al PA
    double azimutPA = 0.0; // Grados - Azimut verdadero del PA
    double azimutRelativo = 0.0; // Azimut relativo al al PA o PF (Dependiendo al que esté yendo)
    double distanciaRelativa = 0.0; // Distancia del BP al PA o PF (Dependiendo al que esté yendo)

    QString movimientoActual = "Adelante Toda"; // TODO: Ver como arranca
    QString proximoMovimiento = "Adelante Media"; // TODO

    void reset() { *this = FondeoSessionState{}; }
};