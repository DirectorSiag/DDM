#include "deletecursorscommand.h"
#include "../services/cursorservice.h"

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

    CursorService cursorService(&ctx);
    CursorOperationResult result = cursorService.deleteCursorById(id);
    if (!result.success) {
        return {false, QString("No existe el cursor id=%1").arg(id)};
    }

    return {true, QString("OK deletecursor → id=%1").arg(id)};
}
