#include "haCommand.h"
#include "../services/haService.h"

CommandResult HaCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const
{
    if (inv.args.isEmpty()) {
        return { false, QStringLiteral("Faltan argumentos.\n%1").arg(usage()) };
    }

    // Parsear tokens en mapa de opciones
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
        if (!ctx.haSession.active) {
            return { false, QStringLiteral("[HA] No hay ninguna emergencia activa en este momento.\n") };
        }
        service.stopSession();
        return { true, QStringLiteral("[HA] Emergencia finalizada.") };
    }

    // ha --info
    if (opts.contains(QStringLiteral("info"))) {
        if (!ctx.haSession.active) {
            return { false, QStringLiteral("[HA] No hay ninguna emergencia activa en este momento.\n") };
        }
        const HaSessionState& s = ctx.haSession;
        QString response;
        response += QStringLiteral("\n======================================================\n");
        response += QStringLiteral("               HOMBRE AL AGUA\n");
        response += QStringLiteral("======================================================\n");
        response += QStringLiteral("Hora de Caida (Local): %1  |  UTC: %2\n")
                        .arg(s.fallTimeLocal)
                        .arg(s.fallTimeUtc);
        response += QStringLiteral("Tiempo Transcurrido:   %1\n").arg(s.elapsedTime);
        response += QStringLiteral("------------------------------------------------------\n");
        response += QStringLiteral("Azimut Verdadero:      %1 grados\n")
                        .arg(s.trueAzimuthDeg, 0, 'f', 1);
        response += QStringLiteral("Marcacion Relativa:    %1 grados  (%2)\n")
                        .arg(s.relativeBearingDeg, 0, 'f', 1)
                        .arg(s.banda);
        response += QStringLiteral("Distancia:             %1 yardas\n")
                        .arg(s.distanceYards, 0, 'f', 0);
        if (s.etaValid) {
            response += QStringLiteral("\n--> TIEMPO DE ARRIBO: %1 minutos\n")
            .arg(s.timeToArrivalMin, 0, 'f', 1);
        } else {
            response += QStringLiteral("\n--> TIEMPO DE ARRIBO: N/D (velocidad en 0)\n");
        }
        response += QStringLiteral("======================================================\n\n");
        return { true, response };
    }

    // ha --popa
    if (opts.contains(QStringLiteral("popa"))) {
        service.startSessionAtOwnShip();
        return { true, QString() };
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
        service.startSessionAtCursor(xDm, yDm);
        return { true, QString() };
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
        if (lat < -90.0 || lat > 90.0) {
            return { false, QStringLiteral("--lat debe estar en el rango [-90, 90].") };
        }
        if (lon < -180.0 || lon > 180.0) {
            return { false, QStringLiteral("--lon debe estar en el rango [-180, 180].") };
        }
        service.startSessionAtLatLon(lat, lon);
        return { true, QString() };
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
        if (az < 0.0 || az >= 360.0) {
            return { false, QStringLiteral("--az debe estar en el rango [0, 360).") };
        }
        if (d <= 0.0) {
            return { false, QStringLiteral("--d debe ser un numero positivo (en yardas).") };
        }
        service.startSessionAtBearing(az, d);
        return { true, QString() };
    }

    return { false, QStringLiteral("Flag no reconocido.\n%1").arg(usage()) };
}
