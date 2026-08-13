#pragma once

#include "commandContext.h"
#include "model/dsi/dsiCalculator.h"
#include <QMap>
#include <QList>

struct DSIOperationResult {
    bool    ok = false;
    QString message;
};

// Parámetros de creación — agrupa las tres secciones (A, B, C) para
// pasarlas al Service en una sola llamada. Mismo esquema que TextCreateParams.
struct DSICreateParams {
    // Sección A
    QString      labelText;                          // default "DSI 50MN" si viene vacío
    double       radiusDm       = 50.0;               // default 50 MN
    DSILineStyle lineStyle      = DSILineStyle::Continua;
    bool         hasLabelColor      = false;
    DSIColor     labelColor         = DSIColor::Blanco;
    bool         hasLabelBackground = false;
    DSIColor     labelBackground    = DSIColor::Blanco;

    // Sección B — solo uno de estos métodos debe estar poblado
    DSIPositionMethod positionMethod = DSIPositionMethod::Manual;

    double manXDm = 0.0;
    double manYDm = 0.0;

    double azimuthDeg   = 0.0;
    double distanceDm   = 0.0;
    bool   useVerdadero = true;

    double latDecimal = 0.0;
    double lonDecimal = 0.0;

    // Sección C — opcional
    bool asociarAlCrear   = false;
    int  trackToAssociate = -1;
};

class DSIService {
public:
    explicit DSIService(CommandContext* ctx);

    DSIOperationResult createDSI(const DSICreateParams& params);
    DSIOperationResult editDSI(int tn, const QMap<QString, QString>& fields);
    DSIOperationResult deleteDSI(int tn);

    DSIOperationResult associateTrack(int dsiTn, int trackId);
    DSIOperationResult dissociateTrack(int dsiTn);

    DSIOperationResult listDSI() const;
    DSIOperationResult infoDSI(int tn) const;

    // Recorre todas las DSI cada 80ms (mismo timer que Texto/HA/2W):
    //  1) recalcula el centro de las DSI asociadas a un track (adopción directa)
    //  2) evalúa la alarma de proximidad para cada track contra cada DSI
    //     (dispara notificación de ingreso y gestiona el ciclo de 5s)
    void update();

    // ── Helpers de conversión string <-> enum ────────────────────────────
    static DSIColor     stringToColor(const QString& s, bool& ok);
    static DSILineStyle stringToLineStyle(const QString& s, bool& ok);

private:
    CommandContext* m_ctx;

    void evaluateProximityAlarm(DSIEntity& zone);

    static QString colorToString(DSIColor color);
    static QString lineStyleToString(DSILineStyle style);
    static QString positionMethodToString(DSIPositionMethod method);
};
