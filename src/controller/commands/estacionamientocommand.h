#pragma once

#include "iCommand.h"

class EstacionamientoCommand : public ICommand
{
    Q_OBJECT
public:
    QString getName() const override { return QStringLiteral("estacionamiento"); }
    QString getDescription() const override { return QStringLiteral("Calcula rumbo y tiempo de estacionamiento entre TRACK-A y TRACK-B. Si --track-a se omite o es 0000, usa OwnShip como TRACK-A (debe estar inicializado)."); }
    QString usage() const override {
        return QStringLiteral(
            "estacionamiento [--track-a=<id|0000>] --track-b=<id_externo> --az=<deg> --d=<dm> "
            "(--vd=<knots> | --du=<hours>)\n"
            "Ejemplo: estacionamiento --track-b=0012 --az=90 --d=15 --vd=20"
        );
    }

    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};
