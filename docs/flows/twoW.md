# Flujo: Formación 2W

## Descripción general

Este flujo documenta la gestión y el cálculo cinemático continuo de la **Disposición de Formación Táctica 2W** para el Buque Propio (`ownShip`) y fuerzas aliadas respecto a un buque designado como Guía (`guideTrack`). 

A partir de una estación asignada (basada en los casilleros del 1 al 68 de la Tabla A) y un factor de escala por radio, el sistema proyecta la posición teórica que debería ocupar el buque, evalúa la discrepancia táctica en tiempo real (desvío en azimut y distancia) y calcula el rumbo recomendado de intercepción directa junto con su tiempo estimado de arribo (ETA) en minutos.

---

## Lista de archivos y clases

| Archivo | Clase/Struct | Responsabilidad |
|---|---|---|
| `src/controller/commands/TwoWCommand.cpp` | `TwoWCommand` | Entrada CLI para iniciar (`--guia`), detener (`--stop`) y consultar (`--info`) la formación. |
| `src/controller/services/TwoWService.cpp` | `TwoWService` | Orquestador del ciclo de vida de la sesión (start/stop) e integrador con el bucle de actualización. |
| `src/model/2w/twoWCalculator.cpp` | `TwoWCalculator` | Motor matemático puro encargado de proyectar las estaciones y resolver la cinemática de intercepción. |
| `src/model/2w/twoWSessionState.h` | `TwoWSessionState` | Estructura de datos que persiste el estado dinámico y los resultados analizados de la formación. |
| `src/model/2w/twoWStationTable.h` | `TwoWStationTable`, `TwoWStationEntry` | Matriz estática indexada que encapsula la geometría base de las 68 estaciones de la Tabla A. |
| `src/model/commandContext.h` | `CommandContext` | Contenedor global que aloja la sesión activa `twoWSession` dentro del pipeline del sistema. |

---

## Clases principales

### TwoWCommand

- **Rol**: Wrapper CLI encargado del análisis sintáctico (parsing) de tokens y de la validación de las reglas de negocio de entrada.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Ejecutar | `CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const` | Modula entre los modos operativos (`--info`, `--stop`, o inicialización de sesión). |

- **Reglas de Validación CLI**:
  - `--bp` debe ser un entero válido comprendido estrictamente en el rango $[1, 68]$.
  - `--radio` (opcional, por defecto `1.0`) debe ser un valor de punto flotante estrictamente $> 0.0$.

### TwoWService

- **Rol**: Interfaz de control operativo que manipula el estado de la sesión táctica y actúa como puente hacia el calculador.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Iniciar Sesión | `void startSession(int guideTrackId, int bpStation, double circleRadiusNm)` | Modifica el contexto fijando el buque guía, la estación del Buque Propio y el radio de escala. |
| Finalizar Sesión | `void stopSession()` | Llama al método `reset()` de la estructura de datos para apagar el procesamiento dinámico. |
| Actualización | `void update()` | Extrae las posiciones cinemáticas actuales de los vectores globales e invoca el recálculo geométrico. |

### TwoWCalculator

- **Rol**: Motor de cómputo plano que realiza las transformaciones de coordenadas y proyecciones trigonométricas relativas al Guía.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Calcular | `static void calculate(...)` | Proyecta el centro de la estación asignada al Buque Propio y aliados, calculando desvíos, rumbo directo y ETA. |

- **Consideraciones técnicas del motor**:
  - **Conversión de unidades**: Mapea distancias utilizando el factor constante $1\text{ NM} = 185.2\text{ Decámetros (DM)}$ para unificar criterios con el plano cartesiano del radar.

---

## Flujo de datos (Ciclo de Vida del Comando)

```mermaid
flowchart TD
    A([Inicio: Comando 2w recibido]) --> B{"¿Flag detectado?"}
    
    B -->|"--guia=ID --bp=N"| C["Validar rangos: BP 1..68 y Radio > 0"]
    C --> D{"¿Datos válidos?"}
    D -->|No| E[Retornar Error CLI] --> FE1([Fin con error])
    D -->|Sí| F[TwoWService::startSession]
    F --> G["Persistir variables en ctx->twoWSession"] --> Z1([Sesión Iniciada])

    B -->|"--info"| H{"¿Sesión activa?"}
    H -->|No| I[Error: Disposición no activa] --> FE2([Fin con error])
    H -->|Sí| J[Construir reporte con datos de telemetría actualizados] --> ZF([Mostrar en Consola])

    B -->|"--stop"| K[TwoWService::stopSession]
    K --> L[Llamar reset en dosWSession] --> Z2([Disposición Finalizada])

    classDef error fill:#ffcccc,stroke:#cc0000,color:#800000
    classDef ok fill:#ccffcc,stroke:#007700,color:#004400
    class E,I,FE1,FE2 error
    class Z1,Z2,ZF ok
```
  ### Flujo CLI (Paso a Paso)

El usuario ejecuta el comando en la consola utilizando la sintaxis de inicialización:

```bash
2w --guia=<trackId> --bp=<estacion> [--radio=<mn>]
```

1. `TwoWCommand::execute` intercepta los argumentos y los procesa en un mapa local de opciones (`QMap<QString, QString> opts`), normalizando las claves a minúsculas y removiendo los guiones iniciales (`--`).
2. El comando valida las reglas de negocio críticas antes de interactuar con el servicio:
   - Verifica la existencia obligatoria de los parámetros `guia` y `bp`.
   - Comprueba que los valores ingresados puedan convertirse a números enteros.
   - Valida los límites de la Tabla A: la estación (`bp`) debe estar comprendida estrictamente entre `1` y `68`.
   - Si se incluye el flag `--radio`, valida que sea un número flotante estrictamente positivo (`> 0.0`). Si se omite, se establece de forma predeterminada en `1.0`.
3. Si las validaciones fallan, el comando interrumpe la ejecución de inmediato y retorna un `CommandResult` con estado de éxito en `false` junto con la ayuda de uso (`usage()`).
4. Si los parámetros son correctos, se instancia `TwoWService` pasando el puntero del contexto actual y se invoca `service.startSession(guiaId, bpEst, radio)`.
5. El servicio modifica el estado global dentro de `CommandContext::twoWSession`:
   - Setea el flag `active` en `true`.
   - Registra los IDs de configuración táctica y limpia los contenedores dinámicos.
6. Se imprime en la consola (`ctx->out`) el mensaje operativo confirmando el inicio de la disposición táctica y el comando retorna un resultado exitoso.

---

### Ciclo de Recálculo Continuo (Bucle Activo)

A diferencia de otros comandos puntuales, la formación se evalúa de manera continua en el bucle principal de la aplicación (`main.cpp`) mediante un temporizador asincrónico:

```mermaid
flowchart TD
    T1([updatePositionTimer cada 80ms]) --> T2["TwoWService::update()"]
    T2 --> T3{"¿Existe el track guía?"}

    T3 -->|No| T4[Llamar stopSession de emergencia] --> FT1([Fin sesión por pérdida])
    T3 -->|Sí| T5[Obtener posiciones de Guía y OwnShip]

    T5 --> T6["TwoWCalculator::calculate()"]
    T6 --> T7[Buscar índices en TwoWStationTable]
    T7 --> T8[Proyectar coordenadas cartesianas out_ownCenter]
    T8 --> T9[Calcular rumbo recomendado, desvíos y ETA]
    T9 --> T10{"¿ownSpeed > 0 y distEstacion > 0?"}

    T10 -->|Sí| T11["Calcular ETA en minutos: (dist/speed) * 60"]
    T10 -->|No| T12[Fijar kinematicsValid = false y ETA = 0.0]

    T11 --> T13["Actualizar ctx->twoWSession"]
    T12 --> T13
    T13 --> FT2([Métricas listas])

    classDef error fill:#ffcccc,stroke:#cc0000,color:#800000
    class T4,FT1 error
```

---

## Estructuras de Datos Clave

### Estructura de la Tabla A (`TwoWStationEntry`)

La matriz estática posee la siguiente composición geométrica rígida para modelar las estaciones:

- **`azimuthDeg`**: Ángulo verdadero en grados `[0, 360)` tomado desde el Guía (origen de la cuadrícula).
- **`distanceNm`**: Distancia radial calculada con un radio base equivalente a `1 NM`.

### Estado de Sesión (`TwoWSessionState`)

Mantiene la separación limpia entre parámetros de configuración y métricas dinámicas listas para el frontend:

- **Datos de control**: `active`, `guideTrackId`, `bpStation`, `circleRadiusNm`, `selectedStations`.
- **Centros cartesianos**: `guideCircleCenter` (Verde), `ownCircleCenter` (Azul), `allyCircleCenters` (Ámbar).
- **Telemetría calculada**: `courseToStationDeg`, `timeToStationMin`, `currentAzimuthDeg`, `currentDistanceNm`, `expectedAzimuthDeg`, `expectedDistanceNm`.

---

## Manejo de Errores y Casos de Borde

- **Validación de rangos numéricos**: Rechazo de ejecuciones CLI con estaciones inexistentes en la matriz de la Tabla A (posiciones marcadas como `0.0, 0.0` o índices fuera del intervalo `[1, 68]`).
- **Tratamiento de indeterminaciones cinemáticas**: Si la velocidad del Buque Propio es exactamente `0.0`, el sistema deshabilita de manera segura el indicador `kinematicsValid` para saltarse el cálculo del ETA y evitar la división por cero en la ecuación temporal.
- **Pérdida crítica de referencia**: Si el buque designado como Guía es eliminado de la lista global de vectores tácticos (`findTrackById(s.guideTrackId)` devuelve `nullptr`), el servicio intercepta la anomalía en el siguiente ciclo de `80 ms` y fuerza un `stopSession()` inmediato para proteger la integridad del sistema.

---

## Módulos Relacionados

- `src/model/commandContext.h` — Estructura general de ejecución.
- `docs/modules/estacionamiento.md` — Pipeline homólogo de maniobras relativas.