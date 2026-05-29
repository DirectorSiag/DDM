#include "twoWCommand.h"
#include "../services/twoWService.h"

CommandResult TwoWCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const
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

    TwoWService service(&ctx);

    // 2w --info
    if (opts.contains(QStringLiteral("info"))) {
        if (!ctx.twoWSession.active) {
            return { false, QStringLiteral("[2W] La Disposicion 2W no se encuentra activa en este momento.\n") };
        }

        const TwoWSessionState& s = ctx.twoWSession;
        QString response;

        response += QStringLiteral("\n======================================================\n");
        response += QStringLiteral("               SITUACION 2W\n");
        response += QStringLiteral("======================================================\n");

        response += QStringLiteral("Guia Track ID: %1  |  Estacion BP Asignada: %2\n")
                        .arg(s.guideTrackId)
                        .arg(s.bpStation);

        response += QStringLiteral("------------------------------------------------------\n");

        if (s.kinematicsValid) {
            response += QStringLiteral("Marcacion Real al Guia: %1 grados | Tabla A: %2 grados\n")
            .arg(s.currentAzimuthDeg, 0, 'f', 1)
                .arg(s.expectedAzimuthDeg, 0, 'f', 1);
            response += QStringLiteral("Distancia Real al Guia: %1 MN | Tabla A: %2 MN\n")
                            .arg(s.currentDistanceNm, 0, 'f', 2)
                            .arg(s.expectedDistanceNm, 0, 'f', 2);
            response += QStringLiteral("\n--> RUMBO RECOMENDADO (INTERCEPCION DIRECTA): %1 grados\n")
                            .arg(s.courseToStationDeg, 0, 'f', 1);
            response += QStringLiteral("--> ETA: %1 minutos\n")
                            .arg(s.timeToStationMin, 0, 'f', 1);
        } else {
            response += QStringLiteral("Velocidades en 0\n");
        }

        response += QStringLiteral("======================================================\n\n");

        return { true, response };
    }

    // 2w --stop
    if (opts.contains(QStringLiteral("stop"))) {
        service.stopSession();
        return { true, QStringLiteral("Disposicion 2W finalizada.") };
    }

    // 2w --guia=<id> --bp=<estacion> [--radio=<mn>]
    if (!opts.contains(QStringLiteral("guia")) || !opts.contains(QStringLiteral("bp"))) {
        return { false, QStringLiteral("Faltan --guia y --bp.\n%1").arg(usage()) };
    }

    bool okGuia = false, okBp = false;
    const int guiaId = opts.value("guia").toInt(&okGuia);
    const int bpEst  = opts.value("bp").toInt(&okBp);

    if (!okGuia || !okBp) {
        return { false, QStringLiteral("--guia y --bp deben ser enteros.") };
    }
    if (bpEst < 1 || bpEst > 68) {
        return { false, QStringLiteral("--bp debe estar entre 1 y 68.") };
    }

    double radio = 1.0;
    if (opts.contains(QStringLiteral("radio"))) {
        bool okR = false;
        const double r = opts.value("radio").toDouble(&okR);
        if (!okR || r <= 0.0) {
            return { false, QStringLiteral("--radio debe ser un numero positivo.") };
        }
        radio = r;
    }

    service.startSession(guiaId, bpEst, radio);
    return { true, QStringLiteral("Disposicion 2W iniciada.") };
}