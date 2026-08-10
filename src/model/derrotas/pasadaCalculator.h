#pragma once
#include <QList>
#include <QPointF>
#include <QDateTime>
#include <QString>

struct DerrotasLogPoint {
    QString   trackName;
    QDateTime timestamp;
    double    latDeg;
    double    lonDeg;
    double    rvDeg;
    double    vdKn;
};

struct DerrotasWayPoint {
    QPointF position;     // Coordenadas en DM, relativas al origen
    bool    isEndpoint;   // true = primer o ultimo punto de la derrota
    QString dateTimeLabel; // solo valido si isEndpoint == true
    double  rvDeg;
    double  vdKn;
};

class PasadaCalculator {
public:
    // convierte la secuencia completa del log a coordenadas DM relativas al origen (ownLatDeg/ownLonDeg), para trazar la estela continua.
    static void buildTrackPoints(
        const QList<DerrotasLogPoint>& logPoints,
        double originLatDeg,
        double originLonDeg,
        QList<QPointF>& out_trackPoints
        );

    // recorre el log y genera un Way Point cada vez que cambia el rumbo (RV) respecto al punto anterior, mas los
    // dos extremos (inicio/fin) que siempre son Way Point con fecha/hora.
    static void buildWayPoints(
        const QList<DerrotasLogPoint>& logPoints,
        double originLatDeg,
        double originLonDeg,
        QList<DerrotasWayPoint>& out_wayPoints
        );
};