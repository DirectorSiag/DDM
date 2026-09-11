#pragma once

#include "iCommand.h"

class FondeoCommand : public ICommand
{
    Q_OBJECT
public:
    QString getName() const override { return QStringLiteral("fondeo"); }
    QString getDescription() const override {
        return QStringLiteral("Inicia (modo coordenadas GMS o modo track de referencia), detiene (--stop) o consulta el estado (--info) de una maniobra de fondeo.");
    }
    QString usage() const override {
        return QStringLiteral(
            "Uso por Coordenadas (GMS):\n"
            "  fondeo --pf-lat-deg=<grados> --pf-lat-min=<minutos> --pf-lat-sec=<segundos> --pf-lon-deg=<grados> --pf-lon-min=<minutos> --pf-lon-sec=<segundos> --pa-az=<grados 0..359> --pa-dt=<yds> --r1=<yds> --r2=<yds> --r3=<yds> --r4=<yds> --r5=<yds>\n\n"
            "Uso por Track de Referencia:\n"
            "  fondeo --track=<id> --az=<grados> --dt=<mn> --pa-az=<grados 0..359> --pa-dt=<yds> --r1=<yds> --r2=<yds> --r3=<yds> --r4=<yds> --r5=<yds>\n\n"
            "  fondeo --stop     (detiene la maniobra activa)\n"
            "  fondeo --info     (muestra distancias/azimuts PF-PA, próximo movimiento e ids de anillos)\n\n"
            "Ejemplo: fondeo --track=0012 --az=45 --dt=5 --pa-az=90 --pa-dt=300 --r1=100 --r2=200 --r3=300 --r4=400 --r5=500\n"
            );
    }
    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};