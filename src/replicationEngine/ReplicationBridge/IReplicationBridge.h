#pragma once
// Contrato ADR-011 — canal de notificaciones RE → DDM.
// Tras la Opción C de ADR-011 esta interfaz queda reducida a:
// registerListener() + los seis notify*() (espejo 1:1 de IReplicationListener).
// Stub local: espeja el header público que entregará SiOp con el .so real.

#include <string>

#include "ReplicatedObject.h"

namespace replication_engine {

class IReplicationListener;

class IReplicationBridge {
public:
    virtual ~IReplicationBridge() = default;

    // DDM registra su listener durante el arranque, antes de cualquier evento.
    virtual void registerListener(IReplicationListener* listener)   = 0;

    // Invocados por RE (Worker Thread) — reenvían al listener registrado.
    virtual void notifyInjectObject(const ReplicatedObject& obj)    = 0;
    virtual void notifyRemoveObject(const std::string& guid)       = 0;
    virtual void notifyClearAllObjects()                            = 0;
    virtual void notifySnapshotProgress(int current, int total)     = 0;
    virtual void notifySnapshotCompleted()                          = 0;
    virtual void notifyNetworkStatusChanged(int status)             = 0;
};

} // namespace replication_engine
