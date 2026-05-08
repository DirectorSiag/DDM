# Protocolo: JSON Command API

## Descripción general

Este documento describe el protocolo JSON de entrada/salida entre el cliente frontend (Qt/QML) y el backend DDM. El punto de entrada es `JsonCommandHandler`, que parsea datagramas JSON, enruta por `command` y devuelve respuestas estandarizadas generadas por `JsonResponseBuilder`.

El objetivo del protocolo es exponer operaciones tácticas del backend (tracks, líneas, geometría, ownship, CPA/PPP y estacionamiento) con un formato uniforme y validación explícita de errores.

## Lista de archivos y clases

| Archivo | Clase/Struct | Responsabilidad |
|---|---|---|
| `src/controller/json/jsoncommandhandler.h` | `JsonCommandHandler` | Parseo de JSON, routing de comandos y despacho a handlers/servicios. |
| `src/controller/json/jsonresponsebuilder.h` | `JsonResponseBuilder` | Construcción de respuestas JSON uniformes de éxito/error. |
| `src/controller/json/validators/jsonvalidator.h` | `JsonValidator`, `ValidationResult` | Validación de campos JSON (tipo, presencia, rango y formato). |

## Clases principales

### JsonCommandHandler

- **Rol**: Entrada principal de comandos JSON recibidos por transporte. Extrae `command` y `args`, deriva al mapa de comandos (`m_commandMap`) y envía respuesta.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor | `JsonCommandHandler(CommandContext* context, ITransport* transport, ObmService* obmService, QObject *parent = nullptr)` | Inicializa handlers y servicios y carga `m_commandMap`. |
| Entrada | `void processJsonCommand(const QByteArray& jsonData)` | Parsea documento y despacha comando. |
| Refresh CPA | `void refreshActiveCpaSessions()` | Recalcula sesiones CPA activas. |
| Parse | `bool parseJson(const QByteArray& jsonData, QJsonObject& outObject)` | Valida JSON válido y raíz objeto. |
| Routing | `void routeCommand(const QString& command, const QJsonObject& args)` | Ejecuta lambda asociada al comando. |
| Respuesta | `void sendResponse(const QByteArray& responseData)` | Envía respuesta por `ITransport`. |
| Error parseo | `void sendParseError(const QString& errorDetail)` | Respuesta `INVALID_JSON`. |
| Error comando | `void sendUnknownCommandError(const QString& command)` | Respuesta `UNKNOWN_COMMAND`. |

- **Structs/Tipos definidos**:
- `using CommandHandler = std::function<QByteArray(const QJsonObject&)>`
- **Dependencias**:
- `TrackCommandHandler`, `CursorCommandHandler`, `GeometryCommandHandler`, `OwnShipCommandHandler`
- `CPAService`, `EstacionamientoService`
- `ITransport`, `CommandContext`

### JsonResponseBuilder

- **Rol**: Factory de respuestas JSON compactas para éxito/error.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Éxito genérico | `static QByteArray buildSuccessResponse(const QString& command, const QJsonObject& args)` | Arma `status=success`. |
| Éxito create line | `static QByteArray buildLineCreatedResponse(const QString& command, const QString& createdId, const QJsonArray& allLines)` | Respuesta para altas de línea. |
| Éxito delete line | `static QByteArray buildLineDeletedResponse(const QString& command, const QString& deletedId, const QJsonArray& remainingLines)` | Respuesta para baja de línea. |
| Éxito list lines | `static QByteArray buildLineListResponse(const QString& command, const QJsonArray& allLines)` | Respuesta para listado de líneas. |
| Error genérico | `static QByteArray buildErrorResponse(const QString& command, const QString& errorCode, const QString& errorMessage, const QJsonObject& details = QJsonObject())` | Arma `status=error`. |
| Error validación | `static QByteArray buildValidationErrorResponse(const QString& command, const QString& fieldName, const QString& fieldValue, const QString& expectedRange)` | Error de campo inválido. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- Qt JSON (`QJsonObject`, `QJsonArray`, `QJsonDocument`)

### JsonValidator

- **Rol**: Validador estático para campos JSON; separa validación de la lógica de negocio.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Validar numérico | `static ValidationResult validateNumericField(const QJsonObject& args, const QString& fieldName, double minValue, double maxValue, bool isRequired = true, double defaultValue = 0.0)` | Valida presencia/tipo/rango numérico. |
| Validar entero | `static ValidationResult validateIntegerField(const QJsonObject& args, const QString& fieldName, int minValue, int maxValue, bool isRequired = true, int defaultValue = 0)` | Valida presencia/tipo/rango entero. |
| Validar string | `static ValidationResult validateStringField(const QJsonObject& args, const QString& fieldName, bool isRequired = true)` | Valida string requerido/no vacío. |
| Validar LINE id | `static ValidationResult validateLineIdFormat(const QString& lineId)` | Espera formato `LINE_<número>`. |
| Extract numérico | `static double getNumericValue(const QJsonObject& args, const QString& fieldName, double defaultValue = 0.0)` | Helper de extracción. |
| Extract entero | `static int getIntValue(const QJsonObject& args, const QString& fieldName, int defaultValue = 0)` | Helper de extracción. |

- **Structs/Tipos definidos**:
- `ValidationResult` (`isValid`, `errorCode`, `errorMessage`, `fieldName`, `fieldValue`, `expectedRange`)
- **Dependencias**:
- Qt JSON

## Flujo de datos

1. Llega payload por transporte y `MessageRouter` lo identifica como JSON.
2. `JsonCommandHandler::processJsonCommand` parsea y extrae:
   - `command` (string)
   - `args` (objeto)
3. `routeCommand` busca comando en `m_commandMap`.
4. Se ejecuta handler/servicio de dominio.
5. Se construye respuesta con `JsonResponseBuilder`.
6. Se envía `QByteArray` JSON compacto por `ITransport`.

## Comandos soportados (mapa actual)

`create_line`, `delete_line`, `list_lines`, `create_area`, `delete_area`, `create_circle`, `delete_circle`, `create_polygon`, `delete_polygon`, `list_shapes`, `create_track`, `delete_track`, `list_tracks`, `ownship_update`, `cpa_start`, `ppp_graph`, `ppp_finish`, `ppp_clear_track`, `estacionamiento_calc`, `estacionamiento_stop`.

## Formato de mensajes

### Request

```json
{
  "command": "create_track",
  "args": {
    "x": 12.5,
    "y": -3.0
  }
}
```

### Response success

```json
{
  "status": "success",
  "command": "create_track",
  "args": {
    "id": 7
  }
}
```

### Response error

```json
{
  "status": "error",
  "command": "create_track",
  "args": {
    "code": "VALIDATION_ERROR",
    "message": "...",
    "details": {
      "field": "x",
      "value": "...",
      "expected": "..."
    }
  }
}
```

## Manejo de errores

- JSON inválido o no-objeto: `INVALID_JSON`.
- Comando no registrado: `UNKNOWN_COMMAND`.
- Errores de validación de campos: respuestas de validación (`INVALID_<FIELD>` y/o `VALIDATION_ERROR`).
- Errores de dominio (servicios): códigos específicos como `SESSION_NOT_FOUND`, `INVALID_SLOT`, etc.
- Si falla el envío por transporte, se loguea warning en `sendResponse`.

## Módulos relacionados

- `docs/architecture.md`
- `docs/modules/handlers.md`
- `docs/modules/services.md`
- `docs/modules/command-context.md`
