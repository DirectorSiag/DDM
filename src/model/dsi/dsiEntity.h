#pragma once

#include <QString>
#include <QPointF>
#include <QMap>
#include <QDateTime>

// ─────────────────────────────────────────────────────────────────────────
// DSIEntity — Entidad del módulo DSI (Distancia de Seguridad Impuesta).
//
// Representa una zona circular de alerta visible en el LPD. Contiene sus
// atributos visuales (Sección A), su posición resuelta (Sección B), su
// asociación opcional a un track (Sección C) y el estado del ciclo de
// alarma por track que ingresa a la zona (F1/F2, REQ-DSI-CAL-001/002).
//
// No tiene lógica propia — es una estructura de datos pura, igual que
// TextLabel. Toda la lógica de creación, edición, borrado, asociación y
// evaluación de alarma vive en DSIService y DSICalculator.
//
// NOTA: la circunferencia siempre se dibuja de color azul fijo (F4 de la
// especificación) — por eso no hay un campo de color para el círculo,
// solo para el label (texto/fondo).
// ─────────────────────────────────────────────────────────────────────────

// TODO(confirmar con diseño): la especificación solo indica "8 variantes"
// de estilo de línea sin nombrarlas. Nombres provisorios — ajustar cuando
// el equipo de diseño defina el catálogo real.
enum class DSILineStyle {
    Continua,
    Segmentada,
    Punteada,
    PuntoRaya,
    PuntoRayaRaya,
    DobleLinea,
    Ondulada,
    Gruesa
};

// Misma paleta de 9 colores que TextColor — aplica solo al label (texto y
// fondo), nunca al círculo.
enum class DSIColor {
    Rojo,
    Verde,
    Azul,
    Cian,
    Magenta,
    Amarillo,
    Blanco,
    Purpura,
    Naranja
};

enum class DSIPositionMethod {
    Manual,     // MAN — click en el LPD
    AzimutDist, // AZ/DT — azimut + distancia (Verdadero o Relativo)
    LatLon      // LAT/LON — coordenada geográfica absoluta
    // NOTA: a diferencia de Texto, DSI NO tiene un cuarto método estático
    // "DEL TRACK" en la Sección B — REQ-DSI-UI-015 solo define exclusión
    // mutua entre estos tres. El seguimiento de track vive exclusivamente
    // en la Sección C (associated / associatedTrackId más abajo).
};

struct DSIEntity {

    // ── Identidad ────────────────────────────────────────────────────────
    int tn = -1;   // TN propio de esta DSI (asignado al crear, ver DSISessionState)

    // ── Sección A — contenido y formato visual ───────────────────────────
    double        radiusDm         = 50.0;   // default 50 MN, ver REQ-DSI-UI-002
    DSILineStyle  lineStyle        = DSILineStyle::Continua;
    QString       labelText;                  // default "DSI 50MN", máx 20 caracteres
    // Color/Fondo del label arrancan sin selección — ver confirmación del 13/08
    bool          labelColorSet      = false;
    DSIColor      labelColor         = DSIColor::Blanco;
    bool          labelBackgroundSet = false;
    DSIColor      labelBackground    = DSIColor::Blanco;

    // ── Sección B — posición base (resuelta a DM) ────────────────────────
    DSIPositionMethod positionMethod = DSIPositionMethod::Manual;
    double  baseXDm = 0.0;   // posición base fijada al crear (no cambia)
    double  baseYDm = 0.0;

    // ── Sección C — asociación dinámica opcional ─────────────────────────
    // A diferencia de Texto, la adopción es DIRECTA (REQ-DSI-UI-017):
    // no se guarda ningún offset, el centro pasa a ser exactamente la
    // posición del track mientras dure la asociación.
    bool    associated        = false;
    int     associatedTrackId = -1;   // -1 = sin asociar

    // ── Posición actual (recalculada cada 80ms si está asociada) ─────────
    double  currentXDm = 0.0;
    double  currentYDm = 0.0;

    // ── Estado del ciclo de alarma (F1/F2, REQ-DSI-CAL-001/002) ──────────
    // Por cada track que dispara la alarma al ingresar a la zona, se
    // guarda el timestamp del disparo. Mientras el track esté en este
    // mapa, está "en cooldown" (dentro de la ventana de 5s) y no vuelve
    // a disparar la alarma aunque siga dentro del radio.
    QMap<int, QDateTime> activeAlarms;

    void reset() { *this = DSIEntity{}; }
};
