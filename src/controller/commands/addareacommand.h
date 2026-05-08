#ifndef ADDAREACOMMAND_H
#define ADDAREACOMMAND_H

#include "iCommand.h"
#include <QPointF>
#include <QString>

class AddAreaCommand : public ICommand {
public:
    // Este comando se usa para iniciar un area (figuras tacticas) desde consola.
    // Su equivalente para AR-TDC es el comando JSON create_area.
    QString getName() const override { return "addArea"; }
    QString getDescription() const override { return "Crea un área interactiva definiendo puntos A, B, C, D, tipo y color."; }
    QString usage() const override { return "addArea(ax,ay,bx,by,cx,cy,dx,dy,tipo,color)"; }

    // El método execute es const por interfaz, no se puede cambiar
    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;

};

#endif
