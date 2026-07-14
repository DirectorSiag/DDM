#ifndef STUBREPLICATIONENGINE_H
#define STUBREPLICATIONENGINE_H

#include "iReplicationEngine.h"

/*
 * StubReplicationEngine
 *
 * Implementación de desarrollo de IReplicationEngine: el .so real de SiOp
 * no existe aún. Loguea cada evento recibido para poder verificar el flujo
 * de subida DDM → RE. Se reemplaza por la librería real sin tocar DDM.
 */
class StubReplicationEngine : public replication_engine::IReplicationEngine {
public:
    void start() override;
    void stop() override;
    void onLocalObjectUpserted(const ReplicatedObject& obj) override;
    void onLocalObjectDeleted(const std::string& guid) override;
    void reconnectTransport() override;
    void reinitializeStorage() override;
};

#endif // STUBREPLICATIONENGINE_H
