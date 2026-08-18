#pragma once

#include <QString>
#include <QPointF>
#include <QMap>
#include <QDateTime>

// ─────────────────────────────────────────────────────────────────────────
// DSIEntity — Entidad del módulo DSI (Distancia de Seguridad Impuesta).

// (estilos de linea con nombres provisorios): la especificación solo indica "8 variantes"

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
    Manual,
    AzimutDist,
    LatLon
};

struct DSIEntity {

    int tn = -1;   // TN propio de esta DSI (asignado al crear, ver DSISessionState)

    // Sección A
    double        radiusDm         = 50.0;   // default 50 MN, ver REQ-DSI-UI-002
    DSILineStyle  lineStyle        = DSILineStyle::Continua;
    QString       labelText;                  // default "DSI 50MN", máx 20 caracteres
    // Color/Fondo del label arrancan sin selección
    bool          labelColorSet      = false;
    DSIColor      labelColor         = DSIColor::Blanco;
    bool          labelBackgroundSet = false;
    DSIColor      labelBackground    = DSIColor::Blanco;

    // ── Sección B — posición base (resuelta a DM)
    DSIPositionMethod positionMethod = DSIPositionMethod::Manual;
    double  baseXDm = 0.0;
    double  baseYDm = 0.0;

    // ── Sección C — asociación dinámica
    // no se guarda ningún offset, el centro pasa a ser exactamente la
    // posición del track mientras dure la asociación.
    bool    associated        = false;
    int     associatedTrackId = -1;   // -1 = sin asociar

    // ── Posición actual (recalculada cada 80ms si está asociada)
    double  currentXDm = 0.0;
    double  currentYDm = 0.0;

    // ── Estado del ciclo de alarma
    // Por cada track que dispara la alarma al ingresar a la zona, se
    // guarda el timestamp del disparo. Mientras el track esté en este
    // mapa, está "en cooldown" (dentro de la ventana de 5s) y no vuelve
    // a disparar la alarma aunque siga dentro del radio.
    QMap<int, QDateTime> activeAlarms;

    void reset() { *this = DSIEntity{}; }
};
