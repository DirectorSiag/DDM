/*
    Comando `center`: fija `ctx.centerX/centerY` con `<x> <y>` validando formatos numéricos.
*/

#pragma once
#include "iCommand.h"
#include "commandContext.h"

class CenterCommand : public ICommand {
public:
    QString getName() const override { return "center"; }
    QString getDescription() const override { return "Centra/actualiza el centro global"; }
    QString usage() const override { return "center <x> <y>   (-255..255)\nEjemplo: center 0 0"; }
    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};
