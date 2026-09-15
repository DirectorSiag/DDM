# Plan — Implementación de ReplicationListener + conexión directa con TrackService

**Fecha:** 2026-07-14
**Estado:** Pendiente de aprobación
**Alcance:** Dirección RE → DDM (`onInjectObject` / `onRemoveObject`) con conexión directa a `TrackService`. Refactor de desacople queda para más adelante.

---

## 1. Problemas encontrados en el código actual

1. **`ReplicatedObject.h` hereda de `QObject`** — contradice el ICD §6.1 (struct plano `ReplicatedObjectStruct`) y rompe el patrón de marshal del ICD §9.4: `QObject` no es copiable y la lambda del marshal necesita copiar el objeto (`[this, obj]`). Además falta el `;` final de la clase, faltan includes (`<string>`, `<cstdint>`) y los miembros son privados sin accesores.
2. **Struct duplicado** — `replicationListener.h:19` define `ReplicatedObjectStruct` y `ReplicatedObject.h` define `ReplicatedObject` con los mismos campos. Se debe quedar uno solo.
3. **Errores de compilación en `replicationListener.h`** — el constructor está declarado como `explicit ReplicationBridge()` dentro de la clase `ReplicationListener` (línea 34); faltan los `override`; el `.cpp` solo implementa `onInjectObject` y deja 5 métodos puros sin implementar → clase abstracta, no instanciable.
4. **`iReplicationListener.h` se incluye a sí mismo** (línea 3) y el destructor se llama `~IReplicationListener()` (I mayúscula) que no coincide con la clase `iReplicationListener` → no compila. Además hereda `QObject` cuando el ICD §9.3 la define como interfaz abstracta pura.
5. **`networkStatus.h` está vacío** — según ICD §7.1 debe contener el enum `NetworkStatus` con los 5 estados.
6. **No existe mapping `guid → trackId`** — `Track` solo tiene `id` entero y `TrackService` opera por id. Se necesita un `std::unordered_map<std::string, int>` (por ahora en `ReplicationListener`).
7. **No hay formato definido para `json_payload` ni deserializador** — `serializeTracks()` define un formato de salida pero no existe el parse inverso.

---

## 2. Decisiones tomadas (aprobadas por Cristian, 2026-07-14)

| # | Decisión | Resultado |
|---|----------|-----------|
| 1 | ¿`ReplicatedObject` struct plano o QObject? | **Struct plano** (copiable, según ICD §6.1). Se elimina el duplicado de `replicationListener.h`. |
| 2 | Formato del `json_payload` | **Mismo formato que `serializeTracks()`** (claves: `type`, `identity`, `lat`, `lon`, `velocidad`, `rumbo`, `info`, ...). Un solo schema para todo el sistema. |
| 3 | ¿Marshal Qt desde ahora? | **Sí** — `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` en cada método, según ICD §9.4. Inocuo si se llama desde el mismo hilo Qt. |

Notas de mapeo al deserializar:
- `lon → TrackCreateRequest.x`, `lat → TrackCreateRequest.y` (inverso de `serializeTracks()` que emite `lat` desde `getY()` y `lon` desde `getX()`).
- `type` / `identity` vienen como strings (`TrackData::toQString`) → se necesita conversión string → enum (verificar si ya existe en `TrackData`, si no, agregarla).

---

## 3. Tareas

### Tarea 1 — Sanear headers base
**Entrada:** ICD §6.1 y §7.1.
- `ReplicatedObject.h`: struct plano copiable, miembros públicos, includes `<string>` y `<cstdint>`, punto y coma final.
- `iReplicationListener.h`: quitar auto-include, corregir destructor (`~iReplicationListener`), quitar herencia `QObject` (interfaz pura).
- `networkStatus.h`: agregar enum `NetworkStatus` con los 5 estados del ICD §7.1 (`DISCONNECTED=0`, `CONNECTED=1`, `SYNCING=2`, `TRANSPORT_FAILED=3`, `STORAGE_FAILED=4`).

### Tarea 2 — Corregir clase `ReplicationListener`
- Eliminar el struct duplicado `ReplicatedObjectStruct`.
- Corregir nombre del constructor (`ReplicationListener()`).
- Agregar `override` a los 6 métodos.
- Herencia: `class ReplicationListener : public QObject, public iReplicationListener` (el `QObject` va en la clase concreta para poder hacer el marshal, no en la interfaz).

### Tarea 3 — Conexión directa con `TrackService`
- Constructor recibe `CommandContext*` y crea su `TrackService` interno (mismo patrón que `TrackCommandHandler`).
- Agregar miembro privado `std::unordered_map<std::string, int> m_guidToTrackId`.

### Tarea 4 — Implementar `onInjectObject`
**Entrada:** `ReplicatedObject` con `guid` + `json_payload` (formato `serializeTracks()`).
```cpp
void ReplicationListener::onInjectObject(const ReplicatedObject& obj) override {
    QMetaObject::invokeMethod(this, [this, obj]() {
        // hilo Qt: parsear json_payload con QJsonDocument
        //   guid nuevo    → TrackCreateRequest + createTrack() + registrar guid→id
        //   guid conocido → findTrackById() + setters (silencioso, sin eco — ICD §5)
    }, Qt::QueuedConnection);
}
```

### Tarea 5 — Implementar `onRemoveObject`
- Marshal al hilo Qt.
- Buscar guid en el mapa → `deleteTrackById()` + borrar entrada del mapa.
- Si el guid no existe: solo log diagnóstico, sin error (idempotente — ICD §5).

### Tarea 6 — Stubs de los 4 métodos restantes
- `onClearAllObjects`, `onSnapshotProgress`, `onSnapshotCompleted`, `onNetworkStatusChanged` con `qDebug` para que la clase sea instanciable.

### Tarea 7 — Compilar y verificar
- Build completo del proyecto para confirmar que el módulo queda sano.

---

## 4. Fuera de alcance (para después)

- Dónde se instancia y registra el `ReplicationListener` (main / CommandContext).
- Dirección DDM → RE (`onLocalObjectUpserted` / `onLocalObjectDeleted`, interfaces `iReplicationEngine` / `iReplicationBridge`).
- Refactor para desacoplar la conexión directa con `TrackService`.

---

## 5. Diseño detallado — camino RE → DDM (aprobado 2026-07-14)

**Alcance de esta etapa:** solo recepción. No se toca nada del envío DDM → RE.

### 5.1 `ReplicatedObject.h` — struct plano

```cpp
#include <string>
#include <cstdint>

struct ReplicatedObject {
    std::string guid;              // UUID v4. PK en SQLite y en la red DDS.
    int         object_type = 0;   // Categoría (1 = Track, según ICD §6.2).
    int         source_console_id = 0;
    int64_t     last_updated = 0;  // Unix ms. LWW lo resuelve RE, DDM no compara.
    std::string json_payload;      // Formato = serializeTracks(). Máx 4 KB.
};
```

### 5.2 `networkStatus.h` — enum del ICD §7.1

```cpp
enum class NetworkStatus : int {
    DISCONNECTED = 0, CONNECTED = 1, SYNCING = 2,
    TRANSPORT_FAILED = 3, STORAGE_FAILED = 4,
};
```

### 5.3 `iReplicationListener.h` — interfaz pura

Sin `QObject`, sin auto-include, destructor corregido (`~iReplicationListener`).
Los 6 métodos puros quedan igual que en el ICD §9.3.

### 5.4 `replicationListener.h` — clase concreta

```cpp
class ReplicationListener : public QObject, public iReplicationListener {
    Q_OBJECT
public:
    explicit ReplicationListener(CommandContext* context, QObject* parent = nullptr);
    // ... los 6 métodos con override ...
private:
    void injectTrack(const ReplicatedObject& obj);   // corre en hilo Qt
    void removeTrack(const std::string& guid);       // corre en hilo Qt

    CommandContext* m_context;
    std::unique_ptr<TrackService> m_trackService;
    std::unordered_map<std::string, int> m_guidToTrackId;
};
```

**Decisión — mapa guid ↔ trackId (2026-07-14):** el mapa vive y se mantiene en
`ReplicationListener`, porque el guid es ajeno a la lógica del objeto táctico.
`Track` no conoce su guid. Ciclo de vida del mapa:
- `onInjectObject` con guid nuevo → se agrega la entrada `guid → trackId`.
- `onRemoveObject` → se borra la entrada.
- `onClearAllObjects` → se vacía el mapa completo.

Limitación aceptada: los tracks creados localmente por el operador no están en el
mapa. Se resolverá en la etapa de envío DDM → RE.

### 5.5 Lógica de `onInjectObject`

1. Marshal al hilo Qt (`QMetaObject::invokeMethod` + `Qt::QueuedConnection`), copiando `obj`.
2. Si `obj.object_type != 1` (Track) → `qDebug` y descartar (por ahora solo tracks).
3. Parsear `json_payload` con `QJsonDocument`; si es inválido → log de error y descartar.
4. Buscar `guid` en `m_guidToTrackId`:
   - **No existe** (o el id mapeado ya no está en el contexto) → armar `TrackCreateRequest`
     (mismo patrón que `addCommand.cpp:328`), llamar `createTrack()`, registrar `guid → trackId`.
   - **Existe** → `findTrackById()` + setters de `Track` (`setX/setY/setIdentity/
     setVelocidadDmPerHour/setCursoInt/setInformacionAmpliatoria/setEstadoLinkY`),
     y recalcular PPP con `TrackPppService` si hay OwnShip (igual que `createTrack`).
5. Silencioso: **nunca** dispara eventos hacia RE (anti-eco, ICD §5).

Mapeo JSON → request (inverso de `serializeTracks()`):

| Clave JSON | Campo destino | Conversión |
|------------|--------------|------------|
| `type` | `request.type` | `TrackData::tryParseType()` (ya existe) |
| `identity` | `request.identity` | `tryParseIdentity()` — **nuevo helper en enums.h** |
| `lon` | `request.x` | directo |
| `lat` | `request.y` | directo |
| `velocidad` | `request.speedDmPerHour` | directo (0..99.9) |
| `rumbo` | `request.courseDeg` | directo (0..359) |
| `link` | `request.linkY` | char R/C/T/S → enum; `--` → Invalid |
| `info` | `request.info` | directo |
| `azimut`, `distancia`, `ppp_*`, `type_bits`, `id` | — | **ignorados** (derivados o locales; PPP se recalcula) |

### 5.6 Lógica de `onRemoveObject`

Marshal → buscar guid en el mapa → `deleteTrackById()` + borrar del mapa.
Guid desconocido: solo `qDebug`, sin error (idempotente, ICD §5).

### 5.7 `onClearAllObjects` — implementado (no stub)

Es parte del camino de recepción (repoblación por Snapshot, ICD §5):
marshal → `deleteTrackById()` por cada entrada del mapa → limpiar el mapa.
No dispara ObjectDeleted (silencioso).

### 5.8 Stubs con `qDebug`

`onSnapshotProgress`, `onSnapshotCompleted`, `onNetworkStatusChanged` — solo log.
El feedback visual al operador se hará cuando se conecte la UI.

### 5.9 Cambio en `enums.h`

Agregar `TrackData::tryParseIdentity(const QString&, Identity&)` (mismo estilo que
`tryParseType`, usando los nombres del enum: Pending, PossHostile, etc.).

### 5.10 Limitación conocida del formato de payload

`serializeTracks()` **no** incluye `mode`, `fc`, `asgc`, `priv` ni `link14`, por lo
que esos campos no viajan por replicación en esta etapa. Si más adelante deben
replicarse, hay que extender `serializeTracks()` (afecta a todos los consumidores
del JSON) o versionar el schema.

---

## 6. Diseño final implementado (2026-07-14) — reemplaza §5.4–§5.8

El diseño evolucionó durante la implementación. Cambios clave respecto a §5:

**Comunicación por signals/slots** (en vez de `QMetaObject::invokeMethod`):
el marshal al hilo Qt lo resuelve la conexión `Qt::QueuedConnection`.

**Separación de responsabilidades:**
- `ReplicationListener` = lógica de comunicación: desempaqueta el Envelope, valida
  `object_type` y JSON, convierte el payload a `TrackCreateRequest` y rutea con un
  `switch` por `object_type` que emite la señal correspondiente (tipos futuros:
  `areaReceived`, `cursorReceived`...). Sin estado, sin referencias a DDM.
- `TrackService` = dominio: pasa a heredar `QObject`, recibe por slots, mantiene el
  mapa `guid → trackId` (el guid es clave opaca que no interpreta).

**Alcance de esta iteración: solo alta y baja.** El update replicado (guid ya
conocido) queda para la próxima iteración — hoy se descarta con log.

**Señales / slots:**

| ReplicationListener (señal) | TrackService (slot) |
|-----------------------------|---------------------|
| `trackReceived(QString guid, TrackCreateRequest)` | `onReplicatedTrackCreate` → `createTrack()` + registra mapa |
| `trackRemoved(QString guid)` | `onReplicatedTrackRemoved` → mapa → `deleteTrackById()` (idempotente) |
| `clearAllReceived()` | `onReplicatedClearAll` → borra replicados + vacía mapa |

**Refactor interno de TrackService** (reuso, sin duplicación): se extrajeron de
`createTrack()` los helpers privados `validateRequest()` y `applyRequestOverrides()`.

**Otros detalles:**
- `Q_DECLARE_METATYPE(TrackCreateRequest)` en trackservice.h +
  `qRegisterMetaType` en el ctor del listener (necesario para conexiones encoladas).
- Enum `ReplicationData::ObjectType { Track=1, Cursor=2, Area=3 }` en enums.h.
- `tryParseIdentity()` agregado a `TrackData` en enums.h.
- §5.8 (onSnapshotProgress/Completed/NetworkStatusChanged) quedaron como cuerpos
  vacíos, pendientes de diseño.
- Build verificado con qmake6/Qt6: compila y linkea sin errores.

**Pendiente próxima iteración:** update replicado (slot separado del create, por
decisión de diseño), conexión/instanciación (quién crea listener y service y hace
los `connect`), señales 5.8, envío DDM → RE.

---

## 7. Iteración 2 — Subida DDM → RE (aprobada 2026-07-14, llamada directa)

**Alcance:** espejo de la bajada — solo tracks, solo alta (`ObjectAdded`) y baja
(`ObjectDeleted`). Sin `ObjectModified` (irá con el update en la iteración 3).

**Mecanismo (ICD §4 y §9.2):** la subida es una **llamada directa desde el hilo Qt** a
los métodos de `iReplicationEngine`, que encolan en la cola thread-safe de RE y retornan
inmediatamente. Sin marshal ni señales en esta dirección (el marshal solo era necesario
en la bajada, donde RE llama desde su Worker Thread).

```
bajada:  RE → ReplicationListener (deserializa) --señales Qt--> TrackService (dominio)
subida:  TrackService --llamada directa--> iReplicationEngine (stub) [encola y retorna]
```

### Decisiones tomadas (2026-07-14)

| # | Decisión | Resultado |
|---|----------|-----------|
| 1 | Mecanismo de subida | **Llamada directa** (descartado un ReplicationBridge QObject con señales: el ICD no lo pide y agregaba un salto de cola innecesario). TrackService tiene asociado un `iReplicationEngine*` no-owning por setter. Clase contenedora del engine: otra iteración. |
| 2 | RE real no existe | `StubReplicationEngine` que implementa `iReplicationEngine` y loguea los Envelopes. |
| 3 | Instancia única de TrackService | **Obligatoria** (mapa guid→id y conexiones Qt son por-objeto). Dueño: `main`. `CommandContext` lleva puntero no-owning `TrackService* trackService` — solo acceso, no ownership (patrón `ITransport* transport`). |
| 4 | TrackService = puerta única de tracks | Todo el que crea/borra tracks pasa por TrackService. CommandContext queda como estado puro. |
| 5 | Timestamp | `last_updated` = `QDateTime::currentMSecsSinceEpoch()` en TrackService en el momento del evento (ICD §2.2). |
| 6 | Guid local | TrackService genera `QUuid::createUuid()` (UUID v4, ICD §2.2) en cada alta local y lo registra en el mapa — el mapa pasa a contener TODOS los tracks (el delete local necesita encontrar su guid). |

### Anti-eco (crítico, ICD §4)

`onReplicatedTrackCreate` llama hoy a `createTrack()`. Si `createTrack()` publicara al
engine, cada track replicado se re-publicaría a la red (tormenta de broadcast).
Solución estructural: caminos internos sin publicación — `createTrack()` público =
interno + publish; los slots replicados usan solo el interno. Ídem delete.

### Tareas

1. **`iReplicationEngine.h` real** (entrada: ICD §4/§9.2): interfaz pura sin Qt con
   `onLocalObjectUpserted(const ReplicatedObject&)`, `onLocalObjectDeleted(const std::string&)`,
   `reconnectTransport()`, `reinitializeStorage()` (los dos últimos solo declarados).
2. **`StubReplicationEngine`** (`src/replicationEngine/`, + alta en DDM.pro): loguea
   cada evento (guid, type, timestamp, tamaño del payload).
3. **TrackService publicador**:
   - Extraer `serializeTrack(const Track&)` de `serializeTracks()`.
   - `iReplicationEngine* m_replicationEngine = nullptr` + `setReplicationEngine()`;
     `m_consoleId` provisorio 0 (TODO configuración).
   - `createTrack`: interno + guid nuevo + mapa + publicar Envelope (object_type=Track,
     console_id, timestamp, payload compacto; qWarning si > 4 KB).
   - `deleteTrackById`: interno + guid por búsqueda inversa en el mapa +
     `onLocalObjectDeleted(guid)` + borrar entrada.
   - Slots replicados → caminos internos sin publicación (anti-eco).
   - Sin engine seteado: operación solo local, sin error.
4. **CommandContext**: `TrackService* trackService = nullptr` (fwd decl, solo acceso).
5. **Wiring en `main`**: crear TrackService + StubReplicationEngine + ReplicationListener,
   `setReplicationEngine`, `ctx->trackService`, y los 3 `connect()` de la bajada.
6. **Migrar a la puerta única**: addCommand, deleteCommand, listCommand,
   estacionamientoservice, TrackCommandHandler (elimina su `unique_ptr` propio).
   Bypasses: sitrepservice.cpp:16 y qek.h:79/103 migran también (si QEK necesita campos
   que TrackCreateRequest no tiene, queda TODO documentado). **Excepción:**
   ownshipservice.cpp:31 directo a propósito (OwnShip id 0 no se replica).
7. **Compilar (qmake6) + prueba por stdin**: alta y baja → verificar Envelopes en el
   log del stub. Anti-eco se verifica por inspección (sin RE real no hay inyecciones).

### Puntos abiertos menores

- `source_console_id`: sin configuración de consola aún; provisorio 0 con TODO.

### Resultado (2026-07-14) — IMPLEMENTADO y verificado

Prueba end-to-end por stdin con el stub (`QT_FORCE_STDERR_LOGGING=1`):

```
add -f 10 20  →  [StubRE] onLocalObjectUpserted guid:"133034e9-..." type:1
                 console:0 last_updated:1784070432246 payload(335 bytes, formato serializeTracks)
delete 1      →  [StubRE] onLocalObjectDeleted guid:"133034e9-..."   ← mismo guid: el mapa resolvió bien
```

Ajustes respecto al plan: `iReplicationEngine.h` lo completó Cristian con el contrato
completo (namespace `replication_engine`, `IReplicationEngine` con `start()`/`stop()`).
El anti-eco quedó implementado desde ahora (createTrackInternal/deleteTrackInternal sin
publicación, usados por los slots replicados) tras revisar el riesgo de tormenta.
TrackCommandHandler, sitrepservice y QEK migrados a la puerta única; ownshipservice
queda directo a propósito.
