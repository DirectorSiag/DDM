#pragma once

#include "iCommand.h"

class FondeoCommand : public ICommand
{
    Q_OBJECT
public:
    QString getName() const override { return QStringLiteral("fondeo"); }
    QString getDescription() const override {
        return QStringLiteral("Gestiona la maniobra de fondeo");
    }
    QString usage() const override {
        return QStringLiteral(
            "Uso por Coordenadas:\n"
            "  fondeo --pf-lat-deg=<grados> --pf-lat-min=<minutos> --pf-lat-sec=<segundos> --pf-lon-deg=<grados> --pf-lon-min=<minutos> --pf-lon-sec=<segundos> --pa-az=<grados> --pa-dt=<mn> --r1=<yds> --r2=<yds> --r3=<yds> --r4=<yds> --r5=<yds>\n\n"
            "Uso por Track de Referencia:\n"
            "  fondeo --track=<id> --az=<grados> --dt=<mn> --pa-az=<grados> --pa-dt=<mn> --r1=<yds> --r2=<yds> --r3=<yds> --r4=<yds> --r5=<yds>\n\n"
            "  fondeo --stop\n"
            "  fondeo --info\n\n"
            );
    }
    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};