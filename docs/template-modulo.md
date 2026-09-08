# Requerimientos: [Nombre del Módulo]

---

## Información del documento


| Campo                     | Detalle    |
| ------------------------- | ---------- |
| **Autor**                 |            |
| **Creado**                | YYYY-MM-DD |
| **Estado**                | Borrador   |
| **Versión**              | 0.1        |
| **Última modificación** | YYYY-MM-DD |
| **Modificado por**        |            |
| **Revisores**             | —         |
| **Aprobado por**          | —         |

### Historial de cambios


| Versión | Fecha      | Autor | Descripción del cambio         |
| -------- | ---------- | ----- | ------------------------------- |
| 0.1      | YYYY-MM-DD |       | Creación inicial del documento |

---

## 1. Descripción general

<!-- Qué es este módulo en una o dos oraciones. A qué familia de herramientas/primitivas pertenece. -->

---

## 2. Responsabilidades

### 2.1 Qué hace este módulo

- <!-- bullet: responsabilidad principal -->
- <!-- bullet: ofrece métodos / expone datos -->
- <!-- bullet: reacciona a eventos o actualizaciones de otros módulos -->
- <!-- bullet: serializa / deserializa si aplica -->

### 2.2 Qué NO hace este módulo

- <!-- bullet: qué delega en otro módulo -->
- <!-- bullet: qué no valida o no persiste -->

---

## 3. Integración con el sistema

### 3.1 Módulos que usa

| Módulo / Clase | Rol en la interacción |
| --------------- | --------------------- |
|                 |                       |

### 3.2 Módulos que usan este

| Módulo / Clase | Cómo lo usa |
| --------------- | ------------ |
|                 |             |

### 3.3 Diagrama de relaciones

```
<!-- Diagrama ASCII de cómo fluyen señales/llamadas entre este módulo y los relacionados -->
```

---

## 4. Figura / Representación visual *(omitir si no aplica)*

```
<!-- Diagrama ASCII de la figura geométrica o visual que representa el módulo -->
```

<!-- Descripción de convenciones de unidades, ejes, orientación -->

---

## 5. Parámetros de definición


| Campo | Tipo | Unidad | Descripción |
| ----- | ---- | ------ | ----------- |
|       |      |        |             |

### 5.1 Restricciones de validación

- <!-- regla de validación 1 -->
- <!-- regla de validación 2 -->

---

## 6. Modelo de datos

```cpp
// Enums y structs relevantes
```

---

## 7. API pública de la clase

```cpp
class NombreClase : public QWidget {  // ajustar clase base
public:
    // método 1
    // método 2
};
```

> <!-- Nota sobre qué parte de la API reside en otro módulo (manager, dispatcher, etc.) -->

---

## 8. Ciclo de vida

### 8.1 Creación

```
<!-- Flujo texto/ASCII desde acción de usuario hasta efecto en escena -->
```

### 8.2 Edición

```
<!-- Flujo texto/ASCII -->
```

### 8.3 Eliminación

```
<!-- Flujo texto/ASCII -->
```

---

## 9. Interfaz de usuario (Qt Widget) *(omitir si no tiene UI propia)*

### 9.1 Campos de texto (QLineEdit)

| Campo UI | Variable interna | Formato | Validación |
| -------- | ---------------- | ------- | ---------- |
|          |                  |         |            |

### 9.2 Selectores desplegables (QComboBox) *(si aplica)*

| Campo UI | Variable interna | Valores posibles |
| -------- | ---------------- | ---------------- |
|          |                  |                  |

### 9.3 Botones de acción (QPushButton)

| Botón | Acción |
| ----- | ------ |
|       |        |

---

## 10. Renderizado en escena *(omitir si no tiene representación gráfica)*

### 10.1 Algoritmo de construcción

```
<!-- Pasos del algoritmo de renderizado -->
```

### 10.2 Transformación de coordenadas

<!-- Cómo se convierten unidades de dominio (NM, grados) a píxeles de escena -->

### 10.3 Apariencia visual

| Propiedad | Valor por defecto | Configurable |
| --------- | ----------------- | ------------ |
|           |                   |              |

---

## 11. Comportamiento especial *(asociación a track, seguimiento, etc.)*

<!-- Comportamiento dinámico que no es creación/edición/eliminación simple -->

---

## 12. Persistencia *(omitir si no aplica)*

```json
{
  // estructura JSON mínima para serialización
}
```

---

## 13. Señales y slots Qt relevantes

```cpp
signals:
    // señales que emite este módulo

slots:
    // slots que expone para recibir eventos externos
```

---

## 14. Casos de borde


| Caso | Comportamiento esperado |
| ---- | ----------------------- |
|      |                         |

---

## 15. Dependencias y notas de implementación

- **Framework:** Qt 5.x / 6.x
- **Módulos Qt requeridos:** `QtWidgets`, `QtGui`, `QtCore`
- **Clase base sugerida:**
- <!-- notas técnicas adicionales -->

---
