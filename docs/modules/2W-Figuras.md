# Módulo: 2W — Conexión con Creación de Figuras

## Descripción general

El módulo 2W (disposición de formación táctica) ya tiene su motor de cálculo implementado y funcionando: `TwoWCalculator` recalcula, cada 80 ms desde el timer de `main.cpp`, los centros de los círculos de la formación (Guía, Buque Propio y estaciones aliadas) y el asesoramiento cinemático (rumbo/ETA), y todo eso queda guardado en `TwoWSessionState` (`CommandContext::twoWSession`).

El problema que resuelve este documento: ese cálculo hoy es un callejón sin salida. Nada en el código toma `guideCircleCenter` / `ownCircleCenter` / `allyCircleCenters` y los convierte en una figura real sobre el radar. La propia especificación técnica ya redactada (`docs/modules/Especificación Técnica y de Diseño de Software_ Disposición de Formación 2W.md`, sección 5.2) lo deja explícito: los resultados quedan "disponibles para el refresco de la interfaz gráfica y del radar", pero nunca se documenta ni se implementa ese refresco. Tampoco existe hoy ningún comando JSON para 2W — sólo hay control por CLI (`TwoWCommand`) — mientras que la spec de UI (`2W_DDM.md`) describe botones INICIAR/FINALIZAR y una grilla interactiva de selección de estaciones, lo cual implica un frontend externo hablando por JSON, igual que ya ocurre con círculos/áreas (`GeometryService`) y estacionamiento (`EstacionamientoService`).

Este documento define cómo conectar `TwoWService` con la infraestructura de figuras ya existente (`GeometryService` / `CircleEntity` / `ctx.circles` / `ctx.cursors`) y cómo exponer el control de la disposición vía JSON, sin duplicar ni reinventar mecanismos ya presentes en el backend.

Fuera de alcance de esta iteración (decisión tomada deliberadamente, no un olvido): superponer el número de estación en el centro de cada círculo (REQ-2W-LPD-004). Se anota como punto abierto en la sección correspondiente.

## Principio de diseño: el Guía es el único origen de posición

Antes de entrar en el diseño de las figuras, es importante dejar explícito algo que **ya está implementado y no cambia**, porque es la base de todo lo demás: ningún centro de círculo se calcula de forma independiente o "genérica". Todos están anclados a la posición del track Guía.

En cada ciclo de 80 ms, `TwoWService::update()` busca el track Guía por id (`m_ctx->findTrackById(s.guideTrackId)`) y lee su posición **actual** (`guideTrack->getX()/getY()`). `TwoWCalculator::calculate()` usa esa posición como origen exclusivo:

- `guideCircleCenter = guidePos` (directamente, sin offset).
- `ownCircleCenter` y cada entrada de `allyCircleCenters` = `guidePos + proyección(azimutVerdaderoFijo, distanciaFija × radio)`, donde azimut y distancia salen de la Tabla A (`TwoWStationTable`, 68 entradas estáticas) y son **fijos** — no rotan si el Guía cambia de rumbo (REQ-2W-CAL-003, `IndependenciaRumbo`).

Consecuencia práctica: si el Guía se desplaza, los tres tipos de círculo se trasladan junto con él en cada tick; si el Guía gira sobre su eje sin desplazarse, los círculos de propio/aliadas **no** rotan alrededor de él, porque su azimut de Tabla A es verdadero y fijo, no relativo al rumbo del Guía.

`TwoWService::syncFigures()` — lo nuevo que introduce este documento — **no recalcula ninguna posición por sí mismo**. Sólo toma los tres centros que `TwoWCalculator` ya dejó escritos en `twoWSession` (que a su vez ya dependen del track Guía) y los refleja en `CircleEntity` reales. Es un paso de "publicación geométrica" puro, posterior y separado del cálculo de posición.

## Diseño propuesto

### 1. Estado nuevo en `TwoWSessionState`

Se agregan los IDs de los `CircleEntity` creados, para poder reposicionarlos/borrarlos de forma estable:

```cpp
// src/model/2w/twoWSessionState.h
int        guideCircleId = -1;   // Círculo verde  (Guía)
int        ownCircleId   = -1;   // Círculo azul   (BP)
QList<int> allyCircleIds;        // Círculos ámbar, paralelo a selectedStations

static constexpr int NO_CIRCLE = -1;
```

`reset()` (ya existente, `*this = TwoWSessionState{}`) limpia estos campos automáticamente — pero las figuras deben borrarse **antes** de llamar a `reset()`, nunca después (ver `stopSession()` más abajo).

### 2. Reposicionar in-place, no recrear cada tick

Hoy `GeometryService` sólo tiene `createCircle`/`deleteCircle`. Se agrega `updateCircle`:

```cpp
// geometryservice.h
GeometryResult updateCircle(int circleId, const QPointF& center, double radius);
```

```cpp
// geometryservice.cpp
GeometryResult GeometryService::updateCircle(int circleId, const QPointF& center, double radius)
{
    if (radius <= 0.0) {
        return {false, "INVALID_RADIUS", "El radio debe ser mayor a 0", -1};
    }
    for (CircleEntity& circle : m_context->getCircles()) {   // getCircles() no-const ya existe
        if (circle.getId() == circleId) {
            for (int cid : circle.getCursorIds()) {
                m_context->eraseCursorById(cid);              // limpia los 36 segmentos viejos
            }
            circle.setCenter(center);
            circle.setRadius(radius);
            circle.calculateAndStoreCursors(*m_context);       // regenera con el mismo id de círculo
            return {true, QString(), QString(), circleId};
        }
    }
    return {false, "NOT_FOUND", QString("No se encontro un circulo con ID %1").arg(circleId), circleId};
}
```

No hace falta tocar `CommandContext` (`getCircles()` no-const y `eraseCursorById()` ya son públicos).

**Por qué reposicionar y no "borrar+crear" cada tick**: con hasta 68 estaciones aliadas simultáneas, borrar y crear generaría hasta ~70 altas/bajas de círculo por ciclo de 80 ms, incrementando `ctx->commandCounter` sin necesidad y rompiendo cualquier referencia estable que un frontend externo guarde vía `list_shapes` (el círculo "del Guía" cambiaría de ID todo el tiempo). Reposicionar in-place da IDs estables y hace el mismo trabajo de fondo (siempre hay que regenerar los 36 `CursorEntity`, porque `CircleEntity` no soporta moverse sin recrear cursores), pero sin el costo de gestión de IDs/deque adicional.

### 3. Conversión de unidades del radio

`circleRadiusNm` (millas náuticas) se convierte a Data Miles con el mismo factor que ya usa `TwoWCalculator::calculate` para la Tabla A:

```cpp
radiusDm = circleRadiusNm * TwoWCalculator::kNmToDm;   // kNmToDm = 1.012685 (1 NM = 6076.1154 ft, 1 DM = 6000 ft)
```

Se promueve `kNmToDm` de constante local de función a miembro estático público de `TwoWCalculator`, eliminando la duplicación del literal:

```cpp
// twoWCalculator.h
class TwoWCalculator {
public:
    static constexpr double kNmToDm = 1.012685;
    static void calculate(...);
};
```

Los centros (`guideCircleCenter`, `ownCircleCenter`, `allyCircleCenters`) **no** necesitan conversión: ya están en DM porque provienen de `track->getX()/getY()`.

### 4. Tipos y colores (provisionales)

```cpp
// TwoWService.cpp (constantes de archivo)
static constexpr int kGuideCircleType = 1;
static constexpr int kOwnCircleType   = 2;
static constexpr int kAllyCircleType  = 3;

static const QString kGuideColor = QStringLiteral("#00FF00"); // REQ-2W-LPD-005
static const QString kOwnColor   = QStringLiteral("#0000FF"); // REQ-2W-LPD-006
static const QString kAllyColor  = QStringLiteral("#FFCC00"); // REQ-2W-LPD-007
```

Estos valores de `type` (1/2/3) son **provisionales**: `encoderLPD::appendCursorLong` sólo transmite 3 bits de `lineType` (0-7) al protocolo LPD, y no existe hoy ninguna tabla que traduzca ese entero a un color real en el hardware/renderer (mismo problema ya señalado con el comentario `// cambiar aca` en `appendStationingMarkerMessage`). El campo `color` (string) de `CircleEntity` **no viaja por el protocolo binario en absoluto**; sólo es visible para un consumidor JSON que lea `list_shapes`. Se eligen 1/2/3 para que los círculos de 2W sean distinguibles a nivel de protocolo el día que se defina el mapeo tipo→color — ver "Puntos abiertos".

### 5. `TwoWService::syncFigures()`

Método privado nuevo, invocado al final de `update()` (y una vez, sincrónicamente, al final de `startSession()` para que las figuras aparezcan en el mismo instante de INICIAR, no en el siguiente tick):

```cpp
void TwoWService::syncFigures()
{
    TwoWSessionState& s = m_ctx->twoWSession;
    GeometryService geometry(m_ctx);
    const double radiusDm = s.circleRadiusNm * TwoWCalculator::kNmToDm;

    // Guía
    if (s.guideCircleId == TwoWSessionState::NO_CIRCLE) {
        const GeometryResult r = geometry.createCircle(s.guideCircleCenter, radiusDm, kGuideCircleType, kGuideColor);
        s.guideCircleId = r.success ? r.id : TwoWSessionState::NO_CIRCLE;
    } else {
        geometry.updateCircle(s.guideCircleId, s.guideCircleCenter, radiusDm);
    }

    // Propio (misma lógica con s.ownCircleId / kOwnCircleType / kOwnColor)

    // Aliadas: diff selectedStations vs. allyCircleIds
    while (s.allyCircleIds.size() > s.selectedStations.size()) {
        geometry.deleteCircle(s.allyCircleIds.takeLast());     // sobran (se destildaron)
    }
    for (int i = 0; i < s.selectedStations.size(); ++i) {
        if (i < s.allyCircleIds.size()) {
            geometry.updateCircle(s.allyCircleIds[i], s.allyCircleCenters[i], radiusDm);
        } else {
            const GeometryResult r = geometry.createCircle(s.allyCircleCenters[i], radiusDm, kAllyCircleType, kAllyColor);
            s.allyCircleIds.append(r.success ? r.id : TwoWSessionState::NO_CIRCLE);
        }
    }
}
```

`syncFigures()` sólo lee `s.guideCircleCenter`/`s.ownCircleCenter`/`s.allyCircleCenters` — nunca recalcula una posición; esos valores ya fueron escritos por `TwoWCalculator::calculate()` en el mismo ciclo de `update()`, antes de invocar `syncFigures()`.

**Adenda (post-implementación):** `TwoWSessionState` incorporó `lastSyncedGuideCircleCenter`/`lastSyncedOwnCircleCenter`/`lastSyncedAllyCircleCenters`, y cada rama de `syncFigures()` compara (con epsilon `1e-6` DM) el centro recién calculado contra el último efectivamente publicado antes de llamar a `updateCircle()`. Motivo: una sesión real conectada a un frontend, corriendo el tick de 80 ms indefinidamente, generaba ~144 altas+bajas de `CursorEntity` por ciclo (36 por círculo × 4 círculos) aunque el Guía estuviera quieto — sumado a un `qDebug()` por cursor que quedó de scaffolding de debug (ya eliminado de `CommandContext::addCursorFront`/`emplaceCursorFront`), esto saturaba stdout y terminaba crasheando el proceso bajo Qt Creator. El guard evita el `updateCircle()` (y por lo tanto el churn de cursores) cuando el centro no cambió desde el último tick publicado.

### 6. Endurecer `startSession()` y `stopSession()`

`startSession()` gana dos guardas que hoy faltan (correctitud, no sólo figuras):

1. **Rechazar si ya hay una sesión activa**: hoy una segunda invocación pisaría `guideCircleId`/`ownCircleId`/`allyCircleIds` sin borrar las figuras previas → fuga de círculos y cursores huérfanos en el radar.
2. **Validar que `guideTrackId` exista** antes de activar la sesión: hoy, si se inicia con un track inexistente, la sesión queda `active=true` un instante y el próximo `update()` la autofinaliza silenciosamente, pero el mensaje de respuesta ya dijo "Disposicion iniciada", lo cual es engañoso.

Al final de la validación, `startSession()` llama a `update()` una vez (reutilizando cálculo + `syncFigures()`), en vez de duplicar lógica de posicionamiento inicial.

`stopSession()` borra las figuras **antes** de `reset()`:

```cpp
TwoWOperationResult TwoWService::stopSession()
{
    if (!m_ctx->twoWSession.active) { /* ... sin cambios ... */ }

    TwoWSessionState& s = m_ctx->twoWSession;
    GeometryService geometry(m_ctx);
    if (s.guideCircleId != TwoWSessionState::NO_CIRCLE) geometry.deleteCircle(s.guideCircleId);
    if (s.ownCircleId   != TwoWSessionState::NO_CIRCLE) geometry.deleteCircle(s.ownCircleId);
    for (int id : s.allyCircleIds) geometry.deleteCircle(id);

    s.reset();
    return { true, QStringLiteral("\n[2W] Disposicion finalizada.\n") };
}
```

Esto también cubre "el track del Guía desaparece en pleno vuelo": `update()` ya llama a `stopSession()` cuando `!guideTrack`, así que la limpieza de figuras queda cubierta gratis por ese mismo camino (REQ-2W-INT-008 se cumple también ante pérdida de track, no sólo ante FINALIZAR manual).

### 7. Comandos JSON nuevos

Siguiendo el patrón de `estacionamiento_calc`/`estacionamiento_stop` (handlers inline en `JsonCommandHandler`, sin una clase `*CommandHandler` separada — ese patrón de clase aparte se usa para geometría/tracks/cursores porque tienen mucha lógica de parsing propia; 2W ya tiene toda su lógica en `TwoWService`):

- **`2w_start`** — args: `guide_track` (int, requerido), `bp_station` (int 1-68, requerido), `radius_nm` (double, opcional, default 1.0), `aliadas` (array de int, opcional, default `[]`, ver nota más abajo). Delega a `TwoWService::startSession`. Instancia **sólo** Guía+Propio de inmediato (obligatorios, REQ-2W-LPD-001); si se pasa `aliadas` no vacío, delega internamente a `setStations` después de publicar Guía+Propio, por compatibilidad con el flujo de una sola llamada.
- **`2w_stop`** — sin args. Delega a `TwoWService::stopSession`. Borra Guía+Propio+aliadas juntos (REQ-2W-INT-008: FINALIZAR borra todo el asesoramiento en pantalla).
- **`2w_info`** — sin args, de sólo lectura. Devuelve siempre `success:true` con un campo `active` (booleano); si `active:false`, no es un error (un frontend puede necesitar hacer polling periódico). Devuelve el mismo contenido que hoy imprime `2w --info` por CLI, más los IDs de círculos: `guide_track_id`, `bp_station`, `radius_nm`, `aliadas[]`, `guide_circle_id`, `own_circle_id`, `ally_circle_ids[]`, `current_azimuth_deg`, `current_distance_nm`, `expected_azimuth_deg`, `expected_distance_nm`, `course_to_station_deg`, `eta_valid`, `time_to_station_min`, `track_valid`.
- **`2w_set_stations`** — args: `aliadas` (array de int, requerido, puede ser `[]`). Delega a `TwoWService::setStations`, que reemplaza por completo la lista de estaciones aliadas graficadas (círculos ámbar, opcionales, REQ-2W-LPD-002) **sin tocar** Guía/Propio. Requiere sesión activa (`2w_start` previo); si no hay sesión activa, responde error. Reutiliza el mismo diff `selectedStations` vs. `allyCircleIds` que ya existía en `syncFigures()` — no requirió cambios ahí. Responde `aliadas[]`/`ally_circle_ids[]` (mismos arrays paralelos que `2w_info`). Resuelve el punto abierto #4 de más abajo: ahora se puede cambiar qué aliadas se ven sin reiniciar toda la disposición, tal como describe el caso de uso de `docs/reference/2W REQ.md` (selección en la grilla de estaciones, independiente de Guía/Propio).

A diferencia de `estacionamiento_calc`/`estacionamiento_stop`, **no lleva `index`/slot**: `ctx->twoWSession` es un único struct global (una sola disposición 2W activa a la vez en todo el sistema), no una tabla de slots como estacionamiento (que soporta 10 sesiones concurrentes). Esto es coherente con que la UI describe un único par de botones "INICIAR"/"FINALIZAR" para toda la disposición (Guía/Propio/aliadas comparten el mismo ciclo de vida de sesión, aunque `2w_set_stations` permita cambiar las aliadas sin reiniciarlo).

Registro en `initializeCommandMap()`:

```cpp
m_commandMap[QStringLiteral("2w_start")]        = [this](const QJsonObject& args) { return handleTwoWStart(args); };
m_commandMap[QStringLiteral("2w_stop")]         = [this](const QJsonObject& args) { return handleTwoWStop(args); };
m_commandMap[QStringLiteral("2w_info")]         = [this](const QJsonObject& args) { return handleTwoWInfo(args); };
m_commandMap[QStringLiteral("2w_set_stations")] = [this](const QJsonObject& args) { return handleTwoWSetStations(args); };
```

Se agrega `std::unique_ptr<TwoWService> m_twoWService;` a `jsoncommandhandler.h`, construido en el constructor igual que `m_estacionamientoService`, y las cuatro declaraciones `QByteArray handleTwoWStart/Stop/Info/SetStations(const QJsonObject&);` en la sección privada.

Por CLI, `TwoWCommand` expone lo mismo vía `2w --aliadas=<est1,est2,...>` **sin** `--guia`/`--est` (delega a `setStations`); con `--guia`/`--est` presentes, `--aliadas` sigue funcionando como atajo de una sola línea (comportamiento sin cambios). `2w --aliadas=` (vacío) limpia todas las aliadas sin afectar Guía/Propio.

## Archivos y responsabilidades

| Archivo | Cambio | Responsabilidad |
|---|---|---|
| `src/model/2w/twoWSessionState.h` | Modificar | Agregar `guideCircleId`, `ownCircleId`, `allyCircleIds`, `NO_CIRCLE`. |
| `src/model/2w/twoWCalculator.h` | Modificar | Exponer `static constexpr double kNmToDm` público. |
| `src/model/2w/twoWCalculator.cpp` | Modificar | Usar `kNmToDm` como miembro en vez de constante local de función. |
| `src/controller/services/geometryservice.h` | Modificar | Declarar `updateCircle(int, const QPointF&, double)`. |
| `src/controller/services/geometryservice.cpp` | Modificar | Implementar `updateCircle` (reposiciona sin cambiar ID). |
| `src/controller/services/TwoWService.h` | Modificar | Declarar `syncFigures()` privado; sin cambios de firma pública. |
| `src/controller/services/TwoWService.cpp` | Modificar | Guard "ya activa", validación de track Guía, llamada a `update()` al final de `startSession`, borrado de figuras en `stopSession`, implementación de `syncFigures`. |
| `src/controller/commands/TwoWCommand.cpp` | Modificar (menor) | Extender la salida de `2w --info` para imprimir `guideCircleId`/`ownCircleId`/`allyCircleIds`; agregar rama standalone `2w --aliadas=...` (sin `--guia`/`--est`) que delega a `setStations`. |
| `src/controller/json/jsoncommandhandler.h` | Modificar | Agregar `m_twoWService` y declaraciones `handleTwoWStart/Stop/Info/SetStations`. |
| `src/controller/json/jsoncommandhandler.cpp` | Modificar | Construir `m_twoWService`, registrar `2w_start`/`2w_stop`/`2w_info`/`2w_set_stations`, implementar los cuatro handlers. |
| `docs/modules/services.md` | Modificar (menor) | Sumar a la tabla de servicios la responsabilidad ampliada de `TwoWService` (ciclo de vida de figuras) y `GeometryService::updateCircle`. |

No requieren cambios: `src/model/entities/circleEntity.{h,cpp}` (los setters ya existen), `src/model/commandContext.h` (`getCircles()` no-const y `eraseCursorById` ya son públicos), `src/model/decoders/lpdEncoder.cpp` (el círculo se transmite igual que cualquier otro, vía el broadcast completo de `ctx.cursors` en cada ciclo de `encoder->buildFullMessage`).

## Ciclo de vida / flujo de datos

```
CLI: "2w --guia=5 --est=12 --radio=1 --aliadas=3,7"
JSON: {"command":"2w_start","args":{...}}
        │
        ▼
TwoWService::startSession()
  1. Validar bpStation, circleRadiusNm, aliadas (igual que hoy)
  2. NUEVO: validar que no haya sesión activa (si la hay -> error)
  3. NUEVO: validar que guideTrackId exista como track actual (si no -> error)
  4. Setear campos de twoWSession (igual que hoy)
  5. NUEVO: llamar a update() sincrónicamente
        │
        ▼
TwoWService::update()  [también disparado cada 80ms por updatePositionTimer en main.cpp]
  1. Buscar track del Guía; si no existe -> stopSession() (borra figuras) y return
  2. (a) CÁLCULO: TwoWCalculator::calculate(...) lee la posición ACTUAL del Guía y
        proyecta propio/aliadas por azimut/distancia FIJOS de Tabla A
        -> refresca guideCircleCenter/ownCircleCenter/allyCircleCenters (SIN CAMBIOS)
  3. (b) PUBLICACIÓN GRÁFICA — NUEVO: syncFigures()
        - sólo consume los centros ya calculados en el paso (a), nunca decide posición
        - Guía/Propio: createCircle (1ª vez) o updateCircle (siguientes)
        - Aliadas: diff selectedStations vs allyCircleIds -> create/update/delete
        │
        ▼
GeometryService::createCircle / updateCircle / deleteCircle
        │
        ▼
ctx.circles (CircleEntity) + ctx.cursors (36 CursorEntity por círculo)
        │
        ▼
encoderLPD::buildFullMessage()  [timer de envío en main.cpp, ya existente, sin cambios]
        │
        ▼
Radar LPD / frontend JSON (list_shapes)


CLI: "2w --stop"  /  JSON: {"command":"2w_stop"}
        │
        ▼
TwoWService::stopSession()
  1. deleteCircle(guideCircleId), deleteCircle(ownCircleId), deleteCircle(cada allyCircleId)
  2. twoWSession.reset()
        │
        ▼
Círculos y sus cursores desaparecen del próximo buildFullMessage()
```

## Puntos abiertos / a definir

1. **Mapeo real `type` (0-7) → color en el renderer LPD.** El protocolo binario sólo transmite 3 bits de `lineType`; el string `color` de `CircleEntity` nunca llega al hardware/simulador. Se proponen valores de trabajo (`type=1/2/3` para guía/propio/aliadas) para no bloquear la implementación, pero la tabla definitiva debe coordinarse con el equipo de renderer/frontend LPD — mismo problema ya pendiente que el comentario `// cambiar aca` en `appendStationingMarkerMessage`. Mientras tanto, el color (`#00FF00`/`#0000FF`/`#FFCC00`) sólo es utilizable por un consumidor JSON de `list_shapes`.
2. **REQ-2W-LPD-004 (número de estación en el centro del círculo): explícitamente fuera de alcance de esta iteración.** El mecanismo liviano ya existente para "número en el centro" (`CpaMarkerState`/`StationingSession` + `appendCpaMarkerMessage`/`appendStationingMarkerMessage`) usa un símbolo de tamaño fijo que no escala con el radio en MN, y se decidió no adoptarlo para 2W. Queda pendiente para una iteración futura (posiblemente resuelto del lado del frontend, correlacionando `list_shapes` con la lista de estaciones seleccionadas por índice/orden, ya que hoy `listShapes()` no lleva metadata de "a qué estación pertenece este círculo" más allá del orden de `allyCircleIds` == orden de `selectedStations`).
3. **REQ-2W-LPD-008 (relleno con opacidad 80%)**: `CircleEntity`/`CursorEntity` no tienen hoy ningún concepto de relleno (fill) — son sólo contornos poligonales de 36 segmentos. Este requerimiento depende de cómo el renderer interprete `type`/color (punto 1) y queda fuera del alcance de este documento.
4. ~~**Comando dedicado para editar `selectedStations` en caliente**~~ **Resuelto**: `2w_set_stations` (JSON) / `2w --aliadas=...` standalone (CLI) — ver sección 7. `TwoWService::setStations()` reemplaza la lista completa de aliadas y reutiliza el diff ya existente en `syncFigures()`, sin tocar Guía/Propio ni requerir reiniciar la sesión.
5. **Costo de `eraseCursorById` sobre sesiones con muchas aliadas**: no se considera un riesgo real para el tamaño de formación esperado (unas pocas unidades), pero si se habilitan las 68 estaciones simultáneas de forma rutinaria, conviene revisar el costo O(n) de ese barrido sobre `ctx.cursors`. **Actualización:** se agregó un guard "skip-if-unchanged" en `syncFigures()` (ver sección 5 más arriba) que evita el `updateCircle()`/churn de cursores cuando el centro de un círculo no cambió desde el último tick — esto mitiga el caso común de Guía estacionario, que era además la causa directa de un crash observado (sumado al `qDebug()` por cursor, ya eliminado). Pero **no resuelve** el peor caso que este punto ya señalaba: muchas aliadas con el Guía en movimiento continuo, donde el centro cambia en cada tick igual y el guard no tiene efecto. Sigue abierto para ese escenario.

## Plan de trabajo

**Fase 0 — Refactor mínimo compartido (sin efecto observable, habilita todo lo demás)**
1. En `twoWCalculator.h`: agregar `static constexpr double kNmToDm = 1.012685;` como miembro público de `TwoWCalculator`.
2. En `twoWCalculator.cpp`: eliminar la constante local `kNmToDm` de `calculate()` y reemplazar sus 2 usos por `TwoWCalculator::kNmToDm`.
3. En `geometryservice.h`: declarar `GeometryResult updateCircle(int circleId, const QPointF& center, double radius);`.
4. En `geometryservice.cpp`: implementar `updateCircle` (buscar por id, validar radio, borrar cursores viejos, `setCenter`/`setRadius`, `calculateAndStoreCursors`).

**Fase 1 — Estado y ciclo de vida en `TwoWService`**
5. En `twoWSessionState.h`: agregar `guideCircleId`, `ownCircleId`, `allyCircleIds`, `NO_CIRCLE`.
6. En `TwoWService.h`: declarar `void syncFigures();` privado.
7. En `TwoWService.cpp`: implementar `syncFigures()` (guía/propio create-o-update, aliadas por diff contra `selectedStations`).
8. En `TwoWService::update()`: llamar a `syncFigures()` justo después de `TwoWCalculator::calculate(...)`.
9. En `TwoWService::startSession()`: guard "ya activa", validación de existencia del track Guía, llamada final a `update()`.
10. En `TwoWService::stopSession()`: borrar las figuras antes de `s.reset()`.

**Fase 2 — Exposición JSON**
11. En `jsoncommandhandler.h`: agregar `m_twoWService` y declaraciones de los tres handlers.
12. En `jsoncommandhandler.cpp`: construir `m_twoWService` en el constructor.
13. Implementar `handleTwoWStart` (parseo/validación de `guide_track`, `bp_station`, `radius_nm`, `aliadas`).
14. Implementar `handleTwoWStop`.
15. Implementar `handleTwoWInfo` (no falla si no hay sesión activa; incluye IDs de círculos).
16. Registrar `2w_start`, `2w_stop`, `2w_info` en `initializeCommandMap()`.

**Fase 3 — Ajuste de CLI para verificación**
17. Extender el bloque `2w --info` de `TwoWCommand.cpp` para imprimir `guideCircleId`/`ownCircleId`/`allyCircleIds`.

**Fase 4 — Documentación**
18. Este documento (`docs/modules/2W-Figuras.md`).
19. Actualizar `docs/modules/services.md` con la responsabilidad ampliada de `TwoWService`/`GeometryService`.

**Fase 5 — Futuro, no bloqueante**
20. Comando `2w_set_stations` para edición en caliente de la selección de aliadas.
21. Definir con el equipo de renderer/frontend LPD la tabla real `type` (0-7) → color.
22. Resolver REQ-2W-LPD-004 (número en el centro) y REQ-2W-LPD-008 (relleno 80%).

## Verificación

Dado que el repo es un backend headless (sin UI real; `src/view` sólo parsea líneas de stdin), la verificación end-to-end se hace por CLI (siempre disponible) y, opcionalmente, por el canal JSON.

**1. Build**
```
cd /home/cristian/Documentos/Siag/DDM
qmake DDM.pro -o build/
make -C build -j$(nproc)
```

**2. Caso feliz por CLI**
```
2w --guia=1 --est=5 --radio=1 --aliadas=3,7
2w --info
2w --stop
2w --info
```
Esperado: tras `--guia=...`, "Disposicion iniciada"; `2w --info` muestra `guideCircleId`/`ownCircleId` distintos de `-1` y `allyCircleIds` con 2 elementos; `2w --stop` responde "Disposicion finalizada"; el `2w --info` posterior indica sesión inactiva.

**3. Guardas nuevas**
```
2w --guia=999 --est=5          # track inexistente -> debe fallar, sin crear figuras
2w --guia=1 --est=5
2w --guia=1 --est=6             # segunda vez sin --stop -> debe fallar
2w --stop
```

**4. Pérdida de track en pleno vuelo**
```
2w --guia=1 --est=5
delete 1                        # elimina el track del Guía
```
Esperado: en el próximo tick de 80 ms, `update()` detecta `!guideTrack`, llama a `stopSession()` y borra las figuras.

**5. Verificación por canal JSON (cruzada)**
```
{"command":"list_shapes","args":{}}                                   # baseline
{"command":"2w_start","args":{"guide_track":1,"bp_station":5,"radius_nm":1.0,"aliadas":[3,7]}}
{"command":"list_shapes","args":{}}                                   # baseline + 4 círculos
{"command":"2w_info","args":{}}                                       # active:true, ids consistentes
{"command":"2w_stop","args":{}}
{"command":"list_shapes","args":{}}                                   # vuelve al baseline exacto
```

**6. Chequeo de fugas de cursores**: comparar la cantidad de `cursor_ids` en `list_shapes` antes de `2w --guia=...`, inmediatamente después (debe crecer en `36 * (2 + N_aliadas)`), y después de `2w --stop` (debe volver exactamente al valor original).

**7. Reajuste dinámico de radio**: iniciar con `--radio=1`, `--stop`, reiniciar con `--radio=2`; verificar que el nuevo círculo se cree con el doble de radio en DM.

## Módulos relacionados

- `docs/modules/services.md`
- `docs/modules/entities.md`
- `docs/modules/command-context.md`
- `docs/modules/decoders-encoders.md`
- `docs/modules/2W_DDM.md`
- `docs/modules/Especificación Técnica y de Diseño de Software_ Disposición de Formación 2W.md`
