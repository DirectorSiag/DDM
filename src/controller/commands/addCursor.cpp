#include "addCursor.h"
#include "enums.h"
#include "../services/cursorservice.h"
#include "model/utils/RadarMath.h"

// helper como en add: parseo numérico robusto
static bool takeNumber(const QString& s, double& out) {
    bool ok = false; double v = s.toDouble(&ok);
    if (ok) out = v;
    return ok;
}

CommandResult AddCursorCommand::execute(const CommandInvocation &inv, CommandContext &ctx) const
{
    // Uso: addCursor <tipoLinea> <x> <y> <largo> <angulo>
    const QStringList& args = inv.args;
    if (args.size() != 5) {
        return {false, "Faltan/sobran argumentos. Uso: " + usage()};
    }

    // 1) tipo de línea (entero)
    bool okType = false;
    int lineType = args[0].toInt(&okType);
    qDebug()<<"ADDCURSOR<<"<<lineType;
    if (!okType) {
        return {false, "tipoLinea inválido (entero requerido). Uso: " + usage()};
    }
    // ajustá al rango real de tu enum
    if (lineType < 0 || lineType > 7) {
        return {false, "tipoLinea fuera de rango (0..7)."};
    }

    // 2..5) x, y, largo, angulo (double)
    double xd=0, yd=0, longd=0, angd=0;
    if (!takeNumber(args[1], xd) ||
        !takeNumber(args[2], yd) ||
        !takeNumber(args[3], longd) ||
        !takeNumber(args[4], angd)) {
        return {false, "Valores inválidos. Deben ser números: <x> <y> <largo> <angulo>."};
    }

    CursorService cursorService(&ctx);
    CursorCreateRequest request;
    request.type = lineType;
    request.x = xd;
    request.y = yd;
    request.length = longd;
    request.azimut = RadarMath::normalizeAngle360(angd);

    CursorOperationResult result = cursorService.createCursor(request);
    if (!result.success) {
        return {false, result.message};
    }

    return {
        true,
        QString("OK addCursor → id=%1 tipo=%2 x=%3 y=%4 largo=%5 ang=%6")
            .arg(result.cursorId)
            .arg(lineType)
            .arg(result.x, 0, 'f', 3)
            .arg(result.y, 0, 'f', 3)
            .arg(result.length, 0, 'f', 3)
            .arg(result.angle, 0, 'f', 3)
    };
}
