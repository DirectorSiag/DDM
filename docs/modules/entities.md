# Módulo: Entities

## Descripción general

Este módulo define los tipos de dominio táctico que persisten en `CommandContext` y viajan por la lógica de negocio: tracks, cursores y figuras geométricas (áreas, círculos y polígonos), junto con enums operativos de clasificación e identidad.

Su objetivo es modelar estado táctico consistente y reusable entre CLI, comandos JSON, cálculo CPA/PPP, estacionamiento y codificación LPD.

## Lista de archivos y clases

| Archivo | Clase/Struct | Responsabilidad |
|---|---|---|
| `src/model/entities/track.h` | `Track` | Entidad táctica principal con cinemática, identidad, links y datos SITREP/PPP. |
| `src/model/entities/cursorEntity.h` | `CursorEntity` | Línea/cursor táctico (posición, ángulo, largo, tipo, estado activo). |
| `src/model/entities/areaEntity.h` | `AreaEntity` | Figura de 4 puntos con cursores de borde asociados. |
| `src/model/entities/circleEntity.h` | `CircleEntity` | Círculo con segmentación en cursores asociados. |
| `src/model/entities/polygonoentity.h` | `PolygonoEntity` | Polígono de N puntos con cursores perimetrales. |
| `src/model/enums/enums.h` | `TrackData` | Enumeraciones y conversiones (`Type`, `Identity`, `TrackMode`, links). |

## Clases principales

### Track

- **Rol**: Modelo central de contacto táctico (posición, rumbo, velocidad, identidad y metadata operativa). Soporta representación SITREP y cálculo PPP almacenado por track.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor | `Track(int id, Type type, Identity identity, TrackMode mode, float xDm, float yDm, double speedKnots = 0.0, double courseDeg = 0.0, Environment creationEnvironment = TrackData::SPC)` | Inicializa track completo. |
| Get id | `int getId() const` | Identificador de track. |
| Get tipo | `Type getType() const` | Tipo táctico (SPC, AAW, etc.). |
| Get posición | `float getX() const`, `float getY() const` | Posición en DM. |
| Get cinemática | `double getSpeedKnots() const`, `double getCourseDeg() const` | Velocidad y rumbo. |
| Get derivados | `double getAzimuthDeg() const`, `double getDistanceDm() const` | Cálculo geométrico desde origen. |
| Set posición | `void setX(float xDm)`, `void setY(float yDm)` | Actualización de coordenadas. |
| Set cinemática | `void setSpeedKnots(double kt)`, `void setCourseDeg(double deg)` | Actualización de velocidad/curso. |
| Set SITREP | `void setInformacionAmpliatoria(const QString& text)` | Info textual ampliatoria. |
| Set PPP | `void setSitrepPpp(const SitrepPppData& ppp)` | Persiste resultado PPP para SITREP. |
| Actualizar posición | `void updatePosition(double deltaTimeSeconds)` | Avance cinemático según velocidad/rumbo. |
| ToString | `QString toString() const` | Texto de depuración/inspección. |

- **Structs/Tipos definidos**:
- `Track::SitrepPppData` (incluye `Status`, azimut/distancia/tiempo/razón).
- **Dependencias**:
- `TrackData` (enums)
- `RadarMath` (cálculos derivados)

### CursorEntity

- **Rol**: Representa una línea táctica simple; se usa tanto para cursores explícitos como para bordes/perímetros de figuras.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor | `CursorEntity(QPair<qfloat16, qfloat16> coordinates, qfloat16 angle, qfloat16 length, int lineType, int cursorId, bool active)` | Inicializa cursor. |
| Get coords | `QPair<qfloat16, qfloat16> getCoordinates() const` | Coordenadas actuales. |
| Get ángulo/largo | `qfloat16 getCursorAngle() const`, `qfloat16 getCursorLength() const` | Parámetros geométricos. |
| Get tipo/id | `int getLineType() const`, `int getCursorId() const` | Tipo e id de cursor. |
| Estado | `bool isActive() const`, `void setActive(bool value)` | Activación visual/operativa. |
| Mutadores | `setCoordinates`, `setCursorAngle`, `setCursorLength`, `setLineType`, `setCursorId` | Edición de campos. |
| Toggle | `void switchActive()` | Cambia estado `active`. |
| ToString | `QString toString() const` | Texto de depuración. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- Qt (`QPair`, `qfloat16`, `QString`)

### AreaEntity

- **Rol**: Figura de área definida por cuatro vértices con ids de cursores de cada arista.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor | `AreaEntity(int id, const std::vector<QPointF>& points, int type, const QString& color)` | Inicializa entidad. |
| Get vértices | `getPointA/B/C/D` | Acceso a vértices del área. |
| Set vértices | `setPointA/B/C/D` | Actualiza vértices. |
| Get tipo/color | `int getType() const`, `const QString& getColor() const` | Metadata de figura. |
| Get ids aristas | `getCursorIdAB/BC/CD/DA` | Referencias a cursores asociados. |
| Recalcular bordes | `void calculateAndStoreCursors(CommandContext& ctx)` | Genera cursores para bordes y guarda ids. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `CommandContext`
- `CursorEntity`
- `RadarMath`

### CircleEntity

- **Rol**: Figura circular con lista de ids de cursores segmentados para representación perimetral.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor | `CircleEntity(int id, const QPointF& center, double radius, int type, const QString& color)` | Inicializa círculo. |
| Get centro/radio | `const QPointF& getCenter() const`, `double getRadius() const` | Parámetros geométricos. |
| Set centro/radio | `setCenter`, `setRadius` | Actualiza geometría. |
| Get ids | `const std::vector<int>& getCursorIds() const` | Cursores perimetrales. |
| Recalcular perímetro | `void calculateAndStoreCursors(CommandContext& ctx)` | Genera/reasigna cursores de perímetro. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `CommandContext`
- `CursorEntity`

### PolygonoEntity

- **Rol**: Figura poligonal genérica con N vértices y cursores del perímetro.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor | `PolygonoEntity(int id, const std::vector<QPointF>& points, int type, const QString& color)` | Inicializa polígono. |
| Get puntos | `const std::vector<QPointF>& getPoints() const` | Vértices del polígono. |
| Set puntos | `void setPoints(const std::vector<QPointF>& points)` | Reemplaza vértices. |
| Get ids | `const std::vector<int>& getCursorIds() const` | Referencias a cursores de borde. |
| Recalcular | `void calculateAndStoreCursors(CommandContext& ctx)` | Regenera perímetro en cursores. |
| Update | `void update(CommandContext& ctx)` | Recalcula entidad y cursores asociados. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `CommandContext`
- `CursorEntity`

### TrackData

- **Rol**: Namespace/clase utilitaria de enums para tipo de track, identidad, modo de seguimiento y estados de enlace.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Tipo a texto | `static QString toQString(Type value)` | Conversión enum -> string para serialización. |
| Identidad a texto | `static QString toQString(Identity value)` | Conversión para salida JSON/UI. |
| Modo a texto | `static QString toQString(TrackMode value)` | Conversión de modo de track. |
| Parse tipo | `static bool fromQString(const QString& text, Type& out)` | Parseo string -> enum. |
| Parse identidad | `static bool fromQString(const QString& text, Identity& out)` | Parseo string -> enum. |

- **Structs/Tipos definidos**:
- `Type`, `Identity`, `TrackMode`, `LinkYStatus`, `Link14Status`
- **Dependencias**:
- Qt (`QString`)

## Flujo de datos

1. Servicios crean/actualizan entidades desde requests CLI/JSON.
2. Entidades quedan persistidas en `CommandContext` (deques/mapas).
3. Módulos de salida (`TrackService::serializeTracks`, `CursorService::serializeCursors`, `encoderLPD`) consumen estas entidades.
4. Figuras compuestas (`AreaEntity`, `CircleEntity`, `PolygonoEntity`) generan/referencian `CursorEntity` para representación en display.

## Manejo de errores

- Validaciones de rango y forma se hacen mayormente en services (no en entidades puras).
- Entidades compuestas usan guardas de tamaño de puntos para evitar generación inválida.
- Actualizaciones de curso/velocidad y normalizaciones en `Track` evitan estados fuera de dominio operativo.

## Módulos relacionados

- `docs/architecture.md`
- `docs/modules/command-context.md`
- `docs/modules/services.md`
- `docs/reference/enums.md` (referencia planificada)
