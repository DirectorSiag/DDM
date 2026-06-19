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


    HaOperationResult startSessionAtOwnShip(); // Disp 1: a popa de BP
    HaOperationResult startSessionAtCursor(double cursorXDm, double cursorYDm); // Disp 2: sobre cursor
    HaOperationResult startSessionAtLatLon(double lat, double lon); // Disp 3: por lat y long
    HaOperationResult startSessionAtBearing(double azimuthDeg, double distanceYards); // Disp 4: por azimut verdadero y dist en yardas desde el BP

    HaOperationResult stopSession();

    HaOperationResult infoReport() const;

    void update();

private:
    void initSession(double xDm, double yDm);

    CommandContext* m_ctx;
    HaSessionTimer  m_timer;
};