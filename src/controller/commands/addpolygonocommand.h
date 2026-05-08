#ifndef ADDPOLYGONOCOMMAND_H
#define ADDPOLYGONOCOMMAND_H

#include "iCommand.h"
#include <QString>

class AddPolygonoCommand : public ICommand {
public:
    // Este comando se usa para iniciar un poligono tactico desde CLI.
    // En AR-TDC se invoca con JSON create_polygon.
    QString getName() const override { return "addPolygono"; }
    QString getDescription() const override { return "Crea un polígono interactivo definiendo N puntos, seguido de tipo y color."; }
    // Espera una serie de coordenadas (x,y), luego tipo y color.
    // Ejemplo: addPolygono x1 y1 x2 y2 x3 y3 tipo color (8 args)
    QString usage() const override { return "addPolygono(x1, y1, x2, y2, ..., xn, yn, tipo, color)"; }

    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};

#endif // ADDPOLYGONOCOMMAND_H
