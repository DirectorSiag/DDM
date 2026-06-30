#pragma once
#include <cstdint>
#include <string>

struct ReplicatedObjectStruct {
    std::string guid;
    int         object_type       = 0;
    int         source_console_id = 0;
    int64_t     last_updated      = 0;
    std::string json_payload;
};

// Alias por si el header de OS usa el nombre corto (ICD §9.3 lo llama ReplicatedObject).
using ReplicatedObject = ReplicatedObjectStruct;
