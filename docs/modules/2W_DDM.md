## Cosas a definir:

1. Terminar de definir el formato final para esta herramienta y el resto.  
   1. formato requerimientos?  
   2. Yo mejoraría el formato que ya se usa en SRS-DDM para explicar PPP.  
2. Estaría bueno explicar también en que parte del SRS-DDM iría ubicada la especificación de la herramienta  
3. ¿Cómo incluimos la Tabla A de estaciones en el documento?

# 2W

## 1\. Descripción y Finalidad

La Disposición 2W es una configuración de unidades de superficie orientada a la vigilancia y defensa ante múltiples amenazas.  
Su diseño ubica a una o varias unidades capitales en el centro de la formación, organizando a los buques escolta en sectores circulares (formato hexagonal) para establecer una pantalla defensiva sin "puntos ciegos".  
La finalidad principal de la herramienta es representar gráficamente estas estaciones circulares en la pantalla radar, brindando una rápida apreciación del cuadro táctico para el Comando. Además, proporciona un modelo de asesoramiento para calcular el rumbo y tiempo necesarios para que el buque propio ocupe su estación designada.

## 

## 2\. Modelo de Interacción (Inputs y Outputs)

La interfaz de la funcionalidad se divide en dos áreas principales de operación: la configuración de la disposición y la selección gráfica de estaciones

### 2.1 Entradas de Datos (Inputs):

* #### Radio Círculo: Define el diámetro de cada círculo hexagonal. Por defecto se establece en 1.0 Milla Náutica (MN), pero es modificable por el operador.

* #### Guía: Designación del track de superficie que actuará como centro de la disposición.

* #### Estación BP: Número de estación (del 1 al 68\) asignada al Buque Propio

* #### Grilla de Estaciones: Un panel lateral interactivo que permite seleccionar cualquier otra estación adicional para graficarla en pantalla.

### 2.2 Asesoramiento y Visualización (Outputs):

* #### Azimut al Guía: Se muestran dos cuadros comparativos. El primero indica la marcación verdadera actual; el segundo indica la marcación esperada según la Tabla A.

* #### Distancia al Guía: Dos cuadros comparativos que muestran la distancia actual versus la esperada por Tabla A.

* #### Rumbo a Estación: Asesoramiento del rumbo calculado para llegar al centro del círculo de la estación asignada.

* #### Tiempo a Estación: Tiempo estimado para completar la maniobra hacia la estación.

## 

## 3\. Requerimientos Funcionales

Para garantizar su correcta programación y consistencia con la consola AR-TDC original, la herramienta debe cumplir con los siguientes requerimientos:

| Cálculo y Lógica (CAL) |  |
| ----- | :---- |
| **.Asesoramiento \[REQ-2W-CAL-001\]** | **Acción** El sistema debe utilizar de manera excluyente los	valores estáticos de Azimut Verdadero y Distancia provistos por la Tabla A (estaciones 1 a 68\) para calcular las marcaciones esperadas, el Rumbo a Estación y el Tiempo a Estación.  |
| **.CentroPuntoLlegada \[REQ-2W-CAL-002\]** | **Acción** El sistema deberá establecer matemáticamente el centro del círculo de la estación ordenada como el punto de llegada exacto para los cálculos de asesoramiento, independientemente de que la unidad se considere estacionada al ingresar al radio estipulado. |
| **.IndependenciaRumbo \[REQ-2W-CAL-003\]** | **Acción** El sistema deberá establecer la ubicación de cada estación basándose estrictamente en su azimut verdadero, indistintamente del rumbo actual del buque guía y sus alteraciones.  |
| **.EjecuciónAsesoramiento \[REQ-2W-CAL-004\]** | **Acción** El sistema deberá ejecutar el cálculo de estacionamiento del buque propio (Rumbo y Tiempo) hacia el centro de su estación únicamente tras la activación del comando INICIAR  |
| **Pantalla Radar (LPD)** |  |
| **.VisibilidadMandatoria \[REQ-2W-LPD-001\]** | **Acción** El sistema deberá graficar de forma permanente y obligatoria el círculo correspondiente a la estación del buque guía y el círculo de la estación asignada al buque propio.  |
| **.VisibilidadOpcional \[REQ-2W-LPD-002\]** | **Acción** El sistema deberá permitir graficar los círculos del resto de las estaciones de la fuerza únicamente si estas son seleccionadas previamente por el usuario.  |
|  **.ParametrizacionDimension \[REQ-2W-LPD-003\]** |  **Acción** El sistema deberá renderizar los círculos y hexágonos con un diámetro por defecto de 1 milla náutica, permitiendo su reajuste dinámico según el valor ingresado por el operador.  |
| **.IdentificadorNumerico \[REQ-2W-LPD-004\]** | **Acción** El sistema deberá superponer de forma visible el número de identificación de la estación en el centro de cada círculo graficado en la pantalla radar.  |
| **.ColorGuia \[REQ-2W-LPD-005\]** | **Acción** El sistema deberá representar el círculo de la estación del buque guía aplicando el color verde (HEX \#00FF00). |
| **.ColorPropio \[REQ-2W-LPD-006\]** | **Acción** El sistema deberá representar el círculo de la estación del buque propio aplicando el color azul (HEX \#0000FF). |
| **.ColoresEstacionOp \[REQ-2W-LPD-007\]** | **Acción** El sistema deberá permitir graficar el resto de las estaciones de la fuerza en color naranja (HEX \#FFCC00) cuando el usuario seleccione la opción. |
| **.ContrasteRadar \[REQ-2W-LPD-008\]** | **Acción** El sistema deberá aplicar a todos los círculos graficados un relleno con opacidad del 80% para garantizar la legibilidad de la fuente blanca de los identificadores numéricos sobre el crudo radar.  |
| **Interfaz de Usuario (UI)** |  |
| **.GestionEspacioPantalla \[REQ-2W-UI-001\]** | **Acción** El sistema deberá maximizar el uso de la pantalla ubicando el panel del módulo 2W en el espacio correspondiente a las tablas de información de Track Radar y AIS.  |
| **.CapaZIndex \[REQ-2W-UI-002\]** | **Acción** El sistema deberá ocultar las tablas de Track Radar y AIS enviándolas a una capa visual inferior (o saltando de pantalla) mientras el módulo 2W se encuentre activo.  |
|  **.DistribuciónPaneles \[REQ-2W-UI-003\]** |  **Acción** El sistema deberá estructurar el panel del módulo en dos espacios de información separados: un área para el ingreso de Datos (Radio, Guía, Estación BP) y un área para los cuadros de Asesoramiento.  |
| **.ContrasteCrudoRadar \[REQ-2W-UI-004\]** | **Acción** El sistema deberá modificar el color nativo del fondo del crudo radar al color estipulado por paleta (EXAAA5F32) para evitar conflictos visuales con las fuentes de los menús contextuales y modales. |
| **Controles e Interacción (INT)** |  |
| **.GrillaSeleccion \[REQ-2W-INT-001\]** | **Acción** El sistema deberá proveer una representación gráfica de la disposición (del 1 al 68\) que actúe como botonera, permitiendo al usuario seleccionar múltiples estaciones haciendo clic sobre ellas.  |
| **.InteracciónDefault \[REQ-2W-INT-002\]** | **Acción** El sistema deberá aplicar a los botones de la grilla de selección en su estado inactivo o por defecto un borde de color EXAAA5F32 y un relleno transparente (0% de opacidad). |
| **.InteracciónMouseover \[REQ-2W-INT-003\]** | **Acción** El sistema deberá aplicar a los botones de la grilla de selección un relleno de color EXAC8B75 en el instante en que el usuario posicione el cursor sobre ellos (estado Mouseover). |
| **.InteracciónOnclick \[REQ-2W-INT-004\]** | **Acción** El sistema deberá aplicar a los botones de la grilla de selección un relleno de color EXAAA5F32 al ser presionados o seleccionados por el usuario (estado Onclick). |
| **.NavegacionTabla \[REQ-2W-INT-005\]** | **Acción** El sistema deberá presentar la tabla de estaciones (del N°1 al 68\) en formato carrusel con desplazamiento vertical por arrastre, prohibiendo el uso de barras de desplazamiento tradicionales (scrollbars) para mejorar la usabilidad. |
| **.JerarquiaTexto \[REQ-2W-INT-006\]** | **Acción** El sistema debe establecer la regla de UX de mantener estrictamente en color negro (color-black) la tipografía del segundo cuadro de la sección Asesoramiento para no generar distracciones al operador. |
|  **.DisparadorGrafico \[REQ-2W-INT-007\]** |  **Acción** El sistema deberá plasmar gráficamente en la pantalla radar todas las estaciones seleccionadas únicamente en el instante en que el usuario presione el botón "INICIAR". |
|  **.LimpiezaPantalla \[REQ-2W-INT-008\]** |  **Acción** El sistema deberá borrar de la pantalla radar todas las representaciones gráficas de las estaciones y limpiar el panel de asesoramiento al presionar el botón "FINALIZAR". |

	  
	

	.**ContrasteRadar**		El sistema deberá aplicar un relleno con opacidad del  
80%  a los círculos representados en la LPD para garantizar el contraste de los números de estación (en color blanco) sobre el crudo radar.

	**Nota:** Recomendamos la visualización del documento [RFC 2W](https://docs.google.com/document/u/0/d/1OjgZIGMQ17rtuvWRxC4iswPYXTmrnh-XIGGrjCA4kkA/edit) para poder contrastar los requerimientos con los gráficos provistos.

## 

## 4\. Casos de Uso

### 4.1 Escenario de despliegue táctico

1. El Comando de la fuerza ordena formar una disposición 2W. Se designa como Guía al track S0010 y se establece el radio de los círculos en 1 MN.  
2. El Buque Propio (BP) recibe la orden de ocupar la estación 12\. Según la Tabla A, el sistema debe tomar como punto de destino las coordenadas relativas correspondientes a un azimut de 270° y una distancia de 4.0 MN desde el guía.  
3. Adicionalmente, se informa que otras unidades aliadas ocuparán las estaciones 8, 9, 13, 16 y 17\. El operador selecciona estas estaciones en la grilla y presiona "Graficar".

**Resultado esperado**: La Pantalla Radar (LPD) deberá mostrar al S0010 rodeado por un círculo verde, al buque propio en su aproximación hacia un círculo azul (ubicado al Oeste a 4 MN) y los círculos correspondientes a las estaciones aliadas (8, 9, 13, 16 y 17\) graficados simultáneamente en color ámbar. Los cuadros de asesoramiento de la interfaz deberán estar devolviendo el Rumbo y el Tiempo requeridos para que el BP alcance el centro del círculo de la estación 12\.