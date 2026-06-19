#include "haCommand.h"
#include "../services/haService.h"

CommandResult HaCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const
{
    if (inv.args.isEmpty()) {
        return { false, QStringLiteral("Faltan argumentos.\n%1").arg(usage()) };
    }

    QMap<QString, QString> opts;
    for (const QString& token : inv.args) {
        const int eq = token.indexOf('=');
        if (eq > 0) {
            QString key = token.left(eq);
            while (key.startsWith('-')) key.remove(0, 1);
            opts.insert(key.toLower(), token.mid(eq + 1).trimmed());
        } else {
            QString key = token;
            while (key.startsWith('-')) key.remove(0, 1);
            opts.insert(key.toLower(), QStringLiteral("true"));
        }
    }

    HaService service(&ctx);

    // ha --stop
    if (opts.contains(QStringLiteral("stop"))) {
        const HaOperationResult r = service.stopSession();
        return { r.ok, r.message };
    }

    // ha --info
    if (opts.contains(QStringLiteral("info"))) {
        const HaOperationResult r = service.infoReport();
        return { r.ok, r.message };
    }

    // ha --popa
    if (opts.contains(QStringLiteral("popa"))) {
        const HaOperationResult r = service.startSessionAtOwnShip();
        return { r.ok, r.message };
    }

    // ha --cursor=<xDm>,<yDm>
    if (opts.contains(QStringLiteral("cursor"))) {
        const QString val = opts.value(QStringLiteral("cursor"));
        const QStringList parts = val.split(',');
        if (parts.size() != 2) {
            return { false, QStringLiteral("--cursor requiere formato <xDm>,<yDm>.\n%1").arg(usage()) };
        }
        bool okX = false, okY = false;
        const double xDm = parts[0].trimmed().toDouble(&okX);
        const double yDm = parts[1].trimmed().toDouble(&okY);
        if (!okX || !okY) {
            return { false, QStringLiteral("--cursor: los valores deben ser numericos.") };
        }

        const HaOperationResult r = service.startSessionAtCursor(xDm, yDm);
        return { r.ok, r.message };
    }

    // ha --latlon --lat=<grados> --lon=<grados>
    if (opts.contains(QStringLiteral("latlon"))) {
        if (!opts.contains(QStringLiteral("lat")) || !opts.contains(QStringLiteral("lon"))) {
            return { false, QStringLiteral("--latlon requiere --lat=<grados> y --lon=<grados>.\n%1").arg(usage()) };
        }
        bool okLat = false, okLon = false;
        const double lat = opts.value(QStringLiteral("lat")).toDouble(&okLat);
        const double lon = opts.value(QStringLiteral("lon")).toDouble(&okLon);
        if (!okLat || !okLon) {
            return { false, QStringLiteral("--lat y --lon deben ser valores numericos.") };
        }

        const HaOperationResult r = service.startSessionAtLatLon(lat, lon);
        return { r.ok, r.message };
    }

    // ha --az=<azimut> --d=<distancia_yardas>
    if (opts.contains(QStringLiteral("az"))) {
        if (!opts.contains(QStringLiteral("d"))) {
            return { false, QStringLiteral("--az requiere --d=<distancia_yardas>.\n%1").arg(usage()) };
        }
        bool okAz = false, okD = false;
        const double az = opts.value(QStringLiteral("az")).toDouble(&okAz);
        const double d  = opts.value(QStringLiteral("d")).toDouble(&okD);
        if (!okAz || !okD) {
            return { false, QStringLiteral("--az y --d deben ser valores numericos.") };
        }

        const HaOperationResult r = service.startSessionAtBearing(az, d);
        return { r.ok, r.message };
    }

    return { false, QStringLiteral("Flag no reconocido.\n%1").arg(usage()) };
}