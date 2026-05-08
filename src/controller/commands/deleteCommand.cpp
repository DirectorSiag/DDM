/*
    Comando `delete`/`del`/`rm`: elimina un `Track` por `id` del `ctx.tracks` con validaciones de argumentos.
*/
#include "deleteCommand.h"
#include "../services/trackservice.h"

CommandResult DeleteCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const {
    const QStringList& args = inv.args;
    if (args.size() != 1) {
        return {false, "Uso: " + usage()};
    }

    bool ok = false;
    const int id = args[0].toInt(&ok);
    if (!ok) {
        return {false, "ID inválido"};
    }

    TrackService trackService(&ctx);
    TrackOperationResult result = trackService.deleteTrackById(id);
    if (result.success) {
        return {true, QString("OK delete → id=%1").arg(id)};
    }
    return {false, QString("No existe el track id=%1").arg(id)};
}

