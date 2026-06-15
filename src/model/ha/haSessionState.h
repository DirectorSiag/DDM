#pragma once

#include <QString>
#include <QPointF>
#include <QDateTime>

// Escritores:
//   - HaService::startSession() → fija el punto de caída y la hora
//   - HaService::update()       → recalcula asesoramiento cada 80ms
//   - HaService::stopSession()  → reset completo

// Lectores:
//   - lpdEncoder           → lee haIconCenter para dibujar en radar
//   - JsonResponseBuilder  → lee asesoramiento para la botonera

struct HaSessionState {

    // escrito por HaService::startSession()
    bool    active          = false;   // true mientras la emergencia está activa
    double  fallPointX      = 0.0;    // Coordenada X del punto de caída [DM]
    double  fallPointY      = 0.0;    // Coordenada Y del punto de caída [DM]

    // escrito por HaService::update() cada 80ms
    double  trueAzimuthDeg      = 0.0;  // Azimut verdadero al punto [°]
    double  relativeBearingDeg  = 0.0;  // Marcación relativa [0–180°]
    QString banda               = QStringLiteral("-");  // "BABOR", "ESTRIBOR", ("PROA", "POPA" también por el momento)
    double  distanceYards       = 0.0;  // Distancia al punto [yardas]
    bool    etaValid            = false;
    double  timeToArrivalMin    = 0.0;  // Tiempo de arribo [minutos]

    // formato tiempo
    QString fallTimeLocal   = QStringLiteral("--:--:--");  // Hora de caída local (congelada)
    QString fallTimeUtc     = QStringLiteral("--:--:--");  // Hora de caída UTC (congelada)
    QString elapsedTime     = QStringLiteral("00:00:00");  // Cronómetro HH:MM:SS

    // Para radar
    QPointF haIconCenter;  // Posición del ícono HA en coordenadas DM
    //
    void reset() { *this = HaSessionState{}; }
};