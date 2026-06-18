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
            "ha --latlon --lat=<grados> --lon=<grados>\n"
            "ha --az=<azimut> --d=<distancia_yardas>\n"
            "ha --stop\n"
            "ha --info\n"
            );
    }
    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};
