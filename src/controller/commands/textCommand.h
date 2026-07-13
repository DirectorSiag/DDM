#pragma once
#include "iCommand.h"

class TextCommand : public ICommand {
    Q_OBJECT
public:
    QString getName()        const override { return QStringLiteral("texto"); }
    QString getDescription() const override {
        return QStringLiteral("Gestiona etiquetas de texto en el LPD");
    }
    QString usage() const override {
        return QStringLiteral(
            "texto --nuevo --texto=<str> --tamano=<xs|md|lg> --color=<c> "
            "[--borde=<c>] [--fondo=<c>] "
            "(--man=<x>,<y> | --az=<g> --dt=<dm> (--v|--r) | "
            "--lat=<g>,<m>,<s> --lon=<g>,<m>,<s> | --deltrack=<tn>) "
            "[--altrack=<tn>]\n"
            "texto --editar=<tn> [--texto=<str>]\n"
            "texto --borrar=<tn>\n"
            "texto --altrack=<trackId> --tn=<tn>\n"
            "texto --deltrack --tn=<tn>\n"
            "texto --info=<tn>\n"
            "texto --list\n"
        );
    }
    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};
