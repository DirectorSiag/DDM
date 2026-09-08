#pragma once

#include "commandContext.h"
#include <QMap>
#include <QList>

struct DSIOperationResult {
    bool    ok = false;
    QString message;
};


struct DSICreateParams {
    // Sección A
    QString      labelText;
    double       radiusDm       = 50.0;
    DSILineStyle lineStyle      = DSILineStyle::Continua;
    bool         hasLabelColor      = false;
    DSIColor     labelColor         = DSIColor::Blanco;
    bool         hasLabelBackground = false;
    DSIColor     labelBackground    = DSIColor::Blanco;

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

    void update();

    static DSIColor     stringToColor(const QString& s, bool& ok);
    static DSILineStyle stringToLineStyle(const QString& s, bool& ok);

private:
    CommandContext* m_ctx;

    void evaluateProximityAlarm(DSIEntity& zone);

    static QString colorToString(DSIColor color);
    static QString lineStyleToString(DSILineStyle style);
    static QString positionMethodToString(DSIPositionMethod method);
};
