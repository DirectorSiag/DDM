#include "derrotasCommand.h"
#include "../services/derrotasService.h"

CommandResult DerrotasCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const
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

    // derrotas --info
    if (opts.contains(QStringLiteral("info"))) {
        const DerrotasFuturaState& f = ctx.derrotasSession.futura;
        const DerrotasPasadaState& p = ctx.derrotasSession.pasada;

        if (!f.active && !p.recordingActive) {
            return { false, QStringLiteral("[Derrotas] No hay asesoramientos ni grabaciones activas.\n") };
        }

        QString response;
        response += QStringLiteral("\n======================================================\n");
        response += QStringLiteral("               SITUACION DE DERROTAS\n");
        response += QStringLiteral("======================================================\n");

        if (f.active) {
            response += QStringLiteral("DERROTA FUTURA — Track: %1 | Tiempo: %2 min\n")
                            .arg(f.config.trackId).arg(f.config.timeMinutes);
            response += QStringLiteral("Umbral Grados: %1 | Umbral Nudos: %2\n")
                            .arg(f.config.thresholdDeg, 0, 'f', 0)
                            .arg(f.config.thresholdKn, 0, 'f', 0);
            response += QStringLiteral("Distancia Total: %1 DM | Alarma: %2\n")
                            .arg(f.totalDistanceDm, 0, 'f', 2)
                            .arg(f.alarmTriggered ? QStringLiteral("DISPARADA") : QStringLiteral("normal"));
            response += QStringLiteral("------------------------------------------------------\n");
        } else {
            response += QStringLiteral("DERROTA FUTURA — sin actividad.\n");
            response += QStringLiteral("------------------------------------------------------\n");
        }

        if (p.recordingActive) {
            response += QStringLiteral("GRABACION — Track: %1 | Segmento: %2\n")
                            .arg(p.recordingTrackId).arg(p.currentSegmentIndex);
            response += QStringLiteral("Archivo actual: %1\n").arg(p.currentLogFileName);
        } else {
            response += QStringLiteral("GRABACION — sin actividad.\n");
        }
        response += QStringLiteral("======================================================\n\n");

        return { true, response };
    }

    // derrotas --borrar
    if (opts.contains(QStringLiteral("borrar"))) {
        const DerrotasOperationResult r = m_derrotasService->clearAll();
        return { r.success, r.message };
    }

    // derrotas --rec-iniciar --track=<id>
    if (opts.contains(QStringLiteral("rec-iniciar"))) {
        if (!opts.contains(QStringLiteral("track"))) {
            return { false, QStringLiteral("--rec-iniciar requiere --track=<id>.\n%1").arg(usage()) };
        }
        bool ok = false;
        const int trackId = opts.value(QStringLiteral("track")).toInt(&ok);
        if (!ok) {
            return { false, QStringLiteral("--track debe ser un entero.") };
        }
        const DerrotasOperationResult r = m_derrotasService->startRecording(trackId);
        return { r.success, r.message };
    }

    // derrotas --rec-finalizar
    if (opts.contains(QStringLiteral("rec-finalizar"))) {
        const DerrotasOperationResult r = m_derrotasService->stopRecording();
        return { r.success, r.message };
    }

    // derrotas --finalizar
    if (opts.contains(QStringLiteral("finalizar"))) {
        const DerrotasOperationResult r = m_derrotasService->stopFutura();
        return { r.success, r.message };
    }

    // derrotas --track=<id> --tiempo=<min> --grados=<deg> --nudos=<kn>
    if (!opts.contains(QStringLiteral("track"))
        || !opts.contains(QStringLiteral("tiempo"))
        || !opts.contains(QStringLiteral("grados"))
        || !opts.contains(QStringLiteral("nudos"))) {
        return { false, QStringLiteral("Faltan argumentos.\n%1").arg(usage()) };
    }

    bool okTrack = false, okTiempo = false, okGrados = false, okNudos = false;
    const int    trackId     = opts.value(QStringLiteral("track")).toInt(&okTrack);
    const int    timeMinutes = opts.value(QStringLiteral("tiempo")).toInt(&okTiempo);
    const double thresholdDeg = opts.value(QStringLiteral("grados")).toDouble(&okGrados);
    const double thresholdKn  = opts.value(QStringLiteral("nudos")).toDouble(&okNudos);

    if (!okTrack || !okTiempo || !okGrados || !okNudos) {
        return { false, QStringLiteral("--track, --tiempo, --grados y --nudos deben ser numericos.") };
    }

    const DerrotasOperationResult r = m_derrotasService->startFutura(trackId, timeMinutes, thresholdDeg, thresholdKn);
    return { r.success, r.message };
}