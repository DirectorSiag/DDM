#include "addCursor.h"
#include "enums.h"

// helper como en add: parseo numérico robusto
static bool takeNumber(const QString& s, double& out) {
    bool ok = false; double v = s.toDouble(&ok);
    if (ok) out = v;
    return ok;
}

CommandResult addCursor::execute(const CommandInvocation &inv, CommandContext &ctx) const
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

    if (longd < 0) return {false, "El largo no puede ser negativo."};

    // normalizar ángulo a [0, 360)
    while (angd < 0)     angd += 360.0;
    while (angd >= 360.) angd -= 360.0;

    // construir dominio
    const int id = ctx.nextCursorId++;
    const QPair<qfloat16,qfloat16> coord(qfloat16(xd), qfloat16(yd));
    const qfloat16 cursorAngle = qfloat16(angd);
    const qfloat16 cursorLong  = qfloat16(longd);
    const bool state = true; // al crear, activo (cámbialo si tu lógica dice otra cosa)

    // alta O(1) como en add/emplace de tracks
    CursorEntity& ref = ctx.emplaceCursorFront(
        QPair<qfloat16,qfloat16>(qfloat16(xd), qfloat16(yd)), // coordenadas
        qfloat16(angd),   // ángulo
        qfloat16(longd),   // largo
        lineType,                 // tipo de línea
        id,                  // id
        true);

    return {
        true,
        QString("OK addCursor → id=%1 tipo=%2 x=%3 y=%4 largo=%5 ang=%6")
            .arg(id)
            .arg(lineType)
            .arg(double(ref.getCoordinates().first),  0, 'f', 3)
            .arg(double(ref.getCoordinates().second), 0, 'f', 3)
            .arg(double(ref.getCursorLength()),          0, 'f', 3)
            .arg(double(ref.getCursorAngle()),            0, 'f', 3)
    };
}
