#pragma once

#include "iCommand.h"
#include "commandContext.h"

class DisplayModeCommand : public ICommand {
public:
    QString getName() const override { return "display"; }
    QString getDescription() const override { return "Gestiona modo de presentacion cinematica (relative/true motion)"; }
    QString usage() const override {
        return "display mode <relative|rm|true|true_motion|tm|show>   (sin valor o 'show': muestra el modo actual)\n"
               "Ejemplo: display mode true_motion";
    }

    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};
