# Plan: conectar SECTOR al backend real (create_sector / delete_sector)

## Diferencia clave respecto a AREA y POLIGONO — leer primero

En AREA y POLIGONO el trabajo pendiente estaba del lado del **cliente** (`tdc-botonera/botonera`): el servidor ya tenía `create_area`/`create_polygon` en el mapa de comandos JSON, y había que cablear la UI QML para que los llamara.

**En SECTOR es al revés.** Se verificó el código real de ambos repos y:

- El **cliente ya está 100% implementado**: `JsonCommandBuilder::buildCreateSectorCommand`/`buildDeleteSectorCommand`, `DDMController::createSector`/`deleteSector` (con sus señales `sectorCreated`/`sectorDeleted` y handlers de éxito/error), `BackendService.qml::createSector`/`deleteSector`, y un `SectorPanel.qml` completo y funcional (`DDM/workspaces/superficie/figuras/SectorPanel.qml`) ya integrado en `FigurasWorkspace.qml` (menú índice 3). Todo esto ya envía el comando JSON `create_sector`/`delete_sector` al backend.
- El **servidor (este repo, `DDM`) nunca llegó a exponer `create_sector`/`delete_sector` en el protocolo JSON**. `JsonCommandHandler::initializeCommandMap` (`src/controller/json/jsoncommandhandler.cpp:90-170`) registra `create_area`, `delete_area`, `create_circle`, `delete_circle`, `create_polygon`, `delete_polygon`, `list_shapes`, etc. — pero **no** `create_sector` ni `delete_sector`. `GeometryCommandHandler` (`src/controller/handlers/geometrycommandhandler.h/.cpp`) tampoco tiene métodos `createSector`/`deleteSector`.
- Lo único que existe para SECTOR del lado servidor es un comando de **consola de texto legacy**, completamente separado del protocolo JSON/WebSocket: `AddSectorCommand`/`DeleteSectorCommand` (`src/controller/commands/addSectorCommand.cpp`, `deleteSectorCommand.cpp`), registrados en `main.cpp:96-97` sobre un `CommandRegistry`/`CommandDispatcher` que lee líneas de stdin (`args.size() < 7`, formato `addSector <az_izq> <az_der> <rad_int> <rad_ext> <color> <origen_x> <origen_y> [id_track]`). Este mecanismo **no** es el que usa la GUI — la GUI habla JSON sobre el transporte real (`ITransport`), igual que LINEA/CIRCULO/AREA/POLIGONO.

**Conclusión**: hoy, si un usuario aprieta "INICIAR" en el panel SECTOR del cliente, el comando `create_sector` llega al servidor y no es reconocido (cae en el camino de "comando desconocido" de `JsonCommandHandler`). El trabajo que falta es **puramente de servidor**, en este repo, no en el cliente.

Este documento describe qué falta agregar en el servidor para que el contrato coincida exactamente con lo que el cliente ya envía. **No implementa nada** — es un plan de mano, igual que `planArea.md`/`poligono.md`, pero para quien continúe del lado servidor (podés ser vos mismo u otro agente trabajando en este repo).

---

## Lo que el cliente ya envía (contrato real, verificado en el código)

### `create_sector`

Armado en `JsonCommandBuilder::buildCreateSectorCommand` (`tdc-botonera/botonera/src/controller/json/jsoncommandbuilder.cpp:126-146`):

```json
{
  "command": "create_sector",
  "args": {
    "az_izq": 350.0,
    "az_der": 10.0,
    "rad_int": 5.0,
    "rad_ext": 8.0,
    "color": "RGB1",
    "id_track": 0,
    "origen": { "x": 0.0, "y": 0.0 }
  }
}
```

Notas importantes sobre estos valores, tal como los arma hoy el cliente (`SectorPanel.qml:15-52`):

- `color`: el cliente traduce los nombres en español de la UI (`ROJO`, `VERDE`, `AZUL`, `CIAN`, `MAGENTA`, `AMARILLO` — el mismo `lineColorsModel` que usan LINEA/CIRCULO/AREA/POLIGONO) a los valores que espera `SectorColor` en el servidor (`RGB1`, `RGB2`, `RGB3`, `CMYK1`, `CMYK2`, `CMYK3`), vía un mapa fijo en `jsoncommandbuilder.cpp:131-134`. El servidor solo necesita parsear estos 6 valores literales — coincide exactamente con `SectorColor` (`src/model/entities/sectorEntity.h:17-20`).
- `id_track`, `origen.x`, `origen.y`: `SectorPanel.qml:47` los llama siempre con `BackendService.createSector(azIzq, azDer, radInt, radExt, selectedSectorColor, 0, 0, 0)` — es decir, **hoy la UI siempre manda `id_track=0` y `origen=(0,0)`**. No hay todavía ningún control en la UI para asociar un track real o fijar una posición de origen. Esto es una limitación conocida del cliente, no algo a resolver acá (ver "Fuera de alcance").
- **`type`/`tipo` no se envía**: a diferencia de AREA/POLIGONO, `SectorPanel.qml` tiene un dropdown "TIPO" en la UI (`tipoSectorInputLoader`, usa `lineTypesModel`) pero la función `crearSector()` **no lo incluye** en el comando — hay un comentario explícito en el código (`SectorPanel.qml:40-41`): *"TIPO se mantiene solo en la UI; el backend de sector aún no recibe un campo de tipo"*. Es coherente con que `SectorCreateRequest` (servidor) tampoco tiene campo `tipo` — `GeometryService::createSector` hardcodea `SectorTipo::ZonaAlerta` (`geometryservice.cpp:100`). No hace falta agregar soporte de `type` para que esto funcione.

Respuesta esperada por el cliente (`DDMController::handleCreateSectorSuccess`, `ddmcontroller.cpp:598-605`): `{"created_id": <int>}` (con fallback a `"id"` si no está `created_id`).

### `delete_sector`

```json
{ "command": "delete_sector", "args": { "id": 1335 } }
```

`id` ya se manda como entero JSON real (`args["id"] = sectorId` con `sectorId` tipado `int` en `jsoncommandbuilder.cpp:151`) — **no** repite el bug de `delete_line`/`delete_circle` (esos mandan el id como string). No hay nada que corregir en el cliente para esto.

Respuesta esperada (`handleDeleteSectorSuccess`, `ddmcontroller.cpp:614-621`): `{"deleted_id": <int>}` (con fallback a `"id"`).

---

## Lo que ya existe en el servidor y se puede reutilizar tal cual

No hay que escribir lógica de negocio nueva — ya existe y está probada por el camino de consola:

- `GeometryService::createSector(const SectorCreateRequest&)` (`src/controller/services/geometryservice.cpp:86-104`): valida `rad_ext > rad_int >= 0`, `az_izq`/`az_der` en `[0.0, 360.0)`, crea el `SectorEntity`, llama `calculateAndStoreCursors`, lo agrega al contexto y devuelve `GeometryResult{success, errorCode, message, id}`.
- `GeometryService::deleteSector(int)` (`geometryservice.cpp:107-113`): valida id no negativo y existencia, devuelve `GeometryResult`.
- `SectorCreateRequest` (`geometryservice.h:12-20`): struct con `az_izq`, `az_der`, `rad_int`, `rad_ext`, `color` (`SectorColor`), `origen` (`QPointF`), `id_track`.
- `GeometryService::listShapes()` ya arma un array `"sectors"` completo (`geometryservice.cpp:177-194`, con `id`, `az_izq`, `az_der`, `rad_int`, `rad_ext`, `color` como string, `id_track`, `origen`) — hoy es código "muerto" porque nada llama `create_sector` para poblarlo, pero en cuanto se agregue el handler, `list_shapes` ya devuelve sectores reales sin tocar nada más.
- Parseo de color string → `SectorColor`: ya existe una función exactamente así, `parseSectorColor` (`src/controller/commands/addSectorCommand.cpp:6-14`), aunque es `static` dentro de ese `.cpp` y no reutilizable directamente. Sirve como referencia exacta del mapeo a replicar (o extraer a un lugar compartido, a criterio de quien implemente).

Lo único que falta es la **capa de traducción JSON → `SectorCreateRequest`/`int`**, igual que ya existe para AREA/CIRCULO/POLIGONO en `GeometryCommandHandler`.

---

## Cambios concretos a implementar (todos en este repo, `DDM`)

### 1. `src/controller/handlers/geometrycommandhandler.h`

Agregar, junto a los métodos existentes:

```cpp
QByteArray createSector(const QJsonObject& args);
QByteArray deleteSector(const QJsonObject& args);
```

### 2. `src/controller/handlers/geometrycommandhandler.cpp`

Seguir exactamente el patrón de `createArea`/`deleteArea` (`geometrycommandhandler.cpp:19-62`):

```cpp
static bool parseSectorColorJson(const QString& s, SectorColor& out) {
    // mismo mapeo que addSectorCommand.cpp:6-14 (RGB1/RGB2/RGB3/CMYK1/CMYK2/CMYK3)
    ...
}

QByteArray GeometryCommandHandler::createSector(const QJsonObject& args)
{
    if (!args.contains("az_izq") || !args.contains("az_der") ||
        !args.contains("rad_int") || !args.contains("rad_ext")) {
        return JsonResponseBuilder::buildValidationErrorResponse("create_sector", "az_izq/az_der/rad_int/rad_ext", "", "requeridos");
    }

    SectorColor color;
    if (!parseSectorColorJson(args.value("color").toString(), color)) {
        return JsonResponseBuilder::buildValidationErrorResponse("create_sector", "color", args.value("color").toString(), "RGB1|RGB2|RGB3|CMYK1|CMYK2|CMYK3");
    }

    QJsonObject origenObj = args.value("origen").toObject();

    SectorCreateRequest req;
    req.az_izq   = args.value("az_izq").toDouble();
    req.az_der   = args.value("az_der").toDouble();
    req.rad_int  = args.value("rad_int").toDouble();
    req.rad_ext  = args.value("rad_ext").toDouble();
    req.color    = color;
    req.origen   = QPointF(origenObj.value("x").toDouble(), origenObj.value("y").toDouble());
    req.id_track = args.value("id_track").toInt(0);

    GeometryResult result = m_geometryService->createSector(req);
    if (!result.success) {
        return JsonResponseBuilder::buildErrorResponse("create_sector", result.errorCode, result.message);
    }
    return JsonResponseBuilder::buildSuccessResponse("create_sector", QJsonObject{{"created_id", result.id}});
}

QByteArray GeometryCommandHandler::deleteSector(const QJsonObject& args)
{
    if (!args.contains("id")) {
        return JsonResponseBuilder::buildValidationErrorResponse("delete_sector", "id", "", "required");
    }
    const int id = args.value("id").toInt(-1);
    GeometryResult result = m_geometryService->deleteSector(id);
    if (!result.success) {
        return JsonResponseBuilder::buildErrorResponse("delete_sector", result.errorCode, result.message);
    }
    return JsonResponseBuilder::buildSuccessResponse("delete_sector", QJsonObject{{"deleted_id", id}});
}
```

`GeometryService::createSector`/`deleteSector` ya validan todo lo necesario (radios, azimuts, existencia del id) — el handler solo traduce y delega, igual que `createArea`.

### 3. `src/controller/json/jsoncommandhandler.cpp`

Registrar en `initializeCommandMap` (junto a `create_polygon`/`delete_polygon`, `jsoncommandhandler.cpp:120-126`):

```cpp
m_commandMap[QStringLiteral("create_sector")] = [this](const QJsonObject& args) {
    return m_geometryHandler->createSector(args);
};

m_commandMap[QStringLiteral("delete_sector")] = [this](const QJsonObject& args) {
    return m_geometryHandler->deleteSector(args);
};
```

### 4. Nada más

No hace falta tocar `GeometryService`, `SectorEntity`, `CommandContext`, ni `listShapes` — ya están completos y correctos. No hace falta tocar el cliente (`tdc-botonera/botonera`) — ya está completo y correcto.

---

## Fuera de alcance (documentado, no se resuelve acá)

- **UI de asociación a track / posición real**: `SectorPanel.qml` manda siempre `id_track=0`, `origen=(0,0)`. Agregar controles reales de "asociar a track" o fijar posición de origen es trabajo de cliente, no de este documento.
- **Campo `TIPO`**: la UI lo muestra pero no lo envía; el servidor hardcodea `SectorTipo::ZonaAlerta`. Exponer `tipo` de punta a punta (UI → JSON → `SectorCreateRequest` → `SectorEntity`) queda como mejora futura si se necesita distinguir `ZonaExclusion`/`ZonaVigilancia`/`Personalizado`.
- **`sectorIdsModel` vacío**: `FigurasViewModel.qml:9-11` ya deja `sectorIdsModel: []` con un comentario explícito señalando que falta esta integración — una vez que `list_shapes` sea alcanzable (o se agregue un flujo específico), poblar los dropdowns EDITAR/BORRAR de `SectorPanel.qml` con IDs reales. Mismo patrón de limitación ya aceptado para AREA/POLIGONO.
- **Comando de consola legacy** (`AddSectorCommand`/`DeleteSectorCommand`): se deja intacto, no se toca ni se unifica con el camino JSON.

---

## Verificación al terminar (cuando se implemente)

1. Compilar el servidor (`DDM`) y confirmar que no rompe `create_area`/`create_circle`/`create_polygon` existentes.
2. Levantar servidor + cliente, ir a Figuras → SECTOR, cargar AZ.IZQ/AZ.DER/RAD.INT/RAD.EXT/COLOR válidos, tocar "INICIAR" (en la columna POSICION compartida).
3. Confirmar en el servidor que llega `create_sector` con los 6 campos (`az_izq`, `az_der`, `rad_int`, `rad_ext`, `color`, `id_track`, `origen`) y responde `{"created_id": N}`.
4. Probar valores inválidos (ej. `rad_ext <= rad_int`, azimut fuera de `[0,360)`) y confirmar que el servidor responde error y el cliente muestra el mensaje correspondiente.
5. Probar `delete_sector` con un id conocido y confirmar `{"deleted_id": N}`.
6. Llamar `list_shapes` y confirmar que el sector recién creado aparece en el array `"sectors"`.
