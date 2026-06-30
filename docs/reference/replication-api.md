# API de ReplicationEngine — Referencia rápida

Qué envía DDM, qué recibe DDM, y en qué formato.

---

## Estructura de datos compartida

Todo el intercambio usa un único envelope:

```cpp
struct ReplicatedObjectStruct {
    std::string guid;              // UUID v4 — generado por DDM
    int         object_type;       // 1 = Track
    int         source_console_id; // ID de esta consola (campo meko en Configuration)
    int64_t     last_updated;      // Unix timestamp en milisegundos
    std::string json_payload;      // JSON con el estado del objeto (máx. 4 KB)
};
```

---

## Lo que DDM envía a ReplicationEngine

DDM llama estos métodos sobre la interfaz `IReplicationBridge` (implementada por el rama):

### `onLocalObjectUpserted(const ReplicatedObjectStruct& obj)`

Cuando un track es creado o modificado localmente.

```
guid              → UUID v4 generado por DDM (persiste en Track::m_guid)
object_type       → 1 (Track)
source_console_id → Configuration::instance().meko
last_updated      → QDateTime::currentMSecsSinceEpoch()
json_payload      → ver formato abajo
```

### `onLocalObjectDeleted(const std::string& guid)`

Cuando un track local es eliminado. Solo se envía el GUID.

---

## Formato de `json_payload` para Track

```json
{
  "guid":                 "550e8400-e29b-41d4-a716-446655440000",
  "type":                 "Surface",
  "identity":             "Hostile",
  "mode":                 "Manual",
  "creation_environment": "Surface",
  "x":                    12.5, // esto tengo que terminar de definir. La idea sería con lat y long I guess
  "y":                    -3.7,
  "speed_dm_h":           15.0,
  "course_deg":           270,
  "info":                 "texto libre",
  "asignacion_fc":        2,
  "codigo_asignacion":    "AB1",
  "link_y":               0,
  "link_14":              1,
  "codigo_privado":       "XYZ"
}
```

Campos opcionales no enviados si están vacíos: `asignacion_fc`, `codigo_asignacion`, `link_y`, `link_14`, `codigo_privado`.

---

## Lo que DDM recibe de ReplicationEngine

RE llama estos métodos sobre `IReplicationListener` (implementada por DDM en `ReplicationBridge`). **Llegan desde el Worker Thread de RE** — DDM hace marshal a Qt main thread internamente.

| Método | Cuándo llega | Datos |
|--------|-------------|-------|
| `onInjectObject(ReplicatedObjectStruct)` | Track recibido de la red o al repoblar desde SQLite al arrancar | Envelope completo |
| `onRemoveObject(std::string guid)` | Track eliminado en otra consola | GUID del track |
| `onClearAllObjects()` | Justo antes de repoblar desde Snapshot | — |
| `onSnapshotProgress(int current, int total)` | Durante sincronización con peers | Lote actual y total |
| `onSnapshotCompleted()` | Sincronización terminada (o arranque aislado) | — |
| `onNetworkStatusChanged(int status)` | Cambio de conectividad | `0`=desconectado `1`=conectado `2`=sincronizando |

### Qué hace DDM con `onInjectObject`

- Deserializa `json_payload` → `Track`
- Si el GUID no existe en `CommandContext`: inserta el track con el próximo `nextTrackId`
- Si el GUID ya existe: actualiza las propiedades del track existente
- **No dispara `onLocalObjectUpserted`** (echo prevention)

### Qué hace DDM con `onRemoveObject`

- Busca el track por GUID en `CommandContext` y lo elimina
- Si el GUID no existe: log diagnóstico, sin error
- **No dispara `onLocalObjectDeleted`**

### Qué hace DDM con `onClearAllObjects`

- Elimina todos los tracks que tienen GUID (persistentes)
- Deja intactos los tracks sin GUID (locales transitorios)
- **No dispara `onLocalObjectDeleted`** para ninguno

---

## Objetos que NO se replican

Solo los **Tracks** cruzan la frontera a ReplicationEngine. No se replican:

- Cursores de medición
- Áreas, círculos, polígonos (Todavía la idea es compartirlo a futuro)
- OwnShip
- Cualquier estado de UI


## Cosas a ver
- Qué tracks puedo modificar (ya lo discutimos pero no lo implementé)
- Dueño del track
- Condiciones de carrera, asociado a todo lo anterior mencionado
