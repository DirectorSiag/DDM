# Flujo: Track Lifecycle

## Descripción general

Este flujo describe el ciclo de vida de un track táctico dentro del backend DDM: alta, persistencia, listado, actualización cinemática y baja.

El flujo se ejecuta por dos entradas principales:

- CLI (comandos `add` y `delete`)
- JSON (comandos `create_track` y `delete_track`)

En ambos casos el estado converge en `CommandContext::tracks` y la lógica de negocio está centralizada en `TrackService`.

## Lista de archivos y clases

| Archivo | Clase/Struct | Responsabilidad |
|---|---|---|
| `src/model/entities/track.h` | `Track` | Entidad de contacto táctico (posición, velocidad, rumbo, identidad, metadata SITREP/PPP). |
| `src/controller/commands/addCommand.cpp` | `AddCommand` | Alta CLI de tracks con parseo de flags y validaciones de entrada. |
| `src/controller/commands/deleteCommand.cpp` | `DeleteCommand` | Baja CLI de tracks por id. |
| `src/controller/handlers/trackcommandhandler.cpp` | `TrackCommandHandler` | Alta/baja/listado JSON de tracks. |
| `src/controller/services/trackservice.h` | `TrackService` | Lógica de dominio: crear, borrar, buscar y serializar tracks. |
| `src/model/commandContext.h` | `CommandContext` | Contenedor de tracks y utilidades de mutación (`emplaceTrackFront`, `eraseTrackById`). |

## Clases principales

### TrackService

- **Rol**: servicio de dominio para todo el lifecycle de tracks.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Crear | `TrackOperationResult createTrack(const TrackCreateRequest& request)` | Valida request, asigna id, crea track en contexto y opcionalmente calcula PPP. |
| Borrar | `TrackOperationResult deleteTrackById(int trackId)` | Elimina track por id y retorna estado. |
| Buscar | `Track* findTrackById(int trackId) const` | Obtiene puntero al track en contexto. |
| Serializar | `QJsonArray serializeTracks() const` | Emite snapshot JSON para API/listados. |

- **Structs/Tipos definidos**:
- `TrackCreateRequest`
- `TrackOperationResult`
- **Dependencias**:
- `CommandContext`
- `Track`
- `TrackPppService`

### AddCommand

- **Rol**: parser CLI de alta de track. Traduce flags legacy y modernas a `TrackCreateRequest`.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Ejecutar | `CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const` | Valida argumentos CLI, arma request y delega en `TrackService::createTrack`. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `TrackService`
- `TrackData`
- `RadarMath`

### TrackCommandHandler

- **Rol**: adapter JSON del lifecycle de tracks.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Alta JSON | `QByteArray createTrack(const QJsonObject& args)` | Valida JSON, arma request y crea track. |
| Baja JSON | `QByteArray deleteTrack(const QJsonObject& args)` | Elimina track por id. |
| Listado JSON | `QByteArray listTracks(const QJsonObject& args)` | Devuelve tracks serializados. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `TrackService`
- `JsonResponseBuilder`

## Flujo de datos

```mermaid
stateDiagram-v2
    [*] --> Creado

    state "Creado" as Creado
    state "Activo" as Activo
    state "ConPPP" as ConPPP
    state "EnSesionCPA" as EnSesionCPA
    state "Eliminado" as Eliminado

    Creado: track existe en CommandContext.tracks
    Activo: posición, rumbo y velocidad válidos

    note right of Activo
      Se actualizan coordenadas por extrapolación cinemática.
      También pueden cambiar rumbo/velocidad según mutaciones del track.
    end note

    note right of EnSesionCPA
      El track participa en una sesión CPA activa.
      Se recalculan TCPA/DCPA y se mantiene marcador en CommandContext.
    end note

    Creado --> Activo: alta exitosa\n(TrackService::createTrack)
    Activo --> Activo: updatePosition(deltaTime)\nextrapolación cinemática periódica
    Activo --> ConPPP: setSitrepPpp()\nresultado PPP calculado por TrackPppService
    ConPPP --> ConPPP: recálculo PPP\nen cada updateTracks()
    ConPPP --> Activo: ownship eliminado\no PPP inválido

    Activo --> EnSesionCPA: CPAService::startCPA()\nreferencia este track
    ConPPP --> EnSesionCPA: CPAService::startCPA()\nreferencia este track
    EnSesionCPA --> EnSesionCPA: graphCPA()\nrecálculo periódico
    EnSesionCPA --> Activo: CPAService::finishCPA() o clearTrack()
    EnSesionCPA --> ConPPP: CPAService::finishCPA() o clearTrack()\n(si tenía PPP)

    Activo --> Eliminado: TrackService::deleteTrackById()
    ConPPP --> Eliminado: TrackService::deleteTrackById()
    EnSesionCPA --> Eliminado: TrackService::deleteTrackById()\n(también cancela sesión CPA)
    Eliminado --> [*]
```

### Alta de track (CLI)

1. `CommandDispatcher` recibe línea `add ...`.
2. Ejecuta `AddCommand::execute`.
3. `AddCommand` parsea flags (`--type`, `--id`, `--spd`, `--crs`, etc.) y coordenadas.
4. Valida rangos básicos (coordenadas, velocidades, curso, FC).
5. Crea `TrackCreateRequest`.
6. Llama `TrackService::createTrack(request)`.
7. `TrackService`:
   - valida dominio
   - asigna `id = nextTrackId++`
   - inserta track con `emplaceTrackFront`
   - opcionalmente recalcula PPP si ownship está válido
8. Retorna `CommandResult` al dispatcher.

### Alta de track (JSON)

1. `JsonCommandHandler` enruta `create_track` a `TrackCommandHandler`.
2. `TrackCommandHandler::createTrack` valida campos `x`, `y`, y opcionalmente `type/creation_environment`.
3. Llama `TrackService::createTrack`.
4. Construye respuesta con `created_id` y `tracks` serializados.

### Baja de track

- CLI: `DeleteCommand::execute` -> `TrackService::deleteTrackById`.
- JSON: `TrackCommandHandler::deleteTrack` -> `TrackService::deleteTrackById`.

En ambos casos la eliminación real se apoya en `CommandContext::eraseTrackById`.

### Evolución cinemática

- Timer en `main.cpp` llama `CommandContext::updateTracks(deltaTime)`.
- `Track::setX/setY` actualiza posición según modo `RELATIVE` o `TRUE_MOTION`.

## Manejo de errores

- `AddCommand` reporta:
  - tipo faltante o inválido
  - flags no soportadas
  - conversiones numéricas inválidas
  - rangos fuera de dominio
- `TrackService::createTrack` valida:
  - coordenadas fuera de rango
  - velocidad DM/h inválida
  - curso inválido
  - FC fuera de 1..6
- `deleteTrackById` devuelve error `NOT_FOUND` si no existe id.
- `TrackCommandHandler` retorna errores JSON de validación (`buildValidationErrorResponse`) y dominio (`buildErrorResponse`).

## Módulos relacionados

- `docs/modules/commands.md`
- `docs/modules/handlers.md`
- `docs/modules/services.md`
- `docs/modules/entities.md`
- `docs/modules/command-context.md`
