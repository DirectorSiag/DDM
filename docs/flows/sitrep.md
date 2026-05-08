# Flujo: SITREP

## Descripción general

Este flujo cubre la vista operativa de tracks para Situation Report (SITREP) desde consola, incluyendo listado, watch en tiempo real, borrado y edición de información ampliatoria.

El flujo se apoya en:

- `SitrepCommand` como entrada CLI.
- `SitrepService` para operaciones sobre contexto.
- `Track` y `CommandContext` como fuente de datos.

## Lista de archivos y clases

| Archivo | Clase/Struct | Responsabilidad |
|---|---|---|
| `src/controller/commands/sitrepcommand.cpp` | `SitrepCommand` | Comando CLI con subcomandos `list`, `watch`, `delete`, `info`. |
| `src/controller/services/sitrepservice.cpp` | `SitrepService` | Snapshot, búsqueda, borrado de track y actualización de info SITREP. |
| `src/model/entities/track.h` | `Track` | Fuente de campos visibles en tabla SITREP (identidad, az/dt, rumbo/velocidad, PPP, info). |
| `src/model/commandContext.h` | `CommandContext`, `SitrepExtra` | Persistencia de tracks e info ampliatoria legacy por id. |
| `src/model/sitrep/sitrep.h` | `sitrep` | Modelo de reporte SITREP declarado (implementación parcial actual). |

## Clases principales

### SitrepCommand

- **Rol**: ejecuta la experiencia CLI de SITREP (tabla y operaciones de mantenimiento).
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Ejecutar | `CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const` | Resuelve subcomando y delega en `SitrepService`/helpers de impresión. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `SitrepService`
- `Track`
- `CommandContext`

### SitrepService

- **Rol**: servicio simple de acceso/mutación para operaciones SITREP.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Snapshot | `std::deque<Track> snapshot() const` | Devuelve copia de tracks actuales para render de tabla. |
| Borrar | `bool deleteTrackById(int id)` | Elimina track y limpia metadata `sitrepExtra`. |
| Buscar | `Track* findTrackById(int id) const` | Retorna track para actualización de info. |
| Set info | `void setSitrepInfo(int id, const QString& text)` | Persistencia legacy de info ampliatoria por id. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `CommandContext`

### sitrep (modelo)

- **Rol**: clase modelo de SITREP con campos de contenido/categoría/timestamp.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor | `sitrep(const QString& title, const QString& body)` | Inicializa título y cuerpo. |
| API declarada | `id/setId`, `getCategory/setCategory`, `title/setTitle`, `body/setBody`, `timestampUtc/setTimestampUtc`, `source/setSource`, `isValid` | Contrato de modelo (declarado en header). |

- **Structs/Tipos definidos**:
- `enum class category`
- **Dependencias**:
- Qt (`QString`, `QDateTime`)

## Flujo de datos

### `sitrep list`

1. `SitrepCommand` toma snapshot (`SitrepService::snapshot`).
2. Itera tracks y renderiza tabla textual con:
   - número track
   - identidad agrupada (Amigo/Hostil/Desconocido)
   - azimut/distancia
   - rumbo/velocidad
   - link
   - info ampliatoria
   - PPP (`Track::SitrepPppData` + `getSitrepPppTimeHHMM`)

### `sitrep watch`

1. Loop infinito con refresh fijo (`2000ms`).
2. Limpia pantalla y reimprime tabla en cada ciclo.
3. Sale por interrupción externa (CTRL+C).

### `sitrep delete <id>`

1. Valida `id` numérico.
2. `SitrepService::deleteTrackById(id)` elimina track del contexto.
3. Limpia `sitrepExtra` asociado para compatibilidad.

### `sitrep info <id> <texto...>`

1. Valida `id` y existencia de track.
2. Actualiza `Track::setInformacionAmpliatoria(text)`.
3. Mantiene compatibilidad escribiendo también en `SitrepService::setSitrepInfo`.

## Manejo de errores

- Subcomando no soportado -> mensaje de uso.
- `trackId` inválido -> error de parseo numérico.
- Track inexistente para `delete/info` -> mensaje explícito.
- En `watch`, el flujo no retorna por diseño (loop continuo).
- Nota de estado actual: la clase `sitrep` del modelo está declarada ampliamente, pero en `sitrep.cpp` sólo tiene implementado el constructor con `title/body`.

## Módulos relacionados

- `docs/modules/commands.md`
- `docs/modules/services.md`
- `docs/modules/entities.md`
- `docs/modules/command-context.md`
