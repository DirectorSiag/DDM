#include "canalCommand.h"
#include "canalService.h"

CommandResult CanalCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const {
    if (inv.args.isEmpty()) {
        return {false, QStringLiteral("Faltan argumentos.\n%1").arg(usage())};
    }

    // 1. Parseo básico de flags (clave=valor o solo clave)
    QMap<QString, QString> opts;
    for (const QString& token : inv.args) {
        int eq = token.indexOf('=');
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

    CanalService service(&ctx);

    // --- MODO INFO ---
    if (opts.contains(QStringLiteral("info"))) {
        QString out = QStringLiteral("\n--- ASESORAMIENTO CANAL ---\n");
        const QString colNames[4] = {"A", "B", "C", "D"};

        for (int i = 0; i < 4; i++) {
            const auto& slot = ctx.canalSession.columnas[i];
            if (slot.active) {
                out += QStringLiteral("Columna %1 | Track: %2 | AZ: %3 | DT: %4 yds | RV: %5 | T: %6 min | Alarma: %7\n")
                .arg(colNames[i])
                    .arg(slot.trackId)
                    .arg(slot.azimutVerdadero, 0, 'f', 1)
                    .arg(slot.distanciaYardas, 0, 'f', 1)
                    .arg(slot.rumboVerdadero, 0, 'f', 1)
                    .arg(slot.etaValid ? QString::number(slot.timeToArrivalMin, 'f', 1) : "N/A")
                    .arg(slot.isAlarmActive ? "ON" : "OFF");
            } else {
                out += QStringLiteral("Columna %1 | [ VACIA ]\n").arg(colNames[i]);
            }
        }
        return {true, out};
    }

    // --- MODO BORRAR ---
    if (opts.contains(QStringLiteral("borrar")) || opts.contains(QStringLiteral("stop"))) {
        CanalOperationResult res = service.stopSession();
        return {res.success, res.message};
    }

    // --- MODO START (Carga dinámica) ---
    if (opts.contains(QStringLiteral("start"))) {
        CanalConfig config;
        bool ok = true;
        bool hasUpdates = false;

        if (opts.contains("a")) {
            config.setA = true;
            config.trackA = opts.value("a").toInt(&ok);
            hasUpdates = true;
            if (!ok) return {false, QStringLiteral("El valor para --a debe ser un número entero.")};
        }
        if (opts.contains("b")) {
            config.setB = true;
            config.trackB = opts.value("b").toInt(&ok);
            hasUpdates = true;
            if (!ok) return {false, QStringLiteral("El valor para --b debe ser un número entero.")};
        }
        if (opts.contains("c")) {
            config.setC = true;
            config.trackC = opts.value("c").toInt(&ok);
            hasUpdates = true;
            if (!ok) return {false, QStringLiteral("El valor para --c debe ser un número entero.")};
        }
        if (opts.contains("d")) {
            config.setD = true;
            config.trackD = opts.value("d").toInt(&ok);
            hasUpdates = true;
            if (!ok) return {false, QStringLiteral("El valor para --d debe ser un número entero.")};
        }

        if (!hasUpdates) {
            return {false, QStringLiteral("Debe especificar al menos una columna para iniciar (--a, --b, --c, --d).\n%1").arg(usage())};
        }

        CanalOperationResult res = service.startSession(config);
        return {res.success, res.message};
    }

    return {false, QStringLiteral("Comando no reconocido.\n%1").arg(usage())};
}