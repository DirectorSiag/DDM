#pragma once

#include "commandContext.h"
#include "model/ha/haSessionTimer.h"

struct HaOperationResult {
    bool    ok = false;
    QString message;
};

class HaService {
public:
    explicit HaService(CommandContext* ctx);

    // Disparador 1: a popa de BP
    HaOperationResult startSessionAtOwnShip();
     // Disparador 2: sobre cursor
    HaOperationResult startSessionAtCursor(double cursorXDm, double cursorYDm);
    // Disparador 3: por latitud y longitud en formato GMS
    HaOperationResult startSessionAtLatLonDms(
        int latDeg, int latMin, double latSec,
        int lonDeg, int lonMin, double lonSec
    );
    // Disparador 4: por azimut verdadero y dist en yardas desde el BP
    HaOperationResult startSessionAtBearing(double azimuthDeg, double distanceYards);

    HaOperationResult stopSession();

    void update();

private:
    void initSession(double xDm, double yDm);

    // Reutilizado por startSessionAtLatLonDms() una vez que ya convirtió GMS a decimal.
    HaOperationResult startSessionAtLatLon(double lat, double lon);

    CommandContext* m_ctx;
    HaSessionTimer  m_timer;
};