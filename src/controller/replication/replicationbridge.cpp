#include "replicationbridge.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaObject>

#include "ReplicationBridge/IReplicationEngine.h"
#include "commandContext.h"
#include "entities/track.h"
#include "json/jsonresponsebuilder.h"
#include "network/iTransport.h"
#include "services/trackservice.h"
#include "tracknetserializer.h"
#include "utils/configuration.h"

using replication_engine::ReplicatedObject;

ReplicationBridge::ReplicationBridge(CommandContext* context,
                                     replication_engine::IReplicationEngine* engine,
                                     QObject* parent)
    : QObject(parent)
    , m_context(context)
    , m_engine(engine)
{
    Q_ASSERT(m_context);
    Q_ASSERT(m_engine);
}

// --- Saliente DDM → RE ---

ReplicatedObject ReplicationBridge::buildEnvelope(const Track& track) const
{
    ReplicatedObject obj;
    obj.guid = track.getGuid().toStdString();
    obj.object_type = kObjectTypeTrack;
    // console_id de ddm.ini — NO es el meko/tipo de overlay.json (otro dato).
    obj.source_console_id = Configuration::instance().consoleId;
    // last_updated: momento semántico del evento. Hoy el evento del operador
    // llega sincrónico al hilo Qt, así que tomarlo acá es equivalente a
    // tomarlo en el handler (ICD §2.2).
    obj.last_updated = QDateTime::currentMSecsSinceEpoch();
    obj.json_payload = TrackNetSerializer::toNetworkPayload(track).toStdString();
    return obj;
}

bool ReplicationBridge::publishTrackAdded(const Track& track)
{
    // OwnShip (id 0) y tracks sin guid no cruzan la frontera (REPlan.md D3).
    if (track.getId() == 0 || track.getGuid().isEmpty()) {
        return false;
    }

    const ReplicatedObject obj = buildEnvelope(track);
    if (obj.json_payload.size() > static_cast<size_t>(kMaxPayloadBytes)) {
        qWarning() << "[ReplicationBridge] payload de" << obj.json_payload.size()
                   << "bytes excede el limite de 4 KB (RNF-09) — track"
                   << track.getId() << "no replicado";
        return false;
    }

    m_engine->onLocalObjectUpserted(obj);
    return true;
}

bool ReplicationBridge::publishTrackDeleted(const QString& guid)
{
    if (guid.isEmpty()) {
        return false;
    }
    m_engine->onLocalObjectDeleted(guid.toStdString());
    return true;
}

// --- Entrante RE → DDM ---
// Cada callback se ejecuta en el Worker Thread de RE: capturar POR VALOR y
// diferir al hilo Qt (ICD §9.4). Nunca tocar m_context desde este hilo.

void ReplicationBridge::onInjectObject(const ReplicatedObject& obj)
{
    QMetaObject::invokeMethod(this, [this, obj]() {
        injectRemoteObject(obj);
    }, Qt::QueuedConnection);
}

void ReplicationBridge::onRemoveObject(const std::string& guid)
{
    const QString guidStr = QString::fromStdString(guid);
    QMetaObject::invokeMethod(this, [this, guidStr]() {
        removeRemoteObject(guidStr);
    }, Qt::QueuedConnection);
}

void ReplicationBridge::onClearAllObjects()
{
    QMetaObject::invokeMethod(this, [this]() {
        clearReplicatedObjects();
    }, Qt::QueuedConnection);
}

void ReplicationBridge::onSnapshotProgress(int current, int total)
{
    QMetaObject::invokeMethod(this, [this, current, total]() {
        qInfo() << "[ReplicationBridge] snapshot lote" << current << "de" << total;
        pushReplicationStatusToUi(QStringLiteral("snapshot_progress"), current, total);
    }, Qt::QueuedConnection);
}

void ReplicationBridge::onSnapshotCompleted()
{
    QMetaObject::invokeMethod(this, [this]() {
        qInfo() << "[ReplicationBridge] snapshot completado — BD sincronizada";
        pushReplicationStatusToUi(QStringLiteral("snapshot_completed"));
    }, Qt::QueuedConnection);
}

void ReplicationBridge::onNetworkStatusChanged(int status)
{
    QMetaObject::invokeMethod(this, [this, status]() {
        qInfo() << "[ReplicationBridge] estado de red:" << status;
        pushReplicationStatusToUi(QStringLiteral("network_status"), status);
    }, Qt::QueuedConnection);
}

// --- Aplicación en el hilo Qt (silenciosa: jamás re-publica, ICD §2.3) ---

void ReplicationBridge::injectRemoteObject(const ReplicatedObject& obj)
{
    if (obj.object_type != kObjectTypeTrack) {
        // 2..5 reservados para figuras (REPlan.md D5) — aún no soportados.
        qWarning() << "[ReplicationBridge] object_type" << obj.object_type
                   << "no soportado — objeto descartado, guid:"
                   << QString::fromStdString(obj.guid);
        return;
    }

    QJsonObject payload;
    if (!TrackNetSerializer::parsePayload(obj.json_payload, payload)) {
        qWarning() << "[ReplicationBridge] json_payload invalido — guid:"
                   << QString::fromStdString(obj.guid);
        return;
    }

    const QString guid = QString::fromStdString(obj.guid);
    Track* existing = m_context->findTrackByGuid(guid);

    if (existing) {
        TrackNetSerializer::applyPayload(payload, *existing);
        qInfo() << "[ReplicationBridge] track remoto actualizado — guid:" << guid
                << "id local:" << existing->getId();
    } else {
        const int id = m_context->nextTrackId++;
        Track& track = m_context->emplaceTrackFront(id, TrackData::SPC,
                                                    TrackData::Pending, TrackData::Auto,
                                                    0.0f, 0.0f);
        track.setGuid(guid);
        TrackNetSerializer::applyPayload(payload, track);
        m_context->setSitrepInfo(id, track.getInformacionAmpliatoria());
        qInfo() << "[ReplicationBridge] track remoto creado — guid:" << guid
                << "id local:" << id << "consola origen:" << obj.source_console_id;
    }

    pushTracksToUi();
}

void ReplicationBridge::removeRemoteObject(const QString& guid)
{
    Track* track = m_context->findTrackByGuid(guid);
    if (!track) {
        // Notificación duplicada o fuera de orden: sin error (ICD §5).
        qInfo() << "[ReplicationBridge] borrado remoto de guid inexistente (ignorado):" << guid;
        return;
    }

    const int id = track->getId();
    m_context->eraseTrackByGuid(guid);
    m_context->sitrepExtra.erase(id);
    m_context->eraseCpaMarkersByTrackId(id);
    qInfo() << "[ReplicationBridge] track remoto eliminado — guid:" << guid << "id local:" << id;

    pushTracksToUi();
}

void ReplicationBridge::clearReplicatedObjects()
{
    int removed = 0;
    auto& tracks = m_context->tracks;
    for (auto it = tracks.begin(); it != tracks.end();) {
        if (!it->getGuid().isEmpty()) {
            m_context->sitrepExtra.erase(it->getId());
            m_context->eraseCpaMarkersByTrackId(it->getId());
            it = tracks.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }
    qInfo() << "[ReplicationBridge] clear de objetos replicados:" << removed << "track(s)";

    pushTracksToUi();
}

// --- Feedback al frontend ---

void ReplicationBridge::pushTracksToUi()
{
    if (!m_context->transport) {
        return;
    }
    QJsonObject args;
    args["tracks"] = TrackService(m_context).serializeTracks();
    m_context->transport->send(
        JsonResponseBuilder::buildSuccessResponse(QStringLiteral("list_tracks"), args));
}

void ReplicationBridge::pushReplicationStatusToUi(const QString& event, int a, int b)
{
    if (!m_context->transport) {
        return;
    }
    QJsonObject args;
    args["event"] = event;
    if (event == QLatin1String("network_status")) {
        args["status"] = a;
    } else if (event == QLatin1String("snapshot_progress")) {
        args["current"] = a;
        args["total"] = b;
    }
    m_context->transport->send(
        JsonResponseBuilder::buildSuccessResponse(QStringLiteral("replication_status"), args));
}
