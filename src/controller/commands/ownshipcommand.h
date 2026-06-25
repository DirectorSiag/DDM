#pragma once

#include "iCommand.h"

class OwnShipCommand : public ICommand
{
    Q_OBJECT
public:
    QString getName() const override { return QStringLiteral("ownship"); }
    QString getDescription() const override { return QStringLiteral("Muestra o actualiza datos del buque propio"); }
    QString usage() const override {
        return QStringLiteral(
            "ownship [show] | "
            "ownship set <course_deg> <speed_knots> [source] | "
            "ownship setgeo <lat_deg> <lon_deg> | "
            "ownship setgeodms <latDeg> <latMin> <latSec> <lonDeg> <lonMin> <lonSec>"
            );
    }

    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};
