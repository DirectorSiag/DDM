#include "replicationBridge.h"
#include "commandContext.h"
#include "configuration.h"
#include "entities/track.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QMetaObject>
#include <QUuid>
#include "json/jsonserializer.h"

ReplicationBridge::ReplicationBridge(CommandContext* ctx,
                                     IReplicationBridge* re,
                                     QObject* parent)
    : QObject(parent)
    , m_ctx(ctx)
    , m_re(re)
{}

// ── Entrada: RE → DDM ────────────────────────────────────────────────────────

void ReplicationBridge::onInjectObject(const ReplicatedObjectStruct& obj)
{
    // Ejecutado en Worker Thread de RE — marshal a Qt main thread
    QMetaObject::invokeMethod(this, [this, obj]() {
        const auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(obj.json_payload));
        if (!doc.isObject()) {
            qDebug() << "ReplicationBridge: json_payload malformado, guid=" << obj.guid;
            return;
        }
        Track track = JsonSerializer::deserializeTrack(doc.object());
        m_ctx->injectRemoteTrack(track);
    }, Qt::QueuedConnection);
}

void ReplicationBridge::onRemoveObject(const std::string& guid)
{
    QMetaObject::invokeMethod(this, [this, guid]() {
        if (!m_ctx->removeRemoteTrack(guid))
            qDebug() << "ReplicationBridge: guid no encontrado para eliminar";
    }, Qt::QueuedConnection);
}

void ReplicationBridge::onClearAllObjects()
{
    QMetaObject::invokeMethod(this, [this]() {
        m_ctx->clearAllPersistentTracks();
    }, Qt::QueuedConnection);
}

void ReplicationBridge::onSnapshotProgress(int current, int total)
{
    QMetaObject::invokeMethod(this, [this, current, total]() {
        emit snapshotProgress(current, total);
    }, Qt::QueuedConnection);
}

void ReplicationBridge::onSnapshotCompleted()
{
    QMetaObject::invokeMethod(this, [this]() {
        emit snapshotCompleted();
    }, Qt::QueuedConnection);
}

void ReplicationBridge::onNetworkStatusChanged(int status)
{
    QMetaObject::invokeMethod(this, [this, status]() {
        emit networkStatusChanged(status);
    }, Qt::QueuedConnection);
}

// ── Salida: DDM → RE ─────────────────────────────────────────────────────────

std::string ReplicationBridge::generateUuid()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
}

void ReplicationBridge::notifyObjectUpserted(const Track& track)
{
    if (!m_re)
        return;

    std::string guid = track.getGuid();
    if (guid.empty()) {
        guid = generateUuid();
        // Persistir el guid en el Track del contexto para que modificaciones
        // posteriores usen el mismo identificador de red.
        Track* ctxTrack = m_ctx->findTrackById(track.getId());
        if (ctxTrack)
            ctxTrack->setGuid(guid);
    }

    ReplicatedObjectStruct obj;
    obj.guid              = guid;
    obj.object_type       = static_cast<int>(ObjectType::Track);
    obj.source_console_id = Configuration::instance().meko;
    obj.last_updated      = QDateTime::currentMSecsSinceEpoch();
    const QJsonObject trackJson = JsonSerializer::serializeTrackForReplication(track);
    obj.json_payload = QJsonDocument(trackJson).toJson(QJsonDocument::Compact).toStdString();

    m_re->onLocalObjectUpserted(obj);
}

void ReplicationBridge::notifyObjectDeleted(const std::string& guid)
{
    if (!m_re)
        return;
    m_re->onLocalObjectDeleted(guid);
}
