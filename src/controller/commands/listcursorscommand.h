#pragma once
#include "iCommand.h"
#include "commandContext.h"

class ListCursorsCommand : public ICommand {
public:
    QString getName() const override { return "listcursors"; }
    QString getDescription() const override { return "Lista los cursores actuales"; }
    QString usage() const override { return "listcursors\nEjemplo: listcursors"; }
    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};
