#pragma once
#include "iCommand.h"
#include "commandContext.h"

class addCursor : public ICommand {
    Q_OBJECT
public:
    QString getName() const override { return "addCursor"; }
    QString getDescription() const override { return "Crea un cursor (tipo, x, y, largo, ángulo)."; }
    QString usage() const override { return "addCursor <tipoLinea> <x> <y> <largo> <angulo>"; }
    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};
