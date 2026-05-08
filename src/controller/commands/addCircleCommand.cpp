#include "addCircleCommand.h"
#include "../services/geometryservice.h"
#include <QString>
#include <iostream>
#include <vector>

CommandResult AddCircleCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const {
    // x, y, radius, type, color (5 argumentos)
    if (inv.args.size() < 5) {
        return {false, "Error: Faltan argumentos. Uso: addCircle(x,y,radius,type,color)"};
    }

    try {
        // Helper lambda to convert QString to double with validation
        auto validateAndConvert = [](const QString& value, const QString& fieldName) -> double {
            bool ok = false;
            double result = value.toDouble(&ok);
            if (!ok) {
                throw std::invalid_argument(("No se pudo convertir '" + fieldName + "' a un número: " + value).toStdString());
            }
            return result;
        };

        double x = validateAndConvert(inv.args[0], "x");
        double y = validateAndConvert(inv.args[1], "y");
        double radius = validateAndConvert(inv.args[2], "radius");
        
        int type = 0;
        {
            bool ok = false;
            type = inv.args[3].toInt(&ok);
            if(!ok) throw std::invalid_argument("El tipo debe ser un entero.");
        }

        QString color = inv.args[4];

        GeometryService geometryService(&ctx);
        GeometryResult result = geometryService.createCircle(QPointF(x, y), radius, type, color);
        if (!result.success) {
            return {false, QString("Error al crear circulo: %1").arg(result.message)};
        }

        return {true, QString("Circulo %1 creado exitosamente en (%2, %3) con radio %4.").arg(result.id).arg(x).arg(y).arg(radius)};

    } catch (const std::exception& e) {
        return {false, QString("Error al procesar argumentos: %1").arg(e.what())};
    }
}
