# Módulo: Overlays & OBM

## Descripción general

Este módulo implementa la interacción táctica proveniente de consola/controles físicos: selección de overlay, ejecución de teclas QEK y operación del OBM (Operating Ball Mechanism).

Se ubica entre la decodificación binaria (`ConcDecoder`) y la mutación del estado táctico (`CommandContext`), permitiendo que eventos de hardware se traduzcan en acciones de dominio (crear/editar tracks, actualizar posición y rango, asociar contacto bajo cursor).

## Lista de archivos y clases

| Archivo | Clase/Struct | Responsabilidad |
|---|---|---|
| `src/controller/overlayHandler.h` | `OverlayHandler` | Coordinador de overlays; instancia QEK concreto y ejecuta comandos QEK. |
| `src/model/obm/iOBMHandler.h` | `IOBMHandler` | Interfaz base para manejo de posición/rango OBM. |
| `src/model/obm/obmHandler.h` | `OBMHandler` | Implementación OBM: integra movimiento, actualiza rango y asocia tracks cercanos. |
| `src/model/qek.h` | `QEK`, `Qek` | Contrato y utilidades de acciones por teclado rápido táctico. |
| `src/model/overlays/spc.h` | `SPC` | Implementación QEK para overlay SPC. |
| `src/model/overlays/linco.h` | `LINCO` | Implementación QEK para overlay LINCO. |
| `src/model/overlays/asw.h` | `ASW` | Implementación QEK para overlay ASW. |
| `src/model/overlays/ops.h` | `OPS` | Implementación QEK para overlay OPS. |
| `src/model/overlays/heco.h` | `HECO` | Implementación QEK para overlay HECO. |
| `src/model/overlays/apc.h` | `APC` | Implementación QEK para overlay APC. |
| `src/model/overlays/aaw.h` | `AAW` | Implementación QEK para overlay AAW. |
| `src/model/overlays/ew.h` | `EW` | Implementación QEK para overlay EW. |

## Clases principales

### OverlayHandler

- **Rol**: Recibe overlay/QEK desde `ConcDecoder`, mantiene overlay activo e invoca la implementación `QEK` adecuada para ejecutar acciones tácticas.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor | `explicit OverlayHandler(QObject* parent = nullptr)` | Inicializa manejador. |
| Set contexto | `void setContext(CommandContext *ctx)` | Inyecta contexto táctico. |
| Set OBM | `void setOBMHandler(OBMHandler *oh)` | Inyecta manejador OBM. |
| Nuevo overlay | `void onNewOverlay(const QString& overlayName)` | Cambia overlay activo e instancia QEK específico. |
| Nuevo QEK | `void onNewQEK(const QString& qekStr)` | Parsea y ejecuta código QEK recibido. |
| Factory overlay | `std::unique_ptr<QEK> instanceNewQEK(const QString& overlayName)` | Resuelve clase concreta por nombre/código. |
| Ejecutar QEK | `void executeQEK(Qek which)` | Dispatcher por índice de tecla. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `QEK` y overlays concretos
- `CommandContext`
- `OBMHandler`

### IOBMHandler

- **Rol**: Define contrato para manejo de posición y rango del OBM, permitiendo desacople respecto de implementación concreta.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Update posición | `virtual void updatePosition(QPair<float, float> newPosition) = 0` | Aplica desplazamiento/posición. |
| Get posición | `virtual QPair<float, float> getPosition() = 0` | Devuelve posición actual. |
| Update rango | `virtual void updateRange(int newRange) = 0` | Ajusta escala/sensibilidad. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- Qt (`QObject`, `QPair`)

### OBMHandler

- **Rol**: Implementa cinemática del OBM y asociación de track más cercano para operaciones contextuales de QEK.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor | `OBMHandler()` | Inicializa posición/rango internos. |
| Update posición | `void updatePosition(QPair<float, float> newPosition) override` | Integra desplazamiento recibido. |
| Get posición | `QPair<float, float> getPosition() override` | Retorna posición actual. |
| Update rango | `void updateRange(int newRange) override` | Actualiza rango operativo. |
| Distancia a track | `double getDistanceFromTrack(const Track& t) const` | Distancia entre OBM y contacto. |
| Asociación | `Track* OBMAssociationProcess(CommandContext* ctx)` | Busca track asociado según proximidad. |
| Set posición | `void setPosition(QPair<float, float> newPosition)` | Fija posición manualmente. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `IOBMHandler`
- `CommandContext`
- `Track`

### QEK (base)

- **Rol**: Clase base de overlays; expone interfaz de ejecución por tecla (`execute20..57`) y helpers operativos para mutar tracks según posición OBM/contexto.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Set contexto | `void setContext(CommandContext* c)` | Inyecta estado táctico. |
| Set OBM | `void setOBMHandler(OBMHandler* h)` | Inyecta fuente de posición/asociación. |
| Execute N | `virtual void execute20()` ... `virtual void execute57()` | Acciones por botón QEK (override por overlay). |
| Add track | `void addTrack(Type type, TrackMode mode)` | Helper de creación de contacto. |
| Cambiar identidad | `bool changeIdentity(Identity identity)` | Helper sobre track asociado al OBM. |
| Cambiar modo | `bool assignTrackMode(TrackMode mode)` | Helper de modo de seguimiento. |
| Eliminar track | `bool wipeTrack()` | Baja track asociado al OBM. |

- **Structs/Tipos definidos**:
- `enum class Qek` (códigos de teclas)
- **Dependencias**:
- `CommandContext`
- `OBMHandler`
- `Track`

### Overlays concretos (`SPC`, `LINCO`, `ASW`, `OPS`, `HECO`, `APC`, `AAW`, `EW`)

- **Rol**: Especializan la semántica de teclas QEK por overlay táctico; cada clase hereda `QEK` y sobreescribe un subconjunto de métodos `executeXX`.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Ejecutores QEK | `void execute20() override`, ... | Acción específica de tecla según overlay. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `QEK`
- `TrackData` y helpers de dominio (en implementación)

## Flujo de datos

1. `ConcDecoder` emite `newOverlay` y `newQEK`.
2. `OverlayHandler::onNewOverlay` selecciona instancia `QEK` concreta.
3. `OverlayHandler::onNewQEK` parsea código (`QEK_20`, etc.) y despacha a `executeQEK`.
4. Métodos `executeXX` en overlay concreto usan helpers `QEK` para crear/modificar/eliminar tracks en `CommandContext` (frecuentemente mediante asociación OBM).
5. En paralelo, `OBMHandler` actualiza posición/rango a partir de señales del decoder (`newRollingBall`, `newRange`).

## Manejo de errores

- `OverlayHandler`:
  - ignora QEK si no hay overlay activo
  - valida string de QEK; log de warning en código inválido
  - warning si overlay desconocido
- `QEK` base usa guardas de punteros (`ctx`, `obmHandler`) para evitar operaciones inválidas.
- `OBMHandler::OBMAssociationProcess` retorna `nullptr` cuando no hay track asociable.

## Módulos relacionados

- `docs/architecture.md`
- `docs/modules/decoders-encoders.md`
- `docs/modules/command-context.md`
- `docs/modules/services.md`
- `docs/README_BACKEND_ARCHITECTURE.md`
