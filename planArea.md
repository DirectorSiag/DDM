# Plan: implementar carga de AREA desde la UI (cliente)

## Objetivo

Cablear la figura AREA en la interfaz gráfica para que mande el comando `create_area`/`delete_area` al backend, siguiendo el mismo patrón usado para LINEA y CIRCULO.

## Repo de trabajo

**Todo el código a modificar vive en el repo cliente:**
`/home/cristian/Documentos/Siag/tdc-botonera/botonera/`

(El repo servidor `DDM` — donde está este archivo — ya tiene `create_area`/`delete_area` completamente implementados y funcionando; no requiere ningún cambio.)

## Contrato real del backend (ya implementado, verificado en el código del servidor)

`src/controller/handlers/geometrycommandhandler.cpp` (repo DDM):

- **`create_area`**: espera `args["points"]` = array de **exactamente 4 objetos** `{"x": <double>, "y": <double>}` (mismo formato que `create_polygon`, NO son campos sueltos `x1,y1,x2,y2...`). También `args["type"]` (int, default 0) y `args["color"]` (string, default "ROJO"). Responde `{"created_id": <int>}` en éxito. **No** devuelve una lista completa de áreas (a diferencia de `create_line`, que sí devuelve `"lines"`).
- **`delete_area`**: espera `args["id"]` como **entero real** (el servidor hace `args.value("id").toInt(-1)`; si se manda un JSON string ahí, `toInt()` devuelve -1 y falla). Responde `{"deleted_id": <int>}`.
- No hay lista de áreas disponible salvo vía `list_shapes` (devuelve `"areas": [...]` completo con `id`, `type`, `color`, `cursor_ids`) — fuera de alcance de este plan.

⚠️ **Bug existente a NO replicar**: `buildDeleteLineCommand`/`buildDeleteCircleCommand` (cliente) mandan `args["id"] = lineId` donde `lineId` es un `QString`, lo cual serializa como JSON string — probablemente ya rompe `delete_line`/`delete_circle` en el servidor real. Para AREA, `deleteArea` debe tomar `int` y mandar un entero JSON genuino.

## Patrón elegido: seguir a CIRCULO (no a LINEA)

CIRCULO no mantiene ninguna lista (`circlesList`) del lado del cliente, porque `create_circle`/`delete_circle` tampoco devuelven una lista completa — igual que AREA. LINEA sí mantiene `linesList` porque `create_line` devuelve `"lines"`. Por lo tanto:

- **No** agregar `Q_PROPERTY areasList` a `DDMController`.
- **No** agregar ningún struct nuevo a `JsonResponseParser` (ni `AreaData` ni `extractAreaIds`) — parsear `created_id`/`deleted_id` inline en `DDMController.cpp`, siguiendo el patrón ya usado para las entidades "track" (`handleCreateTrackSuccess`/`handleDeleteTrackSuccess`), que tampoco usan `JsonResponseParser` y parsean directo del `QJsonObject args`.

## Cambios concretos por archivo

### 1. `src/controller/json/jsoncommandbuilder.h`

Agregar:
```cpp
QByteArray buildCreateAreaCommand(const QVariantList& points, int type, const QString& color) const;
QByteArray buildDeleteAreaCommand(int areaId) const;
```

Usar `QVariantList` (no `QVector<QPointF>`): cuando QML pasa un array JS de objetos `{x:.., y:..}` a un método `Q_INVOKABLE`, Qt lo convierte automáticamente a `QVariantList` de `QVariantMap`, **no** a `QVector<QPointF>`. Si se declara el parámetro como `QVector<QPointF>`, la llamada desde QML fallará silenciosamente o no compilará el binding.

### 2. `src/controller/json/jsoncommandbuilder.cpp`

```cpp
QByteArray JsonCommandBuilder::buildCreateAreaCommand(const QVariantList& points, int type, const QString& color) const
{
    QJsonArray pointsArray;
    for (const QVariant& pv : points) {
        QVariantMap p = pv.toMap();
        QJsonObject obj;
        obj["x"] = p.value("x").toDouble();
        obj["y"] = p.value("y").toDouble();
        pointsArray.append(obj);
    }

    QJsonObject args;
    args["points"] = pointsArray;
    args["type"] = type;
    args["color"] = color;

    return buildCommand("create_area", args);
}

QByteArray JsonCommandBuilder::buildDeleteAreaCommand(int areaId) const
{
    QJsonObject args;
    args["id"] = areaId;   // entero real, NO QString — a diferencia de buildDeleteLineCommand/buildDeleteCircleCommand

    return buildCommand("delete_area", args);
}
```

### 3. `src/controller/json/jsonresponseparser.h` / `.cpp`

**Sin cambios.** No hace falta agregar ningún struct ni método — AREA parsea `created_id`/`deleted_id` directo en `DDMController.cpp` (ver punto 4), igual que las entidades "track".

### 4. `src/controller/protocol/ddmcontroller.h`

Agregar (sin `Q_PROPERTY areasList`):
```cpp
Q_INVOKABLE void createArea(const QVariantList& points, int type, const QString& color);
Q_INVOKABLE void deleteArea(int areaId);

signals:
    void areaCreated(int areaId, bool success);
    void areaDeleted(int areaId, bool success);

private:
    void handleCreateAreaSuccess(const QJsonObject& args);
    void handleCreateAreaError(const QJsonObject& args);
    void handleDeleteAreaSuccess(const QJsonObject& args);
    void handleDeleteAreaError(const QJsonObject& args);
```

### 5. `src/controller/protocol/ddmcontroller.cpp`

```cpp
void DDMController::createArea(const QVariantList& points, int type, const QString& color)
{
    QByteArray command = m_commandBuilder.buildCreateAreaCommand(points, type, color);
    emit sendBackendCommand(command);
}

void DDMController::deleteArea(int areaId)
{
    QByteArray command = m_commandBuilder.buildDeleteAreaCommand(areaId);
    emit sendBackendCommand(command);
}

void DDMController::handleCreateAreaSuccess(const QJsonObject& args)
{
    int createdId = args.value("created_id").toInt(-1);
    emit areaCreated(createdId, true);
}

void DDMController::handleCreateAreaError(const QJsonObject& args)
{
    Q_UNUSED(args);
    emit areaCreated(-1, false);
}

void DDMController::handleDeleteAreaSuccess(const QJsonObject& args)
{
    int deletedId = args.value("deleted_id").toInt(-1);
    emit areaDeleted(deletedId, true);
}

void DDMController::handleDeleteAreaError(const QJsonObject& args)
{
    Q_UNUSED(args);
    emit areaDeleted(-1, false);
}
```

Registrar en el `static const QMap<QString, QMap<QString, HandlerFunc>> commandHandlers` dentro de `handleBackendResponse`:
```cpp
{"create_area", {{"success", &DDMController::handleCreateAreaSuccess}, {"error", &DDMController::handleCreateAreaError}}},
{"delete_area", {{"success", &DDMController::handleDeleteAreaSuccess}, {"error", &DDMController::handleDeleteAreaError}}},
```

(Verificar el nombre exacto del tipo `HandlerFunc` y la firma real del mapa leyendo el archivo antes de escribir — debe coincidir con las entradas ya existentes para `create_line`/`create_circle`/`create_track`.)

### 6. `DDM/core/BackendService.qml`

Agregar, junto a `createCircle`/`deleteCircle`:
```qml
function createArea(points, type, color) { if (_ctrl) { _ctrl.createArea(points, type, color) } }
function deleteArea(areaId) { if (_ctrl) { _ctrl.deleteArea(areaId) } }
```

Sin agregar ninguna propiedad `areasList` (no existe contraparte del lado servidor).

### 7. Nuevo componente `DDM/workspaces/superficie/figuras/AreaPanel.qml`

Extraer el bloque `Rectangle` que hoy está inline en `FigurasWorkspace.qml` (líneas ~79-225, dentro del `border.color: Constants.colorNaranja`) y convertirlo en un componente propio, con `property var viewModel` (mismo patrón que `LineaPanel.qml`/`CirculoPanel.qml`).

Contenido actual del mockup (a corregir):
- Fila superior: MAN (`RadioIndicator`), TIPO (`DataDropdown` bindeado a `vm.lineTypesModel`), COLOR (`DataDropdown` bindeado a `vm.lineColorsModel`) — **reusar tal cual**, mismos modelos que LINEA/CIRCULO.
- 4 filas de vértices A/B/C/D, cada una con dos `DataInput` en formato LAT/LON (placeholder `"-- ° -- ´ -- . -- ´´ S/W"`), **sin `id`**, sin validador real.
- Botones `IniciarButton {}` / `ResetButton {}` **vacíos** (sin `onClicked`).
- Dropdowns EDITAR y BORRAR ambos bindeados por error a `vm.linesModelOrEmpty()` (bug copiado de CIRCULO).
- El botón BORRAR tiene `isLabel: true` sin `onClicked` → está deshabilitado (`WsButton.enabled: !buttonRoot.isLabel`), por eso hoy no hace nada.

Cambios a hacer en `AreaPanel.qml`:

a) **Convertir los 8 campos de LAT/LON (formato DMS) a campos numéricos simples LONG/LAT**, con `id` explícito y validador numérico. Los labels de la UI deben decir **"LONG"** y **"LAT"** (no "X"/"Y"), pero el dato de entrada sigue siendo un número simple (no el formato DMS `-- ° -- ´ -- . -- ´´`) — igual de simple que iba a ser X/Y, solo cambia la etiqueta:
```qml
DataInput {
    id: longAInput
    validator: DoubleValidator { bottom: -99999.9; top: 99999.9; decimals: 1; notation: DoubleValidator.StandardNotation }
}
DataInput {
    id: latAInput
    validator: DoubleValidator { bottom: -99999.9; top: 99999.9; decimals: 1; notation: DoubleValidator.StandardNotation }
}
```
(repetir para B, C, D → `longBInput/latBInput`, `longCInput/latCInput`, `longDInput/latDInput`). El rango `-99999.9..99999.9` es genérico — ajustar si aparece un rango real documentado en otro lado del proyecto.

> **Nota para más adelante (fuera de alcance de este plan)**: por ahora `long`/`lat` se mandan tal cual al backend bajo las claves `x`/`y` que espera `create_area` (ver contrato del backend arriba, que no cambia). Más adelante se va a agregar una conversión real de latitud/longitud a coordenadas cartesianas antes de armar el payload — cuando eso se implemente, `longAInput`/`latAInput` etc. pasan por esa conversión antes de mapearse a `x`/`y`. Este plan no la incluye.

b) **`IniciarButton.onClicked`** — validar los 8 campos y armar el array de puntos como objetos `{x,y}` (NO como `QPointF`, ya que del lado QML solo existen como números). El campo LONG se mapea a `x` y el campo LAT se mapea a `y` (contrato actual del backend, sin conversión geográfica todavía):
```qml
IniciarButton {
    anchors.horizontalCenter: parent.horizontalCenter
    onClicked: {
        var allOk = longAInput.acceptableInput && latAInput.acceptableInput &&
                    longBInput.acceptableInput && latBInput.acceptableInput &&
                    longCInput.acceptableInput && latCInput.acceptableInput &&
                    longDInput.acceptableInput && latDInput.acceptableInput
        if (allOk) {
            var points = [
                { x: parseFloat(longAInput.inputText.replace(",", ".")), y: parseFloat(latAInput.inputText.replace(",", ".")) },
                { x: parseFloat(longBInput.inputText.replace(",", ".")), y: parseFloat(latBInput.inputText.replace(",", ".")) },
                { x: parseFloat(longCInput.inputText.replace(",", ".")), y: parseFloat(latCInput.inputText.replace(",", ".")) },
                { x: parseFloat(longDInput.inputText.replace(",", ".")), y: parseFloat(latDInput.inputText.replace(",", ".")) }
            ]
            var type = tipoInputLoader.currentIndex
            var selectedColor = colorInputLoader.currentText
            if (areaPanelRoot.viewModel) { areaPanelRoot.viewModel.statusMessage = "Creando área..." }
            BackendService.createArea(points, type, selectedColor)
            longAInput.clear(); latAInput.clear()
            longBInput.clear(); latBInput.clear()
            longCInput.clear(); latCInput.clear()
            longDInput.clear(); latDInput.clear()
            if (areaPanelRoot.viewModel) { areaPanelRoot.viewModel.statusMessage = "Comando create_area enviado (color " + selectedColor + ")" }
        } else {
            if (areaPanelRoot.viewModel) { areaPanelRoot.viewModel.statusMessage = "Error: Datos inválidos" }
        }
    }
}
```
⚠️ **`type` se manda como `currentIndex` crudo (0-based), sin `+1`** — confirmado leyendo `LineaPanel.qml`, que hace lo mismo. No replicar ningún `+1` que aparezca en `docs/area.md`.

c) **`ResetButton.onClicked`**: limpiar los 8 campos (`longAInput`/`latAInput` ... `longDInput`/`latDInput`) y resetear TIPO/COLOR a `currentIndex: 0`.

d) **Dropdowns EDITAR y BORRAR**: cambiar `model: vm.linesModelOrEmpty()` (bug) por `model: []`. El backend no devuelve una lista de áreas en `create_area`/`delete_area` (a diferencia de LINEA), así que no hay de dónde poblar estos dropdowns hoy. Dejar un comentario corto indicando que se necesitaría `list_shapes` para poblarlos de verdad (mejora futura, fuera de alcance).

e) **Botón BORRAR**: quitar `isLabel: true` (root cause de que hoy esté deshabilitado — `WsButton.enabled: !buttonRoot.isLabel`) y agregar `onClicked` real:
```qml
WsButton {
    text: "BORRAR"
    width: 92
    enabled: borrarAreaDisplayLoader.model.length > 0
    onClicked: {
        var areaId = parseInt(borrarAreaDisplayLoader.currentText)
        if (!isNaN(areaId)) {
            BackendService.deleteArea(areaId)
        }
    }
}
```
(Ajustar nombres exactos de los `id` de los Loaders/dropdowns según lo que ya exista en el bloque actual de `FigurasWorkspace.qml` al extraerlo — no inventar nombres nuevos si ya hay unos definidos.)

### 8. `FigurasWorkspace.qml`

Reemplazar el bloque `Rectangle { ... }` de AREA (líneas ~79-225) por:
```qml
AreaPanel {
    visible: vm.selectedMenuIndex === 2
    // mismos anchors/Layout que tenía el Rectangle original
    viewModel: vm
}
```

## Fuera de alcance

- SECTOR y POLIGONO.
- `list_shapes` para poblar EDITAR/BORRAR de AREA con datos reales (y arreglar el mismo bug ya presente en CIRCULO) — mejora futura.
- Arreglar el bug de `buildDeleteLineCommand`/`buildDeleteCircleCommand` mandando `id` como string — no forma parte de AREA, solo hay que evitar repetirlo.
- Rango real de validación LONG/LAT — no hay spec de backend; el rango en el plan es genérico.
- Conversión real de lat/long a coordenadas cartesianas antes de armar el payload `points` — se agrega en un cambio posterior; por ahora `long`→`x` y `lat`→`y` se mandan tal cual.

## Verificación al terminar

1. Compilar el cliente (`tdc-botonera/botonera`) y confirmar que no rompe LINEA/CIRCULO existentes.
2. Levantar la app, ir a Figuras → AREA, cargar 4 puntos LONG/LAT válidos + tipo + color, tocar "Iniciar".
3. Del lado del servidor (repo `DDM`), confirmar que llega `create_area` con `"points"` = array de 4 `{x,y}` (con `x` = long, `y` = lat, sin conversión geográfica aún) y que responde `{"created_id": N}`.
4. Probar con campos inválidos/incompletos y confirmar que la UI bloquea el envío (validación `acceptableInput`).
5. Probar `BackendService.deleteArea(id)` con un ID conocido (ya que el dropdown queda vacío por diseño) y confirmar que el servidor responde `{"deleted_id": N}`.
