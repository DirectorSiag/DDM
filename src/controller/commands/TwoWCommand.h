#pragma once

#include "iCommand.h"

class TwoWCommand : public ICommand
{
    Q_OBJECT
public:
    QString getName() const override { return QStringLiteral("2w"); }
    QString getDescription() const override {
        return QStringLiteral("Gestiona la Disposición 2W: inicia sesión con guía y estación (--guia/--est), actualiza estaciones aliadas (--aliadas), detiene (--stop) o consulta el estado (--info).");
    }
    QString usage() const override {
        return QStringLiteral(
            "2w --guia=<trackId> --est=<estacion 1..68> [--radio=<mn>] [--aliadas=<est1,est2,...>]\n"
            "2w --aliadas=<est1,est2,...>   (actualiza solo las estaciones aliadas graficadas; requiere sesion activa)\n"
            "2w --stop\n"
            "2w --info\n"
            "Ejemplo: 2w --guia=0001 --est=5 --radio=10\n"
            );
    }
    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};