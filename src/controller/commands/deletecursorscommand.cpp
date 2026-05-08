#include "deletecursorscommand.h"

CommandResult DeleteCursorsCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const {
    const QStringList& args = inv.args;
    if (args.size() != 1) {
        return {false, "Uso: " + usage()};
    }

    bool ok = false;
    const int id = args[0].toInt(&ok);
    if (!ok) {
        return {false, "ID inválido"};
    }

    // Borrado lineal como en eraseTrackById, pero sobre ctx.cursors
    for (auto it = ctx.cursors.begin(); it != ctx.cursors.end(); ++it) {
        if (it->getCursorId() == id) {
            ctx.cursors.erase(it);
            return {true, QString("OK deletecursor → id=%1").arg(id)};
        }
    }

    return {false, QString("No existe el cursor id=%1").arg(id)};
}
