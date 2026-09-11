#pragma once
#include "iCommand.h"
#include "commandContext.h"

class DeleteCursorsCommand : public ICommand {
public:
    QString getName() const override        { return "deletecursor"; }
    QString getDescription() const override { return "Elimina un cursor por id"; }
    QString usage() const override          { return "deletecursor <id>\nEjemplo: deletecursor 2"; }
    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};
