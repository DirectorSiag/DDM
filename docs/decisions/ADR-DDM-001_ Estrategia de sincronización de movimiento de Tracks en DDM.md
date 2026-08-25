# **ADR-001: Estrategia de sincronización de movimiento de Tracks en DDM**

---

## **Estado**

Pendiente

## **Fecha**

2026-08-25

## **Contexto**

El sistema está compuesto por múltiples terminales DDM que comparten una misma realidad de tracks en movimiento. Cada nodo necesita ver la posición de todas las entidades de forma consistente, incluyendo las que no controla directamente.

Se evaluaron dos enfoques extremos:

1. **Sincronización por posición absoluta**: cada nodo manda la nueva ubicación de la entidad en cada actualización, y los demás nodos simplemente la reflejan.

   * Ventaja: siempre consistente, sin desvíos.  
   * Desventaja: alto consumo de ancho de banda (requiere enviar datos con mucha frecuencia para que el movimiento se vea fluido), y el movimiento se ve "a saltos" si hay latencia o pérdida de paquetes.  
   *   
2. **Sincronización por parámetros de movimiento (dirección y velocidad)**: cada nodo calcula la posición localmente a partir de la última dirección/velocidad conocida.

   * Ventaja: tráfico de red mínimo (solo se manda información cuando cambia el estado de movimiento), animación fluida en cada terminal.  
   * Desventaja: riesgo de desincronización acumulativa entre nodos por errores de redondeo, jitter de red, retrasos de procesamiento, o pérdida de mensajes.

Ninguno de los dos extremos es adecuado por sí solo: el primero es demasiado costoso en ancho de banda, el segundo puede divergir sin límite si no hay un mecanismo de corrección.

## **Decisión**

Se adopta un esquema **híbrido de extrapolación local con corrección periódica**, basado en la técnica de *dead reckoning* usada en sistemas distribuidos de simulación (DIS/HLA) y juegos multijugador en tiempo real.

### **Principios de la solución**

**1\. Cada nodo extrapola el movimiento localmente** Cada entidad se modela en todos los nodos con el mismo estado mínimo: {posición, dirección, velocidad}. Entre actualizaciones de red, cada nodo avanza la posición localmente con un cálculo simple (posición \+= velocidad \* dirección \* Δt) en cada frame/tick. Esto mantiene el movimiento fluido sin depender de la red.

**2\. El nodo dueño de la entidad emite actualizaciones de estado** El nodo que controla una entidad (el "dueño") es la única fuente de verdad sobre su movimiento. Este nodo envía snapshots de estado a los demás:

* De forma periódica, a un intervalo fijo configurable (por ejemplo, cada 100–200 ms), y/o  
* De forma reactiva, cuando el valor real de posición diverge de lo que el modelo de extrapolación predeciría más allá de un umbral tolerado.

Esto evita mandar la posición en cada frame, pero garantiza una resincronización regular antes de que el desvío sea perceptible.

**3\. El mensaje de sincronización lleva estado completo y timestamp** Cada actualización de red incluye:

{  
  entidad\_id,  
  posición,  
  dirección,  
  velocidad,  
  timestamp  
}

El timestamp permite a cada nodo receptor compensar el retraso de red: al recibir el mensaje, extrapola desde la posición reportada hasta el instante actual usando el tiempo transcurrido, en vez de asumir que la posición llegó "en tiempo real".

**4\. Corrección suave, no salto brusco** Si al llegar una actualización la posición calculada localmente difiere de la reportada por el dueño, el nodo receptor no reemplaza la posición de golpe (evita el efecto de "teletransporte" visual). En cambio, interpola gradualmente hacia la posición correcta a lo largo de los siguientes frames, mientras retoma la extrapolación local desde ese punto corregido.

### **Resumen del flujo**

Nodo dueño de la entidad:  
  \- Mueve la entidad según input/lógica propia  
  \- Cada Δt\_sync o si hay desvío \> umbral:  
        emite {posición, dirección, velocidad, timestamp}

Todos los nodos (incluido el dueño):  
  \- En cada frame local:  
        posición\_local \+= velocidad \* dirección \* Δt\_frame  
  \- Al recibir snapshot de red:  
        posición\_esperada \= posición\_recibida \+ velocidad \* (ahora \- timestamp)  
        si |posición\_local \- posición\_esperada| \> umbral:  
            interpolar suavemente hacia posición\_esperada  
        retomar extrapolación desde posición corregida

## **Alternativas consideradas**

| Alternativa | Motivo de descarte |
| ----- | ----- |
| Enviar posición absoluta en cada tick | Ancho de banda excesivo; no escala con muchas entidades o nodos |
| Solo dirección/velocidad, sin corrección | Riesgo de desincronización sin límite acotado a lo largo del tiempo |
| Lockstep (todos los nodos avanzan sincronizados, esperando confirmación mutua) | Introduce latencia perceptible por el usuario; frágil ante nodos lentos o caídos |

## **Consecuencias**

**Positivas**

* Tráfico de red bajo y predecible, independiente de la tasa de refresco visual.  
* Movimiento fluido en cada terminal, sin depender de la latencia de red para renderizar cada frame.  
* Corrección acotada: el desvío entre nodos nunca crece de forma ilimitada, se resincroniza en cada intervalo o al superar el umbral.

**Negativas / riesgos a mitigar**

* Requiere reloj razonablemente sincronizado entre nodos (o al menos un delta de tiempo confiable) para que el cálculo con timestamp sea correcto.  
* Hay que definir bien el umbral de corrección y la velocidad de interpolación: muy chico genera correcciones constantes (tráfico similar al enfoque de posición absoluta); muy grande permite desvíos visibles antes de corregir.  
* Si se pierde un mensaje de sincronización, el nodo sigue extrapolando con datos viejos hasta el próximo snapshot — hay que decidir un timeout o comportamiento de fallback (por ejemplo, detener la entidad o marcarla como "desconocida") si pasa demasiado tiempo sin novedades del dueño.  
* Necesita definir claramente quién es el "dueño" de cada entidad para evitar que dos nodos intenten corregir la misma entidad con autoridad simultánea.

## **Puntos abiertos para próximas decisiones**

* Valor concreto del intervalo de sincronización periódica (Δt\_sync) y del umbral de desvío.  
* Mecanismo de detección de pérdida de nodo dueño y reasignación de autoridad sobre una entidad.  
* Protocolo de transporte a usar para los mensajes de sincronización (UDP con manejo de pérdida vs. TCP).

