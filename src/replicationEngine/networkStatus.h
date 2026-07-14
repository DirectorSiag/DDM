#ifndef NETWORKSTATUS_H
#define NETWORKSTATUS_H

// include/ReplicationBridge/NetworkStatus.h
namespace replication_engine {

enum class NetworkStatus : int {
    DISCONNECTED      = 0,  // Sin peers. Operación aislada normal (RF-RE-005).
    CONNECTED         = 1,  // Operación normal con peers descubiertos.
    SYNCING           = 2,  // Recibiendo Snapshot de un líder.
    TRANSPORT_FAILED  = 3,  // Middleware DDS caído en runtime. Ver ADR-009.
    STORAGE_FAILED    = 4,  // SQLite caído en runtime. Ver ADR-010.
};

} // namespace replication_engine


#endif // NETWORKSTATUS_H
