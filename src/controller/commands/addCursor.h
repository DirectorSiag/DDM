#pragma once
#include "iCommand.h"
#include "commandContext.h"

class AddCursorCommand : public ICommand {
    Q_OBJECT
public:
    // Este comando se usa para iniciar una linea/cursor (PPP) desde CLI.
    // La GUI AR-TDC usa la misma logica por JSON create_line.
    QString getName() const override { return "addCursor"; }
    QString getDescription() const override { return "Crea un cursor (tipo, x, y, largo, ángulo)."; }
    QString usage() const override { return "addCursor <tipoLinea> <x> <y> <largo> <angulo>"; }
    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};
