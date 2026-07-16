#pragma once
#include "commandContext.h"

struct TwoWOperationResult {
    bool    ok = false;
    QString message;
};

class TwoWService {
public:
    explicit TwoWService(CommandContext* ctx);
    TwoWOperationResult startSession(int guideTrackId, int bpStation, double circleRadiusNm, const QList<int>& aliadas = {});
    TwoWOperationResult stopSession();

    // Reemplaza la lista completa de estaciones aliadas graficadas, sin tocar
    // Guia/Propio. Requiere sesion activa (ver startSession). Puede llamarse
    // en cualquier momento para cambiar que se ve, incluyendo lista vacia
    // para borrar todas las aliadas.
    TwoWOperationResult setStations(const QList<int>& aliadas);

    void update();
private:
    CommandContext* m_ctx;

    // Valida que cada estacion este en [1,68] y presente en la Tabla A.
    // Reutilizado por startSession() y setStations().
    static bool validateStationList(const QList<int>& stations, QString& errorMessage);

    // Publica en el radar (via GeometryService) los centros ya calculados por
    // TwoWCalculator::calculate(). No recalcula ninguna posicion por su cuenta.
    void syncFigures();
    void deleteAllFigures();
};