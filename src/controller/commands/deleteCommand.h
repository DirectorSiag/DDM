/*
    Comando `delete`: elimina un `Track` por `id` del `ctx.tracks` con validaciones de argumentos.
*/

#pragma once
#include "iCommand.h"
#include "commandContext.h"

class DeleteCommand : public ICommand {
public:
    QString getName() const override { return "delete"; }
    QString getDescription() const override { return "Elimina un track por id"; }
    QString usage() const override { return "delete <id>\nEjemplo: delete 0007"; }
    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};
