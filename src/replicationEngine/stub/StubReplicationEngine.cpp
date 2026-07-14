#include "StubReplicationEngine.h"

#include <iostream>

#include "ReplicationBridge/IReplicationListener.h"
#include "ReplicationBridge/NetworkStatus.h"

using replication_engine::NetworkStatus;

StubReplicationEngine::StubReplicationEngine(int domainId)
    : m_domainId(domainId)
{
    std::cout << "[StubRE] instanciado con domain_id=" << m_domainId << std::endl;
}

void StubReplicationEngine::logEnvelope(const char* event, const ReplicatedObject& obj) const
{
    std::cout << "[StubRE] " << event
              << " guid=" << obj.guid
              << " type=" << obj.object_type
              << " console=" << obj.source_console_id
              << " ts=" << obj.last_updated
              << " payload(" << obj.json_payload.size() << "B)=" << obj.json_payload
              << std::endl;
}

// --- IReplicationEngine (DDM → RE) ---

void StubReplicationEngine::onLocalObjectUpserted(const ReplicatedObject& obj)
{
    logEnvelope("onLocalObjectUpserted", obj);
    m_db[obj.guid] = obj;
}

void StubReplicationEngine::onLocalObjectDeleted(const std::string& guid)
{
    std::cout << "[StubRE] onLocalObjectDeleted guid=" << guid
              << (m_db.erase(guid) ? "" : " (no estaba en la BD)") << std::endl;
}

void StubReplicationEngine::reconnectTransport()
{
    std::cout << "[StubRE] reconnectTransport — simulando reconexion exitosa" << std::endl;
    notifyNetworkStatusChanged(static_cast<int>(NetworkStatus::CONNECTED));
}

void StubReplicationEngine::reinitializeStorage()
{
    std::cout << "[StubRE] reinitializeStorage — recreando BD y repoblando como late joiner" << std::endl;
    simulateSnapshot(1);
}

// --- IReplicationBridge (RE → DDM) ---

void StubReplicationEngine::registerListener(replication_engine::IReplicationListener* listener)
{
    m_listener = listener;
    std::cout << "[StubRE] registerListener " << (listener ? "OK" : "(null)") << std::endl;
}

void StubReplicationEngine::notifyInjectObject(const ReplicatedObject& obj)
{
    if (m_listener) m_listener->onInjectObject(obj);
}

void StubReplicationEngine::notifyRemoveObject(const std::string& guid)
{
    if (m_listener) m_listener->onRemoveObject(guid);
}

void StubReplicationEngine::notifyClearAllObjects()
{
    if (m_listener) m_listener->onClearAllObjects();
}

void StubReplicationEngine::notifySnapshotProgress(int current, int total)
{
    if (m_listener) m_listener->onSnapshotProgress(current, total);
}

void StubReplicationEngine::notifySnapshotCompleted()
{
    if (m_listener) m_listener->onSnapshotCompleted();
}

void StubReplicationEngine::notifyNetworkStatusChanged(int status)
{
    if (m_listener) m_listener->onNetworkStatusChanged(status);
}

// --- Helpers de prueba ---

void StubReplicationEngine::simulateRemoteUpsert(const ReplicatedObject& obj)
{
    logEnvelope("simulateRemoteUpsert", obj);
    m_db[obj.guid] = obj;
    notifyInjectObject(obj);
}

void StubReplicationEngine::simulateRemoteDelete(const std::string& guid)
{
    std::cout << "[StubRE] simulateRemoteDelete guid=" << guid << std::endl;
    m_db.erase(guid);
    notifyRemoveObject(guid);
}

void StubReplicationEngine::simulateNetworkStatus(int status)
{
    std::cout << "[StubRE] simulateNetworkStatus status=" << status << std::endl;
    notifyNetworkStatusChanged(status);
}

void StubReplicationEngine::simulateSnapshot(int totalBatches)
{
    if (totalBatches < 1) totalBatches = 1;
    std::cout << "[StubRE] simulateSnapshot batches=" << totalBatches
              << " objetos=" << m_db.size() << std::endl;

    notifyNetworkStatusChanged(static_cast<int>(NetworkStatus::SYNCING));
    for (int i = 0; i <= totalBatches; ++i) {
        notifySnapshotProgress(i, totalBatches);
    }
    notifyClearAllObjects();
    for (const auto& [guid, obj] : m_db) {
        notifyInjectObject(obj);
    }
    notifySnapshotCompleted();
    notifyNetworkStatusChanged(static_cast<int>(NetworkStatus::CONNECTED));
}

void StubReplicationEngine::dumpDb() const
{
    std::cout << "[StubRE] BD simulada: " << m_db.size() << " objeto(s)" << std::endl;
    for (const auto& [guid, obj] : m_db) {
        std::cout << "  - guid=" << guid
                  << " type=" << obj.object_type
                  << " console=" << obj.source_console_id
                  << " ts=" << obj.last_updated << std::endl;
    }
}
