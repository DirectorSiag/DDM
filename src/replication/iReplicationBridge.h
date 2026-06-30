#pragma once
#include "replicatedObject.h"
#include <string>

// Contrato ICD §9 — interfaz expuesta por ReplicationEngine (.so).
// DDM la llama desde Qt main thread; RE la implementa con encola thread-safe interna.
class IReplicationBridge {
public:
    virtual ~IReplicationBridge() = default;

    virtual void onLocalObjectUpserted(const ReplicatedObjectStruct& obj) = 0;
    virtual void onLocalObjectDeleted(const std::string& guid)             = 0;
};

// Stub usado hasta que el .so de RE esté disponible.
class ReplicationBridgeStub : public IReplicationBridge {
public:
    void onLocalObjectUpserted(const ReplicatedObjectStruct&) override {}
    void onLocalObjectDeleted(const std::string&)             override {}
};
