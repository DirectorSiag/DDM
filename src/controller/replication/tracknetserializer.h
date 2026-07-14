#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <string>

class Track;

/**
 * @brief Serialización del payload de red para Track (schema v1)
 *
 * Formato NUEVO y exclusivo de la replicación entre consolas (ICD §6):
 * no confundir con el JSON de UI de TrackService::serializeTracks(), que
 * es para el frontend local. El guid y el object_type NO viajan en el
 * payload — van en los metadatos del Envelope.
 *
 * Limitación documentada (REPlan.md): x_dm/y_dm son coordenadas de display
 * relativas al ownship local; entre consolas con ownships distintos las
 * posiciones no son comparables. Se revisará junto con ObjectModified.
 */
namespace TrackNetSerializer {

    /// Versión del schema incluida en la clave "v" del payload.
    constexpr int kSchemaVersion = 1;

    /// Serializa el estado táctico replicable del track a JSON compacto.
    /// Excluye m_id (identidad visual local) y m_sitrepPpp (derivado local).
    QByteArray toNetworkPayload(const Track& track);

    /// Parsea un json_payload recibido de la red. Retorna false si el JSON
    /// es inválido o no es un objeto.
    bool parsePayload(const std::string& jsonPayload, QJsonObject& out);

    /// Aplica un payload parseado sobre un track (create-or-update).
    /// Tolerante: las claves ausentes no modifican el campo correspondiente.
    /// Usa los setters de Track, que normalizan/clampan valores.
    void applyPayload(const QJsonObject& payload, Track& track);

} // namespace TrackNetSerializer
