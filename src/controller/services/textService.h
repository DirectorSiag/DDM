#pragma once

#include "commandContext.h"
#include "model/texto/textCalculator.h"
#include <QMap>

struct TextOperationResult {
    bool    ok = false;
    QString message;
};

// Parámetros de creación — agrupa las tres secciones (A, B, C) para
// pasarlas al Service en una sola llamada.
struct TextCreateParams {
    // Sección A
    QString      texto;
    TextFontSize fontSize        = TextFontSize::MD;
    TextColor    fontColor       = TextColor::Blanco;
    TextColor    borderColor     = TextColor::Blanco;
    TextColor    backgroundColor = TextColor::Blanco;

    // Sección B — solo uno de estos métodos debe estar poblado
    TextPositionMethod positionMethod = TextPositionMethod::Manual;

    // Para Manual
    double manXDm = 0.0;
    double manYDm = 0.0;

    // Para AzimutDist
    double azimuthDeg   = 0.0;
    double distanceDm   = 0.0;
    bool   useVerdadero = true;   // true = V, false = R

    // Para LatLon (ya convertido a decimal por el Command)
    double latDecimal = 0.0;
    double lonDecimal = 0.0;

    // Para DelTrack
    int refTrackId = -1;

    // Sección C — opcional
    bool asociarAlCrear = false;
    int  trackToAssociate = -1;
};

class TextService {
public:
    explicit TextService(CommandContext* ctx);

    TextOperationResult createLabel(const TextCreateParams& params);
    TextOperationResult editLabel(int tn, const QMap<QString, QString>& fields);
    TextOperationResult deleteLabel(int tn);

    TextOperationResult associateTrack(int textTn, int trackId);
    TextOperationResult dissociateTrack(int textTn);

    TextOperationResult listLabels() const;
    TextOperationResult infoLabel(int tn) const;

    void update();

private:
    CommandContext* m_ctx;

    static QString fontSizeToString(TextFontSize size);
    static QString colorToString(TextColor color);
    static QString positionMethodToString(TextPositionMethod method);
};
