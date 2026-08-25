# Flujo: Fondeo

## Descripción general

Este flujo documenta la gestión y el cálculo cinemático continuo del módulo de **Maniobra de Fondeo**. Su propósito es asistir al operador en la aproximación de precisión del Buque Propio (`Track 0`) hacia un **Punto de Fondeo (PF)** designado, con guía continua a través de un **Punto Auxiliar (PA)** de aproximación intermedia.

El sistema calcula automáticamente en cada ciclo las distancias y azimuts verdaderos hacia el PF y el PA, la marcación relativa al objetivo activo según la fase de la maniobra, y un **panel predictivo de cinco anillos** que recomienda el movimiento de máquinas requerido (ej. AD. TODA, PARA MAQ) indicando la distancia exacta en yardas al umbral de accionamiento.

Al iniciar la maniobra, el módulo publica además la **representación gráfica** en el radar (ver `docs/modules/planFondeo.md`): los 5 anillos de marcha como círculos concéntricos reales (`CircleEntity`, vía `GeometryService`) centrados en el PF con radios `r1..r5` (yardas → DM), más un círculo en el PA con radio igual al umbral de llegada (50 yds). Como PF, PA y radios son estáticos durante toda la sesión, las figuras se crean una sola vez en `startSession()` y se borran en `stopSession()`; el único evento gráfico intermedio es el borrado del círculo del PA al alcanzarlo (transición a Fase 2).

El PF puede ser designado mediante dos modos mutuamente excluyentes: **Track de referencia** (azimut y distancia desde un track existente) o **coordenadas GMS** (latitud y longitud en grados, minutos y segundos).

El módulo es alcanzable por dos vías paralelas que convergen en el mismo `FondeoService`/`ctx->fondeoSession`: la consola CLI (`FondeoCommand`, uso interno/testing) y el pipeline JSON (`JsonCommandHandler`, comandos `fondeo_start`/`fondeo_stop`/`fondeo_info`/`fondeo_tipos`) que consume la UI de botonera (`FondeoWorkspace.qml`).

---

## Lista de archivos y clases

| Archivo | Clase/Struct | Responsabilidad |
|---|---|---|
| `src/controller/commands/fondeoCommand.cpp` | `FondeoCommand` | Entrada CLI para iniciar (`--track` o `--pf-lat-*`), detener (`--stop`) y consultar (`--info`) la maniobra. |
| `src/controller/services/fondeoService.cpp` | `FondeoService` | Orquestador del ciclo de vida de la sesión (start/stop), publicación de figuras en el radar (`createFigures`/`deleteFigures`) e integrador con el bucle de actualización. |
| `src/model/fondeo/fondeoCalculator.cpp` | `FondeoCalculator` | Motor matemático puro encargado de proyectar los puntos y resolver la cinemática de aproximación. |
| `src/model/fondeo/fondeoSessionState.h` | `FondeoSessionState`, `FondeoConfig` | Estructuras de datos que persisten la configuración y el estado dinámico de la maniobra. |
| `src/model/fondeo/fondeoTiposUnidad.h` | `FondeoTiposUnidad`, `RadiosFondeo` | Catálogo de radios de marcha (r1..r5) por tipo de unidad naval. |
| `src/model/enums/enums.h` | `FondeoData::TipoUnidad` | Enum `Q_GADGET` de tipos de unidad (Meko360, Meko140, Patagonia, Otro) + conversión a/desde `QString`. |
| `src/controller/json/jsoncommandhandler.cpp` | `JsonCommandHandler` | Entrada JSON: `fondeo_start`, `fondeo_stop`, `fondeo_info`, `fondeo_tipos`. Vía usada por la UI de botonera. |
| `src/model/commandContext.h` | `CommandContext` | Contenedor global que aloja la sesión activa `fondeoSession` dentro del pipeline del sistema. |

---

## Clases principales

### FondeoCommand

- **Rol**: Wrapper CLI encargado exclusivamente del análisis sintáctico de tokens. Detecta el modo de operación (Track vs. GMS), valida la presencia de parámetros obligatorios y convierte las cadenas de texto a tipos primitivos. No contiene reglas de negocio: delega toda validación semántica y ejecución a `FondeoService`.

- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Ejecutar | `CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const` | Modula entre los modos operativos (`--info`, `--stop`, o inicialización de sesión por Track o GMS). |

- **Validación en CLI**: limitada a la presencia de parámetros obligatorios (`--pa-az`, `--pa-dt`, `--r1` a `--r5`), la detección del modo (`--track` vs. `--pf-lat-deg`), la exclusividad mutua entre modos, y la conversión de tipos primitivos (`toInt`, `toDouble`). Las reglas de negocio (radios decrecientes, track existente, azimut en rango, distancia positiva) se resuelven en `FondeoService`.

### FondeoService

- **Rol**: Interfaz de control operativo. Centraliza las reglas de validación de negocio (exclusividad de modos, validez del azimut del PA, distancia del PA positiva, radios estrictamente decrecientes y positivos, existencia del track de referencia o del Track 0), orquesta la resolución de los puntos estáticos mediante `FondeoCalculator`, y administra el ciclo de vida de la sesión. Coordina también la lógica de transición de fases (PA → PF) y la detención automática.

- **Struct de resultado**:

```cpp
struct FondeoOperationResult {
    bool success;
    QString message;
};
```

- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Iniciar Sesión | `FondeoOperationResult startSession(const FondeoConfig& config)` | Rechaza si ya hay una sesión activa; valida reglas de negocio, resuelve las coordenadas estáticas del PF y el PA, activa la sesión en el contexto y publica las figuras (`createFigures`). |
| Finalizar Sesión | `FondeoOperationResult stopSession()` | Borra las figuras (`deleteFigures`), llama a `reset()` de la sesión y registra el evento en consola. |
| Actualización | `void update()` | Extrae la posición y el rumbo del Buque Propio, delega los cálculos al `FondeoCalculator` y gestiona las transiciones de fase (incluido el borrado del círculo del PA al alcanzarlo) y la detención automática. |
| Publicar figuras | `void createFigures()` (privado) | Crea los 5 anillos (`GeometryService::createCircle`, centro `puntoFondeo`, radios `yardsToDm(r1..r5)`, type 4, verde) y el círculo del PA (centro `puntoAuxiliar`, radio `yardsToDm(50)`, type 5); persiste los IDs en `anillosCircleIds`/`paCircleId`. |
| Borrar figuras | `void deleteFigures()` (privado) | Borra por ID todos los círculos publicados y limpia los campos de estado. |


### FondeoCalculator

- **Rol**: Motor de cómputo puro que opera bajo un esquema de pipeline con funciones estáticas. En la inicialización, proyecta las coordenadas cartesianas del PF y el PA. Durante el ciclo dinámico, actualiza distancias, azimuts, marcación relativa y el panel predictivo, modificando el `FondeoSessionState` por referencia sin alterar su ciclo de vida.

- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Resolver PF (Track) | `static QPointF resolvePuntoFondeo(const FondeoConfig&, const QPointF& trackPos)` | Proyecta el PF utilizando un vector de desplazamiento polar desde un blanco existente. |
| Resolver PF (GMS) | `static QPointF resolvePuntoFondeo(const FondeoConfig&, double ownLatDeg, double ownLonDeg)` | Proyecta el PF convirtiendo coordenadas geodésicas absolutas a DM mediante `RadarMath::latLonToDm`. |
| Resolver PA | `static QPointF resolvePuntoAuxiliar(const QPointF& pf, double paAz, double paDt)` | Proyecta el PA desde el PF usando el azimut y la distancia configurados. |
| Calcular distancias y azimuts | `static void calculateDistAzPfPa(const QPointF& ownPos, FondeoSessionState& out_state)` | Calcula las distancias (DM → yardas) y los azimuts verdaderos desde el BP hacia el PF y el PA, actualizando el estado. |
| Calcular marcación relativa | `static void calculateMarcacionRelativa(double ownCourseDeg, FondeoSessionState& out_state)` | Determina el azimut relativo al objetivo activo (PA en Fase 1, PF en Fase 2) neutralizando el rumbo del BP. |
| Panel predictivo | `static void calculatePanelPredictivo(FondeoSessionState& out_state)` | Evalúa la distancia al PF contra los cinco anillos y actualiza las estructuras `movimientoActual` y `proximoMovimiento` (asignando etiqueta y distancia al anillo correspondiente). |

- **Consideraciones técnicas del motor**:
  - **Conversión de unidades**: `trackDt` (modo Track) está en millas náuticas (Mn) y se convierte a Data Miles (DM) con el factor **1 Mn = 1.012685 DM**. `paDt` está en yardas y se convierte a DM con el factor constante **1 DM = 2000 yardas** (`RadarMath::yardsToDm`). Las distancias de salida (`distanciaPF`, `distanciaPA`, etc.) se calculan en DM y se convierten a yardas con el mismo factor (`RadarMath::dmToYards`).
  - **Proyección trigonométrica**: Los ángulos en grados se convierten a radianes antes del cálculo. El eje Y apunta al Norte y el eje X al Este: `x = x_origen + d * sin(θ)`, `y = y_origen + d * cos(θ)`.
  - **Sobrecarga de Métodos**: El motor implementa *Method Overloading* en `resolvePuntoFondeo` para procesar de forma transparente tanto la geometría polar (Track) como la proyección geodésica directa (GMS).

---

## Flujo de datos (Ciclo de Vida del Comando)

```mermaid
flowchart TD
    A([Inicio: Comando fondeo recibido]) --> B{"¿Flag detectado?"}

    B -->|"--track=ID --az=X --dt=Y ..."| C1[Modo Track]
    B -->|"--pf-lat-deg=X ... "| C2[Modo GMS]
    B -->|"--info"| H{"¿Sesión activa?"}
    B -->|"--stop"| K[FondeoService::stopSession]

    C1 --> VAL[FondeoCommand: validar presencia de --pa-az, --pa-dt, --r1..r5]
    C2 --> VAL

    VAL --> SVC[FondeoService::startSession]
    SVC --> D{"¿Validaciones de negocio OK?"}
    D -->|No| E[Retornar error con mensaje] --> FE1([Fin con error])
    D -->|Sí| F[FondeoCalculator::resolvePuntoFondeo + resolvePuntoAuxiliar]
    F --> G[Persistir PF, PA y config en ctx->fondeoSession]
    G --> G2["createFigures(): 5 anillos en PF + círculo PA"] --> Z1([Sesión Iniciada])

    H -->|No| I[Informar: maniobra no activa] --> ZF([Mostrar en Consola])
    H -->|Sí| J[Construir reporte con estado actual] --> ZF([Mostrar en Consola])

    K --> L2["deleteFigures(): borra anillos + PA"] --> L[fondeoSession.reset] --> Z2([Maniobra Finalizada])

    classDef error fill:#ffcccc,stroke:#cc0000,color:#800000
    classDef ok fill:#ccffcc,stroke:#007700,color:#004400
    class E,I,FE1,FE2 error
    class Z1,Z2,ZF ok
```

### Flujo CLI (Paso a Paso)

El usuario ejecuta el comando utilizando una de las siguientes sintaxis:

```bash
# Modo Track de referencia
fondeo --track=<id> --az=<grados> --dt=<mn> --pa-az=<grados> --pa-dt=<mn> --r1=<yds> --r2=<yds> --r3=<yds> --r4=<yds> --r5=<yds>

# Modo GMS
fondeo --pf-lat-deg=<g> --pf-lat-min=<m> --pf-lat-sec=<s> --pf-lon-deg=<g> --pf-lon-min=<m> --pf-lon-sec=<s> --pa-az=<grados> --pa-dt=<mn> --r1=<yds> --r2=<yds> --r3=<yds> --r4=<yds> --r5=<yds>

fondeo --stop
fondeo --info
```

1. `FondeoCommand::execute` intercepta los argumentos y los procesa en un `QMap<QString, QString>`, normalizando claves a minúsculas y removiendo guiones.
2. El comando detecta el modo (`--track` vs. `--pf-lat-deg`) y verifica exclusividad mutua. Si se activan ambos o ninguno, retorna error inmediato.
3. Se valida la presencia de `--pa-az`, `--pa-dt` y los cinco radios (`--r1` a `--r5`). Solo se verifica presencia y conversión de tipo; la semántica queda en el servicio.
4. Se construye un `FondeoConfig` y se invoca `FondeoService::startSession(config)`.
5. El servicio valida las reglas de negocio. Si alguna falla, devuelve el estado y el mensaje correspondiente mediante la estructura `FondeoOperationResult`.
6. Si todo es válido, el servicio delega al `FondeoCalculator` la proyección estática del PF y el PA, persiste los resultados en `ctx->fondeoSession` y activa la sesión.
7. A partir del inicio exitoso, el bucle de actualización continuo toma el control.

---

### Flujo JSON (Paso a Paso)

Es la vía que usa la UI de botonera (`BackendService.iniciarFondeo/detenerFondeo/consultarFondeo/consultarTiposFondeo` → `FondeoWorkspace.qml`). `JsonCommandHandler` mantiene su propia instancia de `FondeoService` (`m_fondeoService`), separada de la que usa el bucle CLI/timer, pero ambas operan sobre el mismo `ctx->fondeoSession`.

| Comando JSON | Args | Descripción |
|---|---|---|
| `fondeo_start` | `track_id`/`track_az`/`track_dt` (modo Track) **o** `pf_lat_deg/min/sec`, `pf_lon_deg/min/sec` (modo GMS, mutuamente excluyentes) + `pa_az`, `pa_dt`, `r1`..`r5` | Valida presencia/exclusividad de modo y llama `FondeoService::startSession`. |
| `fondeo_stop` | — | Llama `FondeoService::stopSession`. |
| `fondeo_info` | — | Lectura pura de `ctx->fondeoSession` (sin mutar estado): `active`, `pa_alcanzado`, `distancia_pf`, `azimut_pf`, `distancia_pa`, `azimut_pa`, `azimut_relativo`, `distancia_relativa`, `movimiento_actual` (+ `_distancia`), `proximo_movimiento` (+ `_distancia`), `anillos_circle_ids` (array de 5, orden r1..r5) y `pa_circle_id` (`-1` si no existe o ya fue alcanzado). Pensado para polling periódico desde la UI (no hay push espontáneo de JSON en el sistema). |
| `fondeo_tipos` | — | Devuelve el catálogo completo de `FondeoTiposUnidad::todosLosTipos()` con sus 5 radios cada uno. Fetch único (dato de doctrina estático, no se repollea). |

1. `JsonCommandHandler::handleFondeoStart` parsea `args` a mano (mismo estilo que `handleEstacionamiento`, sin `JsonValidator`) construyendo un `FondeoConfig`.
2. Llama `FondeoService::startSession(config)` y traduce `FondeoOperationResult` a `JsonResponseBuilder::buildSuccessResponse`/`buildErrorResponse`.
3. `handleFondeoInfo` no valida ni muta nada: solo serializa el `FondeoSessionState` actual a JSON.
4. `handleFondeoTipos` recorre `FondeoTiposUnidad::todosLosTipos()` y arma un array `{nombre, r1..r5}` por tipo.

---

### Ciclo de Recálculo Continuo (Bucle Activo)

La maniobra se evalúa de forma continua mediante el temporizador asincrónico de `main.cpp`:

```mermaid
flowchart TD
    T1([updatePositionTimer cada 80ms]) --> T2["FondeoService::update()"]
    T2 --> T3{"¿Sesión activa?"}
    T3 -->|No| T_END([Retorno inmediato])

    T3 -->|Sí| T4[Obtener posición y rumbo del Track 0]
    T4 --> T5["FondeoCalculator::calculateDistAzPfPa()"]
    T5 --> T6{"¿distanciaPA <= 50.0 y !paAlcanzado?"}

    T6 -->|Sí| T7[paAlcanzado = true — Transición a Fase 2 — borra círculo del PA]
    T6 -->|No| T8{"¿paAlcanzado y distanciaPF <= 15.0?"}

    T7 --> T8
    T8 -->|Sí| T9["Imprimir éxito → stopSession()"] --> FT_STOP([Sesión finalizada: PF alcanzado])
    T8 -->|No| T10["FondeoCalculator::calculateMarcacionRelativa()"]

    T10 --> T11["FondeoCalculator::calculatePanelPredictivo()"]
    T11 --> T12[Actualizar ctx->fondeoSession]
    T12 --> FT([Métricas disponibles para UI y LPD])

    classDef ok fill:#ccffcc,stroke:#007700,color:#004400
    classDef stop fill:#fff3cd,stroke:#cc8800,color:#664400
    class FT ok
    class FT_STOP stop
```

---

## Estructuras de Datos Clave

### Asesoramiento de Movimiento (`AsesoramientoMovimiento`)


Estructura anidada que representa un estado del panel predictivo.

- **Orden de máquinas**: `label` Cadena de texto con la orden de máquinas (ej. "AD. TODA", "MAQ AT").
- **Distancia al anillo**: `distancia` Valor decimal (double) con la distancia remanente en yardas hacia el anillo correspondiente.

### Configuración de Sesión (`FondeoConfig`)

Estructura inmutable durante la sesión. Almacena todos los parámetros ingresados por el operador al momento de invocar el comando:

- **Selectores de modo**: `useTrack`, `useGms` (mutuamente excluyentes).
- **Modo Track**: `trackId`, `trackAz` (grados), `trackDt` (Mn).
- **Modo GMS**: `pfLatDeg`, `pfLatMin`, `pfLatSec`, `pfLonDeg`, `pfLonMin`, `pfLonSec`.
- **Punto Auxiliar**: `paAz` (grados), `paDt` (yardas).
- **Radios de marcha**: `r1` a `r5` (yardas), con la invariante `r1 > r2 > r3 > r4 > r5 > 0`.

### Estado de Sesión (`FondeoSessionState`)

Mantiene la separación limpia entre datos estáticos fijados al inicio y métricas dinámicas actualizadas en cada ciclo:

- **Control de sesión**: `active`, `paAlcanzado` (bandera de transición de fase).
- **Puntos estáticos**: `puntoFondeo` (QPointF en DM), `puntoAuxiliar` (QPointF en DM).
- **Figuras publicadas**: `anillosCircleIds` (5 IDs de `CircleEntity`, orden paralelo a r1..r5), `paCircleId` (`NO_CIRCLE = -1` si no existe).
- **Configuración**: `config` (instancia de `FondeoConfig`).
- **Telemetría dinámica**: `distanciaPF`, `azimutPF`, `distanciaPA`, `azimutPA` (distancias en yardas, azimuts en grados).
- **Asesoramiento activo**: `azimutRelativo`, `distanciaRelativa` (referenciados al objetivo de la fase actual: PA o PF).
- **Panel predictivo**: `movimientoActual`, `proximoMovimiento` (estructura AsesoramientoMovimiento que contiene la etiqueta técnica del comando y la distancia en yardas restante hacia el anillo correspondiente).

---

## Manejo de Errores y Casos de Borde

- **Validación de Señal Geográfica (GPS)**: Al iniciar la maniobra usando coordenadas GMS, el servicio verifica primero que el Buque Propio (`m_ctx->ownShip`) tenga datos reales de posicionamiento. Si la unidad no tiene señal o su posición no es válida, se aborta el comando para evitar proyectar coordenadas desde el origen `(0,0)`.
- **Sesión ya activa**: `FondeoService::startSession` rechaza un segundo inicio mientras haya una maniobra en curso (antes hacía `reset()` implícito; con figuras publicadas eso dejaría círculos y cursores huérfanos en el radar). Para reconfigurar: `fondeo --stop` / `fondeo_stop` y reiniciar.
- **Exclusividad mutua de modos**: `FondeoCommand` rechaza de inmediato si se detectan simultáneamente `--track` y `--pf-lat-deg`, o si no se detecta ninguno de los dos.
- **Radios no decrecientes**: `FondeoService` rechaza la sesión si no se cumple estrictamente `r1 > r2 > r3 > r4 > r5 > 0`, protegiendo la lógica de la máquina de estados del panel predictivo.
- **Azimut del PA fuera de rango**: `FondeoService` valida que `paAz ∈ [0, 360)` antes de iniciar.
- **Track de referencia inexistente**: Si el modo Track está activo y `findTrackById(config.trackId)` devuelve `nullptr`, la sesión no se inicia y se retorna un error descriptivo.
- **Proyección estática e inmunidad a pérdida de track**: Las coordenadas del PF y el PA se calculan una sola vez durante la inicialización. Si el track de referencia desaparece durante la maniobra, los puntos persisten sin riesgo de punteros nulos, y la maniobra continúa normalmente.
- **Normalización angular**: Todos los azimuts de salida se normalizan al rango `[0, 360)` mediante `RadarMath::normalizeAngle360` y la operación módulo, previniendo valores negativos o ≥360° en la interfaz.
- **Detención automática por éxito**: Cuando `paAlcanzado == true` y `distanciaPF <= 15.0`, el servicio imprime un mensaje de éxito y llama a `stopSession()` automáticamente, sin intervención del operador — lo que borra también todas las figuras publicadas.

---

## Módulos Relacionados

- `docs/modules/planFondeo.md` — Diseño de la conexión cálculo → figuras (anillos + PA), decisiones y puntos abiertos.
- `src/model/commandContext.h` — Estructura general de ejecución.
- `docs/protocols/json-command-api.md` — Protocolo JSON general (`fondeo_start`/`fondeo_stop`/`fondeo_info`/`fondeo_tipos` forman parte del mapa de comandos de `JsonCommandHandler`).
- `docs/modules/commands.md` — Registro CLI de `FondeoCommand` en `CommandRegistry`.
- `docs/modules/ha.md` — Pipeline homólogo de emergencia con conversión GMS y cálculo cinemático continuo.
- `docs/modules/2w.md` — Pipeline homólogo de formación táctica continua.
