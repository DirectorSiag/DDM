#ifndef ADDSECTORCOMMAND_H
#define ADDSECTORCOMMAND_H

#include "iCommand.h"

class AddSectorCommand : public ICommand {
    Q_OBJECT
public:
    QString getName()        const override { return "addSector"; }
    QString getDescription() const override { return "Crea un sector anular definido por azimuts, radios, color y origen."; }
    QString usage()          const override {
        return "addSector <az_izq> <az_der> <rad_int> <rad_ext> <color> <origen_x> <origen_y> [id_track]";
    }

    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};

#endif // ADDSECTORCOMMAND_H
