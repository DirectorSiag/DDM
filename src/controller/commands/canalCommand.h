#ifndef CANAL_COMMAND_H
#define CANAL_COMMAND_H

#include "iCommand.h"

class CanalCommand : public ICommand {
    Q_OBJECT
public:
    QString getName() const override { return QStringLiteral("canal"); }

    QString getDescription() const override {
        return QStringLiteral("Asesoramiento de canal en 4 columnas (A-D): inicia/actualiza columnas con --start, limpia la sesión con --borrar (o --stop) y consulta el estado con --info.");
    }

    QString usage() const override {
        return QStringLiteral(
            "  canal --start [--a=<track>] [--b=<track>] [--c=<track>] [--d=<track>]   (al menos una columna)\n\n"
            "  canal --borrar  (alias: --stop)  (Limpia todas las columnas)\n"
            "  canal --info    (Muestra el estado actual)\n"
            "Ejemplo: canal --start --a=0001 --b=0002\n"
            );
    }

    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};

#endif // CANAL_COMMAND_H