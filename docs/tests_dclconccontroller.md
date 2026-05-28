# Tests de DclConcController

Este documento registra los primeros casos de prueba definidos para `DclConcController` y las reglas de comportamiento usadas como criterio. La intención es que los tests validen el comportamiento esperado del sistema/protocolo, no una copia de la implementación actual.

## Supuesto de alcance

Para esta etapa se asume que los datagramas binarios que llegan a `DclConcController` corresponden a mensajes DCL CONC válidos o, al menos, a mensajes que ya fueron derivados a este componente por el flujo de ruteo. Por lo tanto, estos tests no verifican si el mensaje recibido es efectivamente DCL CONC.

La validación estricta del tipo de mensaje queda fuera de este primer alcance y se considera una posible extensión futura, especialmente en pruebas sobre `MessageRouter` o sobre una capa de validación de protocolo.

## Casos implementados

### 1. Inversion de datos por logica negativa

Archivo:

`tests/testingModules/tst_dclconccontroller.cpp`

Test:

```cpp
void negateData_invierte_todos_los_bytes();
```

Regla de comportamiento:

El payload DCL CONC se transmite utilizando logica negativa. Antes de ser interpretado, DDM debe invertir bit a bit cada byte del contenido recibido.

Entrada usada:

```text
00 FF AA 55
```

Salida esperada:

```text
FF 00 55 AA
```

Justificacion:

Este caso valida la transformacion basica requerida antes de entregar el contenido al decoder. No valida todavia el flujo completo de recepcion, sino la operacion de inversion aplicada al payload.

### 2. Datagramas menores a la longitud minima

Archivo:

`tests/testingModules/tst_dclconccontroller.cpp`

Test:

```cpp
void onDatagram_ignora_datagrama_menor_a_tres_bytes();
```

Regla de comportamiento:

Un datagrama que no contiene al menos la palabra inicial de control no tiene informacion suficiente para extraer secuencia ni responder ACK. En ese caso, el controlador debe ignorarlo sin enviar mensajes y sin intentar decodificar contenido.

Entradas usadas:

```text
<vacio>
00
00 00
```

Resultado esperado:

```text
No se envia ningun mensaje por ITransport.
```

Justificacion:

El mensaje minimo que puede procesar `DclConcController` necesita tres bytes iniciales para formar una palabra de 24 bits. Cualquier datagrama menor a ese tamano no permite extraer informacion valida de control.

### 3. ACK con secuencia correcta

Archivo:

`tests/testingModules/tst_dclconccontroller.cpp`

Test:

```cpp
void onDatagram_envia_ack_con_secuencia_correcta();
```

Regla de comportamiento:

Cuando DDM recibe un datagrama DCL CONC con una palabra inicial valida, debe responder con un ACK que conserve los 15 bits de secuencia recibidos y active el bit de ACK.

Entrada usada:

```text
00 12 34
```

Interpretacion:

```text
Palabra inicial: 0x001234
Secuencia:       0x1234
ACK field:       0x8000 | 0x1234 = 0x9234
```

ACK esperado:

```text
04 92 34
```

Justificacion:

Este caso valida que el controlador responda la recepcion del datagrama con un ACK asociado a la misma secuencia. La prueba usa un datagrama sin payload para aislar la verificacion del ACK respecto de la decodificacion posterior.

## Dobles de prueba utilizados

### FakeTransport

Para estos tests se utiliza un doble de prueba de `ITransport`.

Motivo:

`DclConcController` no necesita una red real para validar su logica de procesamiento. Lo relevante en estas pruebas es observar que datos intenta enviar por la interfaz de transporte.

El fake implementa:

```cpp
bool send(const QByteArray& data) override
```

y guarda los mensajes enviados en:

```cpp
QList<QByteArray> sentMessages;
```

Esto permite verificar, por ejemplo, que no se envio nada ante datagramas invalidos o que se envio exactamente el ACK esperado.

La validacion del transporte real, como UDP, deberia realizarse en pruebas de integracion separadas.

## Observaciones sobre la implementacion actual


### Validacion de longitud del payload

Actualmente, si el datagrama tiene mas de tres bytes, el contenido restante se invierte y se entrega al decoder.

Observacion:

Si el protocolo define una longitud exacta para DCL CONC, convendria validar ese tamano antes de decodificar. En esta etapa no se considera obligatorio porque el alcance asume mensajes bien formados.

### Validacion del tipo de mensaje

El controlador no verifica que el binario recibido sea realmente DCL CONC.

Observacion:

Esto es aceptable bajo el supuesto actual de entrada valida, pero debe quedar identificado como riesgo o mejora futura. La validacion podria pertenecer a `MessageRouter`, a una capa de protocolo o al propio controlador si se redefine su responsabilidad.


## Casos pendientes recomendados

1. Datagrama con solo header envia ACK y no decodifica payload.
2. Datagrama con header y payload entrega al decoder el payload invertido.
3. Secuencia con bit alto activo conserva solo los 15 bits de secuencia.
4. Orden de operaciones: el ACK se envia antes de decodificar el payload.
5. Pedido periodico DCL CONC generado por el timer, preferentemente despues de hacer controlable el inicio del polling.


