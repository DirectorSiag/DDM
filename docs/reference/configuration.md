# Referencia: Configuration

## Alcance

Este documento describe la configuración global definida en:

- `src/model/utils/configuration.h`
- `src/model/utils/configuration.cpp`

La clase `Configuration` implementa un singleton de parámetros operativos, direcciones de red y carga de overlay desde recursos Qt.

## Macros de configuración

Definidas en `configuration.h`:

### Puertos

| Macro | Valor |
|---|---|
| `PORT_NOTEBOOK` | `8001` |
| `PORT_FPGA` | `9000` |
| `PORT_RADAR` | `8000` |
| `PORT_MASTER` | `6340` |
| `PORT_SLAVE` | `6340` |

### Direcciones IP (texto)

| Macro | Valor |
|---|---|
| `IP_FPGA` | `172.16.0.100` |
| `IP_MASTER` | `172.16.0.99` |
| `IP_SLAVE` | `172.16.0.101` |

### Otros identificadores

| Macro | Valor |
|---|---|
| `MAC_ADDRESS` | `34BDFA99B530` |
| `DA_CONCENTRADOR` | `0x04` |
| `DA_AND1` | `0x02` |
| `DA_AND2` | `0x03` |
| `DA_LPD` | `0x00` |
| `DA_PEDIDO_CONCENTRADOR` | `0x01` |

### Secuencias ANSI

| Macro | Valor |
|---|---|
| `ANSI_YELLOW` | `\x1b[33m` |
| `ANSI_RESET` | `\x1b[0m` |

## Clase `Configuration`

## Patrón de instancia

- `Configuration::instance()` retorna referencia a instancia estática local (singleton).
- Constructor privado.
- Constructor copia y operador asignación eliminados.

## Estado público

Campos públicos declarados:

| Campo | Tipo | Valor inicial |
|---|---|---|
| `master` | `bool` | sin inicialización explícita en cabecera |
| `meko` | `int` | sin inicialización explícita en cabecera |
| `overlay` | `QString` | default de `QString` |
| `jsonArray` | `QJsonArray` | default de `QJsonArray` |
| `midi` | `int` | `-1` |
| `useLocalIpc` | `bool` | `true` |

## API pública

| Método | Firma | Función |
|---|---|---|
| Instancia | `static Configuration& instance()` | Acceso singleton. |
| Carga | `void loadConfiguration()` | Lee recursos JSON y llena campos internos (`jsonArray`, `meko`, `master`). |
| Set overlay | `void setOverlay(QString)` | Setea overlay seleccionado. |
| Set midi | `void setMidi(int)` | Setea canal/dispositivo MIDI. |
| Get key | `QString getOverlayKey() const` | Busca la clave asociada al valor de overlay en `jsonArray`. |

## Direcciones estáticas `QHostAddress`

Declaradas en `configuration.h` y definidas en `configuration.cpp`:

| Símbolo | Valor |
|---|---|
| `Configuration::masterIpDhcNetwork` | `172.16.0.99` |
| `Configuration::slaveIpDhcNetwork` | `172.16.0.101` |
| `Configuration::masterIpRadarNetwork` | `192.168.7.41` |
| `Configuration::slaveIpRadarNetwork` | `192.168.7.42` |
| `Configuration::ipFpga` | `172.16.0.100` |
| `Configuration::SPERRY` | `192.168.7.50` |
| `Configuration::SCAN_CONVERTER` | `192.168.7.40` |

## Flujo de `loadConfiguration()`

Implementación actual en `configuration.cpp`:

1. Abre recurso `:/json/json/overlay.json` en modo lectura.
2. Abre también `:/json/json/properties.json` (objeto `File`).
3. Lee bytes usando `file.readAll()` (archivo overlay).
4. Parsea `QJsonDocument::fromJson(...)`.
5. Verifica que el documento sea objeto JSON.
6. Verifica presencia de `overlay` como array.
7. Guarda `jsonArray = OverlayJson["overlay"].toArray()`.
8. Carga:
   - `meko = OverlayJson["tipo"].toString().toInt()`
   - `master = OverlayJson["master"].toBool()`

Si alguna verificación falla, la función retorna sin lanzar excepción.

## Resolución de clave de overlay

`getOverlayKey()` recorre `jsonArray`:

1. Para cada elemento, convierte a objeto.
2. Itera claves del objeto.
3. Si el valor string de esa clave coincide con `overlay`, retorna la clave.
4. Si no encuentra coincidencia, retorna `QString()` vacío.

## Observaciones fieles al código actual

- En `configuration.cpp` hay includes repetidos de los mismos headers.
- `loadConfiguration()` abre `properties.json`, pero en esta implementación no usa su contenido.
- Los warnings de apertura/parsing están comentados (`qWarning`), por lo que no hay logging activo de error en esos casos.
