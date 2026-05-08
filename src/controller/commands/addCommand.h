#pragma once

#include "iCommand.h"
#include "commandContext.h"

class AddCommand : public ICommand {
public:
    // Este comando se usa para iniciar/crear un track (PPP/SITREP) desde CLI.
    // El frontend AR-TDC puede disparar la misma logica via JSON create_track.
    QString getName() const override { return "add"; }
    QString getDescription() const override { return "Crea un track (modelo SURFACE TRACKS)."; }

    QString usage() const override {
        return
            "add <--type <SPC|LINCO|ASW|OPS|HECO|APC|AAW|EW>|-f|-e|-u> [identidad] <x> <y> [legacyVelKnots] [legacyCourseDeg]\n"
            "\n"
            "Tipo táctico / ambiente (obligatorio):\n"
            "  --type <SPC|LINCO|ASW|OPS|HECO|APC|AAW|EW>\n"
            "  Compat legacy: -f -> SPC   -e -> AAW   -u -> ASW\n"
            "\n"
            "Identidad (opcional, por defecto: P):\n"
            "  --id <P|A|F|E|H|U|Y>\n"
            "  (compat: -s friend, -a hostile, -b unknown)\n"
            "\n"
            "Campos opcionales (modelo):\n"
            "  velocidad <DM/h>          (0..99.9)\n"
            "  curso <deg>           (0..359)\n"
            "  --fc <1..6>\n"
            "  --asgc <texto>\n"
            "  --linky <R|C|T|S>\n"
            "  --link14 <T>\n"
            "  --info <texto...>\n"
            "  --priv <texto...>\n"
            "\n"
            "Ejemplos:\n"
                "  add --type SPC --id F 50 50 --spd 12.5 --crs 90 --fc 3 --asgc ALFA --linky R --info contacto recibido\n"
            "  add -f -s 10 20 15 180   (legacy: 15kt 180deg)\n"
            "\n";
    }

    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};
