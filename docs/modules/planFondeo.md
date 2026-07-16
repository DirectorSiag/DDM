# Módulo: Fondeo — Conexión con Creación de Figuras

## Descripción general

El módulo de Maniobra de Fondeo ya tiene su motor de cálculo implementado y funcionando: `FondeoService::update()` recalcula, cada 80 ms desde el timer de `main.cpp`, las distancias/azimuts del Buque Propio (Track 0) hacia el Punto de Fondeo (PF) y el Punto Auxiliar (PA), la marcación relativa según la fase, y el panel predictivo de cinco anillos (`AD. TODA` → `MAQ. AT`), dejando todo en `FondeoSessionState` (`CommandContext::fondeoSession`).

El problema que resuelve este documento es el mismo que resolvió `docs/modules/2W-Figuras.md` (rama `connection2w`) para el módulo 2W: ese cálculo hoy es un callejón sin salida gráfico. Nada en el código toma `puntoFondeo`, `puntoAuxiliar` ni los radios `r1..r5` y los convierte en figuras reales sobre el radar. El operador recibe el asesoramiento por texto (`fondeo --info` / `fondeo_info`), pero los cinco anillos de marcha — que son literalmente círculos con radios en yardas centrados en el PF — nunca se dibujan, y el PA (el punto hacia el que navega en toda la Fase 1) tampoco tiene representación.

Este documento define cómo conectar `FondeoService` con la infraestructura de figuras ya existente (`GeometryService` / `CircleEntity` / `ctx.circles` / `ctx.cursors`), siguiendo la lógica ya validada en 2W pero aprovechando una diferencia estructural clave del Fondeo que lo hace más simple (ver siguiente sección).

Fuentes de especificación (Google Drive, no versionadas en el repo — son el "documento de especificación FONDEO" que cita el comentario de `fondeoCalculator.cpp:34`):

- **`[3] FONDEO - Ilacqua.docx`** — requerimiento original: pide explícitamente "una representación gráfica de los círculos de distancia concéntricos al punto de fondeo, cada uno con un radio igual a la indicación de máquinas a sugerir", graficar PF y PA al iniciar, y un "segmento punteado de asesoramiento buque-punto" (ver Puntos abiertos).
- **`[3] FONDEO.docx`** — spec de UI/UX (secciones A/B/C de la botonera): confirma PF por lat/long o az/dt de track, PA por az/dt en yardas desde el PF, y define el estilo de los círculos: "Color line: green, stroke: 2px".

**Prerrequisito acordado (2026-07-15):** antes de implementar este plan se mergea la rama `connection2w` en esta rama. El plan no depende de nada de esa rama (no usa `updateCircle`/`syncFigures`), pero el merge trae los `type=1/2/3` de 2W con los que este documento evita colisionar, y la infraestructura que el futuro segmento punteado podría reutilizar.

## Principio de diseño: PF y PA son estáticos — figuras que se crean una vez y no se reposicionan

En 2W todos los centros de círculo dependían de la posición **actual** del track Guía, releída en cada tick de 80 ms; por eso `2W-Figuras.md` tuvo que introducir `GeometryService::updateCircle` (reposicionar in-place preservando ID), un `syncFigures()` por tick y un guard "skip-if-unchanged" para no regenerar 36 `CursorEntity` por círculo por ciclo.

En Fondeo la dependencia de datos es la opuesta, y esto **ya está implementado y no cambia**:

- `s.puntoFondeo` se resuelve **una única vez** en `FondeoService::startSession()` (`fondeoservice.cpp:42/54`), a partir de la posición del track de referencia *en ese instante* (`FondeoCalculator::resolvePuntoFondeo(config, trackPos)`, proyección polar `trackAz`/`trackDt` en Mn) o de coordenadas GMS absolutas (`resolvePuntoFondeo(config, ownLat, ownLon)` vía `RadarMath::latLonToDm`). El resultado queda en DM.
- `s.puntoAuxiliar` se resuelve inmediatamente después (`fondeoservice.cpp:57`) como proyección polar desde el PF (`resolvePuntoAuxiliar`, `paAz` en grados, `paDt` en **yardas** → DM vía `RadarMath::yardsToDm`).
- Ninguno de los dos puntos se recalcula jamás durante la sesión: `update()` solo mide distancias del BP *hacia* ellos. La doc de flujo ya lo destaca como virtud ("inmunidad a pérdida de track": si el track de referencia desaparece, los puntos persisten).
- Los radios de los anillos (`config.r1..r5`, yardas) son inmutables durante la sesión por definición de `FondeoConfig`.

Consecuencia práctica: **las figuras de Fondeo se crean una vez en `startSession()` y se borran en `stopSession()`; no existe ningún dato que las haga moverse ni cambiar de tamaño entre medio.** Por lo tanto:

- No hace falta `GeometryService::updateCircle` (que en esta rama `ConnectionFondeo` no existe — solo está en `connection2w`). Con `createCircle`/`deleteCircle`, ya presentes acá, alcanza.
- No hace falta un `syncFigures()` por tick ni guards anti-churn de cursores. El único evento gráfico durante la sesión es puntual: borrar el círculo del PA cuando se alcanza (ver más abajo).
- Cuando eventualmente se mergee `connection2w`, no hay conflicto conceptual: Fondeo simplemente no usa `updateCircle`.

## Diseño propuesto

### 1. Qué se dibuja

| Figura | Cantidad | Centro (dato de origen) | Radio (dato de origen) |
|---|---|---|---|
| Anillos de marcha | 5 | `s.puntoFondeo` (DM, fijado en `startSession`) | `RadarMath::yardsToDm(config.r1..r5)` |
| Círculo del PA | 1 | `s.puntoAuxiliar` (DM, fijado en `startSession`) | `RadarMath::yardsToDm(50.0)` — mismo umbral de llegada al PA que ya usa `update()` (`distanciaPA <= 50.0`, `fondeoservice.cpp:95`); se extrae a una constante compartida `kPaArrivalYds` para que umbral y figura no puedan divergir |

Decisiones tomadas (confirmadas con Cristian, 2026-07-15):

1. **El PF no lleva marcador propio.** La spec de Ilacqua dice "se graficarán ambos puntos"; se interpreta así: el PA tiene su círculo, y el PF queda señalado por el centro común de los cinco anillos (`r5`, el anillo más chico — 50 yds en Meko 360 — funciona como marcador de facto). Se descartó el patrón de "marcador táctico" tipo CPA/Estacionamiento (símbolo fijo + número) por la misma razón que en 2W: no escala con radios en yardas y el mecanismo tiene el mapeo de color pendiente (`// cambiar aca`).
2. **El círculo del PA se borra al alcanzarlo.** Cuando `update()` detecta `distanciaPA <= kPaArrivalYds` y pone `paAlcanzado = true` (transición a Fase 2), en ese mismo punto se borra el círculo del PA: ya cumplió su función de guía de Fase 1 y dejarlo solo ensucia la aproximación final. Los anillos permanecen todos hasta el final de la sesión.
3. **Los anillos no se van "apagando" al cruzarlos.** El panel predictivo ya comunica la fase por texto; borrar anillos a medida que `distanciaPF` cruza cada `r_i` agregaría eventos gráficos por tick sin requerimiento que lo pida. Queda anotado como punto abierto si doctrina lo pide después.
4. **Los radios válidos de Meko 360 son los del código** (`fondeoTiposUnidad.h`: 1500/1000/800/100/50 yds, marcados "confirmado"), no los del ejemplo de la spec de UI (2000/1500/800/100/50). Los anillos se instancian convirtiendo esos valores de yardas a DM (`RadarMath::yardsToDm`), igual que cualquier `r1..r5` que llegue por parámetro.
5. **El segmento punteado BP → punto más cercano queda fuera de esta iteración** (ver Puntos abiertos): es el único elemento dinámico que pide la spec y se diseña aparte, después del merge con `connection2w`.

### 2. Estado nuevo en `FondeoSessionState`

```cpp
// src/model/fondeo/fondeoSessionState.h
static constexpr int NO_CIRCLE = -1;

QList<int> anillosCircleIds;   // 5 IDs, orden paralelo a r1..r5
int paCircleId = NO_CIRCLE;
```

`reset()` (ya existente, `*this = FondeoSessionState{}`) limpia estos campos automáticamente — pero las figuras deben borrarse **antes** de llamar a `reset()`, nunca después (misma regla que en 2W).

### 3. Tipos y colores (provisionales)

```cpp
// fondeoservice.cpp (constantes de archivo)
static constexpr int kAnilloCircleType = 4;
static constexpr int kPaCircleType     = 5;

static const QString kAnilloColor = QStringLiteral("#00FF00"); // verde — spec UI: "Color line: green, stroke: 2px"
static const QString kPaColor     = QStringLiteral("#FF00FF"); // magenta (provisional)
```

El verde de los anillos viene de la spec de UI (`[3] FONDEO.docx`, sección B); el "stroke 2px" no tiene representación en `CircleEntity` ni en el protocolo (los círculos son 36 segmentos de cursor sin grosor) y queda del lado del renderer. Se eligen `type=4/5` para no colisionar con los `1/2/3` que 2W reservó para guía/propio/aliadas en `connection2w`. Aplica exactamente el mismo punto abierto que en 2W: `encoderLPD::appendCursorLong` solo transmite 3 bits de `lineType` (0-7) y no existe tabla `type → color` en el renderer; el string `color` de `CircleEntity` no viaja por el protocolo binario y solo es visible vía JSON (`list_shapes`).

### 4. `FondeoService::createFigures()` / `deleteFigures()`

Dos métodos privados nuevos en `FondeoService`. Igual que en 2W, `GeometryService` se instancia localmente con el mismo `CommandContext` (`GeometryService geometry(m_ctx);`) — es un servicio sin estado propio.

```cpp
void FondeoService::createFigures()
{
    FondeoSessionState& s = m_ctx->fondeoSession;
    GeometryService geometry(m_ctx);

    const double radiosYds[5] = { s.config.r1, s.config.r2, s.config.r3, s.config.r4, s.config.r5 };
    for (double rYds : radiosYds) {
        const GeometryResult r = geometry.createCircle(
            s.puntoFondeo, RadarMath::yardsToDm(rYds), kAnilloCircleType, kAnilloColor);
        s.anillosCircleIds.append(r.success ? r.id : FondeoSessionState::NO_CIRCLE);
    }

    const GeometryResult rPa = geometry.createCircle(
        s.puntoAuxiliar, RadarMath::yardsToDm(kPaArrivalYds), kPaCircleType, kPaColor);
    s.paCircleId = rPa.success ? rPa.id : FondeoSessionState::NO_CIRCLE;
}

void FondeoService::deleteFigures()
{
    FondeoSessionState& s = m_ctx->fondeoSession;
    GeometryService geometry(m_ctx);

    for (int id : s.anillosCircleIds) {
        if (id != FondeoSessionState::NO_CIRCLE) geometry.deleteCircle(id);
    }
    s.anillosCircleIds.clear();

    if (s.paCircleId != FondeoSessionState::NO_CIRCLE) {
        geometry.deleteCircle(s.paCircleId);
        s.paCircleId = FondeoSessionState::NO_CIRCLE;
    }
}
```

`createFigures()` solo **lee** `s.puntoFondeo` / `s.puntoAuxiliar` / `s.config.r1..r5` — nunca calcula una posición; esos valores ya fueron escritos por `FondeoCalculator` dentro de `startSession()`, antes de invocarlo. La validación de negocio `r1 > r2 > r3 > r4 > r5 > 0` (ya existente en `startSession`) garantiza que los cinco `createCircle` pasan el chequeo `radius > 0` de `GeometryService`.

### 5. Enganches en el ciclo de vida (`startSession` / `stopSession` / `update`)

**`startSession()`** gana un guard nuevo y la llamada de creación:

1. **Rechazar si ya hay una sesión activa** (nuevo, mismo criterio que 2W). Hoy una segunda invocación hace `reset()` y pisa la sesión silenciosamente; con figuras, ese `reset()` limpiaría `anillosCircleIds`/`paCircleId` sin haberlas borrado → 6 círculos y 216 cursores huérfanos en el radar. Es un cambio de comportamiento deliberado: para reconfigurar, el operador hace `fondeo_stop` y vuelve a iniciar.
2. Al final, después de `active = true`: llamar `createFigures()`. Las figuras aparecen en el mismo instante del inicio, no en el siguiente tick.

**`stopSession()`** llama `deleteFigures()` **antes** de `m_ctx->fondeoSession.reset()`. Esto cubre gratis los tres caminos de finalización, porque todos convergen en `stopSession()`:

- `fondeo --stop` (CLI) y `fondeo_stop` (JSON).
- La **detención automática por éxito** en `update()` (`paAlcanzado && distanciaPF <= 15.0` → `stopSession()`, `fondeoservice.cpp:102`): al fondear, anillos y PA desaparecen solos del próximo `buildFullMessage()`.

**`update()`**: en el bloque de transición de fase ya existente (`fondeoservice.cpp:95-98`, donde se imprime "Se ha alcanzado el Punto Auxiliar" y se setea `paAlcanzado = true`), agregar el borrado puntual del círculo del PA (`deleteCircle(s.paCircleId)` + `s.paCircleId = NO_CIRCLE`). Es el único evento gráfico entre el inicio y el fin de la sesión.

Nota sobre instancias: existen tres instanciaciones de `FondeoService` (la del timer en `main.cpp`, la persistente `m_fondeoService` de `JsonCommandHandler` y una local por invocación en `FondeoCommand::execute`). Ninguna guarda estado propio: los IDs de círculo viven en `ctx->fondeoSession`, compartido por todas, así que cualquiera puede crear/borrar las figuras de forma consistente — mismo esquema que ya funciona hoy con `active`/`paAlcanzado`.

### 6. Exposición JSON y CLI

**No hay comandos nuevos** (a diferencia de 2W, que no tenía ninguno: `fondeo_start`/`fondeo_stop`/`fondeo_info`/`fondeo_tipos` ya existen). Solo se extiende la visibilidad:

- `handleFondeoInfo` (`jsoncommandhandler.cpp:721`) agrega dos campos de solo lectura: `anillos_circle_ids` (array de 5 int, orden r1..r5) y `pa_circle_id` (int, `-1` si no existe/ya alcanzado). Un frontend puede correlacionarlos con `list_shapes`.
- El bloque `fondeo --info` de `FondeoCommand::execute` (`fondeoCommand.cpp:26-50`) imprime los mismos IDs, para verificación por consola.

## Archivos y responsabilidades

| Archivo | Cambio | Responsabilidad |
|---|---|---|
| `src/model/fondeo/fondeoSessionState.h` | Modificar | Agregar `NO_CIRCLE`, `anillosCircleIds`, `paCircleId`. |
| `src/controller/services/fondeoservice.h` | Modificar | Declarar `void createFigures();` y `void deleteFigures();` privados. |
| `src/controller/services/fondeoservice.cpp` | Modificar | Constantes `kAnilloCircleType`/`kPaCircleType`/colores/`kPaArrivalYds`; guard "ya activa" + `createFigures()` en `startSession`; `deleteFigures()` en `stopSession`; borrado del círculo PA en la transición de fase de `update()`. |
| `src/controller/commands/fondeoCommand.cpp` | Modificar (menor) | Extender la salida de `fondeo --info` con los IDs de círculos. |
| `src/controller/json/jsoncommandhandler.cpp` | Modificar (menor) | Extender `handleFondeoInfo` con `anillos_circle_ids`/`pa_circle_id`. |
| `docs/flows/fondeo.md` | Modificar | Documentar la publicación gráfica en el ciclo de vida (start/stop/transición PA). |
| `docs/modules/services.md` | Modificar (menor) | Responsabilidad ampliada de `FondeoService` (ciclo de vida de figuras). |

No requieren cambios: `geometryservice.{h,cpp}` (con `createCircle`/`deleteCircle` alcanza — no se necesita `updateCircle`), `circleEntity.{h,cpp}`, `commandContext.h`, `fondeoCalculator.{h,cpp}`, `lpdEncoder`/`encoderLPD` (los círculos viajan por el broadcast completo de `ctx.cursors` ya existente), `fondeoTiposUnidad.h`, `enums.h`.

## Ciclo de vida / flujo de datos

```
CLI:  "fondeo --track=5 --az=90 --dt=2 --pa-az=180 --pa-dt=2000 --r1..--r5=..."
JSON: {"command":"fondeo_start","args":{...}}
        │
        ▼
FondeoService::startSession()
  1. NUEVO: si fondeoSession.active -> error (evita fuga de figuras por reset implícito)
  2. Validaciones de negocio (modo, paAz, paDt, radios decrecientes)      (SIN CAMBIOS)
  3. resolvePuntoFondeo + resolvePuntoAuxiliar -> puntos ESTÁTICOS en DM  (SIN CAMBIOS)
  4. Persistir config/puntos, active = true                               (SIN CAMBIOS)
  5. NUEVO: createFigures()
       - 5 anillos: centro = s.puntoFondeo, radio = yardsToDm(r1..r5)
       - círculo PA: centro = s.puntoAuxiliar, radio = yardsToDm(kPaArrivalYds)
       - guarda IDs en anillosCircleIds / paCircleId
        │
        ▼
GeometryService::createCircle  ->  ctx.circles + ctx.cursors (36 CursorEntity c/u)
        │
        ▼
encoderLPD::buildFullMessage()  [timer de envío ya existente, sin cambios]
        │
        ▼
Radar LPD / frontend JSON (list_shapes, fondeo_info)

Durante la sesión [FondeoService::update() cada 80 ms — cálculo SIN CAMBIOS]:
  - Las figuras NO se tocan: PF/PA/radios son inmutables por diseño.
  - Único evento gráfico: distanciaPA <= kPaArrivalYds por primera vez
      -> paAlcanzado = true  ->  NUEVO: deleteCircle(paCircleId)

CLI: "fondeo --stop" / JSON: {"command":"fondeo_stop"}
  ... o automáticamente cuando paAlcanzado && distanciaPF <= 15.0
        │
        ▼
FondeoService::stopSession()
  1. NUEVO: deleteFigures()   (anillos + PA si sigue vivo)
  2. fondeoSession.reset()
        │
        ▼
Círculos y cursores desaparecen del próximo buildFullMessage()
```

## Puntos abiertos / a definir

1. **Mapeo real `type` (0-7) → color en el renderer LPD.** Idéntico al punto abierto #1 de `2W-Figuras.md`; se reservan `type=4` (anillos) y `type=5` (PA) para que Fondeo sea distinguible a nivel protocolo el día que exista la tabla. Coordinar con el equipo de renderer/frontend LPD junto con los `type=1/2/3` de 2W.
2. **Etiquetas sobre los anillos** ("AD. TODA" junto al anillo r1, etc.): sin mecanismo actual — mismo problema que el número de estación de 2W (REQ-2W-LPD-004, fuera de alcance allá también). Un frontend JSON puede resolverlo correlacionando `anillos_circle_ids` (orden r1..r5) con `fondeo_tipos`/`fondeo_info`.
3. **Segmento punteado BP → punto más cercano** (`[3] FONDEO - Ilacqua.docx`: "se comparará cuál de las dos distancias desde la unidad es menor... lo que determinará desde cuál iniciar el segmento punteado de asesoramiento buque-punto"). Decisión 2026-07-15: **punto abierto, fuera de esta iteración**. Es el único elemento gráfico dinámico de la spec — el BP se mueve cada tick de 80 ms, así que la línea habría que republicarla por tick, el mismo problema que 2W resolvió para círculos con `updateCircle` + skip-if-unchanged. Se diseña después del merge con `connection2w`, evaluando un `updateLine` análogo (hoy `GeometryService` solo tiene `createLine`/`deleteLine`). Mientras tanto, el frontend ya recibe azimut/distancia al punto activo por `fondeo_info` y puede dibujarlo por su cuenta.
4. **Radios de doctrina faltantes** (`fondeoTiposUnidad.h`, TODO existente): Meko 140 / Patagonia / Otro están en `0.0`. Meko 360 quedó ratificado en 1500/1000/800/100/50 (decisión #4). No bloquea este trabajo — la validación `r5 > 0` ya rechaza esas configuraciones antes de crear figura alguna — pero la UI de botonera no podrá dibujar anillos para esos tipos hasta que se carguen valores reales.
5. **Anillos que se apagan al cruzarlos**: descartado en esta iteración (decisión #3). Si doctrina lo pide, el enganche natural es `calculatePanelPredictivo`/`update()` + `deleteCircle` del anillo cruzado.
6. **Umbrales hardcodeados** `50.0` (PA) y `15.0` (PF) siguen marcados como "valor de prueba" en `fondeoservice.cpp`. Este plan extrae el 50 a `kPaArrivalYds` (lo comparte con el radio del círculo PA) pero no cambia su valor; confirmar ambos con doctrina.
7. **Intermitencia/colores del panel predictivo** (`[3] FONDEO.docx`, sección C: cuadro de texto intermitente al acercarse al círculo, "delay 1000ms, curva linear 300ms"): es presentación pura de la botonera; el backend ya publica todo lo necesario (`movimiento_actual`/`proximo_movimiento` + distancias) vía `fondeo_info`. Sin trabajo backend.

## Plan de trabajo

**Fase 1 — Estado y ciclo de vida en `FondeoService`**

1. En `fondeoSessionState.h`: agregar a `FondeoSessionState` el miembro `static constexpr int NO_CIRCLE = -1;`, el campo `QList<int> anillosCircleIds;` y el campo `int paCircleId = NO_CIRCLE;` (incluir `<QList>`). `reset()` no cambia.
2. En `fondeoservice.h`: declarar en la sección privada `void createFigures();` y `void deleteFigures();`.
3. En `fondeoservice.cpp`: agregar `#include "geometryservice.h"` y `#include "RadarMath.h"`; definir las constantes de archivo `kAnilloCircleType = 4`, `kPaCircleType = 5`, `kAnilloColor = "#00FFFF"`, `kPaColor = "#FF00FF"` y `kPaArrivalYds = 50.0`.
4. En `fondeoservice.cpp`: implementar `createFigures()` — cinco `createCircle(s.puntoFondeo, RadarMath::yardsToDm(r_i), kAnilloCircleType, kAnilloColor)` en orden r1..r5 apilando IDs en `anillosCircleIds`, más `createCircle(s.puntoAuxiliar, RadarMath::yardsToDm(kPaArrivalYds), kPaCircleType, kPaColor)` → `paCircleId`.
5. En `fondeoservice.cpp`: implementar `deleteFigures()` — `deleteCircle` de cada ID válido de `anillosCircleIds` + `clear()`, y del `paCircleId` si es válido, dejándolo en `NO_CIRCLE`.
6. En `FondeoService::startSession()`: agregar al comienzo el guard `if (m_ctx->fondeoSession.active) return { false, ... "ya hay una maniobra activa; use fondeo_stop..." };`.
7. En `FondeoService::startSession()`: después de `active = true` (línea final de seteo de sesión), llamar `createFigures()`.
8. En `FondeoService::stopSession()`: llamar `deleteFigures()` inmediatamente antes de `m_ctx->fondeoSession.reset()`.
9. En `FondeoService::update()`: reemplazar el literal `50.0` por `kPaArrivalYds`; dentro del bloque que setea `paAlcanzado = true`, borrar el círculo del PA (`GeometryService geometry(m_ctx); geometry.deleteCircle(s.paCircleId); s.paCircleId = FondeoSessionState::NO_CIRCLE;`, solo si el ID es válido).

**Fase 2 — Exposición JSON**

10. En `jsoncommandhandler.cpp`, `handleFondeoInfo`: agregar `pa_circle_id` (int) y `anillos_circle_ids` (QJsonArray construido desde `s.anillosCircleIds`).

**Fase 3 — Ajuste de CLI para verificación**

11. En `fondeoCommand.cpp`, bloque `--info`: agregar una línea con los IDs (`ANILLOS (r1..r5): id,id,id,id,id | PA: id`).

**Fase 4 — Documentación**

12. Este documento (`docs/modules/planFondeo.md`) — actualizar si la implementación se desvía.
13. `docs/flows/fondeo.md`: sumar la publicación gráfica al ciclo de vida (diagrama y secciones de start/stop/update) y los campos nuevos de `fondeo_info`.
14. `docs/modules/services.md`: ampliar la fila de `FondeoService` con la responsabilidad de ciclo de vida de figuras.

## Verificación

Backend headless: verificación por CLI (stdin) y canal JSON, igual que en 2W.

**1. Build**
```
cd /home/cristian/Documentos/Siag/DDM
qmake DDM.pro -o build/
make -C build -j$(nproc)
```

**2. Caso feliz por CLI**
```
fondeo --track=1 --az=90 --dt=2 --pa-az=180 --pa-dt=2000 --r1=1500 --r2=1000 --r3=800 --r4=100 --r5=50
fondeo --info      # debe mostrar 5 IDs de anillos y el ID del PA, todos != -1
fondeo --stop
fondeo --info      # maniobra no activa
```

**3. Guard nuevo**
```
fondeo --track=1 ... (inicio válido)
fondeo --track=1 ... (misma línea otra vez)   # debe fallar: "ya hay una maniobra activa"
fondeo --stop
```

**4. Verificación cruzada por JSON**
```
{"command":"list_shapes","args":{}}     # baseline
{"command":"fondeo_start","args":{"track_id":1,"track_az":90,"track_dt":2,"pa_az":180,"pa_dt":2000,"r1":1500,"r2":1000,"r3":800,"r4":100,"r5":50}}
{"command":"list_shapes","args":{}}     # baseline + 6 círculos (5 anillos concéntricos en el PF + 1 en el PA)
{"command":"fondeo_info","args":{}}     # active:true, anillos_circle_ids (5) y pa_circle_id consistentes con list_shapes
{"command":"fondeo_stop","args":{}}
{"command":"list_shapes","args":{}}     # vuelve al baseline exacto
```

**5. Transición de fase (borrado del PA)**: iniciar con el BP navegando hacia el PA (o mover el Track 0 con el simulador hasta `distanciaPA <= 50`); al imprimirse "Se ha alcanzado el Punto Auxiliar", `fondeo --info`/`list_shapes` deben mostrar `pa_circle_id: -1` y 5 círculos (el del PA desapareció; los anillos siguen).

**6. Detención automática por éxito**: continuar hasta `distanciaPF <= 15`; al imprimirse "Se ha alcanzado el Punto de Fondeo", `list_shapes` debe volver al baseline sin ningún `fondeo_stop` manual.

**7. Chequeo de fugas de cursores**: la cantidad de `cursor_ids` en `list_shapes` debe crecer exactamente en `36 × 6 = 216` tras el start, bajar a `36 × 5 = 180` tras alcanzar el PA, y volver exactamente al valor original tras el stop (manual o automático).

**8. Geometría de los anillos**: en `list_shapes`, los 5 anillos deben compartir centro (= PF en DM) y tener radios `r_i / 2000` DM (p. ej. `r1=1500` → `0.75` DM); el círculo del PA debe estar desplazado del PF según `pa_az`/`pa_dt`.

## Módulos relacionados

- `[3] FONDEO - Ilacqua.docx` (Google Drive) — requerimiento original del módulo (círculos de distancia, graficado de puntos, segmento punteado).
- `[3] FONDEO.docx` (Google Drive) — spec de UI/UX de la botonera (secciones A/B/C, estilo de círculos, intermitencia del predictivo).
- `Informe de Cobertura de Test [Fondeo]` (Google Drive) — 55 tests existentes sobre `FondeoCommand`/`FondeoCalculator`/`FondeoService`; el guard "ya activa" (paso 6) agrega un caso nuevo a cubrir.
- `docs/modules/2W-Figuras.md` (rama `connection2w`) — precedente directo: misma conexión cálculo→figuras, con la variante dinámica (`updateCircle`/`syncFigures`) que Fondeo no necesita.
- `docs/flows/fondeo.md` — flujo completo del módulo (cálculo, CLI, JSON, ciclo de 80 ms).
- `docs/modules/services.md`
- `docs/modules/entities.md`
- `docs/modules/command-context.md`
- `docs/protocols/json-command-api.md`
