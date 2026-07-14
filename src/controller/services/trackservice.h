#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <optional>
#include <string>
#include <unordered_map>

#include "enums.h"
#include "entities/track.h"

class CommandContext;

namespace replication_engine {
class IReplicationEngine;
}

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
 *
 * Es QObject para recibir por slots los tracks replicados que emite
 * ReplicationListener (RE → DDM, conexión con Qt::QueuedConnection).
 */
class TrackService : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructor que inyecta el contexto
     * @param context Puntero a CommandContext (no nulo)
     * @param parent Parent Qt opcional
     */
    explicit TrackService(CommandContext* context, QObject* parent = nullptr);

    /**
     * @brief Asocia el motor de replicación (no-owning, puede ser nullptr)
     * @note Con engine seteado, cada alta/baja local se publica vía
     *       onLocalObjectUpserted/onLocalObjectDeleted (ICD §4, llamada directa).
     *       Sin engine, las operaciones son solo locales.
     */
    void setReplicationEngine(replication_engine::IReplicationEngine* engine);

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

    /**
     * @brief Serializa un track individual a JSON
     * @note Mismo formato que serializeTracks(); también se usa como payload
     *       del Envelope de replicación
     */
    QJsonObject serializeTrack(const Track& track) const;

public slots:
    /**
     * @brief Crea un track replicado recibido desde la red (ICD §5, InjectObject)
     * @param guid Identidad de red del objeto. Clave opaca: no se interpreta
     * @param request Estado del track ya deserializado por ReplicationListener
     * @note Guid ya conocido: se descarta con log (update replicado aún no implementado)
     * @note Silencioso: nunca re-notifica a ReplicationEngine (anti-eco)
     */
    void onReplicatedTrackCreate(const QString& guid, const TrackCreateRequest& request);

    /**
     * @brief Elimina un track replicado (ICD §5, RemoveObject)
     * @param guid Identidad de red del objeto a eliminar
     * @note Guid desconocido: operación idempotente, solo logging diagnóstico
     */
    void onReplicatedTrackRemoved(const QString& guid);

    /**
     * @brief Elimina todos los tracks replicados antes de repoblar desde Snapshot (ICD §5)
     * @note No dispara ObjectDeleted para ningún objeto
     */
    void onReplicatedClearAll();

private:
    /**
     * @brief Valida rangos del request (coordenadas, velocidad, curso, FC)
     * @return success=true si es válido; si no, errorCode/message del problema
     */
    TrackOperationResult validateRequest(const TrackCreateRequest& request) const;

    /**
     * @brief Aplica los campos opcionales del request y recalcula PPP contra OwnShip
     */
    void applyRequestOverrides(Track& track, const TrackCreateRequest& request);

    /**
     * @brief Alta sin publicar a replicación — la usan createTrack (que luego
     *        publica) y el slot replicado (anti-eco: lo remoto no se re-publica,
     *        ICD §5)
     */
    TrackOperationResult createTrackInternal(const TrackCreateRequest& request);

    /**
     * @brief Baja sin publicar a replicación — contraparte de createTrackInternal
     */
    TrackOperationResult deleteTrackInternal(int trackId);

    /**
     * @brief Publica el alta al engine: arma el Envelope (ICD §4, ObjectAdded)
     * @note Sin engine seteado no hace nada (operación solo local)
     */
    void publishUpsert(const std::string& guid, const Track& track);

    /**
     * @brief Búsqueda inversa en el mapa: trackId → guid ("" si no tiene)
     */
    std::string guidForTrackId(int trackId) const;

    CommandContext* m_context;

    // Motor de replicación (no-owning). nullptr = sin replicación.
    replication_engine::IReplicationEngine* m_replicationEngine = nullptr;

    // TODO: leer de configuración cuando exista identidad de consola (ICD §6.2).
    int m_consoleId = 0;

    // guid (identidad de red) → trackId (id local). Clave opaca, solo se
    // compara. Contiene todos los tracks: replicados (guid de la red) y
    // locales (guid generado al crear; el delete lo necesita para publicar).
    std::unordered_map<std::string, int> m_guidToTrackId;
};

// Permite que TrackCreateRequest viaje como argumento de señal en
// conexiones encoladas (Qt::QueuedConnection) entre hilos.
Q_DECLARE_METATYPE(TrackCreateRequest)
