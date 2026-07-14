#pragma once
// Contrato ICD §4 + ADR-011 — comandos DDM → RE.
// Los métodos onLocal* encolan el evento en la cola thread-safe de RE y
// retornan inmediatamente (seguro llamarlos desde el hilo Qt).
// reconnectTransport/reinitializeStorage viven acá por ADR-011 (Opción C).
// Stub local: espeja el header público que entregará SiOp con el .so real.

#include <string>

#include "ReplicatedObject.h"

namespace replication_engine {

class IReplicationEngine {
public:
    virtual ~IReplicationEngine() = default;

    // DDM → RE: alta o modificación de origen local (ObjectAdded / ObjectModified)
    virtual void onLocalObjectUpserted(const ReplicatedObject& obj) = 0;

    // DDM → RE: borrado de origen local (ObjectDeleted)
    virtual void onLocalObjectDeleted(const std::string& guid)      = 0;

    // DDM → RE: recuperación tras NetworkStatus::TRANSPORT_FAILED (ADR-009)
    virtual void reconnectTransport()                               = 0;

    // DDM → RE: recuperación tras NetworkStatus::STORAGE_FAILED (ADR-010)
    virtual void reinitializeStorage()                              = 0;
};

} // namespace replication_engine
