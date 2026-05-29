#pragma once

#include <QList>
#include <QPointF>

struct TwoWSessionState {

    bool    active          = false;  // true mientras la disposición está activa
    int     guideTrackId    = -1;     // ID del track designado como Guía (-1 = sin asignar)
    int     bpStation       = -1;     // Estación asignada al Buque Propio (1–68; -1 = sin asignar)
    double  circleRadiusNm  = 1.0;   // Radio de los círculos en millas náuticas
    QList<int> selectedStations;      // Estaciones aliadas activas

    // Centros de círculos para el radar
    QPointF         guideCircleCenter;      // Círculo verde  (posición del Guía)
    QPointF         ownCircleCenter;        // Círculo azul   (estación del BP)
    QList<QPointF>  allyCircleCenters;      // Círculos ámbar (estaciones aliadas)

    // Asesoramiento cinemático para la Botonera
    bool    kinematicsValid       = false;  // false si velocidad = 0 o track inválido
    double  courseToStationDeg    = 0.0;   // Rumbo recomendado para alcanzar la estación [°]
    double  timeToStationMin      = 0.0;   // ETA a la estación [minutos]

    // Situación táctica: posición actual vs. posición esperada
    double  currentAzimuthDeg     = 0.0;   // Azimut real BP → Guía   [°]
    double  currentDistanceNm     = 0.0;   // Distancia real BP → Guía [MN]
    double  expectedAzimuthDeg    = 0.0;   // Azimut de la estación asignada [°]
    double  expectedDistanceNm    = 0.0;   // Distancia de la estación asignada [MN]

    void reset() { *this = TwoWSessionState{}; }

    static constexpr int NO_TRACK   = -1;
    static constexpr int NO_STATION = -1;
};