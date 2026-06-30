# Módulo: ReplicationBridge

## Descripción general

ReplicationBridge es el mediador único entre el DDM y ReplicationEngine (RE). Es el único punto del sistema con responsabilidades hacia ambos lados de la frontera de replicación. Su responsabilidad no es de validación de información: solo pone a disposición del ReplicationEngine los objetos que el sistema DDM quiere compartir.

**Contrato formal:** definido en `docs/ICD.md` (v2.0). DDM implementa `IReplicationListener` para recibir eventos de RE; RE expone `IReplicationBridge` para que DDM emita eventos locales.

Solo los **Tracks seleccionados** cruzan la frontera hacia RE. Áreas, círculos, polígonos y cursores son objetos transitorios que permanecen exclusivamente en memoria DDM.

## Lista de archivos y clases

| Archivo | Clase/Struct | Responsabilidad |
|---|---|---|
| `src/replication/replicationBridge.h/.cpp` | `ReplicationBridge` | Clase principal. Gestiona las dos direcciones del flujo de replicación. |
| `src/replication/replicatedObject.h` | `ReplicatedObjectStruct` | Envelope de intercambio entre DDM y RE. |
| `src/replication/iReplicationListener.h` | `IReplicationListener` | Contrato ICD: interfaz que RE invoca para notificar a DDM. Implementada por `ReplicationBridge`. |
| `src/replication/iReplicationBridge.h` | `IReplicationBridge` | Contrato ICD: interfaz que RE expone para que DDM le envíe objetos. Stub hasta que el `.so` esté disponible. |

## Clase principal: ReplicationBridge

`ReplicationBridge` implementa `IReplicationListener` (entrada desde RE) y mantiene una referencia a `IReplicationBridge` (salida hacia RE). Es la única clase del módulo con lógica propia.

- **Herencia:** `public QObject`, `public IReplicationListener`

### Dirección de entrada — RE llama a ReplicationBridge

Métodos del contrato `IReplicationListener` que RE invoca desde su Worker Thread. Cada uno hace marshal al Qt main thread antes de tocar `CommandContext`.

| Método | Descripción |
|---|---|
| `onInjectObject(const ReplicatedObjectStruct& obj)` | Objeto recibido de red o repoblación desde SQLite. Deserializa payload e inserta o actualiza Track en CommandContext. |
| `onRemoveObject(const std::string& guid)` | Borrado propagado desde otro nodo. Elimina Track de CommandContext por guid. |
| `onClearAllObjects()` | Limpieza previa a Snapshot. Elimina todos los Tracks persistentes de CommandContext. |
| `onSnapshotProgress(int current, int total)` | Emite señal Qt `snapshotProgress` para barra de progreso en UI. |
| `onSnapshotCompleted()` | Emite señal Qt `snapshotCompleted`. Habilita la interacción completa en GUI. |
| `onNetworkStatusChanged(int status)` | Emite señal Qt `networkStatusChanged`. `0=Disconnected`, `1=Connected`, `2=Syncing`. |

> **Crítico:** Estos métodos se ejecutan en el Worker Thread de RE. Deben hacer marshal a Qt main thread vía `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` antes de tocar cualquier `QObject`.

### Dirección de salida — ReplicationBridge llama a RE

Métodos propios que `TrackService` invoca tras un cambio de origen local.

| Método | Descripción |
|---|---|
| `notifyObjectUpserted(const Track& track)` | Construye el Envelope (asigna guid si vacío, genera `last_updated`, serializa payload) y llama `m_re->onLocalObjectUpserted(obj)`. |
| `notifyObjectDeleted(const std::string& guid)` | Llama `m_re->onLocalObjectDeleted(guid)`. |

### Señales Qt

| Señal | Descripción |
|---|---|
| `snapshotProgress(int current, int total)` | Progreso de Snapshot para la UI. |
| `snapshotCompleted()` | Sincronización completa — habilitar GUI. |
| `networkStatusChanged(int status)` | Cambio de estado de red. |

### Constructor

```cpp
explicit ReplicationBridge(CommandContext* ctx, IReplicationBridge* re, QObject* parent = nullptr);
```

Recibe punteros a `CommandContext` y a RE (stub o `.so` real). No posee ninguno de los dos.

### Tipos internos

- `enum class ObjectType { Track = 1 }` — valores de `object_type` en el Envelope.

## Contrato con ReplicationEngine (ICD §9)

Las dos interfaces son el contrato formal definido por RE. DDM no las diseña: las implementa o las consume.

**`IReplicationListener`** — RE → DDM. Define los 6 métodos del bloque de entrada. `ReplicationBridge` los implementa.

**`IReplicationBridge`** — DDM → RE. Define `onLocalObjectUpserted` y `onLocalObjectDeleted`. `ReplicationBridge` llama estos métodos a través del puntero `m_re`. Hasta que el `.so` esté disponible, se usa un stub con implementaciones vacías que permite compilar el lado DDM de forma independiente.

## ReplicatedObjectStruct (Envelope)

Unidad de intercambio entre DDM y RE. RE lo trata como opaco excepto por los campos de metadata.

| Campo | Tipo C++ | Generado por | Descripción |
|---|---|---|---|
| `guid` | `std::string` | DDM (UUID v4) | PK en red y en SQLite. |
| `object_type` | `int` | DDM | Categoría del objeto (`1=Track`). Opaco para RE. |
| `source_console_id` | `int` | DDM | ID de la consola origen. Fuente: `Configuration::instance().meko`. |
| `last_updated` | `int64_t` | DDM | Unix timestamp en ms. Generado en el momento semántico del evento. RE lo usa para LWW. |
| `json_payload` | `std::string` | DDM | Estado táctico serializado. Opaco para RE. Máximo 4 KB (RNF-09). |

## GUID en Track

`Track` incorpora el campo `std::string m_guid`. El `int m_id` sigue siendo el índice local de visualización. `ReplicationBridge` asigna el guid en `notifyObjectUpserted` si está vacío:

```cpp
track.m_guid = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
```

## Echo prevention

`ReplicationBridge` distingue dos orígenes de cambio para evitar bucles de broadcast:

- **Local:** `TrackService` llama `notifyObjectUpserted/Deleted` → ReplicationBridge emite hacia RE.
- **Remoto:** los callbacks `onInjectObject / onRemoveObject / onClearAllObjects` mutan `CommandContext` **sin** llamar de vuelta a RE.

Los métodos `injectRemoteTrack` y `removeRemoteTrack` en `CommandContext` son las únicas rutas de escritura silenciosa. Ningún servicio llama `notifyObject*` desde esas rutas.

## Flujo de datos

### Salida — cambio local

```
TrackService::createTrack() / deleteTrack()
  └─> ReplicationBridge::notifyObjectUpserted(track) / notifyObjectDeleted(guid)
        ├─ asigna guid si vacío (QUuid)
        ├─ genera last_updated (QDateTime::currentMSecsSinceEpoch)
        ├─ serializa track a json_payload (vía JsonSerializer)
        ├─ construye ReplicatedObjectStruct
        └─> m_re->onLocalObjectUpserted(obj)   [enqueue RE, retorna inmediato]
```

### Entrada — inyección remota

```
[RE Worker Thread]
  └─> ReplicationBridge::onInjectObject(obj)
        └─ QMetaObject::invokeMethod(this, Qt::QueuedConnection, [obj]{
               CommandContext::injectRemoteTrack(obj)   // sin re-notificar RE
           })

[Qt main thread]
  └─> CommandContext::injectRemoteTrack(obj)
        ├─ deserializa json_payload
        ├─ si guid no existe → crea Track con ese guid y m_id asignado
        └─ si guid existe → actualiza propiedades
```

### Secuencia de arranque (ICD §8)

```
1. DDM: estado vacío.
2. DDM instancia ReplicationEngine, registra ReplicationBridge como listener.
3. RE lee SQLite → llama onInjectObject por cada objeto → estado previo restaurado en memoria.
4. RE conecta DDS. Espera 2s descubrimiento de peers.
   [Con peers]  onSnapshotProgress → onClearAllObjects → onInjectObject (por lotes) → onSnapshotCompleted
   [Sin peers]  onSnapshotCompleted directamente.
5. DDM habilita interacción completa en GUI.
```

## Manejo de errores

- `onRemoveObject` con guid inexistente: operación completa sin error, solo log diagnóstico.
- `onClearAllObjects` con estado vacío: no-op.
- `json_payload` malformado en `onInjectObject`: log de error, objeto descartado silenciosamente.
- `notifyObjectUpserted` con `m_re == nullptr`: log de warning, no crashea.
- Marshal fallido desde Worker Thread: log crítico.

## Módulos relacionados

- `docs/ICD.md` — contrato formal completo DDM ↔ ReplicationEngine
- `docs/architecture.md`
- `docs/modules/command-context.md`
- `docs/modules/services.md`
