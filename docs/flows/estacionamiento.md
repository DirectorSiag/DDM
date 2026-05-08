# Flujo: Estacionamiento

## Descripción general

Este flujo documenta el cálculo de rumbo/tiempo de estacionamiento táctico entre dos referencias (`track-a`, `track-b`) bajo dos modalidades mutuamente excluyentes:

- `VD` (velocidad objetivo)
- `DU` (duración objetivo)

El pipeline pasa por `EstacionamientoCommand`/`JsonCommandHandler`, `EstacionamientoService` y `EstacionamientoCalculator`, con persistencia opcional de sesiones en `CommandContext` para visualización continua.

## Lista de archivos y clases

| Archivo | Clase/Struct | Responsabilidad |
|---|---|---|
| `src/controller/commands/estacionamientocommand.cpp` | `EstacionamientoCommand` | Entrada CLI para cálculo de estacionamiento. |
| `src/controller/services/estacionamientoservice.h` | `EstacionamientoService`, `CalculationResult`, `OperationResult` | Parseo/validación/orquestación de cálculo. |
| `src/model/estacionamientocalculator.h` | `EstacionamientoCalculator` | Motor matemático puro de estacionamiento. |
| `src/controller/json/jsoncommandhandler.cpp` | `JsonCommandHandler` | Entrada JSON (`estacionamiento_calc`, `estacionamiento_stop`). |
| `src/model/commandContext.h` | `StationingSession` | Persistencia de sesiones activas y resultados recalculables. |

## Clases principales

### EstacionamientoService

- **Rol**: centraliza validación de opciones y resolución de estados cinemáticos antes de invocar el cálculo matemático.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| CLI | `OperationResult executeFromCliArgs(const QStringList& args) const` | Parsea flags `--clave=valor` y devuelve texto operativo. |
| Cálculo | `CalculationResult calculateFromOptions(const QMap<QString, QString>& options) const` | Ejecuta validaciones y cálculo completo. |
| Parse opciones | `static bool parseOptions(...)` | Convierte tokens CLI a mapa de opciones. |
| Validación keys | `static bool validateAllowedOptions(...)` | Restringe parámetros permitidos. |
| Parse campos | `parseDoubleField` / `parseIntegerField` | Conversión y validación de tipos. |
| Carga/validación | `static bool loadAndValidate(...)` | Reglas de negocio (VD/DU excluyentes, ids válidos, distancia>0). |

- **Structs/Tipos definidos**:
- `CalculationResult`
- `OperationResult`
- `CliInput`
- **Dependencias**:
- `CommandContext`
- `TrackService`
- `EstacionamientoCalculator`

### EstacionamientoCalculator

- **Rol**: motor matemático de interceptación/estación relativa.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Compute | `static Result compute(const Input& input)` | Resuelve rumbo, tiempo y velocidad resultante para cumplir condición de estación. |

- **Structs/Tipos definidos**:
- `KinematicState`
- `Input`
- `Result` (`Valid`, `InvalidInput`, `NoPositiveTimeSolution`, `DegenerateSolution`)
- **Dependencias**:
- `RadarMath`

### EstacionamientoCommand

- **Rol**: wrapper CLI del servicio.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Ejecutar | `CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const` | Delega en `EstacionamientoService` y retorna mensaje final. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `EstacionamientoService`

## Flujo de datos

```mermaid
flowchart TD
   A([Inicio: Comando estacionamiento recibido<br/>(JSON o CLI)])
   B{"¿Origen del comando?"}

   J1[JsonCommandHandler]
   J2[EstacionamientoService::calculateFromOptions()]

   C1[EstacionamientoCommand]
   C2[EstacionamientoService::executeFromCliArgs()]

   V1[EstacionamientoService valida parámetros]
   D1{"¿Track A válido en CommandContext?"}
   E1[Error: track_a no encontrado]
   FE1([Fin con error])

   D2{"¿Se especificó Track B?"}
   P1[Usar OwnShip como referencia]
   D3{"¿Track B válido?"}
   E2[Error: track_b no encontrado]
   FE2([Fin con error])

   P2[EstacionamientoCalculator::calculate(input)]
   D4{"¿Modalidad de cálculo?"}
   M1[Con VD: Calcular duración desde velocidad]
   M2[Con DU: Calcular velocidad desde duración]
   M3[Sin modalidad: Cálculo geométrico puro<br/>(rumbo y distancia)]

   P3[EstacionamientoCalculator retorna Result]
   D5{"¿Result válido?"}
   E3[Error: movimiento relativo degenerado]
   FE3([Fin con error])

   P4[Construir respuesta: rumbo, tiempo,<br/>posición de estación]
   P5[Respuesta enviada al solicitante<br/>(JSON o stdout CLI)]
   Z([Fin exitoso])

   A --> B

   B -->|JSON| J1
   J1 --> J2
   J2 --> V1

   B -->|CLI| C1
   C1 --> C2
   C2 --> V1

   V1 --> D1
   D1 -->|No| E1
   E1 --> FE1
   D1 -->|Sí| D2

   D2 -->|No| P1
   P1 --> P2
   D2 -->|Sí| D3
   D3 -->|No| E2
   E2 --> FE2
   D3 -->|Sí| P2

   P2 --> D4
   D4 -->|Con VD| M1
   D4 -->|Con DU| M2
   D4 -->|Sin modalidad| M3

   M1 --> P3
   M2 --> P3
   M3 --> P3

   P3 --> D5
   D5 -->|No| E3
   E3 --> FE3
   D5 -->|Sí| P4

   P4 --> P5
   P5 --> Z

   classDef error fill:#ffcccc,stroke:#cc0000,color:#800000
   classDef ok fill:#ccffcc,stroke:#007700,color:#004400

   class E1,E2,E3,FE1,FE2,FE3 error
   class Z ok
```

### Flujo CLI

1. Usuario ejecuta `estacionamiento --track-a=... --track-b=... --az=... --d=... (--vd=... | --du=...)`.
2. `EstacionamientoCommand::execute` llama `EstacionamientoService::executeFromCliArgs`.
3. El servicio parsea tokens `--k=v`.
4. Valida:
   - claves permitidas
   - ids >= 0
   - `track-b != 0`
   - distancia `d > 0`
   - exactamente una modalidad (`vd` xor `du`)
5. Resuelve estados cinemáticos de A/B:
   - track normal desde `TrackService`
   - ownship (id 0) desde `CommandContext::ownShip` si aplica
6. Construye `EstacionamientoCalculator::Input` y ejecuta `compute`.
7. Si es válido, devuelve rumbo y tiempo en formato operacional.

### Flujo JSON

1. `JsonCommandHandler` procesa `estacionamiento_calc`.
2. Valida `index` en rango [1..10].
3. Recolecta/normaliza `track_a`, `track_b`, `az`, `d`, `vd/du`.
4. Llama `EstacionamientoService::calculateFromOptions`.
5. Si éxito:
   - arma `CommandContext::StationingSession`
   - persiste con `upsertStationingSession`
   - responde valores de rumbo, tiempo y posición de estación.

6. `estacionamiento_stop` elimina sesión por slot (`removeStationingSession`).

### Recalculo continuo

`CommandContext::updateTracks` vuelve a evaluar sesiones activas de estacionamiento usando `EstacionamientoCalculator` conforme se mueven los tracks.

## Manejo de errores

- Errores de parseo de token CLI (`Token invalido ... use --clave=valor`).
- Parámetros no permitidos o faltantes.
- Modalidad inválida (`--vd` y `--du` simultáneos o ambos ausentes).
- Ownship no inicializado cuando `track-a=0`.
- `track-b = 0` prohibido por regla de negocio.
- Resultados matemáticos no resolubles (`no_real_solution`, `no_positive_time_solution`, etc.).
- JSON de slot inválido o sesión inexistente (`INVALID_SLOT`, `SESSION_NOT_FOUND`).

## Módulos relacionados

- `docs/modules/services.md`
- `docs/modules/command-context.md`
- `docs/protocols/json-command-api.md`
- `docs/STATIONING_SYSTEM.md`
