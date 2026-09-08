#pragma once

#include "iCommand.h"

class BorneoService;

class BorneoCommand : public ICommand
{
public:
    QString getName() const override;
    QString getDescription() const override;
    QString usage() const override;

    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;

private:
    CommandResult handleIniciar(const CommandInvocation& inv, CommandContext& ctx) const;
    CommandResult handleFinalizar(CommandContext& ctx) const;
    CommandResult handleInfo(const CommandContext& ctx) const;

    QString infoReport(const CommandContext& ctx) const;
};