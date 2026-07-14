#pragma once
// Contrato ICD §6.1 — Envelope intercambiado entre DDM y ReplicationEngine.
// Stub local: espeja el header público que entregará SiOp con el .so real.
// Este archivo NO debe incluir Qt ni OpenDDS.

#include <cstdint>
#include <string>

namespace replication_engine {

struct ReplicatedObjectStruct {
    std::string guid;              // UUID v4. PK en SQLite y en la red DDS.
    int         object_type;       // Categoría del objeto. Opaco para SiOp.
    int         source_console_id; // ID de la consola emisora.
    int64_t     last_updated;      // Timestamp Unix (ms). Usado por LWW.
    std::string json_payload;      // Estado táctico completo. Opaco para SiOp. Máx. 4 KB.
};

using ReplicatedObject = ReplicatedObjectStruct;

} // namespace replication_engine
