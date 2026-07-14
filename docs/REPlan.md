# REPlan — Plan de implementación de ReplicationBridge

**Rama:** REBridge
**Fecha:** 2026-07-13
**Contrato:** `docs/modules/ICD.md` v2.0 + `docs/modules/ADR-011-callbackbridge-recovery-routing.md`

Reimplementación desde cero del módulo que conecta DDM con ReplicationEngine (RE),
librería `.so` del área SiOp que replica objetos tácticos entre consolas (DDS) y los
persiste (SQLite). El `.so` real aún no está disponible → se implementan stubs que
espejan los headers públicos que SiOp entregará.

---

## Decisiones cerradas

| # | Decisión | Detalle |
|---|----------|---------|
| D1 | `m_guid` en `Track` como `QString` | Consistente con los demás strings del modelo. La conversión a `std::string` ocurre solo al construir el Envelope, en un único punto del bridge. El ICD exige `std::string` solo en `ReplicatedObjectStruct`. |
| D2 | **ObjectModified fuera de alcance** | Solo se implementan ObjectAdded y ObjectDeleted salientes. El entrante `onInjectObject` sí hace create-or-update (obligatorio por contrato ICD §5). |
| D3 | **OwnShip (track id 0) NO se replica** | Se crea vía `OwnShipService::syncOwnShipVirtualTrack` sin guid; guards adicionales en los puntos de publicación. |
| D4 | **Payload de red = JSON nuevo, distinto del JSON de UI** | Se construye dentro del módulo ReplicationBridge (`TrackNetSerializer`). El JSON de UI (`TrackService::serializeTracks`) es solo para el frontend local y no se toca. |
| D5 | **`Track` es el único tipo persistente en esta primera iteración**, pero el diseño queda preparado para más tipos | A futuro se compartirán también figuras (áreas, círculos, polígonos, sectores). Por eso: el `object_type` del Envelope discrimina el tipo (1=Track; 2..N reservados para figuras), `injectRemoteObject` despacha por `object_type` (hoy descarta ≠1 con log), y agregar un tipo nuevo = nuevo serializer de red + nuevo case de despacho + hook en su service, sin tocar el bridge ni el contrato. Cursores y elementos de UI siguen siendo transitorios (ICD §3.1). |
| D6 | Hook saliente vía puntero en `CommandContext` | `ReplicationBridge* replicationBridge = nullptr;` — mismo patrón que `ITransport* transport`. Evita tocar los ≥5 sitios que instancian `TrackService` ad-hoc. |
| D7 | Anti-eco estructural (ICD §2.3) | El flujo entrante escribe en `CommandContext` directamente, nunca vía `TrackService`; el saliente solo existe en `TrackService`. El bucle es imposible por construcción. |
| D8 | `ReplicationBridge : QObject + IReplicationListener`, clase separada | `CommandContext` es un struct plano sin QObject; el ICD §9.3 admite "clase fachada equivalente". Marshal al hilo Qt con `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` en los 6 callbacks. |
| D9 | `onClearAllObjects` borra tracks con `m_guid` no vacío | Más robusto que "todos menos id 0": los tracks creados por fuera de la replicación (QEK) tampoco se tocan. Cuando haya más tipos persistentes, el clear se extiende a cada colección replicada. |
| D10 | Recuperación por `IReplicationEngine` (ADR-011) | `reconnectTransport()` / `reinitializeStorage()` se llaman sobre `IReplicationEngine*`. `IReplicationBridge` queda solo con `registerListener` + `notify*`. |

## Tabla de `object_type` (Envelope)

| Valor | Tipo | Estado |
|-------|------|--------|
| 1 | Track | Esta iteración |
| 2 | Área | Reservado (futuro) |
| 3 | Círculo | Reservado (futuro) |
| 4 | Polígono | Reservado (futuro) |
| 5 | Sector | Reservado (futuro) |

## Schema del payload de red (Track, v1)

Construido y parseado exclusivamente en `TrackNetSerializer`. Compact JSON. Cada tipo
persistente futuro tendrá su propio schema versionado.

| Clave | Tipo | Fuente en Track |
|-------|------|-----------------|
| `v` | int | — (versión de schema = 1) |
| `type` | int | `m_type` |
| `env` | int | `m_creationEnvironment` |
| `identity` | int | `m_identity` |
| `mode` | int | `m_mode` |
| `x_dm` | double | `m_xDm` |
| `y_dm` | double | `m_yDm` |
| `speed_dmh` | double | `m_speedDmPerHour` |
| `course_deg` | double | `m_courseDeg` |
| `info` | string | `m_infoAmpliatoria` |
| `fc` | int | `m_asignacionFc` |
| `asgc` | string | `m_codigoAsignacion` |
| `link_y` | int | `m_estadoLinkY` |
| `link_14` | int | `m_estadoLink14` |
| `priv` | string | `m_codigoPrivado` |

**Excluidos deliberadamente:** `m_id` (identidad visual local — ICD §6.4), `m_sitrepPpp`
(derivado local contra el ownship propio), `guid` (viaja en el Envelope, no se duplica).
Parseo tolerante: clave ausente → default. Validación RNF-09: payload > 4096 bytes →
rechazo + `qWarning`, sin llamar al engine.

## Tareas

- [ ] **F1 — Este documento** (`docs/REPlan.md`).
- [ ] **F2 — Headers del contrato stub** en `src/replicationEngine/ReplicationBridge/`:
  `ReplicatedObject.h`, `NetworkStatus.h`, `IReplicationListener.h`,
  `IReplicationEngine.h`, `IReplicationBridge.h`. Sin Qt. Alta en `DDM.pro`.
- [ ] **F3 — `m_guid` en Track** (`src/model/entities/track.h`) + generación con
  `QUuid::createUuid()` en `TrackService::createTrack`. Depende de: nada.
- [ ] **F4 — `TrackNetSerializer`** (`src/controller/replication/`):
  `toNetworkPayload(Track)`, `parsePayload(std::string, QJsonObject&)`,
  `applyPayload(QJsonObject, Track&)`. Depende de: Track (F3 no bloquea — el guid no viaja en payload).
- [ ] **F5 — `StubReplicationEngine`** (`src/replicationEngine/stub/`): implementa
  `IReplicationEngine` + `IReplicationBridge`, BD simulada en memoria, helpers
  `simulateRemoteUpsert/Delete`, `simulateNetworkStatus`, `simulateSnapshot`, `dumpDb`.
  Depende de: F2.
- [ ] **F6 — `ReplicationBridge`** (`src/controller/replication/`) + cambios en
  `CommandContext` (`replicationBridge`, `findTrackByGuid`, `eraseTrackByGuid`).
  Saliente: `publishTrackAdded/Deleted` → Envelope → `onLocalObject*`.
  Entrante: 6 callbacks con marshal + despacho por `object_type` + inyección/borrado/clear
  silenciosos + push a UI. Depende de: F2, F3, F4.
- [ ] **F7 — Hooks en `TrackService`**: publicar en `createTrack` (tras éxito) y
  `deleteTrackById` (resolver guid ANTES de borrar). Guards: id 0 y guid vacío.
  Depende de: F6.
- [ ] **F8 — Wiring en `main.cpp`** (instanciar stub + bridge, `registerListener`),
  comando CLI de prueba `rep` (`src/controller/commands/replicationcommand.*`),
  fix `int meko = 0;`, alta de todo en `DDM.pro`, build completo.
  Depende de: F5, F6, F7.
- [ ] **F9 (futuro) — Extensiones**: figuras como tipos persistentes (object_type 2..5,
  serializers propios, hooks en GeometryService/etc.); cerrar brechas QEK y SITREP
  ruteándolas por `TrackService`; ObjectModified; UI de estado de red y acciones de
  recuperación (`reconnectTransport` / `reinitializeStorage`).
- [ ] **F10 — Domain ID de DDS (ADR-012 / DA-03, ICD §8)**: DDM debe leer el
  `domain_id` desde `.ini` o variable de entorno (RF-DDS-006) y pasarlo al
  constructor de RE: `ReplicationEngine(storage, transport, resolver, bridge, domain_id)`.
  Pendientes: (a) decidir fuente (`Configuration` hoy lee de `overlay.json`, no
  `.ini`/env); (b) **preguntar a SiOp** si DDM debe construir también `storage`/
  `transport`/`resolver`/`bridge` o si habrá factory/defaults — la firma del ICD
  los lista como parámetros pero son tipos internos de SiOp. El cambio queda
  contenido en `main.cpp` + `Configuration` (el bridge no se toca).

## Consideraciones e inconsistencias detectadas (documentadas, no se resuelven ahora)

1. **Coordenadas relativas:** `m_xDm/m_yDm` son coordenadas de display relativas al
   ownship local (en modo RELATIVE el ownship se ancla en 0,0). Replicarlas entre
   consolas con ownships distintos produce posiciones incorrectas. Limitación asumida
   mientras ObjectModified esté diferido; a revisar cuando se defina la cinemática de red.
2. **Divergencia cinemática:** `CommandContext::updateTracks` (timer de 80 ms) también
   simula los tracks inyectados desde la red → las posiciones divergen entre consolas
   con el tiempo. Sin ObjectModified no hay corrección periódica.
3. **Ids visuales por consola:** cada consola asigna `m_id` local independiente
   (`nextTrackId++`); el mismo objeto tiene números de track distintos en cada consola.
   El `guid` es la única identidad de red (consistente con ICD §6.4).
4. **Brechas salientes:** QEK (`src/model/qek.h:79` alta, `:103` baja) y SITREP
   (`src/controller/services/sitrepservice.cpp:16` baja) manipulan `ctx->tracks` sin
   pasar por `TrackService` → esos cambios NO se replican. Pendiente F9.
5. **`Configuration::meko` sin inicializar:** no tiene inicializador y
   `loadConfiguration()` no se invoca desde ningún lado → `source_console_id` saldría
   con basura. Fix mínimo en F8 (`int meko = 0;`); pendiente decidir dónde llamar
   `loadConfiguration()` en el arranque.
6. **Push `list_tracks` no solicitado:** tras una inyección remota, el bridge empuja la
   lista de tracks al frontend reutilizando el formato de respuesta de `list_tracks`.
   Verificar que el frontend tolere respuestas no pedidas; si no, acordar un evento nuevo.
7. **Snapshot re-numera:** tras `ClearAllObjects` + re-inyección, los tracks vuelven con
   el mismo guid pero `m_id` local nuevo. Aceptable por contrato (el id es local).

## Migración al `.so` real (cuando SiOp entregue)

1. Reemplazar `INCLUDEPATH += src/replicationEngine/` por el include real del paquete de RE.
2. Agregar `LIBS += -lReplicationEngine`.
3. Quitar del `.pro` los archivos de `src/replicationEngine/` (contrato stub + StubReplicationEngine).
4. Eliminar `ReplicationCommand` (comando de prueba `rep`).
5. En `main.cpp`, reemplazar la instanciación del stub por la factory/bootstrap real de RE.
6. El código de `ReplicationBridge`, `TrackNetSerializer`, `Track` y `TrackService` **no se toca**.

## Verificación manual (con el stub)

`qmake && make && ./DDM`, por CLI:

1. `add 10 20` → log `[StubRE] onLocalObjectUpserted` con guid UUID v4, `console=<meko>`, payload v1 sin `id` ni `ppp`.
2. `delete <id>` → `onLocalObjectDeleted` con el **mismo** guid del alta.
3. `rep inject aaaa-bbbb 50 60 90 10` → track nuevo con id local y guid `aaaa-bbbb`; SIN `onLocalObjectUpserted` en el log (anti-eco).
4. Repetir `rep inject aaaa-bbbb ...` con otro curso → mismo track actualizado, sin duplicado.
5. `rep remove aaaa-bbbb` ×2 → primera borra, segunda solo log diagnóstico (idempotencia).
6. Con ownship creado (id 0): el stub no loguea upsert; `rep snapshot 3` → progreso, clear (id 0 sobrevive), re-inyección, completed.
7. `rep status 3` → log TRANSPORT_FAILED + evento `replication_status` a la UI.
8. `create_track` (JSON) con `info` de ~5000 caracteres → rechazo por límite de 4 KB, sin llamada al engine.
9. Regresión: tracks, cursores, CPA, sitrep y estacionamiento sin cambios de comportamiento.
