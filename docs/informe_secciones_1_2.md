# 1. Resumen del trabajo a realizar

El presente Proyecto Final se enmarca en el desarrollo y validación de software aplicado a sistemas navales y de defensa, particularmente en el contexto de modernización de consolas tácticas utilizadas por unidades de la Armada Argentina. En dicho ámbito, existen sistemas de vigilancia, navegación y control que, aun siendo operativos, fueron desarrollados sobre tecnologías anteriores y requieren una evolución progresiva hacia soluciones digitales, mantenibles y verificables.

La Base Naval Puerto Belgrano constituye el principal asentamiento de la Flota de Mar de la República Argentina. Sus unidades emplean distintos sistemas electrónicos, eléctricos, informáticos y mecánicos para asistir tareas de navegación, vigilancia, control y toma de decisiones operativas. Dentro de este ecosistema, la información proveniente de radares y de fuentes sintéticas resulta central para representar el entorno táctico, identificar contactos, analizar trayectorias y asistir al operador en tiempo real.

Como parte de iniciativas previas, se desarrolló una aplicación orientada a reemplazar y modernizar componentes de la Consola Táctica y Vertical, manteniendo compatibilidad con el sistema heredado y permitiendo una transición gradual. El trabajo realizado por Mario Córdoba constituyó una primera etapa de verificación y validación sobre módulos asociados a la recepción y procesamiento de información crudo-radar y sintética, incorporando pruebas unitarias, automatización mediante integración continua y mediciones iniciales de rendimiento. El presente proyecto toma esa base como antecedente y la extiende hacia una nueva versión del sistema, focalizada en DDM (Digital DHC for MEKO) y en su integración con el ecosistema AR-TDC.

DDM surge como una evolución destinada a reemplazar progresivamente al Data Handling Cabinet (DHC) heredado, asumiendo responsabilidades de procesamiento, representación y control que anteriormente dependían de hardware específico. Esta transición incrementa la importancia de contar con mecanismos sistemáticos de testing, ya que el sistema opera en un dominio crítico donde errores de comunicación, interpretación de datos o actualización de estado pueden impactar directamente en la operación táctica.

El objetivo general del proyecto consiste en diseñar e implementar un módulo de testing que permita verificar y validar el correcto cálculo, procesamiento, recepción y emisión de datos entre componentes del sistema. Para ello se trabajará sobre una aplicación desarrollada en C++ con el framework Qt, aprovechando sus herramientas de prueba, su modelo de señales y slots, y su integración natural con los componentes ya existentes del proyecto.

En una primera etapa, el trabajo se concentrará en la comunicación entre AR-TDC y DDM, particularmente en la recepción, transmisión y decodificación de mensajes tácticos. El foco inicial estará puesto en los mensajes DCL CONC, dado que constituyen el mecanismo mediante el cual AR-TDC informa a DDM el estado operativo de la consola: botones presionados, entradas de handwheel, movimientos de rolling ball y teclas de la MIK. Estos mensajes poseen una estructura binaria específica, compuesta por nueve palabras de 24 bits, por lo que su correcta interpretación requiere validar tanto el formato de la trama como la lógica de decodificación.

El componente principal a validar en esta etapa es `ConcDecoder`, encargado de transformar el contenido binario recibido en eventos internos del sistema DDM. Su salida no se limita a devolver valores simples, sino que dispara cambios de estado mediante señales asociadas a rango, overlay, QEK, rolling ball, cursores propios y otras acciones de operación. Por este motivo, el testing debe comprobar que, ante determinadas tramas de entrada, el sistema produzca las señales y modificaciones esperadas.

Además de `ConcDecoder`, se analizará el comportamiento del flujo completo de comunicación: recepción de datagramas, clasificación de mensajes por parte de `MessageRouter`, procesamiento en `DclConcController`, envío de ACK, aplicación de lógica negativa sobre el payload y posterior decodificación. Este recorrido es relevante porque el sistema actual diferencia mensajes JSON de mensajes binarios mediante un criterio simple; en consecuencia, uno de los riesgos identificados es asumir que todo mensaje binario recibido corresponde necesariamente a DCL CONC. El testing deberá contemplar esta situación y evaluar la robustez del sistema frente a mensajes inválidos, corruptos, incompletos o pertenecientes a otros tipos del protocolo.

En etapas posteriores, el framework de testing podrá extenderse hacia funcionalidades propias de DDM, como herramientas tácticas, manejo de tracks, cursores, overlays, alarmas, figuras, sectores, polígonos y SITREP. También se prevé la integración del proceso de pruebas con mecanismos de automatización, de modo que los tests puedan ejecutarse de forma repetible mediante flujos de integración continua, generando evidencia objetiva sobre el estado del sistema.

La metodología de trabajo será incremental. Inicialmente se estudiará la documentación disponible, el código fuente y los antecedentes del proyecto. Luego se definirán los componentes críticos a validar, se implementarán pruebas unitarias y de integración acotadas, y se avanzará gradualmente hacia escenarios de mayor cobertura funcional. Este enfoque permite acompañar el desarrollo del sistema DDM, que se encuentra en evolución, sin depender de una versión completamente cerrada para comenzar a construir una base de verificación y validación.

En síntesis, el proyecto busca aportar una estructura de testing que mejore la confiabilidad, robustez y mantenibilidad del sistema DDM, reduciendo la probabilidad de errores en el procesamiento de información crítica y facilitando futuras etapas de validación formal u homologación.

# 2. Descripción del sistema

El sistema a analizar forma parte del ecosistema AR-TDC/DDM, orientado a la modernización de consolas tácticas navales. Su finalidad es procesar información proveniente de sensores, entradas del operador y fuentes sintéticas para asistir la visualización, interpretación y operación del entorno táctico de una unidad naval.

La descripción del sistema resulta necesaria para comprender posteriormente qué se va a testear y por qué determinados componentes son considerados críticos. En particular, el proyecto no se limita a probar funciones aisladas, sino que aborda un flujo de información que comienza con eventos de operación, atraviesa mensajes binarios o JSON, modifica el estado interno del backend y produce efectos observables en herramientas tácticas.

## 2.1 Contexto operacional

En los sistemas tácticos navales, el operador necesita visualizar información del entorno, controlar modos de presentación, ingresar comandos, seleccionar herramientas y recibir información actualizada de contactos o blancos. Tradicionalmente, estas funciones eran soportadas por consolas físicas y por un gabinete central de procesamiento de datos. DDM propone digitalizar y reemplazar progresivamente parte de ese comportamiento, manteniendo compatibilidad con los flujos de información existentes.

El sistema procesa datos de radar y fuentes sintéticas, permitiendo representar movimientos relativos de embarcaciones en tiempo real. Sobre esa información se desarrollan herramientas de apoyo a la decisión, como cálculos de navegación, posicionamiento de blancos, trayectorias, gestión de contactos y representaciones geométricas sobre la pantalla táctica.

Debido a que esta información participa en la toma de decisiones operativas, el correcto funcionamiento del sistema requiere verificar no solo resultados numéricos, sino también comunicación, sincronización, consistencia de estado y respuesta ante entradas no esperadas.

## 2.2 Ecosistema AR-TDC y DDM

El ecosistema se organiza conceptualmente en capas. La capa de presentación corresponde a la consola táctica, donde el operador visualiza información y genera entradas mediante botones, teclado, handwheel o rolling ball. Esta capa no concentra la lógica de negocio principal, sino que captura eventos y presenta resultados.

AR-TDC actúa como capa de integración y orquestación. Su función es coordinar el tráfico de información entre la consola, los módulos de visualización y los componentes de procesamiento. En este marco, AR-TDC se comunica con DDM para transmitir eventos de control y recibir respuestas del sistema.

DDM constituye el backend operativo que reemplaza progresivamente al DHC heredado. Su responsabilidad es asumir tareas de procesamiento del estado global, cálculos asociados a navegación y contactos, gestión de información táctica y soporte a funcionalidades de operación. En escenarios de respaldo o emergencia, DDM cobra especial relevancia porque permite sostener la visualización y el control táctico aun cuando se modifica el backend principal.

## 2.3 Comunicación entre componentes

La comunicación entre AR-TDC y DDM se realiza mediante sockets UDP. El sistema intercambia mensajes binarios específicos para sincronización y operación, además de mensajes JSON utilizados para comandos de más alto nivel dentro del backend.

Entre los mensajes identificados en el protocolo se encuentran:

- Pedido DCL CONC.
- DCL CONC.
- AND1.
- AND2.
- ACK.
- LPD.

El mensaje DCL CONC es central para esta etapa del proyecto. Se trata de un mensaje binario que representa el estado instantáneo de la consola táctica. No transporta información gráfica ni táctica en el sentido de representar directamente blancos o figuras, sino que comunica acciones e interfaces de entrada del operador. Por ejemplo, puede reflejar el estado de botones, entradas de handwheel, movimientos de rolling ball y teclas de la MIK.

Según la documentación analizada, el mensaje DCL CONC está compuesto por nueve palabras de 24 bits, es decir, 216 bits en total. Esta estructura exige una decodificación precisa, dado que errores en el desplazamiento de bits, la interpretación de campos o la aplicación de lógica negativa pueden derivar en eventos incorrectos dentro de DDM.

## 2.4 Arquitectura interna del backend DDM

El backend del proyecto está implementado en C++ utilizando Qt. Su arquitectura separa responsabilidades de transporte, enrutamiento, procesamiento de comandos, decodificación binaria y actualización del estado de dominio.

La capa de comunicación está representada por la interfaz `ITransport`, con implementaciones para UDP o IPC local. Esta capa emite mensajes recibidos hacia el resto del sistema y permite enviar respuestas o mensajes periódicos hacia otros componentes.

El componente `MessageRouter` recibe los datagramas entrantes y decide su destino. Actualmente, si el mensaje comienza con `{` y termina con `}`, se interpreta como JSON y se deriva a `JsonCommandHandler`; en caso contrario, se considera un mensaje binario y se envía a `DclConcController`. Esta decisión simplifica el flujo de procesamiento, aunque también introduce un punto relevante para el testing: no todo mensaje binario válido o inválido necesariamente corresponde a DCL CONC.

`JsonCommandHandler` procesa comandos JSON provenientes de otros componentes del sistema. Para ello valida la estructura del mensaje, enruta la acción correspondiente y genera respuestas. Entre las operaciones actuales se encuentran comandos asociados a líneas, cursores y otras acciones del contexto del sistema.

`DclConcController` procesa los mensajes binarios asociados al concentrador. Al recibir un datagrama, extrae información inicial de control, obtiene el número de secuencia, construye y envía el ACK correspondiente, toma el payload, aplica la inversión de bits requerida por el protocolo y delega la interpretación del contenido en `ConcDecoder`.

`ConcDecoder` es el componente encargado de interpretar el payload del mensaje DCL CONC. A partir de los bits recibidos, emite señales Qt que notifican cambios de estado o acciones del operador, tales como cambio de rango, selección de overlay, activación de QEK, desplazamiento de rolling ball o acciones sobre el cursor propio. Estas señales son consumidas por otros módulos, como `OverlayHandler`, `OBMHandler` y `OwnCurs`, que actualizan el estado operacional interno.

El estado compartido del sistema se concentra en `CommandContext`, que contiene información como tracks, cursores, centro de pantalla e identificadores. Sobre este contexto actúan comandos provenientes de consola, mensajes JSON y eventos derivados de la decodificación del concentrador.

## 2.5 Funcionalidades tácticas de DDM

DDM organiza sus herramientas de acuerdo con distintos ambientes operacionales. Esta organización responde a criterios tácticos y facilita que el operador acceda a funciones relacionadas con el dominio de operación correspondiente.

Dentro del ambiente de superficie, el sistema procesa información radar y sintética para analizar movimientos relativos en tiempo real. Algunas de las funcionalidades identificadas son:

- Análisis cinemático de contactos o tracks.
- Cálculo de navegación y posicionamiento de blancos.
- Representación de trayectorias.
- Visualización de figuras y elementos geométricos.
- Gestión del estado operativo de contactos.
- Herramientas asociadas a cursores, overlays, sectores, polígonos y SITREP.

Una herramienta relevante dentro de este ambiente es el Punto de Próximo Pasaje, también identificado como PPP o CPA. Su objetivo es anticipar situaciones de aproximación o posible colisión mediante el cálculo de la distancia mínima entre trayectorias y el tiempo restante para alcanzar dicho punto. Este tipo de funcionalidad muestra por qué el sistema requiere pruebas confiables: un error de cálculo, interpretación o actualización podría afectar directamente la lectura operacional del entorno.

## 2.6 Componentes relevantes para el testing inicial

Para la primera etapa del proyecto, los componentes más relevantes son aquellos vinculados con la comunicación AR-TDC/DDM y la decodificación de mensajes DCL CONC:

- `ITransport`, porque representa la entrada y salida de mensajes del sistema.
- `MessageRouter`, porque decide si un mensaje se procesa como JSON o como binario.
- `DclConcController`, porque gestiona recepción de datagramas binarios, secuencias, ACK e inversión del payload.
- `ConcDecoder`, porque transforma la trama DCL CONC en eventos internos del sistema.
- `OverlayHandler`, `OBMHandler` y `OwnCurs`, porque consumen señales del decoder y actualizan estado operacional.
- `CommandContext`, porque conserva el estado sobre el cual impactan comandos y eventos.

El flujo principal a validar puede describirse de la siguiente manera:

1. AR-TDC genera un evento de control, por ejemplo una acción de botonera o movimiento de rolling ball.
2. El evento se codifica en un mensaje DCL CONC.
3. El mensaje se transmite por UDP hacia DDM.
4. DDM recibe el datagrama mediante la capa de transporte.
5. `MessageRouter` deriva el mensaje binario a `DclConcController`.
6. `DclConcController` extrae la secuencia, responde con ACK, invierte el payload y lo envía a `ConcDecoder`.
7. `ConcDecoder` interpreta los bits del mensaje y emite señales internas.
8. Los handlers correspondientes actualizan el estado operativo del sistema.

Este flujo permite identificar los puntos de observación para las pruebas: formato del datagrama, número de secuencia, generación de ACK, longitud del mensaje, inversión de bits, emisión de señales esperadas y ausencia de efectos ante entradas inválidas. A partir de esta descripción se podrá definir, en las secciones siguientes, el alcance del testing, la planificación de pruebas y los criterios de ejecución y resultado.
