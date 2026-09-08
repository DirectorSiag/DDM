# Plan: conectar la herramienta AREA a comandos reales de backend

## Contexto

`FigurasWorkspace.qml` (índice de menú 2, bloque `visible: vm.selectedMenuIndex === 2`, líneas ~79-225) es hoy una maqueta estática: los `DataInput` de los vértices A/B/C/D no tienen `id` ni validación (solo texto placeholder `"-- ° -- ´ -- . -- ´´ S/W"`), y los botones `IniciarButton {}` / `ResetButton {}` están vacíos, sin `onClicked`.

LINEA y CIRCULO (índices 0 y 1) sí están conectados de punta a punta, vía `LineaPanel.qml` / `CirculoPanel.qml` → `BackendService.qml` (singleton QML) → `DDMController` (`Q_INVOKABLE` en C++) → `JsonCommandBuilder` (arma el JSON `{"command": ..., "args": {...}}`) → señal `sendBackendCommand` → transporte real. Las respuestas vuelven por `DDMController::handleBackendResponse`, que usa `JsonResponseParser` y un mapa `command -> status -> handler`.

**Confirmado con el usuario:**
- El backend real **ya tiene implementado** `create_area`, siguiendo el mismo flujo que `create_line`/`create_circle`.
- Los vértices se envían en **coordenadas cartesianas x/y** (no lat/lon).
- No existe en el repo ninguna función de conversión lat/lon → x/y (ni siquiera LINEA/CIRCULO la usan: hoy mandan `x=0, y=0` fijo). Por lo tanto, los campos de UI para los vértices A/B/C/D pasan de LAT/LON a **X/Y directos**.
- El comando espera **4 vértices fijos** (x,y cada uno) + `type` + `color`.

Lo que sigue es el diseño para que AREA quede wireada igual que LINEA/CIRCULO. **Nada de esto se implementó todavía** — es para revisar contra el contrato real del backend antes de tocar código.

---

## 1. Backend cliente (C++)

### `src/controller/json/jsoncommandbuilder.h` / `.cpp`

Agregar, siguiendo el estilo de `buildCreateLineCommand` / `buildCreateCircleCommand`:

```cpp
// .h
QByteArray buildCreateAreaCommand(double x1, double y1,
                                   double x2, double y2,
                                   double x3, double y3,
                                   double x4, double y4,
                                   int type, const QString& color) const;
QByteArray buildDeleteAreaCommand(const QString& areaId) const;
```

```cpp
// .cpp
QByteArray JsonCommandBuilder::buildCreateAreaCommand(double x1, double y1,
                                                        double x2, double y2,
                                                        double x3, double y3,
                                                        double x4, double y4,
                                                        int type, const QString& color) const
{
    QJsonObject args;
    args["x1"] = x1; args["y1"] = y1;
    args["x2"] = x2; args["y2"] = y2;
    args["x3"] = x3; args["y3"] = y3;
    args["x4"] = x4; args["y4"] = y4;
    args["type"] = type;
    args["color"] = color;

    return buildCommand("create_area", args);
}

QByteArray JsonCommandBuilder::buildDeleteAreaCommand(const QString& areaId) const
{
    QJsonObject args;
    args["id"] = areaId;
    return buildCommand("delete_area", args);
}
```

> ⚠️ **A confirmar contra el backend real**: nombres exactos de los campos de vértices (`x1..y4` vs. un array `"vertices": [{x,y}, ...]`) y si `type`/`color` van así o con otro nombre. El usuario indicó "4 vértices fijos (x,y cada uno) + type/color", así que este es el formato asumido — hay que chequearlo contra la implementación real del servidor antes de codear.

### `src/controller/json/jsonresponseparser.h` / `.cpp`

Mirroring `LineData` / `extractCreateLineData` / `extractDeleteLineData` / `extractLineIds`:

```cpp
// .h
struct AreaData {
    QString id;
    QStringList allAreaIds;
};

QStringList extractAreaIds(const QJsonArray& areasArray) const;
AreaData extractCreateAreaData(const QJsonObject& args) const;
AreaData extractDeleteAreaData(const QJsonObject& args) const;
```

```cpp
// .cpp
QStringList JsonResponseParser::extractAreaIds(const QJsonArray& areasArray) const
{
    QStringList areaIds;
    for (const QJsonValue& areaValue : areasArray) {
        if (areaValue.isObject()) {
            QString areaId = areaValue.toObject().value("id").toString();
            if (!areaId.isEmpty()) areaIds.append(areaId);
        }
    }
    return areaIds;
}

JsonResponseParser::AreaData JsonResponseParser::extractCreateAreaData(const QJsonObject& args) const
{
    AreaData data;
    data.id = args.value("created_id").toString();
    data.allAreaIds = extractAreaIds(args.value("areas").toArray());
    return data;
}

JsonResponseParser::AreaData JsonResponseParser::extractDeleteAreaData(const QJsonObject& args) const
{
    AreaData data;
    data.id = args.value("deleted_id").toString();
    data.allAreaIds = extractAreaIds(args.value("areas").toArray());
    return data;
}
```

> ⚠️ **Supuesto a verificar**: que la respuesta de `create_area`/`delete_area` traiga `created_id`/`deleted_id` + un array `"areas"` de objetos `{id: ...}`, igual que hace `create_line`/`delete_line` con `"lines"`. Si el backend responde distinto (por ejemplo sin lista de áreas, como pasa hoy con `create_circle`, que no mantiene `circlesList` y solo emite `circleCreated`), hay que simplificar y no mantener `areasList` — ver alternativa en la sección 5.

### `src/controller/protocol/ddmcontroller.h` / `.cpp`

```cpp
// .h
Q_PROPERTY(QStringList areasList READ areasList NOTIFY areasListChanged)
...
QStringList areasList() const;
void setAreasList(const QStringList &areas);

Q_INVOKABLE void createArea(double x1, double y1, double x2, double y2,
                             double x3, double y3, double x4, double y4,
                             int type, const QString& color);
Q_INVOKABLE void deleteArea(const QString& areaId);

signals:
    void areasListChanged();
    void areaCreated(const QString& areaId, bool success);
    void areaDeleted(const QString& areaId, bool success);

private:
    void handleCreateAreaSuccess(const QJsonObject& args);
    void handleCreateAreaError(const QJsonObject& args);
    void handleDeleteAreaSuccess(const QJsonObject& args);
    void handleDeleteAreaError(const QJsonObject& args);

    QStringList m_areasList;
```

```cpp
// .cpp — análogo a createLine/deleteLine
void DDMController::createArea(double x1, double y1, double x2, double y2,
                                double x3, double y3, double x4, double y4,
                                int type, const QString& color)
{
    QByteArray command = m_commandBuilder.buildCreateAreaCommand(x1, y1, x2, y2, x3, y3, x4, y4, type, color);
    emit sendBackendCommand(command);
}

void DDMController::deleteArea(const QString& areaId)
{
    QByteArray command = m_commandBuilder.buildDeleteAreaCommand(areaId);
    emit sendBackendCommand(command);
}

void DDMController::handleCreateAreaSuccess(const QJsonObject& args)
{
    JsonResponseParser::AreaData areaData = m_responseParser.extractCreateAreaData(args);
    if (!areaData.id.isEmpty()) {
        setAreasList(areaData.allAreaIds);
        emit areaCreated(areaData.id, true);
    } else {
        emit areaCreated("", false);
    }
}
// handleCreateAreaError / handleDeleteAreaSuccess / handleDeleteAreaError: mismo patrón que las de línea
```

Y registrar en el mapa de `handleBackendResponse`:

```cpp
{"create_area", {
    {"success", &DDMController::handleCreateAreaSuccess},
    {"error", &DDMController::handleCreateAreaError}
}},
{"delete_area", {
    {"success", &DDMController::handleDeleteAreaSuccess},
    {"error", &DDMController::handleDeleteAreaError}
}},
```

---

## 2. Puente QML

### `DDM/core/BackendService.qml`

```qml
property var areasList: _ctrl ? _ctrl.areasList : []

function createArea(x1, y1, x2, y2, x3, y3, x4, y4, type, color) {
    if (_ctrl) _ctrl.createArea(x1, y1, x2, y2, x3, y3, x4, y4, type, color)
}
function deleteArea(areaId) { if (_ctrl) _ctrl.deleteArea(areaId) }
```

### `DDM/workspaces/superficie/figuras/FigurasViewModel.qml`

```qml
// Returns the current area list or an empty list.
function areasModelOrEmpty() {
    return BackendService.areasList
}
```

---

## 3. UI — nuevo `AreaPanel.qml`

Extraer el bloque AREA (hoy inline en `FigurasWorkspace.qml`) a `DDM/workspaces/superficie/figuras/AreaPanel.qml`, siguiendo la misma estructura de `LineaPanel.qml` / `CirculoPanel.qml` (prop `viewModel`, borde `Constants.colorNaranja`, etc.), en vez de seguir agrandando `FigurasWorkspace.qml`.

Cambios respecto al mockup actual:

- Los 8 `DataInput` de A/B/C/D pasan de LAT/LON a **X/Y** (etiquetas `WsButton` "X"/"Y" en vez de "LAT"/"LON"), cada uno con `id` propio (`xAInput`, `yAInput`, `xBInput`, `yBInput`, `xCInput`, `yCInput`, `xDInput`, `yDInput`) y `DoubleValidator` (rango genérico a definir, ej. `-99999.9` a `99999.9`, 1 decimal — ajustar si hay un rango real de coordenadas).
- Los dropdowns TIPO/COLOR ya están bien conectados (`vm.lineTypesModel` / `vm.lineColorsModel`, reutilizados de LINEA) — no cambian.
- `IniciarButton.onClicked` (hoy vacío): validar `acceptableInput` de los 8 campos, parsear con `replace(",", ".")` + `parseFloat`, tomar `type` (`currentIndex`) y `color` (`currentText`), llamar:
  ```qml
  BackendService.createArea(x1, y1, x2, y2, x3, y3, x4, y4, type, color)
  ```
  limpiar los 8 campos y setear `viewModel.statusMessage`, igual que `LineaPanel.qml:112-140`.
- Dropdowns EDITAR/BORRAR (`editarAreaDisplayLoader`, `borrarAreaDisplayLoader`): hoy apuntan (erróneamente) a `vm.linesModelOrEmpty()` — corregir a `vm.areasModelOrEmpty()`.
- Botón BORRAR (hoy sin `onClicked`): agregar handler que llama `BackendService.deleteArea(borrarAreaDisplayLoader.currentText)`, igual que `LineaPanel.qml:90-102`.
- `ResetButton {}` (opcional, hoy vacío en el mockup): limpiar los 8 campos y resetear dropdowns a `currentIndex = 0`.

### `FigurasWorkspace.qml`

Reemplazar el bloque `Rectangle { visible: vm.selectedMenuIndex === 2; ... }` (líneas ~79-225) por:

```qml
AreaPanel {
    visible: vm.selectedMenuIndex === 2
    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.margins: 4
    viewModel: vm
}
```

---

## 4. Fuera de alcance

- **SECTOR** (índice 3) y **POLIGONO** (índice 4): quedan sin tocar. POLIGONO no tiene un número fijo de vértices (hoy es un `Repeater` sobre `["A","B","C","D","E"]`), lo que requeriría una decisión aparte sobre el formato de comando (array de vértices variable). SECTOR es hoy un rectángulo vacío sin ningún campo.

## 5. Puntos a validar contra el backend real antes de implementar

1. Nombre y forma exacta de los argumentos de `create_area` (¿`x1..y4` sueltos o `vertices: [{x,y}]`?).
2. Nombre y forma de `delete_area`.
3. ¿La respuesta de `create_area`/`delete_area` incluye una lista completa de áreas vigentes (como pasa con `"lines"` en `create_line`)? Si no, hay que simplificar `DDMController`/`BackendService` para no mantener `areasList` y resolver EDITAR/BORRAR de otra forma (o dejarlos deshabilitados, como pasa hoy con CIRCULO que reutiliza por error `linesModelOrEmpty()` porque no tiene lista propia).
4. Rango/unidad reales de X/Y para poner un `DoubleValidator` sensato (hoy es un placeholder genérico).
5. ¿`type` es el mismo dominio que usa LINEA (`lineTypesModel`, 8 valores "1".."8") o AREA tiene su propio catálogo de tipos en el backend?
