/*
  add: crea un Track.
  Flags:
    Identidad (opcional): -s (friend) | -a (hostile) | -b (unknown)
    Tipo (obligatorio):  -f (surface) | -e (air)     | -u (subsurface)

  Args:
    <x> <y> en Data Miles (DM)
    [velKnots] [courseDeg] opcionales
*/

#include "Commands/addCommand.h"
#include "enums.h"

#include <QtMath>   // qDegreesToRadians si lo necesitás en otro lado; acá usamos fmod
#include <cmath>    // std::fmod

namespace {

// Trata "-12.3" como número, no como flag.
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

double normalize360(double deg) {
    deg = std::fmod(deg, 360.0);
    if (deg < 0.0) deg += 360.0;
    return deg;
}

} // namespace

CommandResult AddCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const
{
    const QStringList& args = inv.args;
    if (args.isEmpty()) {
        return { false, "Uso: " + usage() };
    }

    int idx = 0;

    bool hasType = false;
    Type type = Type::Surface;

    Identity ident = Identity::Pending;   // default si no viene flag
    // (si querés: bool hasIdent = false; pero no hace falta)

    // --------- FLAGS ---------
    while (idx < args.size()) {
        const QString tok = args[idx];

        if (!tok.startsWith('-') || isNumericToken(tok)) break; // arranca la parte numérica

        const QString f = tok.toLower();

        // Identidad (opcional): -s|-a|-b
        if (f == "-s") { ident = Identity::ConfFriend;  ++idx; continue; }
        if (f == "-a") { ident = Identity::ConfHostile; ++idx; continue; }
        if (f == "-b") { ident = Identity::EvalUnknown; ++idx; continue; }

        // Tipo (obligatorio): -f|-e|-u (solo uno)
        if (f == "-f" || f == "-e" || f == "-u") {
            if (hasType) return { false, "Solo un flag de tipo permitido (-f|-e|-u)." };
            hasType = true;

            if      (f == "-f") type = Type::Surface;
            else if (f == "-e") type = Type::Air;
            else                type = Type::Subsurface;

            ++idx;
            continue;
        }

        return { false, QString("Flag no soportada: %1").arg(tok) };
    }

    // --------- REQUISITOS ---------
    if (!hasType) {
        return { false, "Falta tipo (-f|-e|-u). Uso: " + usage() };
    }

    // --------- COORDENADAS ---------
    if (idx + 1 >= args.size()) {
        return { false, "Faltan coordenadas <x> <y>. Uso: " + usage() };
    }

    double x = 0.0, y = 0.0;
    if (!takeNumber(args[idx], x) || !takeNumber(args[idx + 1], y)) {
        return { false, "Coordenadas inválidas. Deben ser números (x y)." };
    }
    idx += 2;

    // --------- VEL / CURSO (opcionales) ---------
    double velKnots = std::numeric_limits<double>::quiet_NaN();
    double courseDeg = std::numeric_limits<double>::quiet_NaN();

    if (idx < args.size()) {
        if (!takeNumber(args[idx], velKnots))
            return { false, "Velocidad inválida. Debe ser número (knots)." };
        ++idx;
    }

    if (idx < args.size()) {
        if (!takeNumber(args[idx], courseDeg))
            return { false, "Rumbo inválido. Debe ser número (grados)." };
        ++idx;
    }

    // --------- VALIDACIONES ---------
    // Elegí un rango consistente: acá uso [-256, 256]
    if (x < -256.0 || x > 256.0 || y < -256.0 || y > 256.0) {
        return { false, "Coordenadas fuera de rango. Deben estar entre -256 y 256." };
    }

    if (velKnots < 0.0) {
        return { false, "Velocidad fuera de rango. Debe ser >= 0." };
    }

    courseDeg = normalize360(courseDeg);

    // --------- ALTA DEL TRACK ---------
    const int id = ctx.nextTrackId++;

    Track& t = ctx.emplaceTrackFront(
        id,
        type,
        ident,
        TrackMode::Auto,
        static_cast<float>(x),
        static_cast<float>(y),
        velKnots,
        courseDeg
        );

    return {
        true,
        QString("OK add → id=%1 ident=%2 type=%3 x=%4 y=%5 spd=%6kt crs=%7°")
            .arg(t.getId())
            .arg(TrackData::toQString(t.getIdentity()))
            .arg(TrackData::toQString(t.getType()))
            .arg(t.getX(), 0, 'f', 3)
            .arg(t.getY(), 0, 'f', 3)
            .arg(t.getSpeedKnots(), 0, 'f', 1)
            .arg(t.getCourseDeg(),  0, 'f', 1)
    };
}
