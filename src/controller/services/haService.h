#pragma once

#include "commandContext.h"
#include "model/ha/haSessionTimer.h"

class HaService {
public:
    explicit HaService(CommandContext* ctx);

    void startSessionAtOwnShip();     // Disparador 1: a popa del Buque Propio

    void startSessionAtCursor(double cursorXDm, double cursorYDm); // Disparador 2: sobre el cursor (OBM)

    void startSessionAtLatLon(double lat, double lon); // Disparador 3: por latitud y longitud

    void startSessionAtBearing(double azimuthDeg, double distanceYards); // Disparador 4: por azimut verdadero y distancia en yardas desde el BP

    void stopSession();
    void update();

private:
    // Método que fija el punto y arranca la sesión una vez que la coordenada ya fue resuelta.
    void initSession(double xDm, double yDm);

    CommandContext* m_ctx;
    HaSessionTimer  m_timer;
};