# Módulo: Decoders & Encoders

## Descripción general

Este módulo implementa la traducción entre protocolo binario de hardware y modelo interno del backend.

- Entrada: `ConcDecoder` decodifica tramas del concentrador DCL y emite señales tácticas.
- Salida: `encoderLPD` codifica el estado del `CommandContext` al formato LPD para el display físico.

Es una capa clave para integrar el backend headless con hardware naval sin acoplar lógica de dominio al protocolo de bytes.

## Lista de archivos y clases

| Archivo | Clase/Struct | Responsabilidad |
|---|---|---|
| `src/model/decoders/iDecoder.h` | `IDecodificator` | Interfaz abstracta de decodificación binaria. |
| `src/model/decoders/concDecoder.h` | `ConcDecoder`, `RollingSteps` | Decodificación de palabras binarias DCL y emisión de señales de control/datos. |
| `src/model/decoders/lpdEncoder.h` | `encoderLPD` | Construcción de mensajes LPD a partir del estado táctico actual. |

## Clases principales

### IDecodificator

- **Rol**: Contrato mínimo para decodificadores de mensajes binarios en el backend.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Decode | `virtual void decode(const QByteArray &message) = 0` | Decodifica payload binario recibido. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `QObject`, `QByteArray`

### ConcDecoder

- **Rol**: Interpreta payload binario DCL en palabras de 24 bits y lo traduce a eventos Qt (`signals`) para overlay, QEK, range, handwheel, rolling ball, centrado y own cursor.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor | `explicit ConcDecoder()` | Inicializa estados internos del decoder. |
| Decode principal | `void decode(const QByteArray &message) override` | Orquesta decodificación por palabras y emisión de señales. |
| Decode word1 | `void decodeWord1()` | Decodifica rango/display/threat. |
| Decode word2 | `void decodeWord2()` | Decodifica centro/own cursor con flancos. |
| Decode word4 | `void decodeWord4()` | Decodifica QEK master/slave. |
| Decode word5 | `void decodeWord5()` | Decodifica ICM y overlay master/slave. |
| Decode word6 | `void decodeWord6()` | Decodifica handwheel. |
| Decode word7 | `void decodeWord7()` | Decodifica rolling ball. |
| Utilidad bits | `QBitArray byteArrayToBitArray(const QByteArray &byteArray)` | Conversión bytes -> bits. |

- **Structs/Tipos definidos**:
- `RollingSteps`
- **Dependencias**:
- `IDecodificator`
- `QBitArray`, `QJsonObject`
- `OverlayHandler`, `OBMHandler`, `OwnCurs` (vía señales en `main.cpp`)

### encoderLPD

- **Rol**: Serializa estado táctico (`CommandContext`) en bytes del protocolo LPD, incluyendo tracks, cursores, center, OBM, marcadores CPA y sesiones de estacionamiento.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Build completo | `QByteArray buildFullMessage(const CommandContext &ctx)` | Arma mensaje LPD final con encabezado, payload y negación de datos. |
| Set OBM | `void setOBMHandler(OBMHandler* oh)` | Inyecta fuente de posición OBM. |
| Track AB2 | `void appendAB2Message(QByteArray& dst, const Track &track)` | Codifica track como mensaje AB2. |
| Cursor AB3 | `void appendAB3Message(QByteArray& dst, const CursorEntity &cursor)` | Codifica cursor como mensaje AB3. |
| Marcador CPA | `void appendCpaMarkerMessage(QByteArray& dst, const CommandContext::CpaMarkerState& marker)` | Codifica marcador CPA. |
| Marcador EST | `void appendStationingMarkerMessage(QByteArray& dst, const CommandContext::StationingSession& session)` | Codifica marcador de estacionamiento. |
| Coordenada | `void appendCoordinate(QByteArray& dst, double value, uint8_t idBits, bool AP=true, bool PV=false, bool LS=false)` | Cuantiza y empaqueta coordenadas. |
| Ángulo | `void appendAngle(QByteArray& dst, double value, bool e, bool v)` | Codifica ángulo para AB3. |
| Negación | `void negateDataInPlace(QByteArray &data, int startPos)` | Invierte bits del payload según protocolo. |

- **Structs/Tipos definidos**:
- Constantes de protocolo (`AB*_ID_*`, `BIT_*`, `EOMM`, headers/padding)
- **Dependencias**:
- `CommandContext`
- `Track`, `CursorEntity`
- `OBMHandler`

## Flujo de datos

1. `DclConcController` recibe datagrama binario, ACKea y entrega payload a `ConcDecoder`.
2. `ConcDecoder` decodifica bits y emite señales de alto nivel.
3. Señales actualizan contexto y componentes operativos (`OverlayHandler`, `OBMHandler`, `OwnCurs`, lambdas en `main`).
4. Timer de salida invoca `encoderLPD::buildFullMessage(*ctx)`.
5. El mensaje binario resultante se envía por `ITransport` al hardware/display.

## Manejo de errores

- `ConcDecoder` usa guardas de tamaño/estado para evitar procesamiento inválido.
- `DclConcController` valida tamaño mínimo de datagrama antes de decodificar.
- `encoderLPD` usa pre-reserva y codificación incremental para robustez de buffer.
- Si no hay `OBMHandler`, `appendOBM` retorna sin codificar bloque OBM.

## Módulos relacionados

- `docs/architecture.md`
- `docs/modules/network.md`
- `docs/modules/command-context.md`
- `docs/README_BACKEND_ARCHITECTURE.md`
