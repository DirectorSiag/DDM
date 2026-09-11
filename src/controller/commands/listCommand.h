/*
Comando `list`: imprime tabla con `id`, `type`, `ident`, `x`, `y` de los tracks existentes.
*/

#pragma once
#include "iCommand.h"
#include "commandContext.h"

class ListCommand : public ICommand {
public:
    QString getName() const override { return "list"; }
    QString getDescription() const override { return "Lista los tracks actuales"; }
    QString usage() const override { return "list\nEjemplo: list"; }
    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};
