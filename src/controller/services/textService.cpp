#include "textService.h"
#include "model/utils/RadarMath.h"
#include <QtMath>
#include <cmath>

TextService::TextService(CommandContext* ctx)
    : m_ctx(ctx)
{}

// ── Helpers de conversión string <-> enum (públicos, reutilizables) ──────

TextColor TextService::stringToColor(const QString& s, bool& ok) {
    ok = true;
    const QString key = s.trimmed().toLower();
    if (key == QStringLiteral("rojo"))      return TextColor::Rojo;
    if (key == QStringLiteral("verde"))     return TextColor::Verde;
    if (key == QStringLiteral("azul"))      return TextColor::Azul;
    if (key == QStringLiteral("cian"))      return TextColor::Cian;
    if (key == QStringLiteral("magenta"))   return TextColor::Magenta;
    if (key == QStringLiteral("amarillo"))  return TextColor::Amarillo;
    if (key == QStringLiteral("blanco"))    return TextColor::Blanco;
    if (key == QStringLiteral("purpura"))   return TextColor::Purpura;
    if (key == QStringLiteral("naranja"))    return TextColor::Naranja;
    ok = false;
    return TextColor::Blanco;
}

TextFontSize TextService::stringToFontSize(const QString& s, bool& ok) {
    ok = true;
    const QString key = s.trimmed().toLower();
    if (key == QStringLiteral("xs")) return TextFontSize::XS;
    if (key == QStringLiteral("md")) return TextFontSize::MD;
    if (key == QStringLiteral("lg")) return TextFontSize::LG;
    ok = false;
    return TextFontSize::MD;
}

// ── Helpers de conversión enum -> string (privados, para reportes) ───────

QString TextService::fontSizeToString(TextFontSize size) {
    switch (size) {
        case TextFontSize::XS: return QStringLiteral("XS");
        case TextFontSize::MD: return QStringLiteral("MD");
        case TextFontSize::LG: return QStringLiteral("LG");
    }
    return QStringLiteral("MD");
}

QString TextService::colorToString(TextColor color) {
    switch (color) {
        case TextColor::Rojo:             return QStringLiteral("ROJO");
        case TextColor::Verde:            return QStringLiteral("VERDE");
        case TextColor::Azul:             return QStringLiteral("AZUL");
        case TextColor::Cian:             return QStringLiteral("CIAN");
        case TextColor::Magenta:          return QStringLiteral("MAGENTA");
        case TextColor::Amarillo:         return QStringLiteral("AMARILLO");
        case TextColor::Blanco:           return QStringLiteral("BLANCO");
        case TextColor::Purpura:          return QStringLiteral("PURPURA");
        case TextColor::Naranja:          return QStringLiteral("NARANJA");
    }
    return QStringLiteral("BLANCO");
}

QString TextService::positionMethodToString(TextPositionMethod method) {
    switch (method) {
        case TextPositionMethod::Manual:     return QStringLiteral("MAN");
        case TextPositionMethod::AzimutDist: return QStringLiteral("AZ/DT");
        case TextPositionMethod::LatLon:     return QStringLiteral("LAT/LON");
        case TextPositionMethod::DelTrack:   return QStringLiteral("DEL_TRACK");
    }
    return QStringLiteral("MAN");
}

// ── Crear ────────────────────────────────────────────────────────────────

TextOperationResult TextService::createLabel(const TextCreateParams& params) {
    if (params.texto.isEmpty()) {
        return { false, QStringLiteral("[TEXTO] El campo texto no puede estar vacio.") };
    }
    if (params.texto.length() > 20) {
        return { false, QStringLiteral("[TEXTO] El texto no puede superar los 20 caracteres.") };
    }
    if (params.fontColor == params.backgroundColor) {
        return { false, QStringLiteral(
            "[TEXTO] Color y Fondo no pueden ser iguales (falta de contraste). "
            "Elegi combinaciones distintas.") };
    }

    double xDm = 0.0, yDm = 0.0;

    switch (params.positionMethod) {

    case TextPositionMethod::Manual: {
        xDm = params.manXDm;
        yDm = params.manYDm;
        break;
    }

    case TextPositionMethod::AzimutDist: {
        const Track* bp = m_ctx->findTrackById(0);
        if (!bp) {
            return { false, QStringLiteral("[TEXTO] Error: no se encontro el Buque Propio.") };
        }
        const QPointF bpPos(bp->getX(), bp->getY());
        const QPointF resolved = TextCalculator::resolveFromBearing(
            bpPos, params.azimuthDeg, params.distanceDm,
            params.useVerdadero, bp->getCourseDeg()
        );
        xDm = resolved.x();
        yDm = resolved.y();
        break;
    }

    case TextPositionMethod::LatLon: {
        const bool bpHasGeo = m_ctx->ownShip.valid &&
                              !(m_ctx->ownShip.latitudeDeg == 0.0 &&
                                m_ctx->ownShip.longitudeDeg == 0.0);
        if (!bpHasGeo) {
            return { false, QStringLiteral(
                "[TEXTO] LAT/LON no disponible: el BP no tiene coordenadas geograficas reales.") };
        }
        const QPointF resolved = TextCalculator::resolveFromLatLon(
            m_ctx->ownShip.latitudeDeg, m_ctx->ownShip.longitudeDeg,
            params.latDecimal, params.lonDecimal
        );
        xDm = resolved.x();
        yDm = resolved.y();
        break;
    }

    case TextPositionMethod::DelTrack: {
        const Track* refTrack = m_ctx->findTrackById(params.refTrackId);
        if (!refTrack) {
            return { false, QStringLiteral("[TEXTO] El track de referencia %1 no existe.")
                                .arg(params.refTrackId) };
        }
        xDm = refTrack->getX();
        yDm = refTrack->getY();
        break;
    }
    }

    TextLabel label;
    label.tn              = m_ctx->textSession.nextTn++;
    label.texto           = params.texto;
    label.fontSize        = params.fontSize;
    label.fontColor       = params.fontColor;
    label.borderColor     = params.borderColor;
    label.backgroundColor = params.backgroundColor;
    label.positionMethod  = params.positionMethod;
    label.baseXDm         = xDm;
    label.baseYDm         = yDm;
    label.currentXDm      = xDm;
    label.currentYDm      = yDm;

    if (params.asociarAlCrear) {
        const Track* track = m_ctx->findTrackById(params.trackToAssociate);
        if (!track) {
            return { false, QStringLiteral("[TEXTO] El track a asociar %1 no existe.")
                                .arg(params.trackToAssociate) };
        }
        label.associated        = true;
        label.associatedTrackId = params.trackToAssociate;
        label.offsetXDm         = xDm - track->getX();
        label.offsetYDm         = yDm - track->getY();
    }

    m_ctx->textSession.labels.append(label);

    return { true, QStringLiteral("[TEXTO] Texto creado con TN %1.").arg(label.tn) };
}

// ── Editar ───────────────────────────────────────────────────────────────
// Soporta los cinco campos editables: texto, tamano, color, fondo, borde.
// La regla de contraste (color != fondo) se valida sobre el resultado
// FINAL antes de aplicar ningun cambio, para que el label nunca quede
// a medio editar si la validacion falla.

TextOperationResult TextService::editLabel(int tn, const QMap<QString, QString>& fields) {
    TextLabel* label = m_ctx->textSession.findByTn(tn);
    if (!label) {
        return { false, QStringLiteral("[TEXTO] No existe ningun texto con TN %1.").arg(tn) };
    }

    // ── Texto ────────────────────────────────────────────────────────────
    QString newText = label->texto;
    if (fields.contains(QStringLiteral("texto"))) {
        newText = fields.value(QStringLiteral("texto"));
        if (newText.isEmpty()) {
            return { false, QStringLiteral("[TEXTO] El campo texto no puede estar vacio.") };
        }
        if (newText.length() > 20) {
            return { false, QStringLiteral("[TEXTO] El texto no puede superar los 20 caracteres.") };
        }
    }

    // ── Tamano ───────────────────────────────────────────────────────────
    TextFontSize newSize = label->fontSize;
    if (fields.contains(QStringLiteral("tamano"))) {
        bool ok = false;
        newSize = stringToFontSize(fields.value(QStringLiteral("tamano")), ok);
        if (!ok) {
            return { false, QStringLiteral("[TEXTO] --tamano invalido. Usar xs, md o lg.") };
        }
    }

    // ── Color, Fondo, Borde ──────────────────────────────────────────────
    TextColor newFontColor   = label->fontColor;
    TextColor newBgColor     = label->backgroundColor;
    TextColor newBorderColor = label->borderColor;

    if (fields.contains(QStringLiteral("color"))) {
        bool ok = false;
        newFontColor = stringToColor(fields.value(QStringLiteral("color")), ok);
        if (!ok) return { false, QStringLiteral("[TEXTO] --color invalido.") };
    }
    if (fields.contains(QStringLiteral("fondo"))) {
        bool ok = false;
        newBgColor = stringToColor(fields.value(QStringLiteral("fondo")), ok);
        if (!ok) return { false, QStringLiteral("[TEXTO] --fondo invalido.") };
    }
    if (fields.contains(QStringLiteral("borde"))) {
        bool ok = false;
        newBorderColor = stringToColor(fields.value(QStringLiteral("borde")), ok);
        if (!ok) return { false, QStringLiteral("[TEXTO] --borde invalido.") };
    }

    if (newFontColor == newBgColor) {
        return { false, QStringLiteral(
            "[TEXTO] Color y Fondo no pueden ser iguales (falta de contraste).") };
    }

    // Todas las validaciones pasaron -- recién ahora se aplica todo junto.
    label->texto           = newText;
    label->fontSize         = newSize;
    label->fontColor        = newFontColor;
    label->backgroundColor  = newBgColor;
    label->borderColor      = newBorderColor;

    return { true, QStringLiteral("[TEXTO] Texto TN %1 actualizado.").arg(tn) };
}

// ── Borrar ───────────────────────────────────────────────────────────────

TextOperationResult TextService::deleteLabel(int tn) {
    if (!m_ctx->textSession.removeByTn(tn)) {
        return { false, QStringLiteral("[TEXTO] No existe ningun texto con TN %1.").arg(tn) };
    }
    return { true, QStringLiteral("[TEXTO] Texto TN %1 eliminado.").arg(tn) };
}

// ── Asociar / Desasociar ──────────────────────────────────────────────────

TextOperationResult TextService::associateTrack(int textTn, int trackId) {
    TextLabel* label = m_ctx->textSession.findByTn(textTn);
    if (!label) {
        return { false, QStringLiteral("[TEXTO] No existe ningun texto con TN %1.").arg(textTn) };
    }

    const Track* track = m_ctx->findTrackById(trackId);
    if (!track) {
        return { false, QStringLiteral("[TEXTO] El track %1 no existe.").arg(trackId) };
    }

    label->associated        = true;
    label->associatedTrackId = trackId;
    label->offsetXDm         = label->currentXDm - track->getX();
    label->offsetYDm         = label->currentYDm - track->getY();

    return { true, QStringLiteral("[TEXTO] Texto TN %1 asociado al track %2.")
                        .arg(textTn).arg(trackId) };
}

TextOperationResult TextService::dissociateTrack(int textTn) {
    TextLabel* label = m_ctx->textSession.findByTn(textTn);
    if (!label) {
        return { false, QStringLiteral("[TEXTO] No existe ningun texto con TN %1.").arg(textTn) };
    }
    if (!label->associated) {
        return { false, QStringLiteral("[TEXTO] El texto TN %1 no esta asociado a ningun track.")
                            .arg(textTn) };
    }

    label->associated        = false;
    label->associatedTrackId = -1;
    label->offsetXDm         = 0.0;
    label->offsetYDm         = 0.0;
    label->baseXDm           = label->currentXDm;
    label->baseYDm           = label->currentYDm;

    return { true, QStringLiteral("[TEXTO] Texto TN %1 desasociado.").arg(textTn) };
}

// ── Consultas ──────────────────────────────────────────────────────────────

TextOperationResult TextService::listLabels() const {
    if (m_ctx->textSession.labels.isEmpty()) {
        return { false, QStringLiteral("[TEXTO] No hay textos activos.") };
    }

    QString response = QStringLiteral("\n[TEXTO] Textos activos:\n");
    for (const TextLabel& label : m_ctx->textSession.labels) {
        response += QStringLiteral("  TN %1 — \"%2\" — (%3, %4) DM%5\n")
                        .arg(label.tn)
                        .arg(label.texto)
                        .arg(label.currentXDm, 0, 'f', 2)
                        .arg(label.currentYDm, 0, 'f', 2)
                        .arg(label.associated
                                 ? QStringLiteral(" — asociado a track %1").arg(label.associatedTrackId)
                                 : QString());
    }
    return { true, response };
}

TextOperationResult TextService::infoLabel(int tn) const {
    const TextLabel* label = m_ctx->textSession.findByTn(tn);
    if (!label) {
        return { false, QStringLiteral("[TEXTO] No existe ningun texto con TN %1.").arg(tn) };
    }

    QString response;
    response += QStringLiteral("\n======================================================\n");
    response += QStringLiteral("          TEXTO — TN %1\n").arg(tn);
    response += QStringLiteral("======================================================\n");
    response += QStringLiteral("Texto:      \"%1\"\n").arg(label->texto);
    response += QStringLiteral("Tamano:     %1\n").arg(fontSizeToString(label->fontSize));
    response += QStringLiteral("Color:      %1\n").arg(colorToString(label->fontColor));
    response += QStringLiteral("Borde:      %1\n").arg(colorToString(label->borderColor));
    response += QStringLiteral("Fondo:      %1\n").arg(colorToString(label->backgroundColor));
    response += QStringLiteral("Metodo Pos: %1\n").arg(positionMethodToString(label->positionMethod));
    response += QStringLiteral("Posicion:   (%1, %2) DM\n")
                    .arg(label->currentXDm, 0, 'f', 2)
                    .arg(label->currentYDm, 0, 'f', 2);
    if (label->associated) {
        response += QStringLiteral("Asociado a: Track %1\n").arg(label->associatedTrackId);
    } else {
        response += QStringLiteral("Asociado a: (ninguno)\n");
    }
    response += QStringLiteral("======================================================\n\n");

    return { true, response };
}

// ── Update ─────────────────────────────────────────────────────────────────

void TextService::update() {
    for (TextLabel& label : m_ctx->textSession.labels) {
        if (!label.associated) continue;

        const Track* track = m_ctx->findTrackById(label.associatedTrackId);
        if (!track) {
            continue;
        }

        const QPointF trackPos(track->getX(), track->getY());
        const QPointF newPos = TextCalculator::resolveAssociatedPosition(
            trackPos, label.offsetXDm, label.offsetYDm
        );

        label.currentXDm = newPos.x();
        label.currentYDm = newPos.y();
    }
}
