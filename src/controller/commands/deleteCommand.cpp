/*
    Comando `delete`/`del`/`rm`: elimina un `Track` por `id` del `ctx.tracks` con validaciones de argumentos.
*/
#include "Commands/deleteCommand.h"

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

    if (ctx.eraseTrackById(id)) {
        return {true, QString("OK delete → id=%1").arg(id)};
    }
    return {false, QString("No existe el track id=%1").arg(id)};
}

