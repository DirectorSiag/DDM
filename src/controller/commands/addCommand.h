#pragma once

#include "iCommand.h"
#include "commandContext.h"

class AddCommand : public ICommand {
public:
    QString getName() const override { return "add"; }
    QString getDescription() const override { return "Crea un track"; }
    // velocidad/rumbo opcionales
    QString usage() const override { return "add <-s|-a|-b> <-f|-e|-u> <x> <y> [velKnots] [courseDeg]"; }
    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};
