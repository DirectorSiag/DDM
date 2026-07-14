#pragma once
// Contrato ICD §9.3 — interfaz que DDM implementa para recibir eventos de RE.
// ⚠️ Todos los métodos se invocan desde el Worker Thread de RE (ICD §9.4):
// la implementación DEBE hacer marshal al hilo Qt antes de tocar QObjects.
// Stub local: espeja el header público que entregará SiOp con el .so real.

#include <string>

#include "ReplicatedObject.h"

namespace replication_engine {

class IReplicationListener {
public:
    virtual ~IReplicationListener() = default;

    // RE → DDM: objeto recibido de la red o repoblación desde disco
    virtual void onInjectObject(const ReplicatedObject& obj)    = 0;

    // RE → DDM: borrado recibido de la red
    virtual void onRemoveObject(const std::string& guid)        = 0;

    // RE → DDM: limpiar estado antes de repoblar desde Snapshot
    virtual void onClearAllObjects()                            = 0;

    // RE → DDM: progreso de recepción de Snapshot
    virtual void onSnapshotProgress(int current, int total)     = 0;

    // RE → DDM: sincronización completa — habilitar UI
    virtual void onSnapshotCompleted()                          = 0;

    // RE → DDM: cambio de estado de red (valores de NetworkStatus)
    virtual void onNetworkStatusChanged(int status)             = 0;
};

} // namespace replication_engine
