# Plan de Implementación: ReplicationBridge

## Estado general

| Fase | Descripción | Estado |
|---|---|---|
| 1 | Estructura base y contratos | ✅ Completo |
| 2 | Serialización / Deserialización | ✅ Completo |
| 3 | Integración con TrackService | ✅ Completo |
| 4 | Integración con comandos CLI | ✅ Completo (cubierta por Fase 3) |
| 5 | Sustitución del stub por el `.so` real | 🔲 Bloqueado — esperando entregables de OS |

---

## Fase 1 — Estructura base ✅

### Archivos creados

| Archivo | Descripción | Estado |
|---|---|---|
| `src/replication/replicatedObject.h` | `ReplicatedObjectStruct` — Envelope de intercambio | ✅ |
| `src/replication/iReplicationListener.h` | Interfaz pura (RE → DDM), 6 métodos | ✅ |
| `src/replication/iReplicationBridge.h` | Interfaz (DDM → RE) + `ReplicationBridgeStub` | ✅ |
| `src/replication/replicationBridge.h` | Declaración de `ReplicationBridge` | ✅ |
| `src/replication/replicationBridge.cpp` | Implementación con marshal y echo prevention | ✅ |
| `docs/modules/replication-bridge.md` | Documentación del módulo | ✅ |

### Modificaciones en código existente

| Archivo | Cambio | Estado |
|---|---|---|
| `src/model/entities/track.h` | Campo `m_guid`, `getGuid()`, `setGuid()` | ✅ |
| `src/model/entities/track.cpp` | Implementación de `getGuid` / `setGuid` | ✅ |
| `src/model/commandContext.h` | `injectRemoteTrack`, `removeRemoteTrack`, `clearAllPersistentTracks` | ✅ |
| `src/main.cpp` | Instancia `ReplicationBridgeStub` + `ReplicationBridge` | ✅ |
| `DDM.pro` | `src/replication/` en INCLUDEPATH, headers y fuente registrados | ✅ |

---

## Fase 2 — Serialización / Deserialización 🔲

Los métodos `notifyObjectUpserted` y `onInjectObject` tienen TODOs que deben resolverse.

### 2.1 — Generación de GUID ✅

- **Implementado en:** `replicationBridge.cpp` → `generateUuid()` + `notifyObjectUpserted`
- **Librería:** `QUuid` (Qt Core) — sin dependencia extra, ya incluido en el proyecto.
- **Implementación:** `QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString()` — genera UUID v4 via CSPRNG del sistema. `WithoutBraces` produce el formato estándar sin llaves: `xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx`.
- **Comportamiento:** Si `track.getGuid()` está vacío, se genera el UUID, se persiste en el Track de `CommandContext` vía `findTrackById`, y se usa en el Envelope. Modificaciones posteriores del mismo track reutilizan el mismo guid.

### 2.2 — Serialización de Track → `json_payload` ✅

- **Implementado en:** `JsonSerializer::serializeTrackForReplication(const Track&)` → `src/controller/json/jsonserializer.cpp`
- **Nota:** `serializeTracks()` de `TrackService` incluye datos derivados (azimut/distancia) y PPP transitorios que no se replican. El nuevo método serializa solo los campos base necesarios para reconstruir el Track en otra consola.
- **Campos serializados:** `guid`, `type`, `identity`, `mode`, `creation_environment`, `x`, `y`, `speed_dm_h`, `course_deg`, `info`, `asignacion_fc`, `codigo_asignacion`, `link_y`, `link_14`, `codigo_privado`.
- **Conversión a `std::string`:** `QJsonDocument(trackJson).toJson(QJsonDocument::Compact).toStdString()`

### 2.3 — Deserialización de `json_payload` → Track ✅

- **Implementado en:** `JsonSerializer::deserializeTrack(const QJsonObject&)` → `src/controller/json/jsonserializer.cpp`
- **Enums:** `type` y `creation_environment` usan `TrackData::tryParseType`. `identity` y `mode` usan `QMetaEnum::fromType` (son Q_ENUMs de Q_GADGET).
- **id local:** Se deja en `0` — `CommandContext::injectRemoteTrack` asigna el `nextTrackId` al insertar.
- **Manejo de errores:** Si `json_payload` no es un objeto JSON válido, se descarta con log diagnóstico sin crashear.

---

## Fase 3 — Integración con TrackService ✅

### Mecanismo elegido: callbacks en `CommandContext`

En lugar de pasar `ReplicationBridge*` a cada handler o comando, se agregaron dos callbacks opcionales a `CommandContext` — siguiendo el precedente de `ITransport* transport`. `TrackService` los dispara via `m_context`, cubriendo todas las rutas (JSON + CLI) sin modificar handlers ni comandos.

| Archivo | Cambio |
|---|---|
| `src/model/commandContext.h` | `std::function<void(const Track&)> onTrackUpserted` y `std::function<void(const std::string&)> onTrackDeleted` |
| `src/controller/services/trackservice.cpp` | `createTrack`: llama `onTrackUpserted(track)` tras crear. `deleteTrackById`: captura guid antes de borrar, llama `onTrackDeleted(guid)` si el track tenía guid. |
| `src/main.cpp` | Lambdas que conectan los callbacks con `replicationBridge->notifyObjectUpserted/Deleted`. |

**Echo prevention garantizado:** `injectRemoteTrack` / `removeRemoteTrack` / `clearAllPersistentTracks` en `CommandContext` no pasan por `TrackService`, por lo que los callbacks nunca se disparan en la ruta remota.

---

## Fase 4 — Integración con comandos CLI ✅ (cubierta por Fase 3)

`AddCommand` y `DeleteCommand` instancian `TrackService` con el mismo `CommandContext` que reciben en `execute(CommandInvocation, CommandContext& ctx)`. Como los callbacks `onTrackUpserted`/`onTrackDeleted` están seteados en ese `ctx` desde `main.cpp`, se disparan automáticamente sin ningún cambio adicional en los comandos.

---

## Fase 5 — Sustitución del stub por el `.so` real 🔲

Bloqueado hasta que el Área de Sistemas Operativos entregue la librería.

### 5.1 — Entregables necesarios de OS

| Entregable | Para qué |
|---|---|
| `libReplicationEngine.so` | Linkear en DDM |
| Header con `IReplicationBridge` | Confirmar nombres de métodos que DDM llama |
| Header con `IReplicationListener` | Confirmar que nuestras firmas son compatibles |
| Header con la definición del struct Envelope | Confirmar nombre del tipo (el ICD §9.3 lo llama `ReplicatedObject` sin `Struct`) |
| Función factory para crear la instancia RE | Ej. `createReplicationEngine()` |
| Método para registrar el listener | Ej. `re->setListener(IReplicationListener*)` |

### 5.2 — Riesgo de naming ya mitigado

El ICD §9.3 usa `ReplicatedObject` pero nuestro código usa `ReplicatedObjectStruct`. Se agregó el alias en `src/replication/replicatedObject.h`:
```cpp
using ReplicatedObject = ReplicatedObjectStruct;
```
Si el header de OS usa `ReplicatedObject`, compilará sin cambios adicionales.

### 5.3 — Cambios en `DDM.pro`

```qmake
INCLUDEPATH += /path/to/replication-engine/include
LIBS += -L/path/to/replication-engine/lib -lReplicationEngine
```

Si OS entrega sus propios headers de `IReplicationListener` e `IReplicationBridge`, eliminar `src/replication/iReplicationListener.h` e `iReplicationBridge.h` de `HEADERS` en el `.pro`.

### 5.4 — Cambios en `main.cpp` (solo 4 líneas)

```cpp
// Reemplazar:
auto *reStub = new ReplicationBridgeStub();
auto *replicationBridge = new ReplicationBridge(ctx, reStub, &app);

// Con:
IReplicationBridge* re = createReplicationEngine(); // factory del .so
auto *replicationBridge = new ReplicationBridge(ctx, re, &app);
// ... setear ctx->onTrackUpserted y ctx->onTrackDeleted ...
re->setListener(replicationBridge); // dispara SQLite repop + conecta DDS
```

**Orden crítico:** los callbacks de `ctx` deben estar seteados **antes** de `re->setListener(...)`.

### 5.5 — Verificación end-to-end

1. Compilar con el `.so` real — ajustar alias o firmas si hay errores de tipo.
2. Arrancar DDM solo → verificar `onSnapshotCompleted` en log (arranque aislado).
3. Arrancar segunda instancia → verificar `onSnapshotProgress` + `onSnapshotCompleted`.
4. Crear track en DDM-1 → verificar que aparece en DDM-2 vía `onInjectObject`.
5. Borrar track en DDM-1 → verificar que desaparece en DDM-2 vía `onRemoveObject`.
6. Verificar echo prevention: el track creado en DDM-1 no vuelve como `onInjectObject` a DDM-1.

---

## Errores preexistentes (no relacionados con ReplicationBridge)

Errores de compilación que existían antes de esta implementación:

| Archivo | Error | Causa |
|---|---|---|
| `src/main.cpp` | `DWORD`/`HANDLE` undeclared | `enableAnsiColorsOnWindows()` fuera del `#ifdef Q_OS_WIN` |
| `src/model/network/udpClientAdapter.cpp` | `UdpClientAdapter.h` not found | Mismatch de mayúsculas en nombre de archivo (Linux es case-sensitive) |
| `src/model/network/transportFactory.cpp` | `TransportFactory.h` not found | Idem |
| `src/model/network/localipcclient.cpp` | `LocalIpcClient.h` not found | Idem |
| `src/main.cpp:176` | Type mismatch en señal `newHandWheel` | `QPair<float,float>` vs `QPair<qfloat16,qfloat16>` |
