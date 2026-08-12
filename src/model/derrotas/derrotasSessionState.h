#pragma once
#include <QList>
#include <QPointF>
#include <QString>
#include <QDateTime>

struct DerrotasFuturaConfig {
    int    trackId      = 0;
    int    timeMinutes  = 0;
    double thresholdDeg = 0.0;
    double thresholdKn  = 0.0;
};

struct DerrotasWayPoint {
    QPointF position;
    bool    isEndpoint = false;
    QString dateTimeLabel;
    double  rvDeg = 0.0;
    double  vdKn  = 0.0;
};

// Derrotas futuras: proyección futura + alarma. Ciclo de vida propio (INICIAR/FINALIZAR/BORRAR).
struct DerrotasFuturaState {
    bool                  active = false;
    DerrotasFuturaConfig  config;

    // I2: foto cinematica tomada al INICIAR
    double baseCourseDeg  = 0.0;
    double baseSpeedKn    = 0.0;
    bool   alarmTriggered = false;

    QList<QPointF> crossPoints;
    double         totalDistanceDm = 0.0;

    void reset() { *this = DerrotasFuturaState{}; }
};

// Derrotas pasadas: grabación/carga de derrotas pasadas. Ciclo de vida propio (INICIAR/FINALIZAR/CARGAR).
struct DerrotasPasadaState {
    bool      recordingActive  = false;
    int       recordingTrackId = 0;
    QDateTime recordingStartTime;
    QString   currentLogFileName;
    int       currentSegmentIndex = 0;

    QString                 loadedLogFileName;
    QList<QPointF>          loadedTrackPoints;
    QList<DerrotasWayPoint> loadedWayPoints;

    void reset() { *this = DerrotasPasadaState{}; }
};

struct DerrotasSessionState {
    DerrotasFuturaState futura;
    DerrotasPasadaState pasada;

    void reset() { futura.reset(); pasada.reset(); }
};