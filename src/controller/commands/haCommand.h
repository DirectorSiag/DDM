#pragma once
#include "iCommand.h"

class HaCommand : public ICommand
{
    Q_OBJECT
public:
    QString getName() const override { return QStringLiteral("ha"); }
    QString getDescription() const override {
        return QStringLiteral("Gestiona la emergencia Hombre al Agua");
    }
    QString usage() const override {
        return QStringLiteral(
            "ha --popa\n"
            "ha --cursor=<xDm>,<yDm>\n"
            "ha --latlon --lat=<deg>,<min>,<sec> --lon=<deg>,<min>,<sec>\n"
            "ha --az=<azimut> --d=<distancia_yardas>\n"
            "ha --stop\n"
            "ha --info\n"
            );
    }
    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};
