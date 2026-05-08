#pragma once

#include "iCommand.h"

class OwnShipCommand : public ICommand
{
    Q_OBJECT
public:
    QString getName() const override { return QStringLiteral("ownship"); }
    QString getDescription() const override { return QStringLiteral("Muestra o actualiza datos del buque propio"); }
    QString usage() const override {
        return QStringLiteral("ownship [show] | ownship set <course_deg> <speed_knots> [source]");
    }

    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};
