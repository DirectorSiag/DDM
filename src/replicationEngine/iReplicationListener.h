#ifndef IREPLICATIONLISTENER_H
#define IREPLICATIONLISTENER_H

#include <string>

#include "ReplicatedObject.h"

/*
 * Interfaz RE → DDM (ICD §9.3). Interfaz abstracta pura, sin Qt:
 * la define ReplicationEngine y DDM la implementa.
 *
 * Todos los métodos se invocan desde el Worker Thread de RE:
 * la implementación debe hacer marshal al hilo Qt (ICD §9.4).
 */
class iReplicationListener {
public:
    virtual ~iReplicationListener() = default;

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

    // RE → DDM: cambio de estado de red
    virtual void onNetworkStatusChanged(int status)             = 0;
};

#endif // IREPLICATIONLISTENER_H
