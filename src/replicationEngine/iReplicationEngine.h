#pragma once
#include "ReplicatedObject.h"
#include <string>

namespace replication_engine {

class IReplicationEngine {
public:
    virtual ~IReplicationEngine() = default;

    /// Inicia la conexión a OpenDDS y ejecuta la secuencia de arranque.
    /// Lanza std::runtime_error si la inicialización falla.
    virtual void start() = 0;

    /// Detiene todos los hilos y cierra conexiones de forma ordenada.
    virtual void stop() = 0;

    /// Notifica al motor que DDM creó o modificó un objeto local.
    /// El objeto será persistido y publicado en la red de forma concurrente.
    virtual void onLocalObjectUpserted(const ReplicatedObject& obj) = 0;

    /// Notifica al motor que DDM eliminó un objeto local.
    /// El borrado será persistido y propagado a la red.
    virtual void onLocalObjectDeleted(const std::string& guid) = 0;

    /// DDM solicita reconexión del transporte DDS tras TRANSPORT_FAILED.
    /// Encola ReconnectTransportEvent; el Worker Thread ejecuta disconnect()+connect(). Ver ADR-009, ADR-011.
    virtual void reconnectTransport() = 0;

    /// DDM solicita reinicialización de SQLite tras STORAGE_FAILED.
    /// Encola ReinitStorageEvent; el Worker Thread recrea la BD y ejecuta startup sequence. Ver ADR-010, ADR-011.
    virtual void reinitializeStorage() = 0;
};

} // namespace replication_engine
