#ifndef REPLICATEDOBJECT_H
#define REPLICATEDOBJECT_H

#include <cstdint>
#include <string>

/*
 * Envelope intercambiado entre DDM y ReplicationEngine (ICD §6.1).
 * Struct plano y copiable: el marshal al hilo Qt copia el objeto
 * dentro de la lambda (ICD §9.4).
 */
struct ReplicatedObject {
    std::string guid;                  // UUID v4. PK en SQLite y en la red DDS.
    int         object_type = 0;       // Categoría (ReplicationData::ObjectType). Opaco para SiOp.
    int         source_console_id = 0; // ID de la consola emisora.
    int64_t     last_updated = 0;      // Timestamp Unix (ms). Usado por LWW en RE.
    std::string json_payload;          // Estado táctico completo. Opaco para SiOp. Máx. 4 KB.
};

#endif // REPLICATEDOBJECT_H
