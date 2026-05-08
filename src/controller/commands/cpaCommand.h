#pragma once
#include "iCommand.h"
#include "commandContext.h"

class CpaCommand : public ICommand {
    Q_OBJECT
public:
    QString getName() const override { return "cpa"; }
    QString getDescription() const override { return "CPA coamndo"; }
    QString usage() const override {
        return "Uso: cpa <trackId1> <trackId2>";
    }

    CommandResult execute(const CommandInvocation &inv, CommandContext &ctx) const;
};


