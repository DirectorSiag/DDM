#pragma once
#include "iCommand.h"
#include "commandContext.h"

class AddCursorCommand : public ICommand {
    Q_OBJECT
public:
    // Este comando se usa para iniciar una linea/cursor (PPP) desde CLI.
    // La GUI AR-TDC usa la misma logica por JSON create_line.
    QString getName() const override { return "addCursor"; }
    QString getDescription() const override { return "Crea un cursor (línea) con tipo, posición, largo y ángulo."; }
    QString usage() const override { return "addCursor <tipoLinea> <x> <y> <largo> <angulo>   (tipoLinea: 0..7, largo: 0..256, angulo se normaliza a 0..360)\nEjemplo: addCursor 0 10 10 50 90"; }
    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};
