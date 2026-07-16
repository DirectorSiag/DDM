#pragma once

#include "commandContext.h"

class ObmService;

struct HaOperationResult {
    bool    ok = false;
    QString message;
};

class HaService {
public:
    explicit HaService(CommandContext* ctx, ObmService* obmService);

    HaOperationResult startSessionAtOwnShip();
    HaOperationResult startSessionAtCursor();
    HaOperationResult startSessionAtLatLonDms(
        int latDeg, int latMin, double latSec,
        int lonDeg, int lonMin, double lonSec
        );
    HaOperationResult startSessionAtBearing(double azimuthDeg, double distanceYards);

    HaOperationResult stopSession(int slotIndex = -1);  // -1 = todos
    HaOperationResult infoReport(int slotIndex) const;
    HaOperationResult selectSlot(int slotIndex);

    void update();

private:
    void initSession(double xDm, double yDm);
    HaOperationResult startSessionAtLatLon(double lat, double lon);

    // Publica/borra en el radar (via GeometryService) el circulo del punto de
    // caida. Figura estatica: se crea una vez al iniciar la sesion y se borra
    // al finalizarla, sin reposicionamiento por tick.
    void createFigure(HaSessionState& session);
    void deleteFigure(HaSessionState& session);

    int nextFreeSlot() const;

    CommandContext* m_ctx;
    ObmService* m_obmService;
};