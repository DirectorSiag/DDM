#include "fondeoCommand.h"
#include "../services/fondeoservice.h"

CommandResult FondeoCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const
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

    FondeoService service(&ctx);

    if (opts.contains(QStringLiteral("info"))) {
        if (!ctx.fondeoSession.active) {
            return { true, QStringLiteral("[Fondeo] La maniobra de fondeo no se encuentra activa en este momento.\n") };
        }

        const FondeoSessionState& s = ctx.fondeoSession;
        QString response;
        response += QStringLiteral("\n======================================================\n");
        response += QStringLiteral("               SITUACION DE FONDEO\n");
        response += QStringLiteral("======================================================\n");
        response += QStringLiteral("Distancia al PF: %1 | Azimut del PF: %2 grados\n")
                        .arg(s.distanciaPF, 0, 'f', 1).arg(s.azimutPF, 0, 'f', 1);
        response += QStringLiteral("------------------------------------------------------\n");
        response += QStringLiteral("Distancia al PA: %1 | Azimut del PA: %2 grados\n")
                        .arg(s.distanciaPA, 0, 'f', 1).arg(s.azimutPA, 0, 'f', 1);
        response += QStringLiteral("------------------------------------------------------\n");
        response += QStringLiteral("--> AZ. RELATIVO: %1 grados | DIST RELATIVA: %2\n")
                        .arg(s.azimutRelativo, 0, 'f', 1).arg(s.distanciaRelativa, 0, 'f', 1);
        response += QStringLiteral("------------------------------------------------------\n");
        response += QStringLiteral("MOV. ACTUAL: %1\n").arg(s.movimientoActual.label);
        response += QStringLiteral("PROXIMO MOV: %1\n").arg(s.proximoMovimiento.label);
        response += QStringLiteral("======================================================\n\n");

        return { true, response };
    }

    if (opts.contains(QStringLiteral("stop"))) {
        FondeoOperationResult result = service.stopSession();
        return { result.success, result.message };
    }

    // 1. Identificar si es modo Track o modo GMS
    bool hasTrack = opts.contains(QStringLiteral("track"));
    bool hasGms = opts.contains(QStringLiteral("pf-lat-deg"));

    if (hasTrack && hasGms) {
        return { false, QStringLiteral("Error: No se puede activar el modo Track y el modo GMS simultaneamente.") };
    }

    if (!hasTrack && !hasGms) {
        return { false, QStringLiteral("Error: Debe ingresar un track de referencia o coordenadas GMS.\n%1").arg(usage()) };
    }

    // Validar PA
    if (!opts.contains(QStringLiteral("pa-az")) || !opts.contains(QStringLiteral("pa-dt"))) {
        return { false, QStringLiteral("Error: El Punto Auxiliar es obligatorio (--pa-az y --pa-dt).") };
    }

    // Validar Radios
    if (!opts.contains("r1") || !opts.contains("r2") || !opts.contains("r3") ||
        !opts.contains("r4") || !opts.contains("r5")) {
        return { false, QStringLiteral("Error: Faltan definir los 5 radios de marcha (--r1 a --r5).") };
    }

    bool ok = true;
    FondeoConfig config;

    // 2. Parsear según el modo
    if (hasTrack) {
        config.useTrack = true;
        config.useGms = false;

        if (!opts.contains(QStringLiteral("az")) || !opts.contains(QStringLiteral("dt"))) {
            return { false, QStringLiteral("Error: Faltan parametros del track (--az/--dt).\n%1").arg(usage()) };
        }

        config.useTrack = true;
        config.trackId = opts.value("track").toInt(&ok);
        if (!ok) return { false, QStringLiteral("Error: --track debe ser un ID entero.") };

        config.trackAz = opts.value("az").toDouble(&ok);
        config.trackDt = opts.value("dt").toDouble(&ok);
    } else {
        config.useTrack = false;
        config.useGms = true;

        // --- PF GMS ---
        config.pfLatDeg = opts.value("pf-lat-deg").toInt(&ok);
        config.pfLatMin = opts.value("pf-lat-min").toInt(&ok);
        config.pfLatSec = opts.value("pf-lat-sec").toDouble(&ok);

        config.pfLonDeg = opts.value("pf-lon-deg").toInt(&ok);
        config.pfLonMin = opts.value("pf-lon-min").toInt(&ok);
        config.pfLonSec = opts.value("pf-lon-sec").toDouble(&ok);

        if (!ok) return { false, QStringLiteral("Error: Fallo el parseo de las coordenadas GMS. Verifique los tipos de datos.") };
    }

    config.r1 = opts.value("r1").toDouble(&ok);
    config.r2 = opts.value("r2").toDouble(&ok);
    config.r3 = opts.value("r3").toDouble(&ok);
    config.r4 = opts.value("r4").toDouble(&ok);
    config.r5 = opts.value("r5").toDouble(&ok);

    if (!ok) return { false, QStringLiteral("Error: Todos los radios (--r1 a --r5) deben ser numeros.") };

    config.paAz = opts.value("pa-az").toDouble(&ok);
    config.paDt = opts.value("pa-dt").toDouble(&ok);

    if (!ok) return { false, QStringLiteral("Error: Los parametros del Punto Auxiliar (--pa-az y --pa-dt) deben ser numeros.") };

    FondeoOperationResult result = service.startSession(config);
    if (!result.success) {
        return { false, result.message };
    }

    return { true, result.message };
}