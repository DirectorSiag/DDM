#ifndef STUBREPLICATIONENGINE_H
#define STUBREPLICATIONENGINE_H

#include "ReplicationEngine/IReplicationEngine.h"

/*
 * StubReplicationEngine
 *
 * Implementación de desarrollo de IReplicationEngine: la librería real de SiOp
 * (enlace estático, ADR-001) todavía no se instancia en este árbol. Loguea cada
 * evento recibido para poder verificar el flujo de subida DDM → RE. Se reemplaza
 * por replication_engine::ReplicationEngine sin tocar el resto de DDM.
 */
class StubReplicationEngine : public replication_engine::IReplicationEngine {
public:
    void start() override;
    void stop() override;
    void onLocalObjectUpserted(const replication_engine::ReplicatedObject& obj) override;
    void onLocalObjectDeleted(const std::string& guid) override;
    void reconnectTransport() override;
    void reinitializeStorage() override;
};

#endif // STUBREPLICATIONENGINE_H
