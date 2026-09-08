#pragma once
#include "iCommand.h"

class DSICommand : public ICommand {
    Q_OBJECT
public:
    QString getName()        const override { return QStringLiteral("dsi"); }
    QString getDescription() const override {
        return QStringLiteral("Gestiona zonas de Distancia de Seguridad Impuesta en el LPD");
    }
    QString usage() const override {
        return QStringLiteral(
            "dsi --nuevo [--texto=<str>] [--radio=<mn>] [--estilo=<e>] "
            "[--color=<c>] [--fondo=<c>] "
            "(--man=<x>,<y> | --az=<g> --dt=<mn> (--v|--r) | --lat=<g>,<m>,<s> --lon=<g>,<m>,<s>) "
            "[--altrack=<tn>]\n"
            "dsi --editar=<tn> [--texto=<str>] [--radio=<mn>] [--estilo=<e>] "
            "[--color=<c>] [--fondo=<c>]\n"
            "dsi --borrar=<tn>\n"
            "dsi --altrack=<trackId> --tn=<tn>\n"
            "dsi --deltrack --tn=<tn>\n"
            "dsi --info=<tn>\n"
            "dsi --list\n"
            );
    }
    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};
