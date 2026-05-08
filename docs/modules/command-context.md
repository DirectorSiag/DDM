# Módulo: Command Context

## Descripción general

Este módulo define y gobierna el estado táctico compartido del backend. Su pieza central es `CommandContext`, una estructura header-only que concentra entidades operativas (tracks, cursores y geometrías), estado de ownship, marcadores/sesiones de cálculo táctico y utilidades de mutación/consulta.

Existe para asegurar que todos los flujos del sistema trabajen sobre una única fuente de verdad:

- Comandos JSON del frontend (handlers/services)
- Comandos CLI (patrón `ICommand`)
- Procesamiento de hardware (decoder/OBM/overlay/own cursor)
- Codificación de salida al LPD (`encoderLPD`)

Dentro de la arquitectura DDM Backend, este módulo funciona como núcleo de datos y cohesión de reglas de actualización cinemática (`updateTracks`) y mantenimiento de sesiones de estacionamiento/CPA.

## Lista de archivos y clases

| Archivo | Clase/Struct | Responsabilidad |
|---|---|---|
| `src/model/commandContext.h` | `CommandContext` | Contenedor de estado global con operaciones inline para CRUD de entidades y actualización cinemática. |
| `src/model/commandContext.h` | `CommandContext::OwnShipState` | Estado de ownship (rumbo, velocidad, posición, UTC, validez, origen). |
| `src/model/commandContext.h` | `CommandContext::CpaMarkerState` | Representación de marcador CPA para render/encoding LPD. |
| `src/model/commandContext.h` | `CommandContext::StationingSession` | Estado persistente de una sesión de estacionamiento por slot (1..10). |
| `src/model/commandContext.h` | `CommandContext::SitrepExtra` | Metadata SITREP adicional indexada por id de track. |
| `src/model/ownCursor/owncurs.h` | `OwnCurs` | Controla activación/posicionamiento/actualización del cursor propio a partir de eventos del decoder. |
| `src/model/sitrep/sitrep.h` | `sitrep` | Modelo de reporte de situación con campos de identidad, categoría, contenido, timestamp y fuente. |

## Clases principales

### CommandContext

- **Rol**: Es la fuente de verdad del estado táctico en memoria. Integra colecciones de entidades (`tracks`, `cursors`, `areas`, `circles`, `polygons`), estado de ownship, centro de presentación, marcadores CPA y sesiones de estacionamiento. También contiene utilidades de acceso y actualización para evitar lógica duplicada entre servicios/controladores.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor | `CommandContext()` | Inicializa streams de salida/error en UTF-8. |
| Información SITREP | `QString sitrepInfo(int trackId) const` | Retorna info SITREP extra por track o `"-"` si no existe. |
| Set info SITREP | `void setSitrepInfo(int trackId, const QString& text)` | Actualiza/inserta metadata SITREP en mapa por id. |
| Agregado track | `Track& addTrackFront(const Track& t)` | Inserta track al frente de la deque. |
| Emplace track | `template <typename... Args> Track& emplaceTrackFront(Args&&... args)` | Inserción directa de track evitando temporal intermedio. |
| Agregado cursor | `CursorEntity& addCursorFront(const CursorEntity& c)` | Inserta cursor al frente y retorna referencia. |
| Emplace cursor | `template <typename... Args> CursorEntity& emplaceCursorFront(Args&&... args)` | Inserción directa de cursor con forwarding perfecto. |
| Centro actual | `QPointF center() const` | Devuelve centro táctico actual como `QPointF`. |
| Set centro | `void setCenter(QPair<float,float> c)` | Actualiza centro táctico `centerX/centerY`. |
| Reset centro | `void resetCenter()` | Restablece centro a `(0,0)`. |
| Modo movimiento | `void setMotionMode(MotionMode mode)` | Cambia entre `RELATIVE` y `TRUE_MOTION`. |
| Buscar track | `Track* findTrackById(int id)` | Busca track mutable por id. |
| Borrar track | `bool eraseTrackById(int id)` | Elimina track por id. |
| Borrar cursor | `bool eraseCursorById(int id)` | Elimina cursor por id. |
| Upsert marcador CPA | `void upsertCpaMarker(const CpaMarkerState& marker)` | Inserta o reemplaza marcador por `sessionId`. |
| Borrar marcador CPA | `bool eraseCpaMarkerBySessionId(const QString& sessionId)` | Elimina marcador CPA de una sesión. |
| Borrar marcadores por track | `int eraseCpaMarkersByTrackId(int trackId)` | Elimina marcadores asociados a un track y devuelve cantidad. |
| Upsert sesión EST | `bool upsertStationingSession(const StationingSession& session)` | Inserta/actualiza sesión de estacionamiento validando slot 1..10. |
| Borrar sesión EST | `bool removeStationingSession(int slotIndex)` | Elimina sesión de estacionamiento por slot válido. |
| Siguiente track | `Track* getNextTrackById(int currentId)` | Devuelve siguiente track en forma circular. |
| Actualizar tracks | `void updateTracks(double deltaTime)` | Extrapola posiciones según modo de movimiento y recalcula sesiones de estacionamiento activas. |
| Borrar área | `bool deleteArea(int areaId)` | Elimina área y cursores asociados. |
| Borrar círculo | `bool deleteCircle(int circleId)` | Elimina círculo y cursores asociados. |
| Borrar polígono | `bool deletePolygon(int polygonId)` | Elimina polígono y cursores asociados. |

- **Structs/Tipos definidos**:
- `MotionMode`: enum de presentación cinemática (`RELATIVE`, `TRUE_MOTION`).
- `SitrepExtra`: dato adicional de SITREP (`info`) por track.
- `OwnShipState`: snapshot de ownship (`speedKnots`, `courseDeg`, `lat/lon`, `timeUtc`, `dateUtc`, `source`, `valid`).
- `CpaMarkerState`: marcador CPA persistido en contexto (`sessionId`, track A/B, x/y, visibilidad).
- `StationingSession`: parámetros de entrada y resultados del cálculo de estacionamiento por slot.
- **Dependencias**:
- `Track`, `CursorEntity`, `AreaEntity`, `CircleEntity`, `PolygonoEntity`
- `EstacionamientoCalculator`
- `ITransport`

### OwnCurs

- **Rol**: Encapsula la lógica del cursor propio controlado por señales de hardware/decoder. Mantiene una referencia opcional al cursor dentro de `CommandContext` y actualiza su estado/posición/ángulo/largo según eventos (`cuOrOffCent`, `cuOrCent`, handwheel, activación).
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor | `OwnCurs(CommandContext* ctx, OBMHandler* newObm, QObject* parent = nullptr)` | Inyecta contexto y manejador OBM. |
| Posicionar en OBM | `void cuOrOffCent()` | Si existe cursor y OBM, mueve cursor a posición OBM actual. |
| Posicionar al origen | `void cuOrCent()` | Si existe cursor, lo mueve al origen `ORIGIN`. |
| Activar/desactivar | `void ownCursActive(bool value)` | Crea cursor id 1 si no existe y ajusta flag `active`. |
| Actualizar handwheel | `void updateHandwheel(const QPair<qfloat16, qfloat16>& update)` | Actualiza ángulo y longitud del cursor propio. |

- **Structs/Tipos definidos**:
- `cursorRef`: `std::optional<std::reference_wrapper<CursorEntity>>` para referenciar el cursor persistido en contexto.
- **Dependencias**:
- `CommandContext`
- `CursorEntity`
- `OBMHandler`

### sitrep

- **Rol**: Modelo de datos para reportes de situación (SITREP) con campos de categoría, contenido y metadatos temporales/origen.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor default | `sitrep()` | Constructor por defecto. |
| Constructor contenido | `sitrep(const QString& title, const QString& body)` | Inicializa título y cuerpo del reporte. |
| API declarada de identidad | `QString id() const`, `void setId(const QString& id)` | Get/set de identificador lógico del reporte. |
| API declarada de categoría | `category getCategory() const`, `void setCategory(category c)` | Get/set de categoría táctica. |
| API declarada de contenido | `QString title() const`, `void setTitle(const QString& t)`, `QString body() const`, `void setBody(const QString& b)` | Gestión de contenido textual SITREP. |
| API declarada temporal/origen | `QDateTime timestampUtc() const`, `void setTimestampUtc(const QDateTime& tsUtc)`, `QString source() const`, `void setSource(const QString& src)` | Gestión de marca temporal y fuente. |
| Validación declarada | `bool isValid() const` | Contrato de validación mínima del objeto. |

- **Structs/Tipos definidos**:
- `category`: enum (`unknown`, `surface`, `air`, `subsurface`, `ew`, `weapons`, `system`).
- **Dependencias**:
- `QString`, `QDateTime`, `QMetaType`

## Flujo de datos

### Entrada de datos al módulo

- Servicios y handlers de comandos mutan `CommandContext` para crear/actualizar/borrar entidades.
- `OwnShipService` actualiza `CommandContext::ownShip` y sincroniza track virtual id 0.
- Señales del decoder (vía `main.cpp`) activan/actualizan `OwnCurs`, que muta cursor propio en `CommandContext`.
- Temporizador de extrapolación (`main.cpp`) invoca `CommandContext::updateTracks(deltaTime)`.

### Flujo interno relevante

- `updateTracks` calcula desplazamientos por curso/velocidad.
- Si `motionMode == RELATIVE`: ownship (id 0) permanece anclado en origen, y los demás tracks se mueven relativo al propio.
- Si `motionMode == TRUE_MOTION`: todos los tracks avanzan en movimiento absoluto.
- Si hay sesiones de estacionamiento activas:
  - Se resuelve estado cinemático de track A/track B (incluyendo fallback ownship válido).
  - Se recalcula posición objetivo de estación.
  - Se reevalúa resultado con `EstacionamientoCalculator::compute`.

### Salida de datos desde el módulo

- `encoderLPD` lee `CommandContext` para codificar tracks, cursores, center, OBM, marcadores CPA y sesiones EST.
- Servicios de consulta/serialización leen `CommandContext` para respuestas JSON (`list_tracks`, `list_lines`, `list_shapes`, etc.).

## Manejo de errores

El módulo usa manejo defensivo de bajo costo (sin excepciones de control de flujo):

- Validaciones por retorno booleano:
  - `upsertStationingSession` y `removeStationingSession` rechazan slots fuera de rango 1..10.
  - `eraseTrackById`, `eraseCursorById`, `deleteArea/deleteCircle/deletePolygon` retornan `false` si no encuentran entidad.

- Cortes tempranos para robustez numérica y operativa:
  - `updateTracks` retorna inmediatamente si `deltaTime <= 0`.
  - Recalculo de estacionamiento se omite si no hay sesiones o si no se puede resolver estado de tracks.

- Integridad de entidades relacionadas:
  - Al borrar áreas/círculos/polígonos, también se borran cursores asociados para evitar residuos en estado.

- Control de punteros opcionales en `OwnCurs`:
  - Métodos retornan sin acción si no hay `cursorRef` o `obm` disponible.

- Observación de implementación actual de `sitrep`:
  - En el estado actual del código, solo está implementado en `.cpp` el constructor con `title/body`; el resto de la API del header está declarada pero no definida en ese archivo.

## Módulos relacionados

- `docs/architecture.md`: vista global del backend y su orquestación.
- `docs/README_BACKEND_ARCHITECTURE.md`: documentación arquitectónica ampliada.
- `docs/PPP_SYSTEM.md`: modelo y flujo del cálculo PPP/CPA.
- `docs/STATIONING_SYSTEM.md`: cálculo de estacionamiento y sesiones.
