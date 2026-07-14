#ifndef REPLICATIONLISTENER_H
#define REPLICATIONLISTENER_H

#include <QObject>

#include <string>

#include "iReplicationListener.h"
#include "trackservice.h"

/*
 * ReplicationListener
 *
 * Implementación DDM de iReplicationListener (ICD §9.3).
 * Responsable de la lógica de comunicación RE → DDM: desempaqueta el
 * Envelope, valida object_type y json_payload, convierte el payload a
 * tipos de dominio (TrackCreateRequest) y rutea por señales.
 *
 * El marshal al hilo Qt (ICD §9.4) lo resuelve la conexión: los slots
 * receptores deben conectarse con Qt::QueuedConnection, por ejemplo:
 *
 *   connect(listener, &ReplicationListener::trackReceived,
 *           trackService, &TrackService::onReplicatedTrackCreate,
 *           Qt::QueuedConnection);
 *   connect(listener, &ReplicationListener::trackRemoved,
 *           trackService, &TrackService::onReplicatedTrackRemoved,
 *           Qt::QueuedConnection);
 *   connect(listener, &ReplicationListener::clearAllReceived,
 *           trackService, &TrackService::onReplicatedClearAll,
 *           Qt::QueuedConnection);
 */
class ReplicationListener : public QObject, public iReplicationListener {
    Q_OBJECT

public:
    explicit ReplicationListener(QObject* parent = nullptr);

    // RE → DDM: objeto recibido de la red o repoblación desde disco
    void onInjectObject(const ReplicatedObject& obj) override;

    // RE → DDM: borrado recibido de la red
    void onRemoveObject(const std::string& guid) override;

    // RE → DDM: limpiar estado antes de repoblar desde Snapshot
    void onClearAllObjects() override;

    // RE → DDM: progreso de recepción de Snapshot
    void onSnapshotProgress(int current, int total) override;

    // RE → DDM: sincronización completa — habilitar UI
    void onSnapshotCompleted() override;

    // RE → DDM: cambio de estado de red
    void onNetworkStatusChanged(int status) override;

signals:
    void trackReceived(const QString& guid, const TrackCreateRequest& request);
    void trackRemoved(const QString& guid);
    void clearAllReceived();
};

#endif // REPLICATIONLISTENER_H
