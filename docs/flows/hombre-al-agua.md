# Flujo: Hombre al Agua (HA)

## Descripción general

Este flujo documenta la gestión y el cálculo cinemático continuo del módulo de **Rescate de Hombre al Agua (HA)**. Ante el evento de caída de personal por la borda, el sistema fija de forma inmutable la coordenada geográfica del punto de caída y genera un asesoramiento continuo de orientación (azimut verdadero y marcación relativa), distancia en yardas, tiempo estimado de arribo y datos temporales (hora de caída y cronómetro).

El sistema admite cuatro disparadores de inicio mutuamente excluyentes: posición del Buque Propio, posición del cursor (OBM), coordenada geográfica manual (Lat/Lon) y azimut verdadero + distancia en yardas desde el Buque Propio.

---

## Lista de archivos y clases

| Archivo | Clase/Struct | Responsabilidad |
|---|---|---|
| `src/controller/commands/haCommand.cpp` | `HaCommand` | Entrada CLI para iniciar (`--popa`, `--cursor`, `--latlon`, `--az`), detener (`--stop`) y consultar (`--info`) la emergencia. |
| `src/controller/services/haService.cpp` | `HaService` | Orquestador del ciclo de vida de la sesión (start/stop) e integrador con el bucle de actualización. |
| `src/model/ha/haCalculator.cpp` | `HaCalculator` | Motor matemático puro encargado de calcular azimuts, marcación relativa, banda y tiempo de arribo. |
| `src/model/ha/haSessionState.h` | `HaSessionState` | Estructura de datos que persiste el estado dinámico y los resultados calculados de la emergencia. |
| `src/model/ha/haSessionTimer.cpp` | `HaSessionTimer` | Captura y congela el timestamp de inicio; calcula el tiempo transcurrido en formato HH:MM:SS. |
| `src/model/commandContext.h` | `CommandContext` | Contenedor global que aloja la sesión activa `haSession` dentro del pipeline del sistema. |

---

## Clases principales

### HaCommand

- **Rol**: Wrapper CLI encargado del análisis sintáctico de tokens y de la validación de las reglas de negocio de entrada.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Ejecutar | `CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const` | Modula entre los modos operativos (`--info`, `--stop`, o los cuatro disparadores de inicio). |

- **Reglas de Validación CLI**:
  - `--az` (disparador 4) requiere el parámetro adicional `--d` (distancia en yardas, estrictamente `> 0.0`).
  - `--az` debe ser un valor flotante en el rango `[0.0, 360.0)`.
  - `--latlon` requiere los parámetros adicionales `--lat` y `--lon` en rangos geográficos válidos.
  - Los cuatro disparadores son mutuamente excluyentes; no se admite más de uno simultáneamente.

### HaService

- **Rol**: Interfaz de control operativo que resuelve la coordenada del punto de caída según el disparador activo y administra el ciclo de vida de la sesión.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Disparador 1 | `void startSessionAtOwnShip()` | Fija el punto en la posición actual del Buque Propio (Track 0). |
| Disparador 2 | `void startSessionAtCursor(double cursorXDm, double cursorYDm)` | Fija el punto en las coordenadas del cursor OBM. |
| Disparador 3 | `void startSessionAtLatLon(double lat, double lon)` | Fija el punto a partir de una posición geográfica ingresada manualmente. |
| Disparador 4 | `void startSessionAtBearing(double azimuthDeg, double distanceYards)` | Proyecta el punto desde el Buque Propio por azimut verdadero y distancia en yardas. |
| Finalizar | `void stopSession()` | Llama al método `reset()` de la estructura de datos y detiene el cronómetro. |
| Actualización | `void update()` | Extrae la posición cinemática actual del Buque Propio e invoca el recálculo. |

- **Método interno**:
  - `void initSession(double xDm, double yDm)`: método privado común a todos los disparadores. Una vez resuelta la coordenada, fija el punto, inicia el `HaSessionTimer` y congela las horas de caída (local y UTC).

### HaCalculator

- **Rol**: Motor de cómputo puro que, dada la posición actual del Buque Propio y el punto de caída fijo, calcula todos los datos de asesoramiento.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Calcular | `static void calculate(QPointF ownPos, double ownCourseDeg, double ownSpeedDm, QPointF fallPoint, HaSessionState& outState)` | Calcula azimut verdadero, marcación relativa, banda, distancia en yardas y ETA hacia el punto de caída. |

- **Consideraciones técnicas del motor**:
  - **Conversión de unidades**: Utiliza el factor constante $1\text{ DM} = 2000\text{ yardas}$ para expresar la distancia al operador.
  - **Restricción de marcación relativa**: La marcación relativa se fuerza siempre al rango $[0°, 180°]$, devolviendo el valor angular menor hacia el punto.
  - **Tiempo de arribo**: Reutiliza `EstacionamientoCalculator` inyectando como destino el azimut `000°` y distancia `0.0 MN` desde el punto de caída (intercepción directa, distancia cero).
  - **Determinación de banda**: Se calcula según el ángulo relativo del punto respecto a la proa: `ESTRIBOR` `[0°, 180°)`, `BABOR` `(180°, 360°)`, `PROA` en `≈0°` y `POPA` en `≈180°`.

### HaSessionTimer

- **Rol**: Responsable exclusivo del manejo temporal de la emergencia.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Iniciar | `void start()` | Captura y congela el timestamp del sistema operativo al momento del disparo. |
| Hora local | `QString fallTimeLocal() const` | Devuelve la hora de caída en formato `HH:mm:ss` (hora local). |
| Hora UTC | `QString fallTimeUtc() const` | Devuelve la hora de caída en formato `HH:mm:ss` (UTC). |
| Tiempo transcurrido | `QString elapsedTime() const` | Calcula la diferencia entre el timestamp fijo y el instante actual en formato `HH:MM:SS`. |
| Resetear | `void reset()` | Borra el timestamp y vuelve al estado inactivo. |

---

## Flujo de datos (Ciclo de Vida del Comando)

```mermaid
flowchart TD
    A([Inicio: Comando ha recibido]) --> B{"¿Flag detectado?"}

    B -->|"--popa"| D1[HaService::startSessionAtOwnShip]
    B -->|"--cursor=x,y"| D2[HaService::startSessionAtCursor]
    B -->|"--latlon --lat=X --lon=Y"| D3[HaService::startSessionAtLatLon]
    B -->|"--az=X --d=Y"| V1["Validar: az ∈ [0,360) y d > 0"]

    V1 --> VR{"¿Datos válidos?"}
    VR -->|No| E1[Retornar Error CLI] --> FE1([Fin con error])
    VR -->|Sí| D4[HaService::startSessionAtBearing]

    D1 --> BP{"¿Track 0 existe?"}
    BP -->|No| E2[Error: Buque Propio no encontrado] --> FE2([Fin con error])
    BP -->|Sí| INIT

    D2 --> INIT[HaService::initSession xDm, yDm]
    D3 --> PEND[⚠️ Aviso: conversión Lat/Lon pendiente — sesión NO iniciada]
    PEND --> FE4([Fin sin emergencia activa])
    D4 --> INIT

    INIT --> FIX[Fijar punto de caída en haSession]
    FIX --> TIMER[HaSessionTimer::start — congelar hora local y UTC]
    TIMER --> Z1([Emergencia iniciada])

    B -->|"--info"| H{"¿Sesión activa?"}
    H -->|No| I[Error: No hay emergencia activa] --> FE3([Fin con error])
    H -->|Sí| J[Construir reporte con datos de asesoramiento actualizados] --> ZF([Mostrar en Consola])

    B -->|"--stop"| K[HaService::stopSession]
    K --> L[reset en haSession + HaSessionTimer::reset] --> Z2([Emergencia finalizada])

    classDef error fill:#ffcccc,stroke:#cc0000,color:#800000
    classDef ok fill:#ccffcc,stroke:#007700,color:#004400
    classDef pending fill:#fff3cc,stroke:#cc8800,color:#665500
    class E1,E2,I,FE1,FE2,FE3,FE4 error
    class Z1,Z2,ZF ok
    class PEND pending
```

### Flujo CLI (Paso a Paso)

El usuario ejecuta el comando en la consola utilizando cualquiera de las siguientes sintaxis:

```bash
ha --popa
ha --cursor=<xDm>,<yDm>
ha --latlon --lat=<grados> --lon=<grados>
ha --az=<azimut> --d=<distancia_yardas>
ha --stop
ha --info
```

1. `HaCommand::execute` intercepta los argumentos y determina cuál de los seis modos fue invocado.
2. Para el disparador 4 (`--az`, `--d`), el comando valida los rangos antes de continuar:
   - El azimut debe estar en `[0.0, 360.0)`.
   - La distancia debe ser estrictamente `> 0.0` yardas.
3. Si las validaciones fallan, el comando interrumpe la ejecución y retorna un `CommandResult` con estado `false` junto con la ayuda de uso (`usage()`).
4. Si los parámetros son correctos, se invoca el método correspondiente de `HaService`.
5. `HaService` resuelve la coordenada cartesiana del punto de caída (en DM) y llama a `initSession(xDm, yDm)`.
6. `initSession` fija el punto en `CommandContext::haSession`, inicia `HaSessionTimer` y congela `fallTimeLocal` y `fallTimeUtc`.
7. A partir de ese instante, el bucle de actualización continuo toma el control.

---

### Ciclo de Recálculo Continuo (Bucle Activo)

El módulo HA se evalúa de manera continua mediante el temporizador asincrónico de `main.cpp`:

```mermaid
flowchart TD
    T1([updatePositionTimer cada 80ms]) --> T2["HaService::update()"]
    T2 --> T3{"¿Sesión activa?"}
    T3 -->|No| T_END([Retorno inmediato])

    T3 -->|Sí| T4[Obtener posición y cinemática del Track 0]
    T4 --> T5["HaCalculator::calculate()"]

    T5 --> T6[Calcular azimut verdadero BP → punto de caída]
    T6 --> T7["Calcular distancia en yardas (DM × 2000)"]
    T7 --> T8["Calcular marcación relativa restringida a [0°, 180°]"]
    T8 --> T9[Determinar banda: ESTRIBOR / BABOR / PROA / POPA]

    T9 --> T10{"¿ownSpeed > 0 y distancia > 0?"}
    T10 -->|Sí| T11["EstacionamientoCalculator::compute() — destino az=0°, d=0.0 MN"]
    T10 -->|No| T12[etaValid = false, timeToArrivalMin = 0.0]

    T11 --> T13[Actualizar ctx->haSession con resultados]
    T12 --> T13
    T13 --> T14[Actualizar haSession.elapsedTime desde HaSessionTimer]
    T14 --> FT([Métricas disponibles para UI y LPD])

    classDef ok fill:#ccffcc,stroke:#007700,color:#004400
    class FT ok
```

---

## Estructuras de Datos Clave

### Estado de Sesión (`HaSessionState`)

Mantiene la separación limpia entre datos fijos al inicio de la emergencia y métricas dinámicas actualizadas en cada ciclo:

- **Datos de control**: `active`, `fallPointX`, `fallPointY`.
- **Datos temporales fijos**: `fallTimeLocal`, `fallTimeUtc` (congelados al disparar).
- **Dato temporal dinámico**: `elapsedTime` (se recalcula en cada ciclo).
- **Asesoramiento cinemático**: `trueAzimuthDeg`, `relativeBearingDeg`, `banda`, `distanceYards`, `timeToArrivalMin`, `etaValid`.
---

## Manejo de Errores y Casos de Borde

- **Track 0 inexistente**: Si el disparador 1 (`--popa`) o el disparador 4 (`--az`) no encuentran el Track 0 del Buque Propio, el servicio interrumpe `initSession` y retorna un error operativo.
- **Velocidad cero**: Si `ownSpeed == 0.0`, el sistema deshabilita de forma segura `etaValid = false` para evitar divisiones por cero en el cálculo de ETA.
- **Punto de caída sobre el Buque Propio**: Si la distancia al punto es `0.0 DM`, el ETA se inhabilita (`etaValid = false`) y la distancia reportada es `0 yardas`.
- **Disparador Lat/Lon (pendiente)**: Requiere conocer la latitud/longitud del origen cartesiano del radar (punto 0,0 en DM) para poder realizar la conversión geográfica. Hasta que dicho dato esté disponible en el sistema, el disparador imprime un aviso operativo y no inicia la sesión. 
- **Sesión preexistente**: Al invocar cualquier disparador de inicio mientras hay una sesión activa, el sistema sobreescribe el estado anterior mediante `reset()` antes de fijar el nuevo punto, garantizando que no queden residuos de la emergencia anterior.

---

## Módulos Relacionados

- `src/model/commandContext.h` — Estructura general de ejecución.
- `docs/modules/estacionamiento.md` — Pipeline del motor de cálculo reutilizado para el ETA.
- `docs/modules/2w.md` — Pipeline homólogo de formación táctica continua.
