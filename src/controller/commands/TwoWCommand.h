#pragma once

#include "iCommand.h"

class TwoWCommand : public ICommand
{
    Q_OBJECT
public:
    QString getName() const override { return QStringLiteral("2w"); }
    QString getDescription() const override {
        return QStringLiteral("Gestiona la Disposicion 2W");
    }
    QString usage() const override {
        return QStringLiteral(
            "2w --guia=<trackId> --est=<estacion 1..68> [--radio=<mn>] [--aliadas=<est1,est2,...>]\n"
            "2w --stop\n"
            "2w --info\n"
            );
    }
    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};