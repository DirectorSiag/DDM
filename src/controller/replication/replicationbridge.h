#pragma once

#include <QObject>
#include <QString>
#include <string>

#include "ReplicationBridge/IReplicationListener.h"
#include "ReplicationBridge/ReplicatedObject.h"

class Track;
struct CommandContext;

namespace replication_engine {
class IReplicationEngine;
}

/**
 * @brief Punto único de contacto entre DDM y ReplicationEngine (ICD v2.0)
 *
 * Saliente (hilo Qt): TrackService notifica altas/bajas locales vía
 * publishTrackAdded/publishTrackDeleted; el bridge construye el Envelope
 * (guid, object_type, source_console_id, last_updated, json_payload) y lo
 * entrega a IReplicationEngine, que encola y retorna de inmediato.
 *
 * Entrante (Worker Thread de RE): implementa IReplicationListener; cada
 * callback hace marshal al hilo Qt (ICD §9.4) y aplica el cambio en
 * CommandContext en forma silenciosa — nunca pasa por TrackService, de modo
 * que es estructuralmente imposible re-publicar un evento remoto (ICD §2.3).
 */
class ReplicationBridge : public QObject, public replication_engine::IReplicationListener
{
    Q_OBJECT
public:
    /// object_type del Envelope por tipo de objeto (ver REPlan.md).
    /// 2..5 reservados para figuras (área/círculo/polígono/sector) a futuro.
    static constexpr int kObjectTypeTrack = 1;

    /// RNF-09: tamaño máximo del json_payload en bytes.
    static constexpr int kMaxPayloadBytes = 4096;

    ReplicationBridge(CommandContext* context,
                      replication_engine::IReplicationEngine* engine,
                      QObject* parent = nullptr);

    // --- Saliente DDM → RE (llamar solo ante cambios de ORIGEN LOCAL) ---

    /// Publica el alta de un track local. Retorna false si el track no es
    /// replicable (id 0 / guid vacío) o el payload excede 4 KB.
    bool publishTrackAdded(const Track& track);

    /// Publica el borrado de un track local (el guid debe resolverse ANTES
    /// de borrar el track del contexto).
    bool publishTrackDeleted(const QString& guid);

    // --- Entrante RE → DDM (IReplicationListener, Worker Thread de RE) ---
    void onInjectObject(const replication_engine::ReplicatedObject& obj) override;
    void onRemoveObject(const std::string& guid) override;
    void onClearAllObjects() override;
    void onSnapshotProgress(int current, int total) override;
    void onSnapshotCompleted() override;
    void onNetworkStatusChanged(int status) override;

private:
    // Ejecutan en el hilo Qt (destino del marshal).
    void injectRemoteObject(const replication_engine::ReplicatedObject& obj);
    void removeRemoteObject(const QString& guid);
    void clearReplicatedObjects();

    replication_engine::ReplicatedObject buildEnvelope(const Track& track) const;

    /// Empuja la lista completa de tracks al frontend (formato list_tracks)
    /// para refrescar la tabla táctica tras un cambio de origen remoto.
    void pushTracksToUi();

    /// Notifica estado de replicación al frontend (comando replication_status).
    void pushReplicationStatusToUi(const QString& event, int a = -1, int b = -1);

    CommandContext* m_context;
    replication_engine::IReplicationEngine* m_engine;
};
