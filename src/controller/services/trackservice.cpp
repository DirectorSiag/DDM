#include "trackservice.h"

#include "commandContext.h"
#include "entities/track.h"
#include "ReplicationEngine/IReplicationEngine.h"
#include "trackpppservice.h"
#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

TrackService::TrackService(CommandContext* context, QObject* parent)
    : QObject(parent)
    , m_context(context)
{
    Q_ASSERT(m_context);
}

void TrackService::setReplicationEngine(replication_engine::IReplicationEngine* engine)
{
    m_replicationEngine = engine;
}

TrackOperationResult TrackService::validateRequest(const TrackCreateRequest& request) const
{
    if (request.x < -256.0 || request.x > 256.0 || request.y < -256.0 || request.y > 256.0) {
        return {false, "INVALID_COORDINATES", "Coordenadas fuera de rango. Deben estar entre -256 y 256.", -1};
    }

    if (request.speedDmPerHour.has_value()) {
        if (request.speedDmPerHour.value() < 0.0 || request.speedDmPerHour.value() > 99.9) {
            return {false, "INVALID_SPEED", "Velocidad DM/h fuera de rango (0..99.9)", -1};
        }
    }

    if (request.courseDeg.has_value()) {
        if (request.courseDeg.value() < 0 || request.courseDeg.value() > 359) {
            return {false, "INVALID_COURSE", "Curso fuera de rango (0..359)", -1};
        }
    }

    if (request.fc.has_value()) {
        if (request.fc.value() < 1 || request.fc.value() > 6) {
            return {false, "INVALID_FC", "Asignacion FC invalida (1..6)", -1};
        }
    }

    return {true, QString(), QString(), -1};
}

void TrackService::applyRequestOverrides(Track& track, const TrackCreateRequest& request)
{
    if (request.speedDmPerHour.has_value()) track.setVelocidadDmPerHour(request.speedDmPerHour.value());
    if (request.courseDeg.has_value()) track.setCursoInt(request.courseDeg.value());
    if (request.fc.has_value()) track.setAsignacionFc(request.fc.value());
    if (request.asgc.has_value()) track.setCodigoAsignacion(request.asgc.value());
    if (request.linkY.has_value()) track.setEstadoLinkY(request.linkY.value());
    if (request.link14.has_value()) track.setEstadoLink14(request.link14.value());
    if (request.info.has_value()) {
        track.setInformacionAmpliatoria(request.info.value());
        m_context->setSitrepInfo(track.getId(), request.info.value());
    }
    if (request.priv.has_value()) track.setCodigoPrivado(request.priv.value());

    // Solo calculamos PPP cuando OwnShip ya fue seteado.
    // No hay recalc periodico en esta etapa porque los tracks se tratan como estaticos.
    if (m_context->ownShip.valid) {
        TrackPppService(m_context).recalculateTrackAgainstOwnShip(track);
    }
}

TrackOperationResult TrackService::createTrackInternal(const TrackCreateRequest& request)
{
    const TrackOperationResult validation = validateRequest(request);
    if (!validation.success) {
        return validation;
    }

    const int id = m_context->nextTrackId++;

    Track& track = m_context->emplaceTrackFront(
        id,
        request.type,
        request.identity,
        request.mode,
        static_cast<float>(request.x),
        static_cast<float>(request.y),
        request.ctorSpeedKnots,
        request.ctorCourseDeg,
        request.creationEnvironment.value_or(request.type)
    );

    applyRequestOverrides(track, request);

    return {true, QString(), QString(), id};
}

TrackOperationResult TrackService::createTrack(const TrackCreateRequest& request)
{
    const TrackOperationResult result = createTrackInternal(request);
    if (!result.success) {
        return result;
    }

    // Alta de origen local: DDM genera el guid (UUID v4, ICD §2.2),
    // lo registra y publica el objeto a la red.
    const std::string guid =
        QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    m_guidToTrackId[guid] = result.trackId;

    if (Track* track = findTrackById(result.trackId)) {
        publishUpsert(guid, *track);
    }

    return result;
}

TrackOperationResult TrackService::deleteTrackInternal(int trackId)
{
    if (trackId < 0) {
        return {false, "INVALID_ID", "El id del track debe ser no negativo", -1};
    }

    if (!m_context->eraseTrackById(trackId)) {
        return {false, "NOT_FOUND", "Track no encontrado", trackId};
    }

    return {true, QString(), QString(), trackId};
}

TrackOperationResult TrackService::deleteTrackById(int trackId)
{
    const TrackOperationResult result = deleteTrackInternal(trackId);
    if (!result.success) {
        return result;
    }

    // Baja de origen local: publicar el borrado a la red (ICD §4, ObjectDeleted).
    const std::string guid = guidForTrackId(trackId);
    if (!guid.empty()) {
        if (m_replicationEngine) {
            m_replicationEngine->onLocalObjectDeleted(guid);
        }
        m_guidToTrackId.erase(guid);
    }

    return result;
}

Track* TrackService::findTrackById(int trackId) const
{
    if (!m_context) return nullptr;
    return m_context->findTrackById(trackId);
}

QJsonObject TrackService::serializeTrack(const Track& track) const
{
    const Track::SitrepPppData ppp = track.getSitrepPpp();

    QString pppStatus = QStringLiteral("not_computed");
    if (ppp.status == Track::SitrepPppData::NoOwnShip) {
        pppStatus = QStringLiteral("no_ownship");
    } else if (ppp.status == Track::SitrepPppData::DegenerateRelativeMotion) {
        pppStatus = QStringLiteral("degenerate_relative_motion");
    } else if (ppp.status == Track::SitrepPppData::Valid) {
        pppStatus = QStringLiteral("valid");
    }

    QJsonObject trackObj;
    trackObj["id"] = track.getId();
    trackObj["type"] = TrackData::toQString(track.getType());
    trackObj["type_bits"] = TrackData::typeBits(track.getType());
    trackObj["creation_environment"] = TrackData::toQString(track.getCreationEnvironment());
    trackObj["creation_environment_bits"] = TrackData::typeBits(track.getCreationEnvironment());
    trackObj["identity"] = TrackData::toQString(track.getIdentity());
    trackObj["azimut"] = track.getAzimuthDeg();
    trackObj["distancia"] = track.getDistanceDm();
    trackObj["rumbo"] = track.getCursoInt();
    trackObj["velocidad"] = track.getVelocidadDmPerHour();
    trackObj["link"] = track.getEstadoLinkY() == Track::LinkY_Invalid ? "--" :
                       QString(QChar("RCTS"[int(track.getEstadoLinkY())]));
    trackObj["lat"] = track.getY();
    trackObj["lon"] = track.getX();
    trackObj["info"] = track.getInformacionAmpliatoria();
    trackObj["ppp_az"] = ppp.azDeg;
    trackObj["ppp_dt"] = ppp.distanceDm;
    trackObj["ppp_t_hhmm"] = track.getSitrepPppTimeHHMM();
    trackObj["ppp_status"] = pppStatus;
    trackObj["ppp_reason"] = ppp.reason;

    return trackObj;
}

QJsonArray TrackService::serializeTracks() const
{
    QJsonArray arr;
    for (const Track& tr : m_context->getTracks()) {
        arr.append(serializeTrack(tr));
    }

    return arr;
}

// ---------------------------------------------------------------------------
// Replicación — subida (DDM → RE, llamada directa desde el hilo Qt, ICD §4)
// ---------------------------------------------------------------------------

void TrackService::publishUpsert(const std::string& guid, const Track& track)
{
    if (!m_replicationEngine) {
        return; // Sin engine: operación solo local.
    }

    replication_engine::ReplicatedObject envelope;
    envelope.guid = guid;
    envelope.object_type = ReplicationData::Track;
    envelope.source_console_id = m_consoleId;
    // Momento semántico del evento (ICD §2.2) — nunca diferido a una cola.
    envelope.last_updated = QDateTime::currentMSecsSinceEpoch();
    envelope.json_payload = QJsonDocument(serializeTrack(track))
                                .toJson(QJsonDocument::Compact)
                                .toStdString();

    if (envelope.json_payload.size() > 4096) {
        qWarning() << "TrackService: json_payload supera 4 KB (RNF-09):"
                   << envelope.json_payload.size() << "bytes, guid:"
                   << QString::fromStdString(guid);
    }

    m_replicationEngine->onLocalObjectUpserted(envelope);
}

std::string TrackService::guidForTrackId(int trackId) const
{
    for (const auto& entry : m_guidToTrackId) {
        if (entry.second == trackId) {
            return entry.first;
        }
    }
    return {};
}

// ---------------------------------------------------------------------------
// Replicación — bajada (RE → DDM vía ReplicationListener).
// Usan los caminos internos: lo remoto NUNCA se re-publica (anti-eco, ICD §5).
// ---------------------------------------------------------------------------

void TrackService::onReplicatedTrackCreate(const QString& guid, const TrackCreateRequest& request)
{
    const std::string key = guid.toStdString();

    if (m_guidToTrackId.count(key) > 0) {
        // Update replicado: pendiente para la próxima iteración.
        qDebug() << "TrackService: guid ya conocido, update replicado no implementado:" << guid;
        return;
    }

    const TrackOperationResult result = createTrackInternal(request);
    if (!result.success) {
        qWarning() << "TrackService: no se pudo crear track replicado, guid:" << guid
                   << "error:" << result.message;
        return;
    }

    m_guidToTrackId[key] = result.trackId;
}

void TrackService::onReplicatedTrackRemoved(const QString& guid)
{
    const auto it = m_guidToTrackId.find(guid.toStdString());
    if (it == m_guidToTrackId.end()) {
        // Notificación duplicada o fuera de orden: idempotente (ICD §5).
        qDebug() << "TrackService: remove replicado de guid desconocido:" << guid;
        return;
    }

    const TrackOperationResult result = deleteTrackInternal(it->second);
    if (!result.success) {
        qDebug() << "TrackService: track replicado" << it->second
                 << "ya no existía:" << result.message;
    }

    m_guidToTrackId.erase(it);
}

void TrackService::onReplicatedClearAll()
{
    for (const auto& entry : m_guidToTrackId) {
        deleteTrackInternal(entry.second);
    }
    m_guidToTrackId.clear();
}
