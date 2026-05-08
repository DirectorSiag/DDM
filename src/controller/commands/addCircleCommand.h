#ifndef ADDCIRCLECOMMAND_H
#define ADDCIRCLECOMMAND_H

#include "iCommand.h"
#include <QPointF>
#include <QString>

class AddCircleCommand : public ICommand {
public:
    // Este comando se usa para iniciar un circulo de referencia desde CLI.
    // En AR-TDC se invoca con JSON create_circle.
    QString getName() const override { return "addCircle"; }
    QString getDescription() const override { return "Crea un círculo definido por centro (x, y), radio, tipo y color."; }
    QString usage() const override { return "addCircle(x,y,radius,type,color)"; }

    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};

#endif // ADDCIRCLECOMMAND_H
