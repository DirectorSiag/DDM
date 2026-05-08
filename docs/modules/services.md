# Módulo: Services

## Descripción general

Este módulo concentra la lógica de negocio del backend, separada de transporte/protocolo. Los servicios operan sobre `CommandContext` y son reutilizados desde handlers JSON y comandos CLI.

Incluye servicios de tracks, cursores, geometría, ownship, CPA/PPP, estacionamiento y utilitarios de consulta/centro/OBM/SITREP.

## Lista de archivos y clases

| Archivo | Clase/Struct | Responsabilidad |
|---|---|---|
| `src/controller/services/trackservice.h` | `TrackService`, `TrackCreateRequest`, `TrackOperationResult` | CRUD de tracks, validaciones de dominio y serialización JSON. |
| `src/controller/services/cursorservice.h` | `CursorService`, `CursorCreateRequest`, `CursorOperationResult` | CRUD de cursores, conversión `LINE_n` y serialización JSON. |
| `src/controller/services/geometryservice.h` | `GeometryService`, `GeometryResult` | CRUD de áreas/círculos/polígonos y listado agregado. |
| `src/controller/services/ownshipservice.h` | `OwnShipService`, `OwnShipOperationResult` | Gestión de ownship y sincronización de track virtual id 0. |
| `src/controller/services/cpaservice.h` | `CPAService`, `CPATrackRef`, `CPAComputationResult`, `CPASession`, `CPAClearResult` | Cálculo y ciclo de vida de sesiones CPA/PPP. |
| `src/controller/services/trackpppservice.h` | `TrackPppService` | Cálculo PPP de SITREP track-vs-ownship. |
| `src/controller/services/estacionamientoservice.h` | `EstacionamientoService`, `CalculationResult`, `OperationResult` | Parseo/validación y orquestación de cálculo de estacionamiento. |
| `src/controller/services/sitrepservice.h` | `SitrepService` | Operaciones de snapshot, borrado y metadatos SITREP. |
| `src/controller/services/queryservice.h` | `QueryService` | Consultas read-only sobre estado táctico. |
| `src/controller/services/centerservice.h` | `CenterService` | Gestión del centro global de visualización. |
| `src/controller/services/obmservice.h` | `ObmService` | Acceso a posición OBM mediante `IOBMHandler`. |
| `src/model/cpa.h` | `CPA`, `CPAResult` | Clase de cálculo CPA (wrapper sobre motor PPP). |
| `src/model/pppcalculator.h` | `PppCalculator`, `KinematicState`, `Result` | Motor matemático puro para CPA/PPP. |
| `src/model/estacionamientocalculator.h` | `EstacionamientoCalculator`, `KinematicState`, `Input`, `Result` | Motor matemático puro para estacionamiento. |

## Clases principales

### TrackService

- **Rol**: Servicio principal de tracks; valida entradas, crea/elimina tracks, serializa estado y gatilla cálculo PPP cuando corresponde.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor | `explicit TrackService(CommandContext* context)` | Inyecta contexto compartido. |
| Crear | `TrackOperationResult createTrack(const TrackCreateRequest& request)` | Valida y crea track con id autoincremental. |
| Eliminar | `TrackOperationResult deleteTrackById(int trackId)` | Baja por id con resultado estructurado. |
| Buscar | `Track* findTrackById(int trackId) const` | Busca track en contexto. |
| Serializar | `QJsonArray serializeTracks() const` | Retorna tracks con campos operativos y PPP SITREP. |

- **Structs/Tipos definidos**:
- `TrackCreateRequest`
- `TrackOperationResult`
- **Dependencias**:
- `CommandContext`
- `TrackPppService`

### CursorService

- **Rol**: Encapsula validación y CRUD de cursores, además de helper de ids de línea (`LINE_#`).
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Crear cursor | `CursorOperationResult createCursor(const CursorCreateRequest& request)` | Valida tipo/largo y crea cursor en contexto. |
| Eliminar cursor | `CursorOperationResult deleteCursorById(int cursorId)` | Baja de cursor por id. |
| Serializar | `QJsonArray serializeCursors() const` | Lista JSON de cursores activos. |
| Helper id línea | `static QString lineIdFromCursorId(int cursorId)` | Convierte id numérico a `LINE_n`. |
| Helper id cursor | `static int cursorIdFromLineId(const QString& lineId, bool* ok = nullptr)` | Extrae id numérico desde `LINE_n`. |

- **Structs/Tipos definidos**:
- `CursorCreateRequest`
- `CursorOperationResult`
- **Dependencias**:
- `CommandContext`
- `RadarMath`

### GeometryService

- **Rol**: Gestiona figuras compuestas y mantenimiento de cursores derivados de sus bordes/perímetro.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Alta área | `GeometryResult createArea(const std::vector<QPointF>& points, int areaType, const QString& areaColor)` | Crea área de 4 puntos y cursores asociados. |
| Baja área | `GeometryResult deleteArea(int areaId)` | Elimina área y sus cursores relacionados. |
| Alta círculo | `GeometryResult createCircle(const QPointF& center, double radius, int type, const QString& color)` | Crea círculo y segmentos/cursosres. |
| Baja círculo | `GeometryResult deleteCircle(int circleId)` | Elimina círculo y cursores asociados. |
| Alta polígono | `GeometryResult createPolygon(const std::vector<QPointF>& points, int polyType, const QString& polyColor)` | Crea polígono y su perímetro en cursores. |
| Baja polígono | `GeometryResult deletePolygon(int polygonId)` | Elimina polígono y cursores asociados. |
| Listar figuras | `QJsonObject listShapes() const` | JSON agregado de áreas/círculos/polígonos. |

- **Structs/Tipos definidos**:
- `GeometryResult`
- **Dependencias**:
- `CommandContext`
- `AreaEntity`, `CircleEntity`, `PolygonoEntity`

### OwnShipService

- **Rol**: Mantiene consistencia del estado ownship y su representación como track virtual (`id=0`), además de disparar recálculos PPP de SITREP.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Actualizar JSON | `OwnShipOperationResult updateFromJson(const QJsonObject& args)` | Valida y actualiza ownship desde frontend. |
| Actualizar CLI | `OwnShipOperationResult setFromCli(double courseDeg, double speedKnots, const QString& source = QStringLiteral("CLI"))` | Actualiza ownship desde consola. |
| Serializar | `QJsonObject serializeOwnShip() const` | Snapshot JSON de ownship. |
| Formatear | `QString formatOwnShip() const` | Representación textual operativa. |

- **Structs/Tipos definidos**:
- `OwnShipOperationResult`
- **Dependencias**:
- `CommandContext`
- `TrackPppService`

### CPAService

- **Rol**: Servicio de cálculo CPA/PPP por sesiones, incluyendo marcadores en contexto y control de ciclo de vida (`active/finished/expired`).
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Inicio | `CPAComputationResult startCPA(const CPATrackRef& trackA, const CPATrackRef& trackB)` | Crea sesión y cálculo inicial. |
| Regraficar | `CPAComputationResult graphCPA(const QString& sessionId)` | Recalcula sesión activa. |
| Cálculo directo | `CPAComputationResult computeCPA(const CPATrackRef& trackA, const CPATrackRef& trackB) const` | Cálculo puntual sin mutar sesión. |
| Finalizar | `bool finishCPA(const QString& sessionId)` | Cierra sesión CPA. |
| Limpiar por track | `CPAClearResult clearTrack(const CPATrackRef& trackRef)` | Borra sesiones y marcadores asociados. |
| Estado | `bool isSessionActive(const QString& sessionId) const` | Verifica vigencia de sesión. |

- **Structs/Tipos definidos**:
- `CPATrackRef`, `CPAComputationResult`, `CPASession`, `CPAClearResult`
- **Dependencias**:
- `CommandContext`
- `PppCalculator`

### EstacionamientoService

- **Rol**: Traduce opciones CLI/JSON a entrada matemática de estacionamiento, valida y retorna solución operativa.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Cálculo por opciones | `CalculationResult calculateFromOptions(const QMap<QString, QString>& options) const` | Ejecuta cálculo completo con validación de campos. |
| Cálculo por CLI | `OperationResult executeFromCliArgs(const QStringList& args) const` | Parsea flags CLI y ejecuta flujo de cálculo. |

- **Structs/Tipos definidos**:
- `CalculationResult`, `OperationResult`, `CliInput`
- **Dependencias**:
- `CommandContext`
- `EstacionamientoCalculator`

## Flujo de datos

1. Handler/CLI recibe datos de entrada.
2. Servicio valida dominio y convierte tipos.
3. Servicio opera sobre `CommandContext` (alta/baja/update/consulta).
4. Para cálculos:
   - CPA/PPP -> `PppCalculator`
   - Estacionamiento -> `EstacionamientoCalculator`
5. Resultado se retorna como struct tipado (`...Result`) y luego se serializa a JSON o texto CLI.

## Manejo de errores

- Todas las operaciones críticas retornan structs de resultado con `success/valid + errorCode/message`.
- Validaciones típicas:
  - coordenadas fuera de rango
  - velocidades/cursos inválidos
  - ids no encontrados
  - parámetros faltantes/inconsistentes (por ejemplo VD vs DU)
- En cálculos matemáticos se contemplan estados inválidos/degenerados (`InvalidInput`, `DegenerateRelativeMotion`, etc.).

## Módulos relacionados

- `docs/architecture.md`
- `docs/modules/handlers.md`
- `docs/modules/command-context.md`
- `docs/PPP_SYSTEM.md`
- `docs/STATIONING_SYSTEM.md`
