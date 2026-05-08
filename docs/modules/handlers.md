# Módulo: Handlers

## Descripción general

Este módulo contiene adaptadores de protocolo JSON para comandos del frontend. Cada handler traduce `QJsonObject args` a requests de dominio, valida campos, delega la lógica a servicios y construye respuestas normalizadas.

Dentro del backend DDM, los handlers son la capa intermedia entre `JsonCommandHandler` (routing de comandos) y la capa de servicios (`TrackService`, `CursorService`, etc.).

## Lista de archivos y clases

| Archivo | Clase/Struct | Responsabilidad |
|---|---|---|
| `src/controller/handlers/cursorcommandhandler.h` | `CursorCommandHandler` | Comandos JSON de líneas/cursores (`create_line`, `delete_line`, `list_lines`). |
| `src/controller/handlers/trackcommandhandler.h` | `TrackCommandHandler` | Comandos JSON de tracks (`create_track`, `delete_track`, `list_tracks`). |
| `src/controller/handlers/geometrycommandhandler.h` | `GeometryCommandHandler` | Comandos JSON de figuras (`create/delete area/circle/polygon`, `list_shapes`). |
| `src/controller/handlers/ownshipcommandhandler.h` | `OwnShipCommandHandler` | Comando JSON `ownship_update`. |

## Clases principales

### CursorCommandHandler

- **Rol**: Procesa operaciones JSON sobre cursores, validando campos y delegando CRUD a `CursorService`.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor | `CursorCommandHandler(CommandContext* context, ITransport* transport, ObmService* obmService)` | Inicializa servicio y dependencias. |
| Crear línea | `QByteArray createLine(const QJsonObject& args)` | Valida request y crea cursor. |
| Eliminar línea | `QByteArray deleteLine(const QJsonObject& args)` | Elimina cursor por id línea. |
| Listar líneas | `QByteArray listLines(const QJsonObject& args)` | Serializa cursores actuales. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `CursorService`
- `JsonValidator`
- `JsonResponseBuilder`
- `ObmService`
- `CommandContext`, `ITransport`

### TrackCommandHandler

- **Rol**: Adapta comandos JSON de tracks, mapea argumentos de entrada a `TrackCreateRequest` y retorna respuestas estandarizadas.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor | `TrackCommandHandler(CommandContext* context, ITransport* transport)` | Inicializa servicio de tracks. |
| Crear track | `QByteArray createTrack(const QJsonObject& args)` | Alta de track desde JSON. |
| Eliminar track | `QByteArray deleteTrack(const QJsonObject& args)` | Baja por id. |
| Listar tracks | `QByteArray listTracks(const QJsonObject& args)` | Serialización de tracks. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `TrackService`
- `JsonValidator`
- `JsonResponseBuilder`
- `CommandContext`, `ITransport`

### GeometryCommandHandler

- **Rol**: Gestiona figuras tácticas vía JSON y encapsula validaciones de arrays/puntos antes de delegar en `GeometryService`.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor | `GeometryCommandHandler(CommandContext* context, ITransport* transport, ObmService* obmService)` | Inicializa servicio de geometría. |
| Alta área | `QByteArray createArea(const QJsonObject& args)` | Crea `AreaEntity` y cursores asociados. |
| Baja área | `QByteArray deleteArea(const QJsonObject& args)` | Elimina área por id. |
| Alta círculo | `QByteArray createCircle(const QJsonObject& args)` | Crea círculo y sus cursores. |
| Baja círculo | `QByteArray deleteCircle(const QJsonObject& args)` | Elimina círculo por id. |
| Alta polígono | `QByteArray createPolygon(const QJsonObject& args)` | Crea polígono y cursores perimetrales. |
| Baja polígono | `QByteArray deletePolygon(const QJsonObject& args)` | Elimina polígono por id. |
| Listado figuras | `QByteArray listShapes(const QJsonObject& args)` | Retorna áreas/círculos/polígonos activos. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `GeometryService`
- `JsonValidator`
- `JsonResponseBuilder`
- `ObmService`
- `CommandContext`, `ITransport`

### OwnShipCommandHandler

- **Rol**: Entrada JSON para actualización de ownship y encapsulación de la respuesta.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor | `explicit OwnShipCommandHandler(CommandContext* context)` | Inicializa `OwnShipService`. |
| Actualizar ownship | `QByteArray updateOwnShip(const QJsonObject& args)` | Valida datos y actualiza estado ownship. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `OwnShipService`
- `JsonResponseBuilder`
- `CommandContext`

## Flujo de datos

1. `JsonCommandHandler` recibe comando y deriva al handler correspondiente.
2. Handler valida argumentos (`JsonValidator` o validación propia de campos).
3. Handler arma request de dominio y llama al service.
4. Service muta/consulta `CommandContext`.
5. Handler traduce resultado a JSON (`JsonResponseBuilder`) y retorna `QByteArray`.
6. `JsonCommandHandler` envía payload por `ITransport`.

## Manejo de errores

- Parseo y tipos: validación de campos requeridos, tipo numérico/string y rangos.
- Entidades inexistentes: respuestas de error cuando id no existe en contexto.
- Dominio inválido: errores de servicio propagados como `buildErrorResponse(...)`.
- Convención de salida: todas las rutas retornan JSON `status=success|error`.

## Módulos relacionados

- `docs/architecture.md`
- `docs/modules/services.md`
- `docs/modules/command-context.md`
- `docs/README_BACKEND_ARCHITECTURE.md`
