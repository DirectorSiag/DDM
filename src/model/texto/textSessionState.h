#pragma once

#include <QList>
#include "textLabel.h"

// ─────────────────────────────────────────────────────────────────────────
// TextSessionState — Contenedor de sesión del módulo Texto.
//
// Vive dentro de CommandContext, igual que TwoWSessionState y haSessions.
// A diferencia de HA (límite de 10 slots), Texto NO tiene límite máximo
// de etiquetas simultáneas — se confirmó con el equipo que los operadores
// son responsables de gestionar la cantidad de textos en pantalla.
//
// Por eso usa una lista dinámica en lugar de un array de tamaño fijo.
// ─────────────────────────────────────────────────────────────────────────

struct TextSessionState {

    QList<TextLabel> labels;

    int nextTn = 1;   // contador incremental para asignar TN a nuevos textos

    // Busca un label por su TN. Devuelve nullptr si no existe.
    TextLabel* findByTn(int tn) {
        for (TextLabel& label : labels) {
            if (label.tn == tn) return &label;
        }
        return nullptr;
    }

    const TextLabel* findByTn(int tn) const {
        for (const TextLabel& label : labels) {
            if (label.tn == tn) return &label;
        }
        return nullptr;
    }

    bool removeByTn(int tn) {
        for (auto it = labels.begin(); it != labels.end(); ++it) {
            if (it->tn == tn) {
                labels.erase(it);
                return true;
            }
        }
        return false;
    }

    void reset() {
        labels.clear();
        nextTn = 1;
    }
};
