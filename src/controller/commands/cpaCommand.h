#pragma once
#include "iCommand.h"
#include "commandContext.h"

class CpaCommand : public ICommand {
    Q_OBJECT
public:
    QString getName() const override { return "cpa"; }
    // NOTA: el cálculo real de CPA (TCPA/DCPA vía CPAService) está deshabilitado en este comando;
    // por ahora solo valida los dos IDs recibidos y los imprime, sin calcular nada. El motor real
    // de CPA (CPAService) sólo está conectado al pipeline JSON (JsonCommandHandler), no a esta CLI.
    QString getDescription() const override { return "Valida dos IDs de track y los muestra (aún NO calcula TCPA/DCPA: el cálculo real de CPA no está implementado en este comando CLI)."; }
    QString usage() const override {
        return "cpa <trackId1> <trackId2>\nEjemplo: cpa 0001 0002";
    }

    CommandResult execute(const CommandInvocation &inv, CommandContext &ctx) const;
};


