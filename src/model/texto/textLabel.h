#pragma once

#include <QString>
#include <QPointF>

// ─────────────────────────────────────────────────────────────────────────
// TextLabel — Entidad del módulo Texto.
//
// Representa una etiqueta de texto individual visible en el LPD.
// Contiene sus atributos visuales (Sección A), su posición resuelta
// (Sección B) y su asociación opcional a un track (Sección C).
//
// No tiene lógica propia — es una estructura de datos pura, similar a
// TwoWStationEntry o HaSessionState. Toda la lógica de creación,
// edición, borrado y recálculo vive en TextService y TextCalculator.
// ─────────────────────────────────────────────────────────────────────────

enum class TextFontSize {
    XS = 12,
    MD = 14,
    LG = 16
};

enum class TextColor {
    Rojo,
    Verde,
    Azul,
    Cian,
    Magenta,
    Amarillo,
    Blanco,
    Purpura,
    MarronAnaranjado
};

enum class TextPositionMethod {
    Manual,     // MAN — click en el LPD
    AzimutDist, // AZ/DT — azimut + distancia (Verdadero o Relativo)
    LatLon,     // LAT/LON — coordenada geográfica absoluta
    DelTrack    // DEL TRACK — posición actual de un track como origen
};

struct TextLabel {

    // ── Identidad ────────────────────────────────────────────────────────
    int tn = -1;   // Track Number propio de este texto (asignado al crear)

    // ── Sección A — contenido y formato visual ───────────────────────────
    QString       texto;                                  // máx 20 caracteres
    TextFontSize  fontSize      = TextFontSize::MD;
    TextColor     fontColor     = TextColor::Blanco;
    TextColor     borderColor   = TextColor::Blanco;
    TextColor     backgroundColor = TextColor::Blanco;

    // ── Sección B — posición base (resuelta a DM) ────────────────────────
    TextPositionMethod positionMethod = TextPositionMethod::Manual;
    double  baseXDm = 0.0;   // posición base fijada al crear (no cambia)
    double  baseYDm = 0.0;

    // ── Sección C — asociación dinámica opcional ─────────────────────────
    bool    associated     = false;
    int     associatedTrackId = -1;   // -1 = sin asociar
    double  offsetXDm      = 0.0;     // desvío respecto al track al asociar
    double  offsetYDm      = 0.0;

    // ── Posición actual (recalculada cada 80ms si está asociado) ─────────
    double  currentXDm = 0.0;
    double  currentYDm = 0.0;

    void reset() { *this = TextLabel{}; }
};
