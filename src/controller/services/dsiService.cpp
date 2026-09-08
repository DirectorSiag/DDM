#include "dsiService.h"
#include <QtMath>
#include <cmath>

DSIService::DSIService(CommandContext* ctx)
    : m_ctx(ctx)
{}

// ── Helpers de conversión string <-> enum (públicos, reutilizables) ──────

DSIColor DSIService::stringToColor(const QString& s, bool& ok) {
    ok = true;
    const QString key = s.trimmed().toLower();
    if (key == QStringLiteral("rojo"))     return DSIColor::Rojo;
    if (key == QStringLiteral("verde"))    return DSIColor::Verde;
    if (key == QStringLiteral("azul"))     return DSIColor::Azul;
    if (key == QStringLiteral("cian"))     return DSIColor::Cian;
    if (key == QStringLiteral("magenta"))  return DSIColor::Magenta;
    if (key == QStringLiteral("amarillo")) return DSIColor::Amarillo;
    if (key == QStringLiteral("blanco"))   return DSIColor::Blanco;
    if (key == QStringLiteral("purpura"))  return DSIColor::Purpura;
    if (key == QStringLiteral("naranja"))  return DSIColor::Naranja;
    ok = false;
    return DSIColor::Blanco;
}

DSILineStyle DSIService::stringToLineStyle(const QString& s, bool& ok) {
    ok = true;
    const QString key = s.trimmed().toLower();
    if (key == QStringLiteral("continua"))       return DSILineStyle::Continua;
    if (key == QStringLiteral("segmentada"))     return DSILineStyle::Segmentada;
    if (key == QStringLiteral("punteada"))       return DSILineStyle::Punteada;
    if (key == QStringLiteral("puntoraya"))      return DSILineStyle::PuntoRaya;
    if (key == QStringLiteral("puntorayaraya"))  return DSILineStyle::PuntoRayaRaya;
    if (key == QStringLiteral("doble"))          return DSILineStyle::DobleLinea;
    if (key == QStringLiteral("ondulada"))       return DSILineStyle::Ondulada;
    if (key == QStringLiteral("gruesa"))         return DSILineStyle::Gruesa;
    ok = false;
    return DSILineStyle::Continua;
}

// ── Helpers de conversión enum -> string (privados, para reportes) ───────

QString DSIService::colorToString(DSIColor color) {
    switch (color) {
        case DSIColor::Rojo:     return QStringLiteral("ROJO");
        case DSIColor::Verde:    return QStringLiteral("VERDE");
        case DSIColor::Azul:     return QStringLiteral("AZUL");
        case DSIColor::Cian:     return QStringLiteral("CIAN");
        case DSIColor::Magenta:  return QStringLiteral("MAGENTA");
        case DSIColor::Amarillo: return QStringLiteral("AMARILLO");
        case DSIColor::Blanco:   return QStringLiteral("BLANCO");
        case DSIColor::Purpura:  return QStringLiteral("PURPURA");
        case DSIColor::Naranja:  return QStringLiteral("NARANJA");
    }
    return QStringLiteral("BLANCO");
}

QString DSIService::lineStyleToString(DSILineStyle style) {
    switch (style) {
        case DSILineStyle::Continua:      return QStringLiteral("CONTINUA");
        case DSILineStyle::Segmentada:    return QStringLiteral("SEGMENTADA");
        case DSILineStyle::Punteada:      return QStringLiteral("PUNTEADA");
        case DSILineStyle::PuntoRaya:     return QStringLiteral("PUNTO_RAYA");
        case DSILineStyle::PuntoRayaRaya: return QStringLiteral("PUNTO_RAYA_RAYA");
        case DSILineStyle::DobleLinea:    return QStringLiteral("DOBLE");
        case DSILineStyle::Ondulada:      return QStringLiteral("ONDULADA");
        case DSILineStyle::Gruesa:        return QStringLiteral("GRUESA");
    }
    return QStringLiteral("CONTINUA");
}

QString DSIService::positionMethodToString(DSIPositionMethod method) {
    switch (method) {
        case DSIPositionMethod::Manual:     return QStringLiteral("MAN");
        case DSIPositionMethod::AzimutDist: return QStringLiteral("AZ/DT");
        case DSIPositionMethod::LatLon:     return QStringLiteral("LAT/LON");
    }
    return QStringLiteral("MAN");
}

// ── Crear ────────────────────────────────────────────────────────────────

DSIOperationResult DSIService::createDSI(const DSICreateParams& params) {
    if (params.labelText.length() > 20) {
        return { false, QStringLiteral("[DSI] El texto no puede superar los 20 caracteres.") };
    }
    if (params.radiusDm <= 0.0) {
        return { false, QStringLiteral("[DSI] El radio debe ser mayor a 0.") };
    }

    double xDm = 0.0, yDm = 0.0;

    switch (params.positionMethod) {

    case DSIPositionMethod::Manual: {
        xDm = params.manXDm;
        yDm = params.manYDm;
        break;
    }

    case DSIPositionMethod::AzimutDist: {
        const Track* bp = m_ctx->findTrackById(0);
        if (!bp) {
            return { false, QStringLiteral("[DSI] Error: no se encontro el Buque Propio.") };
        }
        const QPointF bpPos(bp->getX(), bp->getY());
        const QPointF resolved = DSICalculator::resolveFromBearing(
            bpPos, params.azimuthDeg, params.distanceDm,
            params.useVerdadero, bp->getCourseDeg()
        );
        xDm = resolved.x();
        yDm = resolved.y();
        break;
    }

    case DSIPositionMethod::LatLon: {
        const bool bpHasGeo = m_ctx->ownShip.valid &&
                              !(m_ctx->ownShip.latitudeDeg == 0.0 &&
                                m_ctx->ownShip.longitudeDeg == 0.0);
        if (!bpHasGeo) {
            return { false, QStringLiteral(
                "[DSI] LAT/LON no disponible: el BP no tiene coordenadas geograficas reales.") };
        }
        const QPointF resolved = DSICalculator::resolveFromLatLon(
            m_ctx->ownShip.latitudeDeg, m_ctx->ownShip.longitudeDeg,
            params.latDecimal, params.lonDecimal
        );
        xDm = resolved.x();
        yDm = resolved.y();
        break;
    }
    }

    DSIEntity zone;
    zone.tn              = m_ctx->dsiSession.nextTn++;
    zone.labelText        = params.labelText.isEmpty()
                                 ? QStringLiteral("DSI %1MN").arg(params.radiusDm, 0, 'f', 0)
                                 : params.labelText;
    zone.radiusDm          = params.radiusDm;
    zone.lineStyle         = params.lineStyle;
    zone.labelColorSet      = params.hasLabelColor;
    zone.labelColor         = params.labelColor;
    zone.labelBackgroundSet = params.hasLabelBackground;
    zone.labelBackground    = params.labelBackground;
    zone.positionMethod    = params.positionMethod;
    zone.baseXDm           = xDm;
    zone.baseYDm           = yDm;
    zone.currentXDm        = xDm;
    zone.currentYDm        = yDm;

    if (params.asociarAlCrear) {
        const Track* track = m_ctx->findTrackById(params.trackToAssociate);
        if (!track) {
            return { false, QStringLiteral("[DSI] El track a asociar %1 no existe.")
                                .arg(params.trackToAssociate) };
        }
        zone.associated        = true;
        zone.associatedTrackId = params.trackToAssociate;
        // Adopción directa: el centro pasa a ser la posición del track ya
        // mismo, no la posición resuelta en la Sección B.
        zone.currentXDm = track->getX();
        zone.currentYDm = track->getY();
    }

    m_ctx->dsiSession.zones.append(zone);

    return { true, QStringLiteral("[DSI] Zona creada con TN %1.").arg(zone.tn) };
}

// ── Editar ───────────────────────────────────────────────────────────────
// Soporta los campos editables de Sección A: texto, radio, estilo, color,
// fondo. Igual que Texto, valida todo antes de aplicar ningún cambio.

DSIOperationResult DSIService::editDSI(int tn, const QMap<QString, QString>& fields) {
    DSIEntity* zone = m_ctx->dsiSession.findByTn(tn);
    if (!zone) {
        return { false, QStringLiteral("[DSI] No existe ninguna DSI con TN %1.").arg(tn) };
    }

    QString newText = zone->labelText;
    if (fields.contains(QStringLiteral("texto"))) {
        newText = fields.value(QStringLiteral("texto"));
        if (newText.length() > 20) {
            return { false, QStringLiteral("[DSI] El texto no puede superar los 20 caracteres.") };
        }
    }

    double newRadius = zone->radiusDm;
    if (fields.contains(QStringLiteral("radio"))) {
        bool ok = false;
        newRadius = fields.value(QStringLiteral("radio")).toDouble(&ok);
        if (!ok || newRadius <= 0.0) {
            return { false, QStringLiteral("[DSI] --radio debe ser un numero mayor a 0.") };
        }
    }

    DSILineStyle newStyle = zone->lineStyle;
    if (fields.contains(QStringLiteral("estilo"))) {
        bool ok = false;
        newStyle = stringToLineStyle(fields.value(QStringLiteral("estilo")), ok);
        if (!ok) return { false, QStringLiteral("[DSI] --estilo invalido.") };
    }

    bool newHasColor = zone->labelColorSet;
    DSIColor newColor = zone->labelColor;
    if (fields.contains(QStringLiteral("color"))) {
        bool ok = false;
        newColor = stringToColor(fields.value(QStringLiteral("color")), ok);
        if (!ok) return { false, QStringLiteral("[DSI] --color invalido.") };
        newHasColor = true;
    }

    bool newHasBg = zone->labelBackgroundSet;
    DSIColor newBg = zone->labelBackground;
    if (fields.contains(QStringLiteral("fondo"))) {
        bool ok = false;
        newBg = stringToColor(fields.value(QStringLiteral("fondo")), ok);
        if (!ok) return { false, QStringLiteral("[DSI] --fondo invalido.") };
        newHasBg = true;
    }

    // Todas las validaciones pasaron -- recién ahora se aplica todo junto.
    zone->labelText         = newText;
    zone->radiusDm          = newRadius;
    zone->lineStyle         = newStyle;
    zone->labelColorSet      = newHasColor;
    zone->labelColor         = newColor;
    zone->labelBackgroundSet = newHasBg;
    zone->labelBackground    = newBg;

    return { true, QStringLiteral("[DSI] DSI TN %1 actualizada.").arg(tn) };
}

// ── Borrar ───────────────────────────────────────────────────────────────

DSIOperationResult DSIService::deleteDSI(int tn) {
    if (!m_ctx->dsiSession.removeByTn(tn)) {
        return { false, QStringLiteral("[DSI] No existe ninguna DSI con TN %1.").arg(tn) };
    }
    return { true, QStringLiteral("[DSI] DSI TN %1 eliminada.").arg(tn) };
}

// ── Asociar / Desasociar ──────────────────────────────────────────────────

DSIOperationResult DSIService::associateTrack(int dsiTn, int trackId) {
    DSIEntity* zone = m_ctx->dsiSession.findByTn(dsiTn);
    if (!zone) {
        return { false, QStringLiteral("[DSI] No existe ninguna DSI con TN %1.").arg(dsiTn) };
    }

    const Track* track = m_ctx->findTrackById(trackId);
    if (!track) {
        return { false, QStringLiteral("[DSI] El track %1 no existe.").arg(trackId) };
    }

    zone->associated        = true;
    zone->associatedTrackId = trackId;
    // Adopción directa e inmediata (REQ-DSI-UI-017) — sin offset.
    zone->currentXDm = track->getX();
    zone->currentYDm = track->getY();

    return { true, QStringLiteral("[DSI] DSI TN %1 asociada al track %2.")
                        .arg(dsiTn).arg(trackId) };
}

DSIOperationResult DSIService::dissociateTrack(int dsiTn) {
    DSIEntity* zone = m_ctx->dsiSession.findByTn(dsiTn);
    if (!zone) {
        return { false, QStringLiteral("[DSI] No existe ninguna DSI con TN %1.").arg(dsiTn) };
    }
    if (!zone->associated) {
        return { false, QStringLiteral("[DSI] La DSI TN %1 no esta asociada a ningun track.")
                            .arg(dsiTn) };
    }

    // Se congela en su última posición conocida (igual que Texto).
    zone->associated        = false;
    zone->associatedTrackId = -1;
    zone->baseXDm           = zone->currentXDm;
    zone->baseYDm           = zone->currentYDm;

    return { true, QStringLiteral("[DSI] DSI TN %1 desasociada.").arg(dsiTn) };
}

// ── Consultas ──────────────────────────────────────────────────────────────

DSIOperationResult DSIService::listDSI() const {
    if (m_ctx->dsiSession.zones.isEmpty()) {
        return { false, QStringLiteral("[DSI] No hay zonas DSI activas.") };
    }

    QString response = QStringLiteral("\n[DSI] Zonas activas:\n");
    for (const DSIEntity& zone : m_ctx->dsiSession.zones) {
        response += QStringLiteral("  TN %1 — \"%2\" — radio %3MN — (%4, %5) DM%6\n")
                        .arg(zone.tn)
                        .arg(zone.labelText)
                        .arg(zone.radiusDm, 0, 'f', 1)
                        .arg(zone.currentXDm, 0, 'f', 2)
                        .arg(zone.currentYDm, 0, 'f', 2)
                        .arg(zone.associated
                                 ? QStringLiteral(" — asociada a track %1").arg(zone.associatedTrackId)
                                 : QString());
    }
    return { true, response };
}

DSIOperationResult DSIService::infoDSI(int tn) const {
    const DSIEntity* zone = m_ctx->dsiSession.findByTn(tn);
    if (!zone) {
        return { false, QStringLiteral("[DSI] No existe ninguna DSI con TN %1.").arg(tn) };
    }

    QString response;
    response += QStringLiteral("\n======================================================\n");
    response += QStringLiteral("          DSI — TN %1\n").arg(tn);
    response += QStringLiteral("======================================================\n");
    response += QStringLiteral("Texto:      \"%1\"\n").arg(zone->labelText);
    response += QStringLiteral("Radio:      %1 MN\n").arg(zone->radiusDm, 0, 'f', 1);
    response += QStringLiteral("Estilo:     %1\n").arg(lineStyleToString(zone->lineStyle));
    response += QStringLiteral("Color:      %1\n")
                    .arg(zone->labelColorSet ? colorToString(zone->labelColor) : QStringLiteral("(sin seleccionar)"));
    response += QStringLiteral("Fondo:      %1\n")
                    .arg(zone->labelBackgroundSet ? colorToString(zone->labelBackground) : QStringLiteral("(sin fondo)"));
    response += QStringLiteral("Metodo Pos: %1\n").arg(positionMethodToString(zone->positionMethod));
    response += QStringLiteral("Posicion:   (%1, %2) DM\n")
                    .arg(zone->currentXDm, 0, 'f', 2)
                    .arg(zone->currentYDm, 0, 'f', 2);
    if (zone->associated) {
        response += QStringLiteral("Asociado a: Track %1\n").arg(zone->associatedTrackId);
    } else {
        response += QStringLiteral("Asociado a: (ninguno)\n");
    }
    response += QStringLiteral("Alarmas activas (cooldown): %1\n").arg(zone->activeAlarms.size());
    response += QStringLiteral("======================================================\n\n");

    return { true, response };
}

// ── Update ─────────────────────────────────────────────────────────────────

void DSIService::update() {
    for (DSIEntity& zone : m_ctx->dsiSession.zones) {

        // 1) Seguimiento de track asociado — adopción directa (sin offset).
        if (zone.associated) {
            const Track* track = m_ctx->findTrackById(zone.associatedTrackId);
            if (track) {
                const QPointF trackPos(track->getX(), track->getY());
                const QPointF newPos = DSICalculator::resolveAssociatedPosition(trackPos);
                zone.currentXDm = newPos.x();
                zone.currentYDm = newPos.y();
            }
            // Si el track asociado ya no existe, se mantiene la última
            // posición conocida (mismo comportamiento pendiente de
            // confirmación que Texto — ver docs/flows/texto.md).
        }

        // 2) Evaluación de la alarma de proximidad (F1/F2, REQ-DSI-CAL-001/002).
        evaluateProximityAlarm(zone);
    }
}

void DSIService::evaluateProximityAlarm(DSIEntity& zone) {
    const QPointF center(zone.currentXDm, zone.currentYDm);
    const QDateTime now = QDateTime::currentDateTime();

    // Limpia del cooldown a los tracks que ya cumplieron los 5 segundos
    // de alarma, reiniciando el ciclo de evaluación (REQ-DSI-CAL-002).
    // NOTA: se asume que, si el track sigue dentro del radio, la alarma
    // puede volver a dispararse en el próximo ciclo — a confirmar con el
    // equipo si en cambio se requiere que el track salga y reingrese.
    for (auto it = zone.activeAlarms.begin(); it != zone.activeAlarms.end(); ) {
        if (it.value().msecsTo(now) >= 5000) {
            it = zone.activeAlarms.erase(it);
        } else {
            ++it;
        }
    }

    for (const Track& track : m_ctx->getTracks()) {
        const QPointF trackPos(track.getX(), track.getY());
        const bool inside = DSICalculator::isTrackInsideZone(center, zone.radiusDm, trackPos);

        if (inside && !zone.activeAlarms.contains(track.getId())) {
            zone.activeAlarms.insert(track.getId(), now);

            // TODO(integración): disparar acá la notificación real hacia
            // StatusConsole, ej: "%1 INGRESANDO A DSI" — no se agrega la
            // llamada concreta porque el mecanismo de push hacia consola
            // (ITransport / señal Qt) no estaba en los archivos que me
            // pasaste; conectar con el mismo canal que usan HA/2W.
        }
    }
}
