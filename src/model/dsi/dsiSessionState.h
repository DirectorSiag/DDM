#pragma once

#include <QList>
#include "dsiEntity.h"

// ─────────────────────────────────────────────────────────────────────────
// DSISessionState — Contenedor de sesión del módulo DSI.
//
// Vive dentro de CommandContext, igual que textSession y haSessions.
// Igual que Texto, sin límite máximo de zonas simultáneas — se confirmó
// (por analogía, pendiente de ratificar con el equipo) que el operador es
// responsable de gestionar la cantidad de DSI en pantalla.
// ─────────────────────────────────────────────────────────────────────────

struct DSISessionState {

    QList<DSIEntity> zones;

    int nextTn = 1;   // contador incremental para asignar TN a nuevas DSI

    DSIEntity* findByTn(int tn) {
        for (DSIEntity& zone : zones) {
            if (zone.tn == tn) return &zone;
        }
        return nullptr;
    }

    const DSIEntity* findByTn(int tn) const {
        for (const DSIEntity& zone : zones) {
            if (zone.tn == tn) return &zone;
        }
        return nullptr;
    }

    bool removeByTn(int tn) {
        for (auto it = zones.begin(); it != zones.end(); ++it) {
            if (it->tn == tn) {
                zones.erase(it);
                return true;
            }
        }
        return false;
    }

    void reset() {
        zones.clear();
        nextTn = 1;
    }
};
