#include "addCommand.h"
#include "enums.h"
#include "../services/trackservice.h"
#include "model/utils/RadarMath.h"

#include <cmath>
#include <limits>

namespace {

bool isNumericToken(const QString& tok) {
    if (!tok.startsWith('-') || tok.size() < 2) return false;
    const QChar c = tok[1];
    return c.isDigit() || c == '.';
}

bool takeNumber(const QString& s, double& out) {
    bool ok = false;
    const double v = s.toDouble(&ok);
    if (ok) out = v;
    return ok;
}

bool takeInt(const QString& s, int& out) {
    bool ok = false;
    const int v = s.toInt(&ok);
    if (ok) out = v;
    return ok;
}

QString takeTextUntilNextFlag(const QStringList& args, int& idx) {
    // Consume tokens until next "-flag" (pero permitir negativos numéricos)
    QString out;
    while (idx < args.size()) {
        const QString tok = args[idx];
        if (tok.startsWith('-') && !isNumericToken(tok)) break;
        if (!out.isEmpty()) out += " ";
        out += tok;
        ++idx;
    }
    return out;
}

TrackData::Type trackTypeFromToken(const QString& token, bool& ok) {
    TrackData::Type parsedType = TrackData::SPC;
    ok = TrackData::tryParseType(token, parsedType) || TrackData::tryParseTypeBits(token, parsedType);
    return parsedType;
}

TrackData::Identity identityFromCode(const QString& code, bool& ok) {
    ok = true;
    const QString c = code.trimmed().toUpper();
    if (c == "P") return TrackData::Pending;
    if (c == "A") return TrackData::PossFriend;
    if (c == "F") return TrackData::ConfFriend;
    if (c == "E") return TrackData::PossHostile;
    if (c == "H") return TrackData::ConfHostile;
    if (c == "U") return TrackData::EvalUnknown;
    if (c == "Y") return TrackData::Heli;
    ok = false;
    return TrackData::Pending;
}

Track::LinkYStatus linkYFromCode(const QString& code, bool& ok) {
    ok = true;
    const QString c = code.trimmed().toUpper();
    if (c == "R") return Track::LinkY_ReceivedOnly;
    if (c == "C") return Track::LinkY_Correlated;
    if (c == "T") return Track::LinkY_Tx;
    if (c == "S") return Track::LinkY_TxS;
    ok = false;
    return Track::LinkY_Invalid;
}

Track::Link14Status link14FromCode(const QString& code, bool& ok) {
    ok = true;
    const QString c = code.trimmed().toUpper();
    if (c == "T") return Track::Link14_Tx;
    ok = false;
    return Track::Link14_Invalid;
}

} // namespace

CommandResult AddCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const
{
    const QStringList& args = inv.args;
    if (args.isEmpty()) return { false, "Uso:\n" + usage() };

    int idx = 0;

    // --- Tipo (obligatorio) ---
    bool hasType = false;
    TrackData::Type type = TrackData::SPC;

    // --- Identidad (opcional) ---
    TrackData::Identity ident = TrackData::Pending; // default P

    // --- Campos opcionales del modelo ---
    bool hasSpdDm = false;
    double spdDmPerHour = 0.0;

    bool hasKt = false;
    double spdKnots = 0.0;

    bool hasCrs = false;
    int crsDegInt = 0;

    bool hasFc = false;
    int fc = 0;

    bool hasAsgc = false;
    QString asgc = "-";

    bool hasLinkY = false;
    Track::LinkYStatus linkY = Track::LinkY_Invalid;

    bool hasLink14 = false;
    Track::Link14Status link14 = Track::Link14_Invalid;

    bool hasInfo = false;
    QString info = "-";

    bool hasPriv = false;
    QString priv = "-";

    // --- legacy posicional ---
    double legacyVelKnots = std::numeric_limits<double>::quiet_NaN();
    double legacyCourseDeg = std::numeric_limits<double>::quiet_NaN();

    // --------- FLAGS / OPCIONES ---------
    while (idx < args.size()) {
        const QString tok = args[idx];

        if (!tok.startsWith('-') || isNumericToken(tok)) break;

        const QString f = tok.toLower();

        // Tipo (obligatorio): --type o legacy -f|-e|-u
        if (f == "--type" || f == "-t") {
            if (hasType) return { false, "Solo un tipo permitido por comando." };
            if (idx + 1 >= args.size()) return { false, "Falta valor para --type." };
            bool ok = false;
            type = trackTypeFromToken(args[idx + 1], ok);
            if (!ok) {
                return { false, "Tipo invalido: " + args[idx + 1] + " (esperado SPC/LINCO/ASW/OPS/HECO/APC/AAW/EW o bits 0001..1000)" };
            }
            hasType = true;
            idx += 2;
            continue;
        }

        if (f == "-f" || f == "-e" || f == "-u") {
            if (hasType) return { false, "Solo un flag de tipo permitido (-f|-e|-u)." };
            hasType = true;
            if      (f == "-f") type = TrackData::SPC;
            else if (f == "-e") type = TrackData::AAW;
            else                type = TrackData::ASW;
            ++idx;
            continue;
        }

        // Identidad legacy (compat)
        if (f == "-s") { ident = TrackData::ConfFriend;  ++idx; continue; }
        if (f == "-a") { ident = TrackData::ConfHostile; ++idx; continue; }
        if (f == "-b") { ident = TrackData::EvalUnknown; ++idx; continue; }

        // Identidad por código
        if (f == "--id" || f == "-i") {
            if (idx + 1 >= args.size()) return { false, "Falta valor para --id. Uso: --id <P|A|F|E|H|U|Y>" };
            bool ok = false;
            ident = identityFromCode(args[idx + 1], ok);
            if (!ok) return { false, "Identidad inválida: " + args[idx + 1] + " (esperado P/A/F/E/H/U/Y)" };
            idx += 2;
            continue;
        }

        // Velocidad
        if (f == "--spd") {
            if (idx + 1 >= args.size()) return { false, "Falta valor para --spd (DM/h)." };
            double v = 0.0;
            if (!takeNumber(args[idx + 1], v)) return { false, "Valor inválido para --spd: " + args[idx + 1] };
            hasSpdDm = true;
            spdDmPerHour = v;
            idx += 2;
            continue;
        }

        if (f == "--kt") {
            if (idx + 1 >= args.size()) return { false, "Falta valor para --kt (knots)." };
            double v = 0.0;
            if (!takeNumber(args[idx + 1], v)) return { false, "Valor inválido para --kt: " + args[idx + 1] };
            hasKt = true;
            spdKnots = v;
            idx += 2;
            continue;
        }

        // Curso
        if (f == "--crs") {
            if (idx + 1 >= args.size()) return { false, "Falta valor para --crs (0..359)." };
            int v = 0;
            if (!takeInt(args[idx + 1], v)) return { false, "Valor inválido para --crs: " + args[idx + 1] };
            hasCrs = true;
            crsDegInt = v;
            idx += 2;
            continue;
        }

        // Asignación FC
        if (f == "--fc") {
            if (idx + 1 >= args.size()) return { false, "Falta valor para --fc (1..6)." };
            int v = 0;
            if (!takeInt(args[idx + 1], v)) return { false, "Valor inválido para --fc: " + args[idx + 1] };
            hasFc = true;
            fc = v;
            idx += 2;
            continue;
        }

        // Código asignación
        if (f == "--asgc") {
            if (idx + 1 >= args.size()) return { false, "Falta valor para --asgc." };
            hasAsgc = true;
            asgc = args[idx + 1];
            idx += 2;
            continue;
        }

        // Estado Link Y
        if (f == "--linky") {
            if (idx + 1 >= args.size()) return { false, "Falta valor para --linky (R/C/T/S)." };
            bool ok = false;
            linkY = linkYFromCode(args[idx + 1], ok);
            if (!ok) return { false, "Valor inválido para --linky: " + args[idx + 1] + " (esperado R/C/T/S)" };
            hasLinkY = true;
            idx += 2;
            continue;
        }

        // Estado Link 14
        if (f == "--link14") {
            if (idx + 1 >= args.size()) return { false, "Falta valor para --link14 (T)." };
            bool ok = false;
            link14 = link14FromCode(args[idx + 1], ok);
            if (!ok) return { false, "Valor inválido para --link14: " + args[idx + 1] + " (esperado T)" };
            hasLink14 = true;
            idx += 2;
            continue;
        }

        // Info ampliatoria (consume texto hasta próximo flag)
        if (f == "--info") {
            ++idx;
            hasInfo = true;
            info = takeTextUntilNextFlag(args, idx);
            if (info.isEmpty()) info = "-";
            continue;
        }

        // Código privado (consume texto hasta próximo flag)
        if (f == "--priv") {
            ++idx;
            hasPriv = true;
            priv = takeTextUntilNextFlag(args, idx);
            if (priv.isEmpty()) priv = "-";
            continue;
        }

        return { false, "Flag no soportada: " + tok + "\nUso:\n" + usage() };
    }

    // --------- Validación tipo ---------
    if (!hasType) {
        return { false, "Falta tipo (--type o -f|-e|-u).\nUso:\n" + usage() };
    }

    // --------- Coordenadas ---------
    if (idx + 1 >= args.size()) {
        return { false, "Faltan coordenadas <x> <y>.\nUso:\n" + usage() };
    }

    double x = 0.0, y = 0.0;
    if (!takeNumber(args[idx], x) || !takeNumber(args[idx + 1], y)) {
        return { false, "Coordenadas inválidas. Deben ser números (x y)." };
    }
    idx += 2;

    // --------- Legacy posicional [velKnots] [courseDeg] ---------
    if (idx < args.size()) {
        double v = 0.0;
        if (!takeNumber(args[idx], v)) return { false, "Velocidad legacy inválida. Debe ser número (knots)." };
        legacyVelKnots = v;
        ++idx;
    }
    if (idx < args.size()) {
        double v = 0.0;
        if (!takeNumber(args[idx], v)) return { false, "Curso legacy inválido. Debe ser número (grados)." };
        legacyCourseDeg = v;
        ++idx;
    }

    // --------- Reglas básicas ---------
    if (x < -256.0 || x > 256.0 || y < -256.0 || y > 256.0) {
        return { false, "Coordenadas fuera de rango. Deben estar entre -256 y 256." };
    }

    if (hasSpdDm && (spdDmPerHour < 0.0 || spdDmPerHour > 99.9)) {
        return { false, "Velocidad --spd fuera de rango. Debe ser 0..99.9 (DM/h)." };
    }
    if (hasKt && spdKnots < 0.0) {
        return { false, "Velocidad --kt fuera de rango. Debe ser >= 0." };
    }
    if (!std::isnan(legacyVelKnots) && legacyVelKnots < 0.0) {
        return { false, "Velocidad legacy fuera de rango. Debe ser >= 0." };
    }

    if (hasCrs && (crsDegInt < 0 || crsDegInt > 359)) {
        return { false, "Curso --crs fuera de rango. Debe ser 0..359." };
    }

    // FC validación (1..6 o inválido)
    if (hasFc && !(fc >= 1 && fc <= 6)) {
        return { false, "Asignación FC inválida. Debe ser 1..6." };
    }

    // --------- Alta del Track ---------
    TrackCreateRequest request;
    request.type = type;
    request.identity = ident;
    request.mode = TrackData::Auto;
    request.x = x;
    request.y = y;
    request.ctorSpeedKnots = hasKt ? spdKnots : (!std::isnan(legacyVelKnots) ? legacyVelKnots : 0.0);
    request.ctorCourseDeg = RadarMath::normalizeAngle360(!std::isnan(legacyCourseDeg) ? legacyCourseDeg : 0.0);

    if (hasSpdDm) request.speedDmPerHour = spdDmPerHour;
    if (hasCrs) request.courseDeg = crsDegInt;
    if (hasFc) request.fc = fc;
    if (hasAsgc) request.asgc = asgc;
    if (hasLinkY) request.linkY = linkY;
    if (hasLink14) request.link14 = link14;
    if (hasInfo) request.info = info;
    if (hasPriv) request.priv = priv;

    TrackService trackService(&ctx);
    TrackOperationResult createResult = trackService.createTrack(request);
    if (!createResult.success) {
        return {false, createResult.message};
    }

    Track* t = trackService.findTrackById(createResult.trackId);
    if (!t) {
        return {false, "No se pudo recuperar el track recien creado"};
    }
    // NOTE: CLI command must not perform JSON transport operations.
    // Notification to frontend is handled by the JSON handlers (TrackCommandHandler) or by
    // services/handlers responsible for protocol serialization.

    return {
        true,
        QString("OK add → nro=%1 tipo=%2 id=%3 x=%4 y=%5 spd=%6DM/h crs=%7 fc=%8 asgc=%9 ly=%10 l14=%11 info=%12 priv=%13")
            .arg(QString("%1").arg(t->getNumero(), 4, 10, QLatin1Char('0')))
            .arg(QChar(t->getTipo()))
            .arg(t->getIdentityCode())
            .arg(t->getX(), 0, 'f', 3)
            .arg(t->getY(), 0, 'f', 3)
            .arg(t->getVelocidadDmPerHour(), 0, 'f', 1)
            .arg(t->getCursoInt(), 3, 10, QLatin1Char('0'))
            .arg(t->getAsignacionFc() ? QString::number(t->getAsignacionFc()) : QString("-"))
            .arg(t->getCodigoAsignacion())
            .arg(t->getEstadoLinkY() == Track::LinkY_Invalid ? "-" :
                     QString(QChar("RC TS"[int(t->getEstadoLinkY())]))) // R,C,T,S en orden 0..3
            .arg(t->getEstadoLink14() == Track::Link14_Invalid ? "-" : "T")
            .arg(t->getInformacionAmpliatoria())
            .arg(t->getCodigoPrivado())
    };
}
