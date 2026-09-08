#ifndef CANAL_COMMAND_H
#define CANAL_COMMAND_H

#include "iCommand.h"

class CanalCommand : public ICommand {
    Q_OBJECT
public:
    QString getName() const override { return QStringLiteral("canal"); }

    QString getDescription() const override {
        return QStringLiteral("Asesoramiento canal");
    }

    QString usage() const override {
        return QStringLiteral(
            "  canal --start [--a=<track>] [--b=<track>] [--c=<track>] [--d=<track>]\n\n"
            "  canal --borrar  (Limpia todas las columnas)\n"
            "  canal --info    (Muestra el estado actual)\n"
            );
    }

    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};

#endif // CANAL_COMMAND_H