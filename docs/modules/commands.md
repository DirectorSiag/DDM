# Módulo: Commands

## Descripción general

Este módulo implementa la interfaz de comandos de consola del backend DDM usando el patrón Command. Cada comando concreto implementa `ICommand` y opera sobre `CommandContext` compartido. La ejecución se coordina con `CommandRegistry` y `CommandDispatcher`. Lo usamos exclusivamente para testing, no es algo que sirva para el usuario final.

Su propósito es exponer operaciones operativas/tácticas sin frontend GUI: creación/borrado/listado de tracks y cursores, manejo de ownship, SITREP, display mode, CPA y estacionamiento, además de comandos de geometría.

En la arquitectura general, este módulo es la entrada CLI paralela al pipeline JSON (`JsonCommandHandler`). Ambos convergen en el mismo estado (`CommandContext`).

## Lista de archivos y clases

| Archivo | Clase/Struct | Responsabilidad |
|---|---|---|
| `src/controller/commands/iCommand.h` | `ICommand` | Contrato base de comandos (nombre, descripción, uso y ejecución). |
| `src/controller/commands/iCommand.h` | `CommandInvocation` | Estructura parseada de entrada CLI (name/path/opts/args). |
| `src/controller/commands/iCommand.h` | `CommandResult` | Resultado estandarizado (`ok`, `message`). |
| `src/controller/commandRegistry.h` | `CommandRegistry` | Registro de comandos `QSharedPointer<ICommand>` y búsqueda por nombre. |
| `src/controller/commanddispatcher.h` | `CommandDispatcher` | Parseo, despacho y ejecución de comandos; maneja `help` y `exit/salir`. |
| `src/controller/commands/addCommand.h` | `AddCommand` | Alta de tracks tácticos con parámetros completos. |
| `src/controller/commands/deleteCommand.h` | `DeleteCommand` | Baja de track por id. |
| `src/controller/commands/listCommand.h` | `ListCommand` | Listado tabular de tracks. |
| `src/controller/commands/centerCommand.h` | `CenterCommand` | Actualización del centro global. |
| `src/controller/commands/addCursor.h` | `AddCursorCommand` | Alta de cursor/línea por CLI. |
| `src/controller/commands/listcursorscommand.h` | `ListCursorsCommand` | Listado de cursores. |
| `src/controller/commands/deletecursorscommand.h` | `DeleteCursorsCommand` | Baja de cursor por id. |
| `src/controller/commands/sitrepcommand.h` | `SitrepCommand` | Operaciones SITREP (`list/watch/delete/info`). |
| `src/controller/commands/cpaCommand.h` | `CpaCommand` | Comando CLI para cálculo CPA entre tracks. |
| `src/controller/commands/ownshipcommand.h` | `OwnShipCommand` | Mostrar/actualizar ownship desde CLI. |
| `src/controller/commands/estacionamientocommand.h` | `EstacionamientoCommand` | Cálculo de estacionamiento con flags de entrada. |
| `src/controller/commands/displaymodecommand.h` | `DisplayModeCommand` | Cambio de modo de movimiento (`relative`/`true_motion`). |
| `src/controller/commands/addareacommand.h` | `AddAreaCommand` | Alta de área táctica de 4 puntos. |
| `src/controller/commands/addCircleCommand.h` | `AddCircleCommand` | Alta de círculo táctico. |
| `src/controller/commands/addpolygonocommand.h` | `AddPolygonoCommand` | Alta de polígono táctico de N puntos. |
| `src/controller/commands/deleteAreaCommand.h` | `DeleteAreaCommand` | Baja de área por id. |
| `src/controller/commands/deleteCircleCommand.h` | `DeleteCircleCommand` | Baja de círculo por id. |

## Clases principales

### ICommand

- **Rol**: Interfaz base del patrón Command para desacoplar parser/dispatcher de la lógica de cada operación CLI.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Nombre | `virtual QString getName() const = 0` | Nombre invocable en CLI. |
| Descripción | `virtual QString getDescription() const = 0` | Texto para `help`. |
| Uso | `virtual QString usage() const = 0` | Sintaxis esperada del comando. |
| Ejecutar | `virtual CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const = 0` | Lógica del comando sobre estado compartido. |

- **Structs/Tipos definidos**:
- `CommandInvocation`: `name`, `path`, `opts`, `args`.
- `CommandResult`: `ok`, `message`.
- **Dependencias**:
- `CommandContext`

### CommandRegistry

- **Rol**: Mantiene el catálogo de comandos concretos registrados al inicio de la aplicación.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Registrar | `void registerCommand(QSharedPointer<ICommand> cmd)` | Inserta comando por `getName()`. |
| Buscar | `QSharedPointer<ICommand> find(const QString& name) const` | Retorna comando para ejecución. |
| Nombres | `QStringList names() const` | Lista de nombres registrados. |
| Todos | `QList<QSharedPointer<ICommand>> all() const` | Lista completa para help/inspección. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `ICommand`

### CommandDispatcher

- **Rol**: Punto de entrada de la consola. Parsea texto con `IInputParser`, resuelve comando en `CommandRegistry`, ejecuta, imprime resultado y maneja salida del proceso.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor | `CommandDispatcher(CommandRegistry* reg, IInputParser* parser, CommandContext& ctx, QObject* parent = nullptr)` | Inyecta dependencias de despacho. |
| Entrada de línea | `void onLine(const QString& line)` | Maneja parseo, `help`, `exit/salir`, lookup y ejecución. |
| Señal de salida | `void quitRequested()` | Señal para finalizar `QCoreApplication`. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `CommandRegistry`
- `IInputParser`
- `CommandContext`
- `ICommand`

### AddCommand

- **Rol**: Comando más completo de alta de tracks; acepta modo moderno (`--type`, `--id`, `--spd`, `--crs`, etc.) y compatibilidad legacy (`-f/-e/-u`, etc.).
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Nombre | `QString getName() const override` | Retorna `"add"`. |
| Uso | `QString usage() const override` | Describe sintaxis extendida de alta de track. |
| Ejecutar | `CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override` | Valida/parcea opciones y delega creación de track. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `TrackService` (en implementación)
- `CommandContext`

### SitrepCommand

- **Rol**: Interfaz CLI para operaciones de reporte táctico (listado/observación y edición de info asociada a tracks).
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Uso | `QString usage() const override` | Expone subcomandos `list`, `watch`, `delete`, `info`. |
| Ejecutar | `CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override` | Aplica operación SITREP solicitada. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `SitrepService` (en implementación)
- `CommandContext`

## Referencia de comandos

| Nombre del comando | Clase | Sintaxis CLI | Descripción |
|---|---|---|---|
| `add` | `AddCommand` | `add <--type <SPC|LINCO|ASW|OPS|HECO|APC|AAW|EW>|-f|-e|-u> [identidad] <x> <y> [legacyVelKnots] [legacyCourseDeg]` | Crea un track táctico. |
| `delete` | `DeleteCommand` | `delete <id>` | Elimina track por id. |
| `list` | `ListCommand` | `list` | Lista tracks actuales. |
| `center` | `CenterCommand` | `center <x> <y>` | Actualiza centro global. |
| `addCursor` | `AddCursorCommand` | `addCursor <tipoLinea> <x> <y> <largo> <angulo>` | Crea cursor/línea. |
| `listcursors` | `ListCursorsCommand` | `listcursors` | Lista cursores. |
| `deletecursor` | `DeleteCursorsCommand` | `deletecursor <id>` | Elimina cursor por id. |
| `sitrep` | `SitrepCommand` | `sitrep [list]` / `sitrep watch` / `sitrep delete <trackId>` / `sitrep info <trackId> <texto>` | Operaciones SITREP. |
| `cpa` | `CpaCommand` | `Uso: cpa <trackId1> <trackId2>` | Calcula/consulta CPA entre tracks (comando CLI específico). |
| `ownship` | `OwnShipCommand` | `ownship [show] | ownship set <course_deg> <speed_knots> [source]` | Muestra/actualiza ownship. |
| `estacionamiento` | `EstacionamientoCommand` | `estacionamiento [--track-a=<id|0000>] --track-b=<id_externo> --az=<deg> --d=<dm> (--vd=<knots> | --du=<hours>)` | Calcula rumbo/tiempo de estacionamiento. |
| `display` | `DisplayModeCommand` | `display mode <relative|true|true_motion|show>` | Cambia modo de movimiento del display. |
| `addArea` | `AddAreaCommand` | `addArea(ax,ay,bx,by,cx,cy,dx,dy,tipo,color)` | Crea área táctica. |
| `addCircle` | `AddCircleCommand` | `addCircle(x,y,radius,type,color)` | Crea círculo táctico. |
| `addPolygono` | `AddPolygonoCommand` | `addPolygono(x1, y1, x2, y2, ..., xn, yn, tipo, color)` | Crea polígono táctico. |
| `deleteArea` | `DeleteAreaCommand` | `deleteArea(id)` | Elimina área y cursores asociados. |
| `deleteCircle` | `DeleteCircleCommand` | `deleteCircle(id)` | Elimina círculo y cursores asociados. |

## Flujo de datos

1. `StdinReader` emite `lineRead` desde hilo de I/O.
2. `CommandDispatcher::onLine` parsea texto con `IInputParser`.
3. Si es `help` o `exit/salir`, se resuelve localmente en dispatcher.
4. Para comandos de dominio: `CommandRegistry::find(name)` retorna implementación `ICommand`.
5. `ICommand::execute` muta/consulta `CommandContext` (directamente o vía services).
6. Resultado (`CommandResult`) se imprime por streams `ctx.out/ctx.err`.

## Manejo de errores

- Errores de parseo de línea: `CommandDispatcher` reporta “Error de parseo” y sugiere `help`.
- Comando desconocido: `CommandDispatcher` retorna mensaje explícito y sugiere `help`.
- Errores de argumentos/validación de cada comando:
  - cantidad incorrecta de argumentos
  - conversiones numéricas inválidas
  - ids inexistentes
  - opciones fuera de rango
- Patrón uniforme: `CommandResult { ok=false, message=... }` sin abortar proceso.

## Módulos relacionados

- `docs/architecture.md`
- `docs/modules/command-context.md`
- `docs/modules/services.md`
- `docs/README_BACKEND_ARCHITECTURE.md`
