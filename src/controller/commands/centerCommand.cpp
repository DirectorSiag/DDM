/*
    Comando `center`: fija `ctx.centerX/centerY` con `<x> <y>` validando formatos numéricos.
*/

#include "Commands/centerCommand.h"

static bool takeNumber(const QString& s, double& out) {
    bool ok=false; double v = s.toDouble(&ok);
    if (ok) out = v;
    return ok;
}

CommandResult CenterCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const {
    const QStringList& args = inv.args;
    if (args.size() != 2) return {false, "Uso: " + usage()};

    double x=0, y=0;
    if (!takeNumber(args[0], x) || !takeNumber(args[1], y)) {
        return {false, "Coordenadas inválidas. Deben ser números (x y)."};
    }

    if (x < -255 || x > 255 || y < -255 || y > 255) {
        return {false, "Centro fuera de rango (-256 a 256)."};
    }
    ctx.centerX = x;
    ctx.centerY = y;
    return {true, QString("OK center → (%1, %2)").arg(x,0,'f',3).arg(y,0,'f',3)};
}
