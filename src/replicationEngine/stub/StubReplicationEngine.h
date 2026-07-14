#pragma once
// Stub de ReplicationEngine para desarrollo de DDM sin el .so real.
// Implementa las dos interfaces del contrato (IReplicationEngine para
// comandos DDM→RE, IReplicationBridge para notificaciones RE→DDM) y una
// "BD" en memoria que simula la persistencia SQLite de RE.
//
// A diferencia del RE real, invoca al listener sincrónicamente desde el
// hilo que llama (no hay Worker Thread) — el marshal del bridge de DDM
// difiere igual el efecto al event loop de Qt, como en producción.
//
// Sin Qt, fiel a las restricciones del RE real (ADR-007).
// Se elimina del build cuando SiOp entregue el .so.

#include <string>
#include <unordered_map>

#include "ReplicationBridge/IReplicationBridge.h"
#include "ReplicationBridge/IReplicationEngine.h"
#include "ReplicationBridge/ReplicatedObject.h"

class StubReplicationEngine final : public replication_engine::IReplicationEngine,
                                    public replication_engine::IReplicationBridge {
public:
    using ReplicatedObject = replication_engine::ReplicatedObject;

    // El RE real recibe el Domain ID de DDS por constructor (ICD §8, ADR-012:
    // DDM lo lee de ddm.ini y lo inyecta). El stub solo lo loguea.
    explicit StubReplicationEngine(int domainId = 0);

    // --- IReplicationEngine (DDM → RE) ---
    void onLocalObjectUpserted(const ReplicatedObject& obj) override;
    void onLocalObjectDeleted(const std::string& guid) override;
    void reconnectTransport() override;
    void reinitializeStorage() override;

    // --- IReplicationBridge (RE → DDM) ---
    void registerListener(replication_engine::IReplicationListener* listener) override;
    void notifyInjectObject(const ReplicatedObject& obj) override;
    void notifyRemoveObject(const std::string& guid) override;
    void notifyClearAllObjects() override;
    void notifySnapshotProgress(int current, int total) override;
    void notifySnapshotCompleted() override;
    void notifyNetworkStatusChanged(int status) override;

    // --- Helpers de prueba (solo del stub, no forman parte del contrato) ---

    /// Simula la llegada de un objeto desde otra consola (tópico RealTimeReplication).
    void simulateRemoteUpsert(const ReplicatedObject& obj);

    /// Simula una DeletionNotification de otra consola.
    void simulateRemoteDelete(const std::string& guid);

    /// Simula un cambio de estado de red (valores de NetworkStatus).
    void simulateNetworkStatus(int status);

    /// Simula la recepción de un Snapshot completo: progreso por lotes,
    /// ClearAllObjects, re-inyección de toda la BD simulada y SnapshotCompleted.
    void simulateSnapshot(int totalBatches);

    /// Vuelca la BD simulada por stdout.
    void dumpDb() const;

private:
    void logEnvelope(const char* event, const ReplicatedObject& obj) const;

    replication_engine::IReplicationListener* m_listener{nullptr};
    std::unordered_map<std::string, ReplicatedObject> m_db; // SQLite simulada
    int m_domainId{0};
};
