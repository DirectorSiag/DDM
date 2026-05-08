#include "estacionamientocommand.h"

#include "../services/estacionamientoservice.h"

CommandResult EstacionamientoCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const
{
    if (inv.args.isEmpty()) {
        return {false, QStringLiteral("Faltan argumentos. Uso: %1").arg(usage())};
    }

    EstacionamientoService service(&ctx);
    const EstacionamientoService::OperationResult result = service.executeFromCliArgs(inv.args);
    if (!result.success) {
        return {false, QStringLiteral("%1. Uso: %2").arg(result.message, usage())};
    }

    return {true, result.message};
}
