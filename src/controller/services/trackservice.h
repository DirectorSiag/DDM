#pragma once

#include <QJsonArray>
#include <QString>
#include <optional>

#include "enums.h"
#include "entities/track.h"

class CommandContext;

struct TrackCreateRequest {
    TrackData::Type type = TrackData::SPC;
    std::optional<TrackData::Type> creationEnvironment;
    TrackData::Identity identity = TrackData::Pending;
    TrackData::TrackMode mode = TrackData::Auto;
    double x = 0.0;
    double y = 0.0;

    // Legacy constructor fields.
    double ctorSpeedKnots = 0.0;
    double ctorCourseDeg = 0.0;

    // Optional model overrides.
    std::optional<double> speedDmPerHour;
    std::optional<int> courseDeg;
    std::optional<int> fc;
    std::optional<QString> asgc;
    std::optional<Track::LinkYStatus> linkY;
    std::optional<Track::Link14Status> link14;
    std::optional<QString> info;
    std::optional<QString> priv;
};

struct TrackOperationResult {
    bool success = false;
    QString errorCode;
    QString message;
    int trackId = -1;
};

/**
 * @brief Servicio para CRUD e queries de tracks
 * 
 * Encapsula operaciones de creación, eliminación, búsqueda y serialización de tracks,
 * aplicando validaciones centralizadas y manteniendo la consistencia del CommandContext.
 */
class TrackService {
public:
    /**
     * @brief Constructor que inyecta el contexto
     * @param context Puntero a CommandContext (no nulo)
     */
    explicit TrackService(CommandContext* context);

    /**
     * @brief Crea un nuevo track en el contexto
     * @param request Estructura con parámetros del nuevo track
     * @return TrackOperationResult con estado, ID asignado, y mensaje de error si aplica
     * @note Valida: coordenadas (-256 a 256), velocidad (0-99.9 DM/h), curso (0-359)
     * @note Asigna automáticamente número de track y aplica parámetros opcionales
     */
    TrackOperationResult createTrack(const TrackCreateRequest& request);

    /**
     * @brief Elimina un track por ID
     * @param trackId ID del track a eliminar
     * @return TrackOperationResult indicando éxito o error (track no encontrado)
     * @note También limpia entradas asociadas en estructuras como sitrepExtra
     */
    TrackOperationResult deleteTrackById(int trackId);

    /**
     * @brief Encontrar un track específico en el contexto
     * @param trackId ID del track a buscar
     * @return Puntero a Track si existe, nullptr en caso contrario
     * @note Permite lectura/modificación directa de la entidad encontrada
     */
    Track* findTrackById(int trackId) const;

    /**
     * @brief Serializa todos los tracks actuales a JSON
     * @return QJsonArray con objetos JSON representando cada track
     * @note Cada objeto contiene: id, type, identity, azimut, distancia, rumbo, velocidad, link, lat, lon, info
     */
    QJsonArray serializeTracks() const;

private:
    CommandContext* m_context;
};
