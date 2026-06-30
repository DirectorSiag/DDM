#pragma once
#include "replicatedObject.h"
#include <string>

// Contrato ICD §9.3 — RE invoca estos métodos desde su Worker Thread.
// Cada implementación debe hacer marshal a Qt main thread antes de tocar QObjects.
class IReplicationListener {
public:
    virtual ~IReplicationListener() = default;

    virtual void onInjectObject(const ReplicatedObjectStruct& obj) = 0;
    virtual void onRemoveObject(const std::string& guid)           = 0;
    virtual void onClearAllObjects()                               = 0;
    virtual void onSnapshotProgress(int current, int total)        = 0;
    virtual void onSnapshotCompleted()                             = 0;
    virtual void onNetworkStatusChanged(int status)                = 0;
};
