# Referencia: Enums

## Alcance

Este documento describe la definición real de enumeraciones y helpers de conversión declarados en `src/model/enums/enums.h`.

La unidad principal es la clase `TrackData` (con `Q_GADGET` y `Q_ENUM`) para exponer metadatos de enums al sistema de metaobjetos de Qt.

## Archivo fuente

- `src/model/enums/enums.h`

## Clase contenedora

### `TrackData`

- Usa `Q_GADGET` para reflexión de enums sin necesidad de `QObject` instanciable.
- Declara seis enums con `Q_ENUM(...)`:
  - `Type`
  - `Identity`
  - `TrackMode`
  - `LinkYStatus`
  - `Link14Status`
  - `AsignacionFC`

## Enumeraciones

### `TrackData::Type`

Valores declarados:

| Valor | Comentario en código |
|---|---|
| `SPC` | Tipo táctico |
| `LINCO` | Tipo táctico |
| `ASW` | Tipo táctico |
| `OPS` | Tipo táctico |
| `HECO` | Tipo táctico |
| `APC` | Tipo táctico |
| `AAW` | Tipo táctico |
| `EW` | Tipo táctico |

Mapeo binario implementado por `typeBits(...)` y `tryParseTypeBits(...)`:

| Bits | Type |
|---|---|
| `0001` | `SPC` |
| `0010` | `LINCO` |
| `0011` | `ASW` |
| `0100` | `OPS` |
| `0101` | `HECO` |
| `0110` | `APC` |
| `0111` | `AAW` |
| `1000` | `EW` |

### `TrackData::Identity`

Valores declarados:

| Valor |
|---|
| `Pending` |
| `PossHostile` |
| `PossFriend` |
| `ConfHostile` |
| `ConfFriend` |
| `EvalUnknown` |
| `Heli` |

Código de una letra implementado por `identityCode(...)`:

| Identity | Código |
|---|---|
| `Pending` | `P` |
| `PossFriend` | `A` |
| `ConfFriend` | `F` |
| `PossHostile` | `E` |
| `ConfHostile` | `H` |
| `EvalUnknown` | `U` |
| `Heli` | `Y` |
| default | `P` |

### `TrackData::TrackMode`

Valores declarados:

| Valor |
|---|
| `FC1` |
| `FC2` |
| `FC3` |
| `FC4` |
| `FC5` |
| `Auto` |
| `TentativeAuto` |
| `AutomaticLost` |
| `RAM` |
| `DR` |
| `Lost` |
| `Blank` |

### `TrackData::LinkYStatus`

Valores declarados:

| Valor |
|---|
| `LinkY_R` |
| `LinkY_C` |
| `LinkY_T` |
| `LinkY_S` |
| `LinkY_Invalid` |

Código de salida implementado por `linkYCode(...)`:

| LinkYStatus | Código |
|---|---|
| `LinkY_R` | `R` |
| `LinkY_C` | `C` |
| `LinkY_T` | `T` |
| `LinkY_S` | `S` |
| default (`LinkY_Invalid`) | `-` |

### `TrackData::Link14Status`

Valores declarados:

| Valor |
|---|
| `Link14_T` |
| `Link14_Invalid` |

Código de salida implementado por `link14Code(...)`:

| Link14Status | Código |
|---|---|
| `Link14_T` | `T` |
| default (`Link14_Invalid`) | `-` |

### `TrackData::AsignacionFC`

Valores declarados:

| Valor | Entero asociado |
|---|---|
| `FC_Invalid` | `0` |
| `FC_1` | `1` |
| `FC_2` | `2` |
| `FC_3` | `3` |
| `FC_4` | `4` |
| `FC_5` | `5` |
| `FC_6` | `6` |

## Helpers de conversión y parseo

### Conversión enum -> QString

Sobrecargas públicas:

- `toQString(Type)`
- `toQString(Identity)`
- `toQString(TrackMode)`
- `toQString(LinkYStatus)`
- `toQString(Link14Status)`
- `toQString(AsignacionFC)`

Todas delegan en helper privado template `enumName(...)`, basado en `QMetaEnum::fromType<E>()` y `valueToKey(...)`.

Si no existe clave válida en metadatos, devuelven fallback `"Unknown"`.

### Parseo de `Type` por etiqueta

`tryParseType(const QString& value, Type& out)`:

- Normaliza con `trimmed().toUpper()`.
- Acepta exactamente: `SPC`, `LINCO`, `ASW`, `OPS`, `HECO`, `APC`, `AAW`, `EW`.
- Retorna `true/false` según éxito.

### Parseo de `Type` por bits

`tryParseTypeBits(const QString& bits, Type& out)`:

- Normaliza con `trimmed()`.
- Acepta cadenas de 4 bits según tabla del enum `Type`.
- Retorna `true/false` según éxito.

### Generación de bits desde `Type`

`typeBits(Type v)` devuelve los mismos códigos de 4 bits del mapeo.

Default actual para valores fuera de switch: `"0001"`.

## Notas de implementación

- El archivo está protegido por include guard `TYPESDATA_H`.
- No usa `enum class`; usa `enum` simple por decisión explícita en comentario del archivo.
- El diseño permite reutilizar nombres y códigos en CLI, JSON y render de vistas sin switches repetidos fuera de `TrackData`.
