# Módulo: Sector

---

## Información del documento


| Campo                     | Detalle       |
| ------------------------- | ------------- |
| **Autor**                 | Cristian Siva |
| **Creado**                | 2026-05-15    |
| **Estado**                | Borrador      |
| **Versión**              | 0.2           |
| **Última modificación** | 2026-05-16    |
| **Modificado por**        | Cristian Siva |
| **Revisores**             | —            |
| **Aprobado por**          | —            |

### Historial de cambios


| Versión | Fecha      | Autor         | Descripción del cambio                                              |
| -------- | ---------- | ------------- | -------------------------------------------------------------------- |
| 0.1      | 2026-05-15 | Cristian Siva | Creación inicial del documento                                      |
| 0.2      | 2026-05-16 | Cristian Siva | Reenfoque: módulo lógico puro; se eliminan secciones de UI y render |

---

## 1. Descripción general

El **Sector** (`sector.cpp` / `sector.h`) es la representación lógica de la figura sector anular. Encapsula los datos que definen la figura (azimuts, radios, color, tipo, track asociado, posición de origen) y expone métodos para consultar y modificar su estado interno.

Esta clase no tiene responsabilidades de visualización ni de interacción con el usuario. Su rol es mantener el estado de la figura, responder a actualizaciones de posición del track asociado, y poder serializarse a/desde JSON para persistencia.

---

## 2. Responsabilidades

### 2.1 Qué hace este módulo

- Encapsula la información que define un sector anular (azimuts, radios, color, tipo, track asociado, origen).
- Ofrece métodos para consultar y modificar el estado interno (getters y setters).
- Actualiza su posición de origen cuando el track asociado se mueve, vía signal/slot.
- Serializa y deserializa su estado a/desde JSON.

### 2.2 Qué NO hace este módulo

- No renderiza ni dibuja nada; la visualización es responsabilidad del sistema AR-TDC.
- No interactúa con el usuario ni con widgets Qt.
- No enruta comandos ni gestiona el ciclo de vida de otros sectores.
- No gestiona el ciclo de vida de los tracks; solo recibe actualizaciones de posición vía slot.
- No valida reglas de negocio más allá de las restricciones geométricas (§5.1).

---

## 3. Integración con el sistema

### 3.1 Módulos que usa

| Módulo / Clase | Rol en la interacción                                            |
| --------------- | ---------------------------------------------------------------- |
| `Track`         | Se suscribe a actualizaciones de posición del track asociado    |

### 3.2 Módulos que usan este

| Módulo / Clase   | Cómo lo usa                                                        |
| ----------------- | ------------------------------------------------------------------- |
| `CommandContext`  | Instancia sectores y les asigna IDs al procesar comandos           |
| `Track`           | Se asocia a un Sector para mantener ambos coordinados en posición  |

### 3.3 Diagrama de relaciones

```
  ┌──────────────────┐   crea / destruye   ┌────────────────┐
  │  CommandContext   │ ───────────────────► │    Sector      │
  └──────────────────┘                      └───────┬────────┘
                                                    │ emite señales
                                                    │ de cambio de estado
  ┌──────────────────┐   posición           │
  │     Track        │ ────────────────────►│ slot: onTrackActualizado()
  └──────────────────┘                      └────────────────┘
```

---

## 4. Figura geométrica

```
          N (0°)
          |
   AZ.IZQ |  AZ.DER
      \    |    /
       \   |   /
        \  |  /   ← apertura angular
         \ | /
          \|/
    -------+-------  (origen / posición del sistema)
          /|\
         / | \
        /  |  \
       /   |   \    RAD.INT = radio interior
      /    |    \
     /_____|_____\   RAD.EXT = radio exterior
```

La figura representada es un **sector anular** (arco de corona circular): el área delimitada por dos azimuts y dos radios con origen en la posición del sistema.

- Los azimuts se miden en **grados desde el Norte, en sentido horario** (convención náutica/radar).
- Los radios se expresan en **millas náuticas (NM)**.
- La apertura angular es `AZ.DER - AZ.IZQ` (módulo 360°).

---

## 5. Parámetros de definición


| Campo      | Tipo     | Unidad            | Descripción                                                                                       |
| ---------- | -------- | ----------------- | -------------------------------------------------------------------------------------------------- |
| `AZ_IZQ`   | `double` | grados (0–359.9) | Azimut verdadero/relativo del límite izquierdo del sector                                         |
| `AZ_DER`   | `double` | grados (0–359.9) | Azimut verdadero/relativo del límite derecho del sector                                           |
| `RAD_INT`  | `double` | millas náuticas  | Radio interior del sector anular. `0.0` = el sector va desde el origen                            |
| `RAD_EXT`  | `double` | millas náuticas  | Radio exterior del sector anular. Debe ser `> RAD_INT`                                            |
| `TIPO`     | `enum`   | —                | Tipo de sector (e.g. ZONA_EXCLUSION, ZONA_ALERTA, PERSONALIZADO)                                  |
| `COLOR`    | `enum`   | —                | Color de relleno y borde. Seis valores: RGB1, RGB2, RGB3, CMYK1, CMYK2, CMYK3                     |
| `ID_TRACK` | `int`    | —                | Identificador del track al que se asocia el sector (opcional; `0` = sin asociación)               |

### 5.1 Restricciones de validación

- `RAD_EXT > RAD_INT >= 0.0`
- `AZ_IZQ` y `AZ_DER` en `[0.0, 360.0)`. Si `AZ_IZQ == AZ_DER`, el sector es de 360° (corona completa).
- Si `AZ_DER < AZ_IZQ`, la apertura cruza el norte (e.g., AZ_IZQ=350, AZ_DER=010 → apertura de 20°).
- `RAD_EXT` no puede ser `0.0`.

---

## 6. Modelo de datos

```cpp
enum class SectorTipo {
    ZonaAlerta, // es el unico tipo de sector que se destaco pero podrian haber mas
};

enum class SectorColor {
    RGB1, RGB2, RGB3,
    CMYK1, CMYK2, CMYK3
};

struct SectorData {
    int         id;          // Identificador único, asignado por el sistema
    double      az_izq;      // Azimut izquierdo [0.0, 360.0)
    double      az_der;      // Azimut derecho   [0.0, 360.0)
    double      rad_int;     // Radio interno en NM (>= 0.0)
    double      rad_ext;     // Radio externo en NM (> rad_int)
    //SectorTipo  tipo;        // este se obvia por el momento.
    SectorColor color;
    int         id_track;    // 0 = sin asociación
    QPointF     origen;      // Posición geográfica del origen (lat/lon o coords de display)
};
```

---

## 7. API pública de la clase

```cpp
class Sector : public QObject {
public:
    explicit Sector(const SectorData& data, QObject* parent = nullptr);

    // Getters
    int         id()      const;
    double      azIzq()   const;
    double      azDer()   const;
    double      radInt()  const;
    double      radExt()  const;
    SectorColor color()   const;
    int         idTrack() const;
    QPointF     origen()  const;

    // Setters — emiten señal datosModificados() al cambiar
    void setAzIzq(double v);
    void setAzDer(double v);
    void setRadInt(double v);
    void setRadExt(double v);
    void setColor(SectorColor c);
    void setIdTrack(int id);

    // Actualiza la posición de origen del sector
    void update(QPointF nuevaOrigen);

    // Serialización
    QJsonObject toJson() const;
    static Sector* fromJson(const QJsonObject& obj, QObject* parent = nullptr);
};
```

---

## 8. Ciclo de vida

### 8.1 Creación

```
CommandContext procesa comando de creación
        │
        ▼
Valida datos (restricciones §5.1)
        │  datos válidos
        ▼
new Sector(SectorData)   ← asigna ID único
        │
        ▼
Sector queda disponible para quien lo gestione (manager a definir)
```

### 8.2 Edición

```
Manager busca el Sector por id
        │
        ▼
Llama setters correspondientes (setAzIzq, setRadExt, etc.)
        │
        ▼
Sector emite datosModificados()
        │
        ▼
Quien escuche la señal reacciona (e.g. actualiza visualización)
```

### 8.3 Eliminación

```
Manager recibe orden de eliminar sector por id
        │
        ▼
Manager destruye la instancia Sector
        │
        ▼
Quienes estaban conectados a sus señales quedan desconectados automáticamente (Qt)
```

---

## 9. Comportamiento de asociación a track

- Un sector almacena el `id_track` del track al que está asociado (`0` = sin asociación).
- Cuando el track asociado cambia de posición, emite una señal; el Sector recibe esa actualización vía `onTrackActualizado()` y actualiza su `origen`.
- Si el track se elimina, el sector conserva la última posición conocida; no se elimina automáticamente.

---

## 10. Persistencia

```json
{
  "id": 1335,
  "az_izq": 350.0,
  "az_der": 10.0,
  "rad_int": 5.0,
  "rad_ext": 8.0,
  "tipo": "ZonaVigilancia",
  "color": "RGB1",
  "id_track": 1335,
  "origen": { "lat": -38.870, "lon": -62.082 }
}
```

---

## 11. Señales y slots Qt

```cpp
signals:
    void datosModificados();           // cualquier setter modificó el estado
    void posicionActualizada(QPointF); // origen cambió por actualización de track

slots:
    void onTrackActualizado(int id_track, QPointF nueva_pos); // desde Track
```

---

## 12. Casos de borde


| Caso                 | Comportamiento esperado                                                              |
| -------------------- | ------------------------------------------------------------------------------------- |
| AZ_IZQ == AZ_DER     | Sector de 360° (corona completa, sin límites angulares)                             |
| AZ_DER < AZ_IZQ      | La apertura cruza el norte; los datos son válidos, el consumidor debe manejarlo     |
| RAD_INT == 0.0       | Sector sólido (sin agujero), tipo pizza; geométricamente válido                     |
| ID_TRACK inexistente | El sector conserva su `origen` actual; loguear advertencia, no lanzar excepción     |

---

## 13. Dependencias y notas de implementación

- **Framework:** Qt 5.x / 6.x (confirmar versión con el equipo)
- **Módulos Qt requeridos:** `QtCore` (QObject, QPointF, QJsonObject)
- La clase hereda de `QObject` para usar el mecanismo de signals/slots; no hereda de ninguna clase gráfica.

---
