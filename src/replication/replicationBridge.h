#pragma once
#include <QObject>
#include "iReplicationListener.h"
#include "iReplicationBridge.h"

struct CommandContext;
class Track;

class ReplicationBridge : public QObject, public IReplicationListener {
    Q_OBJECT

public:
    enum class ObjectType { Track = 1 };

    explicit ReplicationBridge(CommandContext* ctx,
                               IReplicationBridge* re,
                               QObject* parent = nullptr);

    // ── Entrada: RE → DDM (IReplicationListener) ─────────────────────────
    void onInjectObject(const ReplicatedObjectStruct& obj) override;
    void onRemoveObject(const std::string& guid)           override;
    void onClearAllObjects()                               override;
    void onSnapshotProgress(int current, int total)        override;
    void onSnapshotCompleted()                             override;
    void onNetworkStatusChanged(int status)                override;

    // ── Salida: DDM → RE ──────────────────────────────────────────────────
    void notifyObjectUpserted(const Track& track);
    void notifyObjectDeleted(const std::string& guid);

signals:
    void snapshotProgress(int current, int total);
    void snapshotCompleted();
    void networkStatusChanged(int status);

private:
    static std::string generateUuid();

    CommandContext*     m_ctx;
    IReplicationBridge* m_re;
};
