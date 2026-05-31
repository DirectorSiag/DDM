# Tests de MessageRouter

Este documento registra los casos de prueba definidos para `MessageRouter` y las reglas de comportamiento usadas como criterio de validación. La intención es verificar el enrutamiento observable de mensajes entrantes, no reproducir la implementación actual ni copiar su lógica interna.

Archivo de pruebas:

`tests/testingModules/testingMessageRouter/tst_messagerouter.cpp`

## Objetivo de las pruebas

La suite de pruebas de `MessageRouter` tiene como objetivo verificar que el componente derive cada mensaje entrante hacia el receptor correspondiente. En particular, se valida el recorrido de datagramas binarios hacia `DclConcController`, el recorrido de mensajes JSON hacia `JsonCommandHandler` y el tratamiento de entradas vacías, incompletas o con variaciones válidas de formato.

Estas pruebas observan efectos producidos por los componentes receptores mediante dobles de prueba para el transporte y el decoder. No buscan volver a validar exhaustivamente la lógica interna de `DclConcController`, la decodificación semántica realizada por `ConcDecoder`, la recepción de red real ni el comportamiento completo de cada comando JSON.

## Supuesto de alcance

Para esta etapa se asume que `MessageRouter` recibe mensajes desde un único `ITransport` y debe distinguir, como mínimo, entre mensajes JSON y datagramas binarios derivados a `DclConcController`.

El código actual no permite confirmar si por el mismo transporte pueden recibirse otros protocolos binarios. Por lo tanto, la suite no define todavía el comportamiento esperado para mensajes binarios ajenos a DCL CONC. Esa decisión requiere confirmar el contrato de comunicación con el equipo.

También se considera que los espacios exteriores y los saltos de línea son variaciones válidas de un texto JSON. El caso de arrays JSON se documenta por separado porque `JsonCommandHandler` trabaja con objetos y su tratamiento esperado debe confirmarse.

## Resumen de casos de prueba

| ID | Caso de prueba | Propósito | Resultado esperado |
| --- | --- | --- | --- |
| TC-01 | Enrutamiento de datagrama binario | Verificar que un mensaje binario tome el recorrido DCL CONC. | El controller envía el ACK esperado y delega el payload invertido al decoder. |
| TC-02 | Enrutamiento de JSON válido | Verificar que un objeto JSON válido llegue a `JsonCommandHandler`. | Se responde `UNKNOWN_COMMAND` y el decoder no recibe datos. |
| TC-03 | JSON malformado entre llaves | Verificar que un texto con apariencia JSON llegue al handler para obtener un error controlado. | Se responde `INVALID_JSON` y el decoder no recibe datos. |
| TC-04 | Datagrama vacío | Verificar que una entrada sin información sea ignorada. | No se envían mensajes ni se intenta decodificar contenido. |
| TC-05 | Binario menor a tres bytes | Verificar el recorrido defensivo de un binario sin tamaño mínimo para DCL CONC. | No se envían mensajes ni se intenta decodificar contenido. |
| TC-06 | JSON con espacios exteriores | Verificar que el whitespace permitido por JSON no altere su clasificación. | Se responde `UNKNOWN_COMMAND` y el decoder no recibe datos. Actualmente falla. |
| TC-07 | JSON con salto de línea final | Verificar que un salto de línea final no altere la clasificación de un JSON válido. | Se responde `UNKNOWN_COMMAND` y el decoder no recibe datos. Actualmente falla. |
| TC-08 | JSON sin llave exterior de cierre | Verificar que una entrada JSON incompleta llegue al handler y produzca un error controlado. | Se responde `INVALID_JSON` y el decoder no recibe datos. |
| TC-09 | Array JSON | Registrar el comportamiento deseable ante un JSON válido que no es un objeto. | El handler responde `INVALID_JSON` de forma controlada. Actualmente falla y requiere confirmar el contrato. |

## Casos implementados

### TC-01 - Enrutamiento de datagrama binario

Archivo:

`tests/testingModules/testingMessageRouter/tst_messagerouter.cpp`

Test:

```cpp
void onMessageReceived_rutea_datagrama_binario_al_controlador_dcl();
```

Regla de comportamiento:

Un datagrama que no se clasifica como JSON debe recorrer la rama binaria hacia `DclConcController`.

Entrada usada:

```text
00 12 34 00 FF AA 55
```

Salidas o resultado esperado:

```text
ACK enviado:        04 92 34
Payload en decoder: FF 00 55 AA
```

Justificación:

El test verifica el recorrido binario mediante efectos observables del controller. La comprobación del ACK y del payload invertido repite parcialmente responsabilidades cubiertas por la suite específica de `DclConcController`, pero permite demostrar que el router derivó el mensaje hacia ese componente.

### TC-02 - Enrutamiento de JSON válido

Archivo:

`tests/testingModules/testingMessageRouter/tst_messagerouter.cpp`

Test:

```cpp
void onMessageReceived_rutea_json_al_handler_json();
```

Regla de comportamiento:

Un objeto JSON válido debe llegar a `JsonCommandHandler` y no debe enviarse al recorrido binario.

Entrada usada:

```json
{"command":"unsupported","args":{}}
```

Salidas o resultado esperado:

```text
status:  error
command: unsupported
code:    UNKNOWN_COMMAND
```

Justificación:

Se utiliza deliberadamente un comando desconocido para obtener una respuesta identificable sin modificar el estado del sistema. La ausencia de mensajes en el decoder permite comprobar que el JSON no fue derivado a `DclConcController`.

### TC-03 - JSON malformado entre llaves

Archivo:

`tests/testingModules/testingMessageRouter/tst_messagerouter.cpp`

Test:

```cpp
void onMessageReceived_rutea_json_malformado_al_handler_json();
```

Regla de comportamiento:

Una entrada con apariencia de objeto JSON debe llegar al handler JSON para que el error de parseo se gestione de forma controlada.

Entrada usada:

```text
{invalid}
```

Salidas o resultado esperado:

```text
status: error
code:   INVALID_JSON
```

Justificación:

El router no es responsable de validar la sintaxis completa. El test comprueba que delegue el mensaje al componente encargado del parseo y que el decoder binario no reciba información inválida.

### TC-04 - Datagrama vacío

Archivo:

`tests/testingModules/testingMessageRouter/tst_messagerouter.cpp`

Test:

```cpp
void onMessageReceived_ignora_datagrama_vacio();
```

Regla de comportamiento:

Una entrada vacía no contiene información suficiente para generar una respuesta ni iniciar una decodificación.

Entrada usada:

```text
<vacío>
```

Salidas o resultado esperado:

```text
No se envían mensajes por ITransport.
El decoder no recibe mensajes.
```

Justificación:

Este caso verifica un comportamiento defensivo básico ante recepción de información inexistente.

### TC-05 - Binario menor a tres bytes

Archivo:

`tests/testingModules/testingMessageRouter/tst_messagerouter.cpp`

Test:

```cpp
void onMessageReceived_ignora_datagrama_binario_menor_a_tres_bytes();
```

Regla de comportamiento:

Un mensaje binario menor a tres bytes no contiene el tamaño mínimo requerido por `DclConcController` para extraer una secuencia.

Entrada usada:

```text
01 02
```

Salidas o resultado esperado:

```text
No se envían mensajes por ITransport.
El decoder no recibe mensajes.
```

Justificación:

El test recorre el router y comprueba que una entrada binaria incompleta sea descartada de forma segura por el flujo actual. La validación del tamaño mínimo pertenece internamente a `DclConcController`.

### TC-06 - JSON con espacios exteriores

Archivo:

`tests/testingModules/testingMessageRouter/tst_messagerouter.cpp`

Test:

```cpp
void onMessageReceived_acepta_json_con_espacios_exteriores();
```

Regla de comportamiento:

Un JSON válido debe conservar su clasificación aunque incluya espacios exteriores permitidos por el formato.

Entrada usada:

```text
  {"command":"unsupported","args":{}}  
```

Salidas o resultado esperado:

```text
Respuesta UNKNOWN_COMMAND.
El decoder no recibe mensajes.
```

Justificación:

Los espacios exteriores no invalidan el JSON. Actualmente el test falla porque el mensaje no comienza exactamente con `{` y el router lo deriva hacia `DclConcController`.

### TC-07 - JSON con salto de línea final

Archivo:

`tests/testingModules/testingMessageRouter/tst_messagerouter.cpp`

Test:

```cpp
void onMessageReceived_acepta_json_con_salto_de_linea_final();
```

Regla de comportamiento:

Un JSON válido debe conservar su clasificación aunque termine con un salto de línea.

Entrada usada:

```text
{"command":"unsupported","args":{}}
\n
```

Salidas o resultado esperado:

```text
Respuesta UNKNOWN_COMMAND.
El decoder no recibe mensajes.
```

Justificación:

El salto de línea final es whitespace válido. Actualmente el test falla porque el mensaje no termina exactamente con `}` y el router lo deriva hacia `DclConcController`.

### TC-08 - JSON sin llave exterior de cierre

Archivo:

`tests/testingModules/testingMessageRouter/tst_messagerouter.cpp`

Test:

```cpp
void onMessageReceived_rutea_json_sin_llave_de_cierre_al_handler_json();
```

Regla de comportamiento:

Una entrada con apariencia JSON incompleta debe llegar al handler JSON y producir un error de parseo controlado.

Entrada usada:

```text
{"command":"unsupported","args":{}
```

Salidas o resultado esperado:

```text
Respuesta INVALID_JSON.
El decoder no recibe mensajes.
```

Justificación:

La entrada comienza con `{` y termina con la llave de cierre del objeto `args`, aunque le falta la llave exterior final. El flujo actual la deriva al handler JSON, que detecta correctamente el error de sintaxis.

### TC-09 - Array JSON

Archivo:

`tests/testingModules/testingMessageRouter/tst_messagerouter.cpp`

Test:

```cpp
void onMessageReceived_rutea_array_json_al_handler_json();
```

Regla de comportamiento:

Un array es JSON válido, aunque `JsonCommandHandler` espere un objeto. El comportamiento propuesto es delegarlo al handler para que responda `INVALID_JSON` de forma controlada.

Entrada usada:

```json
[]
```

Salidas o resultado esperado:

```text
Respuesta INVALID_JSON.
El decoder no recibe mensajes.
```

Justificación:

Actualmente el test falla porque `MessageRouter` no reconoce arrays como JSON y deriva `[]` hacia la rama binaria. Este resultado debe discutirse con el equipo: si el contrato sólo admite objetos JSON, corresponde decidir si el descarte debe ocurrir en el router o si debe mantenerse una respuesta explícita del handler.

## Cobertura actual de pruebas

La suite cubre las siguientes responsabilidades y recorridos observables de `MessageRouter`:

- Derivación de un datagrama binario hacia `DclConcController`.
- Derivación de un objeto JSON válido hacia `JsonCommandHandler`.
- Tratamiento controlado de JSON malformado con apariencia de objeto.
- Ausencia de respuestas y decodificación ante un datagrama vacío.
- Flujo defensivo ante un binario menor al tamaño mínimo procesable por DCL CONC.
- Clasificación de JSON válido con espacios exteriores.
- Clasificación de JSON válido con salto de línea final.
- Tratamiento de una entrada JSON incompleta.
- Registro del comportamiento esperado ante arrays JSON.

## Dobles de prueba utilizados

### FakeTransport

Motivo:

`MessageRouter` no necesita una red real para validar el destino observable de los mensajes. El fake permite registrar las respuestas generadas por los componentes receptores.

El fake implementa:

```cpp
bool send(const QByteArray& data) override
```

y guarda los mensajes enviados en:

```cpp
QList<QByteArray> sentMessages;
```

Qué permite observar:

- Cantidad de respuestas enviadas.
- ACK producido por el recorrido DCL CONC.
- Respuestas JSON producidas por `JsonCommandHandler`.
- Ausencia de respuestas ante entradas vacías o incompletas.

### RecordingDecoder

Motivo:

El doble permite detectar si un mensaje recorrió la rama binaria sin ejecutar la lógica interna de decodificación de `ConcDecoder`.

El doble redefine:

```cpp
void decode(const QByteArray& message) override
```

y guarda los mensajes recibidos en:

```cpp
QList<QByteArray> decodedMessages;
```

Qué permite observar:

- Si el decoder recibió información.
- Cuántos mensajes recibió.
- Qué payload fue entregado por el recorrido DCL CONC.
- Si un JSON fue derivado incorrectamente hacia la rama binaria.

## Limitaciones actuales de cobertura

### Responsabilidades de otros componentes

- La suite no valida la recepción UDP real ni el transporte IPC local.
- La construcción del ACK, la inversión del payload y la extracción de secuencia pertenecen a `DclConcController`; TC-01 sólo los usa como evidencia observable del recorrido binario.
- La validación semántica de comandos JSON pertenece a `JsonCommandHandler`.
- La interpretación de campos binarios pertenece a `ConcDecoder`.

### Validaciones futuras

- No se verifica todavía el comportamiento ante mensajes binarios pertenecientes a otros protocolos. Es necesario confirmar si pueden llegar por el mismo `ITransport`.
- No se valida filtrado por IP de origen, puerto o encabezado, porque `MessageRouter` recibe únicamente el payload entregado por `ITransport`.
- No se cubren secuencias de múltiples mensajes consecutivos alternando JSON y binario.

### Casos no abordados

- No se prueba una entrada que comience con `{` y termine con un carácter distinto de `}` sin conservar una llave interna final.
- No se prueban otros valores JSON válidos que no sean objetos, como strings, números, booleanos o `null`.
- No se define como requisito definitivo el tratamiento de arrays JSON. TC-09 registra una propuesta que debe validarse con el equipo.

## Casos pendientes recomendados

1. Confirmar si pueden llegar mensajes binarios ajenos a DCL CONC por el mismo transporte y agregar un caso de rechazo o derivación específica.
2. Agregar un caso con apariencia JSON que comience con `{` y no termine con `}`, sin una llave interna final que coincida con la condición actual.
3. Definir el tratamiento esperado para valores JSON válidos que no sean objetos.
4. Agregar una secuencia alternada de mensajes JSON y binarios para verificar que el router no conserve estado indebido entre recepciones.
5. Agregar pruebas de integración separadas para UDP e IPC local si se requiere validar la recepción real.

## Issues detectados para GitLab

### Issue 1 - MessageRouter clasifica incorrectamente JSON válido con whitespace exterior

**Título sugerido:**

`MessageRouter deriva JSON válido con espacios exteriores o salto de línea final hacia DclConcController`

**Descripción:**

`MessageRouter::onMessageReceived()` clasifica un mensaje como JSON únicamente si el primer carácter es `{` y el último carácter es `}`. Esta condición rechaza objetos JSON válidos cuando contienen espacios exteriores o un salto de línea final.

**Pasos para reproducir:**

Enviar cualquiera de los siguientes mensajes:

```text
  {"command":"unsupported","args":{}}  
```

```text
{"command":"unsupported","args":{}}
\n
```

**Resultado actual:**

Los mensajes se derivan a `DclConcController` como si fueran datagramas binarios.

**Resultado esperado:**

Los mensajes deben llegar a `JsonCommandHandler`, que debe procesarlos como JSON válidos.

**Tests de regresión disponibles:**

```cpp
void onMessageReceived_acepta_json_con_espacios_exteriores();
void onMessageReceived_acepta_json_con_salto_de_linea_final();
```

**Ubicación probable:**

`src/controller/messagerouter.cpp`

### Issue 2 - Definir tratamiento de valores JSON válidos que no sean objetos

**Título sugerido:**

`Definir y validar el tratamiento de arrays JSON recibidos por MessageRouter`

**Descripción:**

Un array como `[]` es JSON válido, pero `MessageRouter` no lo reconoce como JSON porque sólo contempla mensajes delimitados por `{` y `}`. Como consecuencia, el array se deriva a `DclConcController`.

`JsonCommandHandler` espera un objeto JSON y ya dispone de una respuesta `INVALID_JSON` para documentos que no sean objetos. Sin embargo, actualmente el mensaje no llega a ese handler.

**Decisión requerida:**

Confirmar si el contrato admite únicamente objetos JSON y definir dónde debe rechazarse un array:

- en `MessageRouter`, mediante descarte explícito; o
- en `JsonCommandHandler`, mediante una respuesta `INVALID_JSON`.

**Test disponible sujeto a confirmación del contrato:**

```cpp
void onMessageReceived_rutea_array_json_al_handler_json();
```

**Ubicación probable:**

`src/controller/messagerouter.cpp`
