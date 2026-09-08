# ADR-011: Routing de reconnectTransport() y reinitializeStorage() en CallbackBridge

## Estado
**Cerrado — Opción C: mover reconnectTransport() y reinitializeStorage() a IReplicationEngine**

---

## Contexto

ADR-009 y ADR-010 establecen que DDM puede solicitar recuperación ante fallos de runtime llamando a `IReplicationBridge::reconnectTransport()` y `IReplicationBridge::reinitializeStorage()` respectivamente. Ambos métodos deben traducirse en eventos (`ReconnectTransportEvent`, `ReinitStorageEvent`) que el Worker Thread de ReplicationEngine procesa de forma serializada.

`CallbackBridge` implementa `IReplicationBridge` pero actualmente no tiene ninguna referencia a la `EventQueue` ni a `ReplicationEngine`. Sus implementaciones son stubs vacíos:

```cpp
void CallbackBridge::reconnectTransport() {
    // TODO: encolar ReconnectTransportEvent (ADR-009)
}
void CallbackBridge::reinitializeStorage() {
    // TODO: encolar ReinitStorageEvent (ADR-010)
}
```

Este ADR decide cómo `CallbackBridge` obtiene el canal necesario para encolar esos eventos.

### Restricciones

- `CallbackBridge` no puede incluir headers de OpenDDS ni de Qt (ADR-007, SDD §2.1).
- `ReplicationEngine` es propietario exclusivo (`unique_ptr`) del `CallbackBridge`. No puede haber referencias cíclicas de ownership.
- La `EventQueue` vive como miembro de `ReplicationEngine` con lifetime igual al del motor.
- DDM llama `reconnectTransport()` / `reinitializeStorage()` desde el hilo Qt — la encolación debe ser thread-safe (lo es por diseño de `EventQueue`).
- El mecanismo elegido debe ser testeable sin instanciar `ReplicationEngine` real.

---

## Opciones Consideradas

### Opción A: Inyectar EventQueue& en el constructor de CallbackBridge

`ReplicationEngine` pasa una referencia a su propia `EventQueue` al construir `CallbackBridge`.

```cpp
// CallbackBridge.h
class CallbackBridge : public IReplicationBridge {
public:
    explicit CallbackBridge(EventQueue& queue);
    ...
private:
    EventQueue&           queue_;
    IReplicationListener* listener_{nullptr};
};

// En ReplicationEngine.cpp (punto de construcción)
bridge_ = std::make_unique<CallbackBridge>(queue_);
```

```cpp
void CallbackBridge::reconnectTransport() {
    queue_.push(ReconnectTransportEvent{});
}
void CallbackBridge::reinitializeStorage() {
    queue_.push(ReinitStorageEvent{});
}
```

La lifetime de `queue_` (miembro de `ReplicationEngine`) es siempre mayor que la de `CallbackBridge` (también miembro de `ReplicationEngine`, destruido antes), por lo que la referencia es segura.

**Ventajas:**
- Implementación mínima: un campo adicional, dos líneas de código.
- Semántica clara: `CallbackBridge` encola directamente sin indirección.
- Thread-safe: `EventQueue::push()` ya usa mutex interno.
- Testeable: se puede pasar un `EventQueue` real o un wrapper en tests.

**Desventajas:**
- `CallbackBridge` pasa a depender del tipo `EventQueue`, que es un detalle de implementación interno de `ReplicationEngine`. Si en el futuro se reemplaza `EventQueue` por otro mecanismo, `CallbackBridge` también cambia.
- El constructor de `CallbackBridge` ya no es trivial — DDM no puede construirlo directamente sin conocer la `EventQueue`. Requiere que `ReplicationEngine` lo construya internamente (no como dependencia externa inyectada por DDM).

---

### Opción B: Inyectar std::function callbacks en el constructor de CallbackBridge

`CallbackBridge` recibe dos funciones en su constructor, una por evento de recuperación.

```cpp
// CallbackBridge.h
class CallbackBridge : public IReplicationBridge {
public:
    using RecoveryFn = std::function<void()>;
    CallbackBridge(RecoveryFn on_reconnect_transport, RecoveryFn on_reinit_storage);
    ...
private:
    RecoveryFn            on_reconnect_transport_;
    RecoveryFn            on_reinit_storage_;
    IReplicationListener* listener_{nullptr};
};

// En ReplicationEngine::start() o constructor
bridge_ = std::make_unique<CallbackBridge>(
    [this]() { queue_.push(ReconnectTransportEvent{}); },
    [this]() { queue_.push(ReinitStorageEvent{}); }
);
```

```cpp
void CallbackBridge::reconnectTransport() {
    if (on_reconnect_transport_) on_reconnect_transport_();
}
void CallbackBridge::reinitializeStorage() {
    if (on_reinit_storage_) on_reinit_storage_();
}
```

**Ventajas:**
- `CallbackBridge` no depende de `EventQueue` ni de ningún tipo interno de `ReplicationEngine`.
- Totalmente desacoplado: en tests se puede pasar cualquier lambda (incluyendo mocks que registren llamadas).
- Extensible: agregar nuevos eventos de recuperación no requiere cambiar el tipo de `CallbackBridge`.

**Desventajas:**
- La firma del constructor se vuelve más larga y menos obvia que en Opción A.
- La semántica de las funciones inyectadas no queda expresada en los tipos — es documentación, no contrato.
- Dos campos `std::function` (con heap allocation interna) vs. una referencia simple.

---

### Opción C: Eliminar reconnectTransport/reinitializeStorage de IReplicationBridge y moverlos a IReplicationEngine

DDM llama directamente a `engine->reconnectTransport()` en lugar de `bridge->reconnectTransport()`. `IReplicationBridge` queda reducida a: `registerListener()` + los seis `notify*()`.

```cpp
// IReplicationEngine.h — agrega métodos de recuperación
virtual void reconnectTransport()   = 0;
virtual void reinitializeStorage()  = 0;

// IReplicationBridge.h — queda solo con listener y notify*
```

DDM mantiene dos punteros: `IReplicationEngine*` para comandos (incluyendo recuperación) e `IReplicationBridge*` solo para `registerListener()`.

**Ventajas:**
- `CallbackBridge` queda sin lógica de routing — solo forwarding de notificaciones.
- Separación de responsabilidades más clara: `IReplicationEngine` recibe comandos, `IReplicationBridge` envía notificaciones.
- No se necesita inyectar nada en `CallbackBridge`.

**Desventajas:**
- Cambia la interfaz pública que DDM ya conoce (`IReplicationBridge`). Requiere coordinación con el equipo de Ingeniería.
- DDM pasa a gestionar dos referencias en lugar de una.
- El ICD §4 describe `reconnectTransport` / `reinitializeStorage` como parte del contrato de `IReplicationBridge`. Cambiar esto requiere actualizar ICD, SDD y el código de DDM.
- Complica la inicialización: actualmente DDM construye `CallbackBridge`, llama `registerListener`, y pasa ownership a `ReplicationEngine`. Con esta opción, el flujo de inicialización se bifurca.

---

## Análisis Comparativo

| Criterio | Opción A (EventQueue&) | Opción B (std::function) | Opción C (mover a IReplicationEngine) |
|---|---|---|---|
| Impacto en interfaz pública | Ninguno | Ninguno | Alto — cambia IReplicationBridge e IReplicationEngine |
| Acoplamiento de CallbackBridge | EventQueue (tipo interno) | Ninguno | Ninguno |
| Testabilidad | Alta (EventQueue real) | Muy alta (lambda arbitraria) | Alta (mock de RE) |
| Complejidad de implementación | Mínima | Baja | Media |
| Coordinación con DDM requerida | No | No | Sí |
| Claridad semántica del constructor | Alta | Media | N/A |

---

## Decisión

**Opción C: mover `reconnectTransport()` y `reinitializeStorage()` a `IReplicationEngine`.**

`IReplicationBridge` tiene una responsabilidad única y coherente: ser el canal de notificaciones RE→DDM (`registerListener()` + `notify*()`). Los métodos de recuperación son comandos DDM→RE y pertenecen a `IReplicationEngine`, donde ya viven `onLocalObjectUpserted()` y `onLocalObjectDeleted()`.

`CallbackBridge` queda libre de toda lógica de routing hacia la cola interna. La brecha de diseño de los TODOs en `reconnectTransport()` / `reinitializeStorage()` desaparece: `ReplicationEngine` implementa los métodos directamente encolando los eventos correspondientes.

El costo de coordinación con Ingeniería es puntual: DDM mueve dos llamadas de `bridge->` a `engine->` y actualiza los imports. No hay cambio de comportamiento.

---

## Consecuencias

- `IReplicationEngine` agrega `virtual void reconnectTransport() = 0` y `virtual void reinitializeStorage() = 0`.
- `ReplicationEngine` implementa ambos métodos encolando `ReconnectTransportEvent` y `ReinitStorageEvent`.
- `IReplicationBridge` elimina `reconnectTransport()` y `reinitializeStorage()`. Queda con: `registerListener()` + seis `notify*()`.
- `CallbackBridge` elimina las implementaciones stub de ambos métodos. No necesita acceso a `EventQueue`.
- DDM actualiza dos llamadas: `bridge->reconnectTransport()` → `engine->reconnectTransport()` y `bridge->reinitializeStorage()` → `engine->reinitializeStorage()`.
- ICD §4 actualiza los eventos ReconnectTransport y ReinitializeStorage para reflejar que van por `IReplicationEngine`.
- SRS RF-RE-007 y RF-RE-008 actualizan la referencia de interfaz.
- SAD actualiza el diagrama de clases (§4.4).
- SDD actualiza §3.2 (IReplicationEngine), §7.2 (IReplicationBridge) y §7.3 (CallbackBridge).

## Referencias

- `decisions/ADR-009-dds-runtime-fault-tolerance.md` — origen de `reconnectTransport()`
- `decisions/ADR-010-sqlite-runtime-fault-tolerance.md` — origen de `reinitializeStorage()`
- `decisions/ADR-005-internal-thread-communication.md` — catálogo de eventos (`ReconnectTransportEvent`, `ReinitStorageEvent`)
- `docs/icd/ICD.md` §4 — contrato DDM→RE para eventos de recuperación
- `docs/sdd/SDD.md` §7 — diseño de ReplicationBridge y CallbackBridge
