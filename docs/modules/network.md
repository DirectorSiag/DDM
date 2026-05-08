# Módulo: Network

## Descripción general

Este módulo abstrae el transporte de mensajes del backend (entrada/salida) para permitir conmutación entre UDP y Local IPC sin modificar controladores de dominio.

El contrato común es `ITransport`; las implementaciones concretas (`UdpClientAdapter`, `LocalIpcClient`) son instanciadas por `makeTransport` en `transportFactory`.

## Lista de archivos y clases

| Archivo | Clase/Struct | Responsabilidad |
|---|---|---|
| `src/model/network/iTransport.h` | `ITransport` | Interfaz de transporte (send/start/stop + señales de conexión y mensaje). |
| `src/model/network/clientSocket.h` | `clientSocket` | Implementación UDP base con `QUdpSocket`. |
| `src/model/network/udpClientAdapter.h` | `UdpClientAdapter` | Adaptador de `clientSocket` al contrato `ITransport`. |
| `src/model/network/localipcclient.h` | `LocalIpcClient` | Transporte local por `QLocalSocket` con framing por longitud. |
| `src/model/network/transportFactory.h` | `TransportKind`, `TransportOpts` | Tipos de selección y opciones de construcción de transporte. |
| `src/model/network/transportFactory.cpp` | `makeTransport(...)` | Factoría de transporte según configuración runtime. |

## Clases principales

### ITransport

- **Rol**: Contrato abstracto para transporte de bytes, desacoplando controladores de detalles de red/IPC.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Enviar | `virtual bool send(const QByteArray& data) = 0` | Envía payload al peer. |
| Conectividad | `virtual bool isConnected() const` | Reporta estado de conexión. |
| Start | `virtual void start()` | Inicializa recurso de transporte. |
| Stop | `virtual void stop()` | Libera recurso de transporte. |
| Señal datos | `void messageReceived(const QByteArray& data)` | Evento de entrada de mensajes. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- Qt (`QObject`, `QByteArray`)

### clientSocket

- **Rol**: Implementación UDP concreta con bind local y envío de datagramas al endpoint configurado.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor | `explicit clientSocket(QObject* parent)` | Configura `QUdpSocket`, bind y readyRead. |
| Envío | `void sendMessage(QByteArray message)` | Envía datagrama UDP. |
| Recepción | `void readPendingDatagrams()` | Drena cola de datagramas y emite señal. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `QUdpSocket`
- `Configuration`

### UdpClientAdapter

- **Rol**: Adapta `clientSocket` al contrato `ITransport` para uso homogéneo por controladores.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor | `explicit UdpClientAdapter(clientSocket* udp, QObject* parent=nullptr)` | Enlaza señales UDP con `ITransport::messageReceived`. |
| Start | `void start() override` | No-op (según implementación actual). |
| Stop | `void stop() override` | No-op (según implementación actual). |
| Send | `bool send(const QByteArray& data) override` | Delega en `clientSocket::sendMessage`. |

- **Structs/Tipos definidos**: No aplica.
- **Dependencias**:
- `ITransport`
- `clientSocket`

### LocalIpcClient

- **Rol**: Implementación `ITransport` para comunicación local con framing de longitud (`uint32 LE + payload`) y reconexión automática.
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Constructor | `explicit LocalIpcClient(QString name, QObject* parent=nullptr)` | Configura nombre de socket local. |
| Start | `void start() override` | Crea socket, conecta handlers y arranca intento de conexión. |
| Stop | `void stop() override` | Desconecta socket y limpia buffer. |
| Send | `bool send(const QByteArray& data) override` | Empaqueta y escribe frame al socket local. |
| Conectividad | `bool isConnected() const override` | Estado conectado/desconectado real. |
| Reintento | `void attemptConnect()` | Reintenta conexión tras error/disconnect. |

- **Structs/Tipos definidos**:
- `pack(const QByteArray& payload)` (helper de framing)
- **Dependencias**:
- `ITransport`
- `QLocalSocket`, `QTimer`

### transportFactory

- **Rol**: Factoría para crear transporte concreto según modo configurado (`Udp` o `LocalIpc`).
- **Métodos clave**:

| Método | Firma | Descripción |
|---|---|---|
| Build | `std::unique_ptr<ITransport> makeTransport(TransportKind kind, const TransportOpts& o, QObject* parent=nullptr)` | Retorna implementación adecuada de transporte. |

- **Structs/Tipos definidos**:
- `enum class TransportKind { Udp, LocalIpc }`
- `struct TransportOpts { QString endpointBind; QString endpointPeer; QString localName; }`
- **Dependencias**:
- `ITransport`, `UdpClientAdapter`, `LocalIpcClient`, `clientSocket`

## Flujo de datos

1. `main.cpp` decide transporte según `Configuration::instance().useLocalIpc`.
2. `makeTransport(...)` construye `ITransport` concreto.
3. `transport->start()` inicia canal.
4. Entrada: implementación concreta emite `messageReceived` y `MessageRouter` procesa.
5. Salida: controladores/handlers envían con `ITransport::send(...)` sin conocer el backend de red.

## Manejo de errores

- `LocalIpcClient`:
  - reconexión automática con `QTimer::singleShot`
  - validación de estado antes/después de `write/flush`
  - señal `error(QString)` para fallas relevantes
- `UdpClientAdapter`: retorna `false` si no hay socket válido.
- `clientSocket`: loop de lectura de datagramas pendientes para no perder paquetes en cola.

## Módulos relacionados

- `docs/architecture.md`
- `docs/modules/decoders-encoders.md`
- `docs/modules/handlers.md`
- `docs/README_BACKEND_ARCHITECTURE.md`
