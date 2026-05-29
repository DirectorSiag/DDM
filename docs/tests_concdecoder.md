# Tests de ConcDecoder

Este documento registra los casos de prueba definidos para `ConcDecoder` y las reglas de comportamiento usadas como criterio de validación. La intención es comprobar el comportamiento esperado del componente frente a tramas de entrada controladas, no reproducir la lógica interna de decodificación ni copiar la implementación actual.

## Objetivo de las pruebas

La suite de pruebas de `ConcDecoder` tiene como objetivo verificar que el componente interprete correctamente una trama binaria de recepción y emita las señales observables correspondientes. En esta etapa se valida la decodificación del rango, la interpretación con signo de los desplazamientos del rolling ball y el tratamiento seguro de tramas truncadas o vacías.

Estas pruebas se concentran en el comportamiento observable del decoder. No buscan validar el ruteo de mensajes, la recepción de red, la carga del archivo JSON de decodificación ni la lógica de otros componentes que alimentan a `ConcDecoder`.

## Supuesto de alcance

Para esta etapa se asume que las tramas que llegan a `ConcDecoder` ya fueron seleccionadas por el flujo de comunicación y que representan mensajes aptos para ser decodificados por este componente. Por lo tanto, la suite trabaja con una trama artificial de 27 bytes construida de forma controlada y con casos de error que simulan datagramas truncados o vacíos.

También se asume que el decoder interpreta los bytes tal como llegan, sin una inversión adicional de bits en la fábrica de pruebas. Bajo ese supuesto, la codificación de Word 1, Word 2, Word 4, Word 5 y Word 7 se alinea con los offsets utilizados por la implementación actual.

## Resumen de casos de prueba

| ID | Caso de prueba | Propósito | Resultado esperado |
| --- | --- | --- | --- |
| TC-01 | Decodificación de rango por Word 1 | Verificar que distintas codificaciones de Word 1 produzcan la señal `newRange` con el valor esperado. | Se emite una única señal `newRange` y el valor coincide con la escala esperada. |
| TC-02 | Decodificación con signo del rolling ball | Verificar que Word 7 se interprete como desplazamientos con signo y se emita `newRollingBall` con valores correctos. | Se emite una única señal `newRollingBall` con `dx` y `dy` válidos y con signo correcto. |
| TC-03 | Ignorar tramas corruptas | Verificar que tramas vacías o truncadas no provoquen señales de salida. | No se emiten señales `newRange` ni `newRollingBall`. |

## Casos implementados

### TC-01 - Decodificación de rango por Word 1

Archivo:

`tests/testingModules/testingConcDecoder/tst_decoder.cpp`

Test:

```cpp
void decode_frame_ranges_data();
void decode_frame_ranges();
```

Regla de comportamiento:

Word 1 contiene el campo de escala del mensaje. La suite debe comprobar que, a partir de distintas codificaciones de ese byte, `ConcDecoder` emita `newRange` con el valor correspondiente a la escala esperada.

Entradas usadas:

```text
Word 1: 0x00, 0x01, 0x05, 0x07
Word 2: 0xFF o 0x00
Word 4: 0x10 o 0x00
Word 5: 0x01 o 0x00
Word 7: 0x00 0x00, 0x7F 0x80, 0xFE 0x03
Tamaño total: 27 bytes
```

Salidas o resultado esperado:

```text
Se emite exactamente una señal newRange por fila.
Los valores esperados son 2, 4, 16 y 256 según la codificación utilizada.
```

Justificación:

Este caso valida la responsabilidad principal de Word 1 dentro del libro DCLCONC: la traducción de la secuencia de escala a un valor de rango operativo. La prueba es data driven porque la misma regla de comportamiento se verifica sobre varias filas con distintas codificaciones de entrada.

### TC-02 - Decodificación con signo del rolling ball

Archivo:

`tests/testingModules/testingConcDecoder/tst_decoder.cpp`

Test:

```cpp
void decode_rolling_signed_values_data();
void decode_rolling_signed_values();
```

Regla de comportamiento:

Word 7 transporta los desplazamientos del rolling ball. La información debe interpretarse como valores de 8 bits con signo, de modo que la decodificación conserve correctamente la lógica de complemento a dos.

Entradas usadas:

```text
Caso 1: dx = 127, dy = -128
Caso 2: dx = -2, dy = 3
Word 1: 0x00
Word 2: 0xFF
Word 4: 0x10
Word 5: 0x01
Tamaño total: 27 bytes
```

Salidas o resultado esperado:

```text
Se emite exactamente una señal newRollingBall por fila.
Los valores emitidos coinciden con dx y dy, respetando el signo de cada byte.
```

Justificación:

Este caso verifica una responsabilidad concreta de la decodificación: la interpretación válida de información numérica con signo. Se usa enfoque data driven porque la misma lógica se evalúa sobre extremos y valores intermedios para reducir el riesgo de errores en el manejo del signo.

### TC-03 - Ignorar tramas corruptas

Archivo:

`tests/testingModules/testingConcDecoder/tst_decoder.cpp`

Test:

```cpp
void decode_corrupt_frame_is_ignored_data();
void decode_corrupt_frame_is_ignored();
```

Regla de comportamiento:

Tramas truncadas o vacías no contienen tamaño suficiente para una decodificación válida. En ese caso, `ConcDecoder` no debe propagar señales observables de salida.

Entradas usadas:

```text
Trama truncada de 10 bytes
Trama vacía
```

Salidas o resultado esperado:

```text
No se emiten señales newRange ni newRollingBall.
```

Justificación:

Este caso valida el tratamiento seguro de información inválida o incompleta. La verificación evita que una secuencia corrupta se convierta en una salida observable falsa dentro del resto del sistema.

## Cobertura actual de pruebas

- Decodificación del rango operativo a partir de Word 1.
- Interpretación con signo de los desplazamientos del rolling ball en Word 7.
- Manejo seguro de tramas vacías o truncadas.
- Verificación de emisión única de señales observables para cada caso válido.

## Dobles de prueba utilizados

- `QSignalSpy`
  - Motivo: observar las señales emitidas por `ConcDecoder` sin modificar el componente.
  - Qué método redefine o implementa: no redefine métodos; se conecta a señales de Qt.
  - Qué permite observar: cantidad de emisiones y argumentos transportados por `newRange` y `newRollingBall`.

- `buildDynamicFrame(...)`
  - Motivo: generar tramas de entrada controladas y repetibles para la suite.
  - Qué método redefine o implementa: no redefine un método del componente; actúa como fábrica de datos de prueba.
  - Qué permite observar: el comportamiento de decodificación bajo diferentes secuencias, tamaños y combinaciones de bytes.

- `qRegisterMetaType<QPair<float, float>>()`
  - Motivo: habilitar el transporte de pares de valores a través de `QSignalSpy`.
  - Qué método redefine o implementa: no redefine métodos; registra un tipo compuesto para el sistema de metaobjetos de Qt.
  - Qué permite observar: los argumentos emitidos por señales que transportan pares numéricos.

## Limitaciones actuales de cobertura

- Responsabilidades de otros componentes
  - No se valida la recepción de red ni el ruteo previo de mensajes.
  - No se verifica la carga o integridad del archivo JSON de decodificación.
  - No se cubren decisiones de otros controllers o de `MessageRouter`.

- Validaciones futuras
  - No hay una verificación explícita de `newQEK`.
  - No hay una verificación explícita de `newOverlay`.
  - No hay una verificación explícita de `newHandWheel`.
  - No se comprueba el detalle interno de `Word 2` más allá de la infraestructura actual de la suite.

- Casos no abordados
  - No se cubren combinaciones de trama con corrupción parcial y salida válida simultánea.
  - No se cubren variaciones sistemáticas de todos los offsets del libro DCLCONC.
  - No se valida el comportamiento ante secuencias de múltiples mensajes consecutivos.

## Casos pendientes recomendados

1. Agregar un caso que valide `newQEK` para una secuencia concreta documentada en `decodificado.json`.
2. Agregar un caso que valide `newOverlay` con una secuencia explícita del libro DCLCONC.
3. Agregar un caso que verifique `newHandWheel` con valores positivos, negativos y cero.
4. Agregar un caso que cubra el contenido de Word 2 con señales individuales de control.
5. Agregar un caso de corrupción parcial donde la trama sea válida en tamaño pero tenga bytes inconsistentes en campos no usados.
6. Agregar un caso con varias tramas consecutivas para observar estabilidad de estado entre llamadas a `decode()`.