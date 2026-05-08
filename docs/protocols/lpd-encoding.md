# Protocolo: LPD Encoding

## Descripción general

Este documento describe el flujo binario de salida hacia LPD y el acoplamiento con el concentrador DCL.

- `encoderLPD` serializa estado táctico (`CommandContext`) a bytes del protocolo LPD.
- `DclConcController` gestiona polling, ACK de secuencia y entrega de payload invertido al decoder.

El objetivo operativo es mantener la sincronía entre estado backend y display físico, con loop periódico de codificación y envío.

## Lista de archivos y clases

| Archivo | Clase/Struct | Responsabilidad |
|---|---|---|
| `src/model/decoders/lpdEncoder.h` | `encoderLPD` | Codificación de tracks, cursores, centro, OBM y marcadores a formato LPD. |
| `src/controller/dclConcController.h` | `DclConcController` | Control del canal DCL: pedido periódico, ACK por secuencia, normalización de payload. |

## Clases principales

### encoderLPD

- **Rol**: Construye el mensaje binario LPD final a partir del estado en memoria.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Build | `QByteArray buildFullMessage(const CommandContext &ctx)` | Genera trama completa con header, entidades y padding. |
| Set OBM | `void setOBMHandler(OBMHandler* oh)` | Inyecta posición OBM para codificación. |
| Track AB2 | `void appendAB2Message(QByteArray& dst, const Track &track)` | Escribe coordenadas y símbolo/modo de track. |
| Cursor AB3 | `void appendAB3Message(QByteArray& dst, const CursorEntity &cursor)` | Escribe ángulo/largo/coordenadas de cursor. |
| Marker CPA | `void appendCpaMarkerMessage(QByteArray& dst, const CommandContext::CpaMarkerState& marker)` | Codifica marcador CPA. |
| Marker EST | `void appendStationingMarkerMessage(QByteArray& dst, const CommandContext::StationingSession& session)` | Codifica marcador de estacionamiento. |
| Coordenada | `void appendCoordinate(QByteArray& dst, double value, uint8_t idBits, bool AP=true, bool PV=false, bool LS=false)` | Cuantiza y empaqueta coordenada en 24 bits. |
| Ángulo | `void appendAngle(QByteArray& dst, double value, bool e, bool v)` | Codifica ángulo AB3. |
| Longitud cursor | `void appendCursorLong(QByteArray& dst, double value, int type)` | Codifica rho y tipo de cursor. |
| Negación | `void negateDataInPlace(QByteArray &data, int startPos)` | Invierte bits del payload desde offset especificado. |

- **Structs/Tipos definidos**:
- IDs de campos (`AB1_ID_X`, `AB2_ID_Y`, `AB3_ID_ANGLE`, etc.)
- Bits de control (`BIT_LS`, `BIT_PV`, `BIT_V`, `BIT_AP`, `BIT_E`)
- Bytes fijos (`HEADER_BYTES`, `PADDING_BYTES`, `INVALID_OWNSHIP`, `EOMM`)
- **Dependencias**:
- `CommandContext`, `Track`, `CursorEntity`, `OBMHandler`

### DclConcController

- **Rol**: Gestiona interfaz de concentrador DCL en recepción y control de enlace.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor | `DclConcController(ITransport* link, ConcDecoder* decodificator, QObject* parent = nullptr)` | Configura timer de pedido al concentrador cada 50 ms. |
| Pedido DCL | `void askForConcentrator()` | Envía trama fija de solicitud al concentrador. |
| Entrada datagrama | `void onDatagram(const QByteArray&)` | Lee secuencia, responde ACK y delega decode de payload. |
| Invertir payload | `QByteArray negateData(const QByteArray &data)` | Invierte bits de cada byte del payload. |
| Build pedido | `static QByteArray buildPedidoDclConc()` | Retorna `010000012E2F` (hex). |
| Build ACK | `static QByteArray buildAckFromSeq(quint16)` | Arma ACK con MSB=1 y 15 bits de secuencia. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `ITransport`
- `ConcDecoder`
- `QTimer`

## Flujo de datos

### Salida LPD

1. Timer en `main.cpp` dispara cada 40 ms.
2. `encoderLPD::buildFullMessage(*ctx)` arma buffer:
   - header
   - center
   - bloque ownship/OBM
   - tracks (`AB2`)
   - marcadores CPA
   - marcadores estacionamiento
   - cursores (`AB3`)
   - padding
3. `negateDataInPlace(buffer, 3)` aplica inversión de payload.
4. Se envía por `ITransport::send(...)`.

### Entrada DCL y ACK

1. `DclConcController::onDatagram` recibe datagrama.
2. Si tamaño < 3, retorna.
3. Extrae secuencia desde primeros 3 bytes (24 bits, usa 15 bits bajos).
4. Genera ACK con `buildAckFromSeq(seq)` y lo envía.
5. Si hay payload, lo invierte (`negateData`) y llama `m_decoder->decode(payload)`.

## Estructura de codificación observada

- Coordenadas:
  - escala: `value * 256`
  - cuantización signed de 17 bits
  - empaquetado en palabra de 24 bits con bits de control + id
- Ángulo cursor:
  - normalización [0, 360)
  - cuantización de 12 bits (`4096/360`)
- Longitud cursor:
  - cuantización `rho * 256`
  - tipo de línea en 3 bits
- Track symbol bytes:
  - depende de `Identity` y familia de símbolo (`Surface/Air/Subsurface`)
  - incluye modo de track y número en 4 dígitos octales ASCII

## Manejo de errores

- `DclConcController` corta temprano en datagrama corto (`<3`).
- Se usan `Q_ASSERT` en constructor para dependencias no nulas.
- `encoderLPD::appendOBM` retorna sin escribir si `obmHandler == nullptr`.
- `negateDataInPlace` valida `startPos` dentro de buffer.

## Módulos relacionados

- `docs/architecture.md`
- `docs/modules/decoders-encoders.md`
- `docs/modules/network.md`
- `docs/modules/command-context.md`
