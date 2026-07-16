#include "haCommand.h"
#include "../services/haService.h"

CommandResult HaCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const
{
    if (inv.args.isEmpty()) {
        return { false, QStringLiteral("Faltan argumentos.\n%1").arg(usage()) };
    }

    QMap<QString, QString> opts;
    QStringList positional;

    for (const QString& token : inv.args) {
        const int eq = token.indexOf('=');
        if (eq > 0) {
            QString key = token.left(eq);
            while (key.startsWith('-')) key.remove(0, 1);
            opts.insert(key.toLower(), token.mid(eq + 1).trimmed());
        } else if (token.startsWith('-')) {
            QString key = token;
            while (key.startsWith('-')) key.remove(0, 1);
            opts.insert(key.toLower(), QStringLiteral("true"));
        } else {
            positional.append(token);
        }
    }

    HaService service(&ctx, m_obmService);

    // ha --list — mostrar todos los slots activos
    if (opts.contains(QStringLiteral("list"))) {
        bool anyActive = false;
        QString response = QStringLiteral("\n[HA] Emergencias activas:\n");
        for (int i = 0; i < CommandContext::kMaxHaSessions; ++i) {
            if (ctx.haSessions[i].active) {
                response += QStringLiteral("  Slot %1 — Punto (%2, %3) DM — Iniciado %4\n")
                                .arg(i + 1)
                                .arg(ctx.haSessions[i].fallPointX, 0, 'f', 2)
                                .arg(ctx.haSessions[i].fallPointY, 0, 'f', 2)
                                .arg(ctx.haSessions[i].fallTimeLocal);
                anyActive = true;
            }
        }
        if (!anyActive)
            return { false, QStringLiteral("[HA] No hay emergencias activas.") };
        return { true, response };
    }

    // ha --stop [<slot>]
    if (opts.contains(QStringLiteral("stop"))) {
        int slot = -1;  // -1 = todos
        if (!positional.isEmpty()) {
            bool ok = false;
            slot = positional[0].toInt(&ok);
            if (!ok)
                return { false, QStringLiteral("--stop: el slot debe ser un entero.") };
        }
        const HaOperationResult r = service.stopSession(slot);
        return { r.ok, r.message };
    }

    // ha --info <slot>
    if (opts.contains(QStringLiteral("info"))) {
        if (positional.isEmpty())
            return { false, QStringLiteral("--info requiere un numero de slot.\n%1").arg(usage()) };
        bool ok = false;
        const int slot = positional[0].toInt(&ok);
        if (!ok)
            return { false, QStringLiteral("--info: el slot debe ser un entero.") };
        const HaOperationResult r = service.infoReport(slot);
        return { r.ok, r.message };
    }

    // ha --popa
    if (opts.contains(QStringLiteral("popa"))) {
        const HaOperationResult r = service.startSessionAtOwnShip();
        return { r.ok, r.message };
    }

    // ha --cursor
    if (opts.contains(QStringLiteral("cursor"))) {
        const HaOperationResult r = service.startSessionAtCursor();
        return { r.ok, r.message };
    }

    // ha --latlon --lat=<g>,<m>,<s> --lon=<g>,<m>,<s>
    if (opts.contains(QStringLiteral("latlon"))) {
        if (!opts.contains(QStringLiteral("lat")) || !opts.contains(QStringLiteral("lon")))
            return { false, QStringLiteral("--latlon requiere --lat=<g,m,s> y --lon=<g,m,s>.\n%1").arg(usage()) };

        const QStringList latParts = opts.value(QStringLiteral("lat")).split(',');
        const QStringList lonParts = opts.value(QStringLiteral("lon")).split(',');

        if (latParts.size() != 3 || lonParts.size() != 3)
            return { false, QStringLiteral("--lat y --lon deben tener formato <grados>,<minutos>,<segundos>.") };

        bool ok = true, okTmp;
        const int    latDeg = latParts[0].trimmed().toInt(&okTmp);    ok &= okTmp;
        const int    latMin = latParts[1].trimmed().toInt(&okTmp);    ok &= okTmp;
        const double latSec = latParts[2].trimmed().toDouble(&okTmp); ok &= okTmp;
        const int    lonDeg = lonParts[0].trimmed().toInt(&okTmp);    ok &= okTmp;
        const int    lonMin = lonParts[1].trimmed().toInt(&okTmp);    ok &= okTmp;
        const double lonSec = lonParts[2].trimmed().toDouble(&okTmp); ok &= okTmp;

        if (!ok)
            return { false, QStringLiteral("--lat y --lon: todos los valores deben ser numericos.") };

        const HaOperationResult r = service.startSessionAtLatLonDms(
            latDeg, latMin, latSec, lonDeg, lonMin, lonSec
            );
        return { r.ok, r.message };
    }

    // ha --az=<azimut> --d=<distancia_yardas>
    if (opts.contains(QStringLiteral("az"))) {
        if (!opts.contains(QStringLiteral("d")))
            return { false, QStringLiteral("--az requiere --d=<distancia_yardas>.\n%1").arg(usage()) };
        bool okAz = false, okD = false;
        const double az = opts.value(QStringLiteral("az")).toDouble(&okAz);
        const double d  = opts.value(QStringLiteral("d")).toDouble(&okD);
        if (!okAz || !okD)
            return { false, QStringLiteral("--az y --d deben ser valores numericos.") };
        const HaOperationResult r = service.startSessionAtBearing(az, d);
        return { r.ok, r.message };
    }

    return { false, QStringLiteral("Flag no reconocido.\n%1").arg(usage()) };
}